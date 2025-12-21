#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include "../include/solver.h"

typedef struct {
    char type[32];
    double cond_min;
    double cond_max;
} RangeRule;

typedef struct {
    char name[64];
    double cond_tol;
    double pivot_tol;
} ToleranceRule;

typedef struct {
    char name[64];
    double cond_min;
    double cond_max;
    double pivot_min;
    double pivot_max;
} SensitivityRule;

static int verbose = 0;
static int dump_json = 0;
static const char *dump_path = "artifacts/failure_dump.json";
static const char *descriptor_log_path = "artifacts/kkt_descriptor_actions_local.csv";
static const char *tolerance_path = "data/approx_tolerances.csv";
static const char *sensitivity_path = "data/parameter_sensitivity_ranges.csv";
static const char *dataset_version_path = "data/dataset_version.txt";
static char dataset_version[64] = "unknown";
static int compare_threads = 0;
static int thread_list[8] = {0};
static int thread_count = 0;

static void dump_failure_json(const char *reason, const SolveResult *res) {
    if (!dump_json)
        return;
    FILE *fp = fopen(dump_path, "w");
    if (!fp)
        return;
    fprintf(fp,
            "{\n"
            "  \"reason\": \"%s\",\n"
            "  \"descriptor_log\": \"%s\",\n"
            "  \"tolerance_csv\": \"%s\",\n"
            "  \"sensitivity_csv\": \"%s\",\n"
            "  \"dataset_version\": \"%s\",\n"
            "  \"dataset_version_path\": \"%s\",\n"
            "  \"threads\": {\"compare\": %d, \"list\": [",
            reason,
            descriptor_log_path,
            tolerance_path,
            sensitivity_path,
            dataset_version,
            dataset_version_path,
            compare_threads);
    for (int i = 0; i < thread_count; ++i) {
        fprintf(fp, "%s%d", (i == 0 ? "" : ","), thread_list[i]);
    }
    fprintf(fp, "]},\n  \"cases\": [\n");
    for (int i = 0; i < res->count; ++i) {
        const ConstraintCase *c = &res->cases[i];
        fprintf(fp,
                "    {\"name\":\"%s\",\"type\":%d,\"time\":%.6f,"
                "\"cond_bound\":%.6e,\"cond_spectral\":%.6e,"
                "\"pivot_min\":%.6e,\"pivot_max\":%.6e,"
                "\"vn\":%.6e,\"vt\":%.6e,\"mu_s\":%.3f,\"mu_d\":%.3f,"
                "\"stick\":%d,\"axis\":[%.6f,%.6f],\"anchor_a\":[%.6f,%.6f],"
                "\"anchor_b\":[%.6f,%.6f],\"contact_point\":[%.6f,%.6f],\"normal\":[%.6f,%.6f],"
                "\"mass_a\":%.6f,\"mass_b\":%.6f,\"inertia_a\":%.6f,\"inertia_b\":%.6f,"
                "\"pivot_valid\":%d,\"cond_valid\":%d,\"j_rows\":[",
                c->name,
                c->type,
                c->time,
                c->condition_bound,
                c->condition_spectral,
                c->min_pivot,
                c->max_pivot,
                c->vn,
                c->vt,
                c->mu_s,
                c->mu_d,
                c->stick,
                c->axis[0],
                c->axis[1],
                c->anchor_a[0],
                c->anchor_a[1],
                c->anchor_b[0],
                c->anchor_b[1],
                c->contact_point[0],
                c->contact_point[1],
                c->normal[0],
                c->normal[1],
                c->mass_a,
                c->mass_b,
                c->inertia_a,
                c->inertia_b,
                (c->min_pivot > 0.0 && c->max_pivot > 0.0 &&
                 isfinite(c->min_pivot) && isfinite(c->max_pivot)),
                (c->condition_spectral > 0.0 && isfinite(c->condition_spectral)));
        for (int r = 0; r < c->j_row_count; ++r) {
            fprintf(fp,
                    "%s[%.6e,%.6e,%.6e,%.6e,%.6e,%.6e]",
                    (r == 0 ? "" : ","),
                    c->j_rows[r][0],
                    c->j_rows[r][1],
                    c->j_rows[r][2],
                    c->j_rows[r][3],
                    c->j_rows[r][4],
                    c->j_rows[r][5]);
        }
        fprintf(fp, "]}%s\n", (i + 1 == res->count) ? "" : ",");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
}

static int load_ranges(const char *path, RangeRule *rules, int max_rules) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open range config %s\n", path);
        return 0;
    }
    char line[256];
    /* skip header */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_rules) {
        char t[32];
        double lo, hi;
        if (sscanf(line, "%31[^,],%lf,%lf", t, &lo, &hi) == 3) {
            strcpy(rules[count].type, t);
            rules[count].cond_min = lo;
            rules[count].cond_max = hi;
            count++;
        }
    }
    fclose(fp);
    return count;
}

static const RangeRule *find_range(const RangeRule *rules, int count, const char *type) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(rules[i].type, type) == 0) {
            return &rules[i];
        }
    }
    return NULL;
}

static int load_dataset_version(void) {
    FILE *fp = fopen(dataset_version_path, "r");
    if (!fp) {
        return 0;
    }
    if (!fgets(dataset_version, sizeof(dataset_version), fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    size_t len = strlen(dataset_version);
    while (len > 0 && (dataset_version[len - 1] == '\n' || dataset_version[len - 1] == '\r')) {
        dataset_version[len - 1] = '\0';
        len--;
    }
    return len > 0;
}

static int load_tolerances(const char *path, ToleranceRule *rules, int max_rules) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_rules) {
        char name[64];
        double cond_tol, pivot_tol;
        if (sscanf(line, "%63[^,],%lf,%lf", name, &cond_tol, &pivot_tol) == 3) {
            strcpy(rules[count].name, name);
            rules[count].cond_tol = cond_tol;
            rules[count].pivot_tol = pivot_tol;
            count++;
        }
    }
    fclose(fp);
    return count;
}

static const ToleranceRule *find_tolerance(const ToleranceRule *rules, int count, const char *name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(rules[i].name, name) == 0) {
            return &rules[i];
        }
    }
    return NULL;
}

static int load_sensitivity(const char *path, SensitivityRule *rules, int max_rules) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_rules) {
        char name[64];
        double cond_min, cond_max, pivot_min, pivot_max;
        if (sscanf(line, "%63[^,],%lf,%lf,%lf,%lf",
                   name, &cond_min, &cond_max, &pivot_min, &pivot_max) == 5) {
            strcpy(rules[count].name, name);
            rules[count].cond_min = cond_min;
            rules[count].cond_max = cond_max;
            rules[count].pivot_min = pivot_min;
            rules[count].pivot_max = pivot_max;
            count++;
        }
    }
    fclose(fp);
    return count;
}

static const SensitivityRule *find_sensitivity(const SensitivityRule *rules, int count, const char *name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(rules[i].name, name) == 0) {
            return &rules[i];
        }
    }
    return NULL;
}

static void write_descriptor_csv(const char *path, const SolveResult *res) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for write\n", path);
        exit(1);
    }
    fprintf(fp,
            "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot,"
            "vn,vt,mu_s,mu_d,stick\n");
    for (int i = 0; i < res->count; ++i) {
        const ConstraintCase *c = &res->cases[i];
        fprintf(fp,
                "%.6f,%s,%s,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%d\n",
                c->time,
                c->name,
                "actions",
                c->condition_bound,
                c->condition_spectral,
                c->min_pivot,
                c->max_pivot,
                c->vn,
                c->vt,
                c->mu_s,
                c->mu_d,
                c->stick);
    }
    fclose(fp);
}

static const ConstraintCase *find_case(const SolveResult *res, const char *name) {
    for (int i = 0; i < res->count; ++i) {
        if (strcmp(res->cases[i].name, name) == 0) {
            return &res->cases[i];
        }
    }
    return NULL;
}

static int validate_schema(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for schema validation\n", path);
        return 0;
    }
    char header[256];
    if (!fgets(header, sizeof(header), fp)) {
        fclose(fp);
        fprintf(stderr, "Empty descriptor file\n");
        return 0;
    }
    fclose(fp);
    const char *expected =
        "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot,vn,vt,mu_s,mu_d,stick\n";
    if (strcmp(header, expected) != 0) {
        fprintf(stderr, "Schema mismatch. Got: %s", header);
        return 0;
    }
    return 1;
}

static int almost_equal(double a, double b, double tol) {
    return fabs(a - b) <= tol;
}

int main(int argc, char **argv) {
    const char *out_path = "artifacts/kkt_descriptor_actions_local.csv";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--descriptor-log") == 0 && i + 1 < argc) {
            out_path = argv[i + 1];
            descriptor_log_path = out_path;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--dump-json") == 0 && i + 1 < argc) {
            dump_json = 1;
            dump_path = argv[i + 1];
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            compare_threads = 1;
            const char *arg = argv[i + 1];
            char buf[128];
            strncpy(buf, arg, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            char *tok = strtok(buf, ",");
            while (tok && thread_count < 8) {
                thread_list[thread_count++] = atoi(tok);
                tok = strtok(NULL, ",");
            }
        }
    }

    SolveResult res = run_coupled_constraint();
    if (res.count <= 0) {
        fprintf(stderr, "No constraint cases produced\n");
        dump_failure_json("no_cases", &res);
        return 1;
    }

    if (!load_dataset_version()) {
        fprintf(stderr, "Missing dataset version file\n");
        dump_failure_json("dataset_version_missing", &res);
        return 1;
    }

    write_descriptor_csv(out_path, &res);
    if (!validate_schema(out_path)) {
        dump_failure_json("schema", &res);
        return 1;
    }

    for (int i = 0; i < res.count; ++i) {
        const ConstraintCase *c = &res.cases[i];
        if (c->min_pivot <= 0.0 || c->max_pivot <= 0.0 || !isfinite(c->min_pivot) ||
            !isfinite(c->max_pivot)) {
            fprintf(stderr, "Invalid pivot for case %s\n", c->name);
            dump_failure_json("pivot_invalid", &res);
            return 1;
        }
        if (c->condition_spectral <= 0.0 || !isfinite(c->condition_spectral)) {
            fprintf(stderr, "Invalid condition for case %s\n", c->name);
            dump_failure_json("cond_invalid", &res);
            return 1;
        }
        if (fabs(c->condition_bound - c->condition_spectral) > 1e-9) {
            fprintf(stderr, "Condition mismatch bound vs spectral for %s\n", c->name);
            dump_failure_json("cond_mismatch", &res);
            return 1;
        }
    }

    /* Load range config */
    RangeRule rules[16];
    int range_count = load_ranges("data/constraint_ranges.csv", rules, 16);
    ToleranceRule tolerances[32];
    int tol_count = load_tolerances(tolerance_path, tolerances, 32);
    SensitivityRule sensitivity[32];
    int sensitivity_count = load_sensitivity(sensitivity_path, sensitivity, 32);

    const ConstraintCase *revolute = find_case(&res, "tele_yaw_control");
    if (!revolute || revolute->condition_bound < 0.5 || revolute->condition_bound > 10.0) {
        fprintf(stderr, "Unexpected revolute condition\n");
        dump_failure_json("revolute_range", &res);
        return 1;
    }

    const ConstraintCase *contact_stick = find_case(&res, "hydraulic_lift_sync");
    const ConstraintCase *contact_slip = find_case(&res, "hydraulic_lift_sync_slip");
    if (!contact_stick || !contact_slip) {
        fprintf(stderr, "Missing contact cases\n");
        dump_failure_json("contact_missing", &res);
        return 1;
    }
    if (contact_slip->condition_bound <= contact_stick->condition_bound) {
        fprintf(stderr, "Slip case should have weaker conditioning than stick\n");
        dump_failure_json("contact_order", &res);
        return 1;
    }
    if (contact_slip->min_pivot >= contact_stick->min_pivot) {
        fprintf(stderr, "Slip pivot should be smaller than stick (more compliant)\n");
        dump_failure_json("contact_pivot", &res);
        return 1;
    }
    if (contact_stick->stick != 1 || contact_slip->stick != 0) {
        fprintf(stderr, "Stick/slip flags incorrect\n");
        dump_failure_json("contact_flag", &res);
        return 1;
    }
    if (contact_stick->mu_s <= 0.0 || contact_slip->mu_d <= 0.0) {
        fprintf(stderr, "Friction coefficients missing\n");
        dump_failure_json("contact_friction", &res);
        return 1;
    }
    /* Apply range rules if available */
    const RangeRule *stick_range = find_range(rules, range_count, "contact_stick");
    const RangeRule *slip_range = find_range(rules, range_count, "contact_slip");
    if (stick_range) {
        if (contact_stick->condition_bound < stick_range->cond_min ||
            contact_stick->condition_bound > stick_range->cond_max) {
            fprintf(stderr, "Stick condition out of configured range: %.3f\n", contact_stick->condition_bound);
            return 1;
        }
    }
    if (slip_range) {
        if (contact_slip->condition_bound < slip_range->cond_min ||
            contact_slip->condition_bound > slip_range->cond_max) {
            fprintf(stderr, "Slip condition out of configured range: %.3f\n", contact_slip->condition_bound);
            dump_failure_json("contact_slip_range", &res);
            return 1;
        }
    }

    const ConstraintCase *mass_ratio = find_case(&res, "mass_ratio_100");
    if (!mass_ratio) {
        fprintf(stderr, "Missing mass_ratio_100 case\n");
        dump_failure_json("mass_ratio_missing", &res);
        return 1;
    }
    if (mass_ratio->condition_bound <= 0.0 || mass_ratio->condition_bound > 50.0) {
        fprintf(stderr, "Mass ratio cond out of range: %.3f\n", mass_ratio->condition_bound);
        dump_failure_json("mass_ratio_cond", &res);
        return 1;
    }
    if (mass_ratio->min_pivot <= 0.0) {
        fprintf(stderr, "Mass ratio pivot invalid\n");
        dump_failure_json("mass_ratio_pivot", &res);
        return 1;
    }

    /* Composite constraint should remain finite but may degrade conditioning */
    const ConstraintCase *comp_planar = find_case(&res, "composite_planar_distance");
    const ConstraintCase *comp_dist = find_case(&res, "composite_distance");
    if (!comp_planar || !comp_dist) {
        fprintf(stderr, "Composite constraint cases missing\n");
        dump_failure_json("composite_missing", &res);
        return 1;
    }
    if (comp_planar->condition_bound <= 0.0 || comp_dist->condition_bound <= 0.0) {
        fprintf(stderr, "Composite cond invalid\n");
        dump_failure_json("composite_invalid", &res);
        return 1;
    }
    if (comp_planar->condition_bound < 0.5 || comp_planar->condition_bound > 60.0 ||
        comp_dist->condition_bound < 0.5 || comp_dist->condition_bound > 60.0) {
        fprintf(stderr, "Composite cond out of range (planar %.3f, dist %.3f)\n",
                comp_planar->condition_bound, comp_dist->condition_bound);
        dump_failure_json("composite_range", &res);
        return 1;
    }

    const ConstraintCase *comp_prismatic = find_case(&res, "composite_prismatic_distance");
    const ConstraintCase *comp_prismatic_aux = find_case(&res, "composite_prismatic_distance_aux");
    if (!comp_prismatic || !comp_prismatic_aux) {
        fprintf(stderr, "Composite prismatic cases missing\n");
        dump_failure_json("composite_prismatic_missing", &res);
        return 1;
    }
    if (comp_prismatic->condition_bound <= 0.0 || comp_prismatic_aux->condition_bound <= 0.0) {
        fprintf(stderr, "Composite prismatic cond invalid\n");
        dump_failure_json("composite_prismatic_invalid", &res);
        return 1;
    }
    if (comp_prismatic->condition_bound < 0.5 || comp_prismatic->condition_bound > 80.0 ||
        comp_prismatic_aux->condition_bound < 0.5 || comp_prismatic_aux->condition_bound > 80.0) {
        fprintf(stderr, "Composite prismatic cond out of range (prismatic %.3f, dist %.3f)\n",
                comp_prismatic->condition_bound, comp_prismatic_aux->condition_bound);
        dump_failure_json("composite_prismatic_range", &res);
        return 1;
    }

    const ConstraintCase *comp_planar_prismatic = find_case(&res, "composite_planar_prismatic");
    const ConstraintCase *comp_planar_prismatic_aux = find_case(&res, "composite_planar_prismatic_aux");
    if (!comp_planar_prismatic || !comp_planar_prismatic_aux) {
        fprintf(stderr, "Composite planar/prismatic cases missing\n");
        dump_failure_json("composite_planar_prismatic_missing", &res);
        return 1;
    }
    if (comp_planar_prismatic->condition_bound <= 0.0 ||
        comp_planar_prismatic_aux->condition_bound <= 0.0) {
        fprintf(stderr, "Composite planar/prismatic cond invalid\n");
        dump_failure_json("composite_planar_prismatic_invalid", &res);
        return 1;
    }
    if (comp_planar_prismatic->condition_bound < 0.5 || comp_planar_prismatic->condition_bound > 80.0 ||
        comp_planar_prismatic_aux->condition_bound < 0.5 || comp_planar_prismatic_aux->condition_bound > 80.0) {
        fprintf(stderr, "Composite planar/prismatic cond out of range (planar %.3f, prismatic %.3f)\n",
                comp_planar_prismatic->condition_bound, comp_planar_prismatic_aux->condition_bound);
        dump_failure_json("composite_planar_prismatic_range", &res);
        return 1;
    }

    for (int i = 0; i < res.count; ++i) {
        const ConstraintCase *c = &res.cases[i];
        const SensitivityRule *rule = find_sensitivity(sensitivity, sensitivity_count, c->name);
        if (!rule)
            continue;
        if (c->condition_bound < rule->cond_min || c->condition_bound > rule->cond_max) {
            fprintf(stderr, "Sensitivity cond out of range for %s: %.3f\n", c->name, c->condition_bound);
            dump_failure_json("sensitivity_cond_range", &res);
            return 1;
        }
        if (c->min_pivot < rule->pivot_min || c->max_pivot > rule->pivot_max) {
            fprintf(stderr, "Sensitivity pivot out of range for %s: %.6e/%.6e\n",
                    c->name, c->min_pivot, c->max_pivot);
            dump_failure_json("sensitivity_pivot_range", &res);
            return 1;
        }
    }

    /* Determinism / OpenMP thread sweep */
    int sweep_count = compare_threads ? thread_count : 1;
    if (sweep_count == 1 && !compare_threads) {
        thread_list[0] = 1;
        thread_list[1] = omp_get_max_threads();
        sweep_count = 2;
    }
    SolveResult ref_res = res;
    for (int t = 0; t < sweep_count; ++t) {
        int th = thread_list[t];
        if (th <= 0)
            th = 1;
        if (th > omp_get_max_threads())
            th = omp_get_max_threads();
        omp_set_num_threads(th);
        SolveResult sweep = run_coupled_constraint();
        if (sweep.count != ref_res.count) {
            fprintf(stderr, "Determinism check failed: count mismatch (threads=%d)\n", th);
            dump_failure_json("determinism_count", &sweep);
            return 1;
        }
        for (int i = 0; i < sweep.count; ++i) {
            const ConstraintCase *a = &ref_res.cases[i];
            const ConstraintCase *b = &sweep.cases[i];
            double cond_tol = 1e-6;
            double pivot_tol = 1e-6;
            const ToleranceRule *tol = find_tolerance(tolerances, tol_count, a->name);
            if (tol) {
                if (tol->cond_tol > 0.0)
                    cond_tol = tol->cond_tol;
                if (tol->pivot_tol > 0.0)
                    pivot_tol = tol->pivot_tol;
            }
            if (strcmp(a->name, b->name) != 0 ||
                !almost_equal(a->condition_bound, b->condition_bound, cond_tol) ||
                !almost_equal(a->min_pivot, b->min_pivot, pivot_tol) ||
                !almost_equal(a->max_pivot, b->max_pivot, pivot_tol)) {
                fprintf(stderr, "Determinism check failed for %s (threads=%d)\n", a->name, th);
                dump_failure_json("determinism_value", &sweep);
                return 1;
            }
        }
    }

    if (verbose) {
        fprintf(stderr, "All %d cases passed. Output: %s\n", res.count, out_path);
    }

    printf("Coupled constraint test passed.\n");
    return 0;
}
