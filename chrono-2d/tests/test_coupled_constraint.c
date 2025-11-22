#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

static void write_descriptor_csv(const char *path, const SolveResult *res) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for write\n", path);
        exit(1);
    }
    fprintf(fp, "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot\n");
    for (int i = 0; i < res->count; ++i) {
        const ConstraintCase *c = &res->cases[i];
        fprintf(fp,
                "%.6f,%s,%s,%.6e,%.6e,%.6e,%.6e\n",
                c->time,
                c->name,
                "actions",
                c->condition_bound,
                c->condition_spectral,
                c->min_pivot,
                c->max_pivot);
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
        "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot\n";
    if (strcmp(header, expected) != 0) {
        fprintf(stderr, "Schema mismatch. Got: %s", header);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *out_path = "artifacts/kkt_descriptor_actions_local.csv";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--descriptor-log") == 0 && i + 1 < argc) {
            out_path = argv[i + 1];
        }
    }

    SolveResult res = run_coupled_constraint();
    if (res.count <= 0) {
        fprintf(stderr, "No constraint cases produced\n");
        return 1;
    }

    write_descriptor_csv(out_path, &res);
    if (!validate_schema(out_path)) {
        return 1;
    }

    for (int i = 0; i < res.count; ++i) {
        const ConstraintCase *c = &res.cases[i];
        if (c->min_pivot <= 0.0 || c->max_pivot <= 0.0 || !isfinite(c->min_pivot) ||
            !isfinite(c->max_pivot)) {
            fprintf(stderr, "Invalid pivot for case %s\n", c->name);
            return 1;
        }
        if (c->condition_spectral <= 0.0 || !isfinite(c->condition_spectral)) {
            fprintf(stderr, "Invalid condition for case %s\n", c->name);
            return 1;
        }
        if (fabs(c->condition_bound - c->condition_spectral) > 1e-9) {
            fprintf(stderr, "Condition mismatch bound vs spectral for %s\n", c->name);
            return 1;
        }
        if (c->condition_bound > 50.0) {
            fprintf(stderr, "Condition too high for %s (%.3f)\n", c->name, c->condition_bound);
            return 1;
        }
    }

    const ConstraintCase *revolute = find_case(&res, "tele_yaw_control");
    if (!revolute || revolute->condition_bound < 0.5 || revolute->condition_bound > 10.0) {
        fprintf(stderr, "Unexpected revolute condition\n");
        return 1;
    }

    const ConstraintCase *contact_stick = find_case(&res, "hydraulic_lift_sync");
    const ConstraintCase *contact_slip = find_case(&res, "hydraulic_lift_sync_slip");
    if (!contact_stick || !contact_slip) {
        fprintf(stderr, "Missing contact cases\n");
        return 1;
    }
    if (contact_slip->condition_bound <= contact_stick->condition_bound) {
        fprintf(stderr, "Slip case should have weaker conditioning than stick\n");
        return 1;
    }

    printf("Coupled constraint test passed.\n");
    return 0;
}
