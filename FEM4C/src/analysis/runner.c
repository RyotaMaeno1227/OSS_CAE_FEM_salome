#include "runner.h"
#include "static.h"
#include "../mbd/constraint2d.h"
#include "../mbd/kkt2d.h"
#include "../io/input.h"
#include "../common/globals.h"
#include "../common/error.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MBD_RUNTIME_MAX_BODIES 8
#define MBD_RUNTIME_MAX_CONSTRAINTS 8
#define MBD_DIAG_E_BODY_PARSE "E_BODY_PARSE"
#define MBD_DIAG_E_BODY_RANGE "E_BODY_RANGE"
#define MBD_DIAG_E_DUP_BODY "E_DUP_BODY"
#define MBD_DIAG_E_DISTANCE_PARSE "E_DISTANCE_PARSE"
#define MBD_DIAG_E_DISTANCE_RANGE "E_DISTANCE_RANGE"
#define MBD_DIAG_E_REVOLUTE_PARSE "E_REVOLUTE_PARSE"
#define MBD_DIAG_E_REVOLUTE_RANGE "E_REVOLUTE_RANGE"
#define MBD_DIAG_E_UNSUPPORTED_DIRECTIVE "E_UNSUPPORTED_DIRECTIVE"
#define MBD_DIAG_E_INCOMPLETE_INPUT "E_INCOMPLETE_INPUT"
#define MBD_DIAG_E_UNDEFINED_BODY_REF "E_UNDEFINED_BODY_REF"
#define MBD_DIAG_E_BODY_COUNT_RANGE "E_BODY_COUNT_RANGE"
#define MBD_DIAG_E_CONSTRAINT_BODY_RANGE "E_CONSTRAINT_BODY_RANGE"
#define MBD_SOURCE_DEFAULT "default"
#define MBD_SOURCE_ENV "env"
#define MBD_SOURCE_CLI "cli"
#define MBD_SOURCE_ENV_INVALID_FALLBACK "env_invalid_fallback"
#define MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK "env_out_of_range_fallback"

typedef struct {
    int id;
    int body_i;
    int body_j;
    int line_no;
} mbd_constraint_source_t;

static int string_equals_ignore_case(const char *lhs, const char *rhs);
static double parse_env_double_or_default(const char *name,
                                          double default_value,
                                          double min_value,
                                          double max_value);
static double parse_env_double_or_default_with_status(const char *name,
                                                      double default_value,
                                                      double min_value,
                                                      double max_value,
                                                      const char **status_out);
static int parse_env_int_or_default_with_status(const char *name,
                                                int default_value,
                                                int min_value,
                                                int max_value,
                                                const char **status_out);
static int source_marker_is_cli(const char *source_marker);
static coupled_integrator_t coupled_integrator_from_env_keys(const char *name_primary,
                                                             const char *name_legacy);
static int mbd_emit_step_trace(int requested_steps,
                               double dt,
                               coupled_integrator_t integrator);

static const char *coupled_integrator_to_string(coupled_integrator_t integrator)
{
    if (integrator == COUPLED_INTEGRATOR_NEWMARK_BETA) {
        return "newmark_beta";
    }
    if (integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        return "hht_alpha";
    }
    return "unknown";
}

static fem_error_t coupled_integrator_parse(const char *text, coupled_integrator_t *integrator)
{
    CHECK_NULL(text, "coupled integrator");
    CHECK_NULL(integrator, "coupled integrator out");

    if (string_equals_ignore_case(text, "newmark_beta") ||
        string_equals_ignore_case(text, "newmark-beta") ||
        string_equals_ignore_case(text, "newmark")) {
        *integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "hht_alpha") ||
        string_equals_ignore_case(text, "hht-alpha") ||
        string_equals_ignore_case(text, "hht")) {
        *integrator = COUPLED_INTEGRATOR_HHT_ALPHA;
        return FEM_SUCCESS;
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Unknown coupled integrator '%s' (expected: newmark_beta|hht_alpha)",
                     text);
}

static coupled_integrator_t coupled_integrator_from_env(void)
{
    return coupled_integrator_from_env_keys("FEM4C_COUPLED_INTEGRATOR", NULL);
}

static coupled_integrator_t mbd_integrator_from_env(void)
{
    /* PM-3 (2026-02-08): keep MBD standalone path independent from coupled env keys. */
    return coupled_integrator_from_env_keys("FEM4C_MBD_INTEGRATOR", NULL);
}

static coupled_integrator_t coupled_integrator_from_env_keys(const char *name_primary,
                                                             const char *name_legacy)
{
    coupled_integrator_t integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    const char *env_integrator = NULL;
    const char *used_name = NULL;

    if (name_primary && name_primary[0] != '\0') {
        env_integrator = getenv(name_primary);
        if (env_integrator && env_integrator[0] != '\0') {
            used_name = name_primary;
        }
    }
    if ((!env_integrator || env_integrator[0] == '\0') &&
        name_legacy && name_legacy[0] != '\0') {
        env_integrator = getenv(name_legacy);
        if (env_integrator && env_integrator[0] != '\0') {
            used_name = name_legacy;
        }
    }

    if (!env_integrator || env_integrator[0] == '\0') {
        return integrator;
    }

    if (coupled_integrator_parse(env_integrator, &integrator) != FEM_SUCCESS) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to 'newmark_beta'\n",
                used_name ? used_name : "integrator env",
                env_integrator);
        integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    }

    return integrator;
}

static double parse_env_double_or_default(const char *name,
                                          double default_value,
                                          double min_value,
                                          double max_value)
{
    return parse_env_double_or_default_with_status(name, default_value,
                                                   min_value, max_value, NULL);
}

static double parse_env_double_or_default_with_status(const char *name,
                                                      double default_value,
                                                      double min_value,
                                                      double max_value,
                                                      const char **status_out)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    double parsed = 0.0;

    if (!env_value || env_value[0] == '\0') {
        if (status_out) {
            *status_out = MBD_SOURCE_DEFAULT;
        }
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }

    errno = 0;
    parsed = strtod(env_value, &end_ptr);
    if (end_ptr == env_value || *end_ptr != '\0' || !isfinite(parsed) || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %.6e..%.6e), fallback to %.6e\n",
                name, env_value, min_value, max_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK;
        }
        return default_value;
    }
    if (status_out) {
        *status_out = MBD_SOURCE_ENV;
    }
    return parsed;
}

static int parse_env_int_or_default_with_status(const char *name,
                                                int default_value,
                                                int min_value,
                                                int max_value,
                                                const char **status_out)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    long parsed = 0;

    if (!env_value || env_value[0] == '\0') {
        if (status_out) {
            *status_out = MBD_SOURCE_DEFAULT;
        }
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }

    errno = 0;
    parsed = strtol(env_value, &end_ptr, 10);
    if (end_ptr == env_value || *end_ptr != '\0' || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %d..%d), fallback to %d\n",
                name, env_value, min_value, max_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK;
        }
        return default_value;
    }
    if (status_out) {
        *status_out = MBD_SOURCE_ENV;
    }
    return (int)parsed;
}

static int string_equals_ignore_case(const char *lhs, const char *rhs)
{
    size_t i = 0;

    if (!lhs || !rhs) {
        return 0;
    }

    while (lhs[i] != '\0' && rhs[i] != '\0') {
        if (tolower((unsigned char)lhs[i]) != tolower((unsigned char)rhs[i])) {
            return 0;
        }
        ++i;
    }

    return lhs[i] == '\0' && rhs[i] == '\0';
}

static int source_marker_is_cli(const char *source_marker)
{
    if (!source_marker || source_marker[0] == '\0') {
        return 0;
    }
    return string_equals_ignore_case(source_marker, MBD_SOURCE_CLI);
}

static int mbd_emit_step_trace(int requested_steps,
                               double dt,
                               coupled_integrator_t integrator)
{
    int step = 0;
    int executed = 0;
    const int compact_threshold = 16;
    const char *integrator_name = coupled_integrator_to_string(integrator);

    if (requested_steps <= 0) {
        return 0;
    }

    for (step = 1; step <= requested_steps; ++step) {
        int should_print = 0;
        ++executed;

        if (requested_steps <= compact_threshold) {
            should_print = 1;
        } else if (step <= 3 || step > requested_steps - 3) {
            should_print = 1;
        }

        if (should_print) {
            const double step_time = dt * (double)step;
            printf("  mbd_step=%d/%d t=%.6e integrator=%s\n",
                   step, requested_steps, step_time, integrator_name);
        } else if (step == 4) {
            printf("  mbd_step=... (%d steps omitted for compact trace)\n",
                   requested_steps - 6);
        }
    }

    return executed;
}

static const char *skip_leading_spaces(const char *text)
{
    const char *p = text;
    while (p && *p != '\0' && isspace((unsigned char)*p)) {
        ++p;
    }
    return p;
}

static int line_starts_with_token(const char *line, const char *token)
{
    const char *p;
    size_t token_len;

    if (!line || !token) {
        return 0;
    }
    p = skip_leading_spaces(line);
    token_len = strlen(token);
    if (strncmp(p, token, token_len) != 0) {
        return 0;
    }
    return p[token_len] == '\0' || isspace((unsigned char)p[token_len]);
}

static int line_starts_with_prefix(const char *line, const char *prefix)
{
    const char *p;
    size_t prefix_len;

    if (!line || !prefix) {
        return 0;
    }
    p = skip_leading_spaces(line);
    prefix_len = strlen(prefix);
    return strncmp(p, prefix, prefix_len) == 0;
}

static void line_to_excerpt(const char *line, char *out, size_t out_size)
{
    size_t i = 0;
    if (!line || !out) {
        return;
    }

    if (out_size == 0) {
        return;
    }

    while (line[i] != '\0' && line[i] != '\n' && line[i] != '\r' && i + 1 < out_size) {
        out[i] = line[i];
        ++i;
    }
    out[i] = '\0';
}

static void mbd_setup_builtin_case(mbd_body_state2d_t *states,
                                   int *num_bodies,
                                   mbd_constraint2d_t *constraints,
                                   int *num_constraints)
{
    const double distance_anchor_i[2] = {0.0, 0.0};
    const double distance_anchor_j[2] = {0.0, 0.0};
    const double revolute_anchor_i[2] = {0.5, 0.0};
    const double revolute_anchor_j[2] = {-0.5, 0.0};

    states[0].x = 0.0;
    states[0].y = 0.0;
    states[0].theta = 0.0;
    states[1].x = 1.2;
    states[1].y = 0.3;
    states[1].theta = 0.1;

    (void)mbd_constraint_init_distance(&constraints[0], 1, 0, 1,
                                       distance_anchor_i, distance_anchor_j, 1.0);
    (void)mbd_constraint_init_revolute(&constraints[1], 2, 0, 1,
                                       revolute_anchor_i, revolute_anchor_j);

    *num_bodies = 2;
    *num_constraints = 2;
}

static int parse_body_line(const char *line,
                           mbd_body_state2d_t *state,
                           int *body_index)
{
    int idx = -1;
    double x = 0.0;
    double y = 0.0;
    double theta = 0.0;
    int scanned;

    if (!line_starts_with_token(line, "MBD_BODY")) {
        return 0;
    }
    scanned = sscanf(line, "MBD_BODY %d %lf %lf %lf", &idx, &x, &y, &theta);
    if (scanned != 4 || idx < 0) {
        return -1;
    }
    if (!isfinite(x) || !isfinite(y) || !isfinite(theta)) {
        return -1;
    }

    *body_index = idx;
    state->x = x;
    state->y = y;
    state->theta = theta;
    return 1;
}

static int parse_distance_line(const char *line, mbd_constraint2d_t *out)
{
    int id = 0;
    int bi = 0;
    int bj = 0;
    double aix = 0.0;
    double aiy = 0.0;
    double ajx = 0.0;
    double ajy = 0.0;
    double dist = 0.0;
    int scanned;

    if (!line_starts_with_token(line, "MBD_DISTANCE")) {
        return 0;
    }
    scanned = sscanf(line, "MBD_DISTANCE %d %d %d %lf %lf %lf %lf %lf",
                     &id, &bi, &bj, &aix, &aiy, &ajx, &ajy, &dist);
    if (scanned != 8) {
        return -1;
    }
    if (!isfinite(aix) || !isfinite(aiy) || !isfinite(ajx) || !isfinite(ajy) || !isfinite(dist)) {
        return -1;
    }
    if (dist <= 0.0) {
        return -1;
    }

    {
        const double ai[2] = {aix, aiy};
        const double aj[2] = {ajx, ajy};
        if (mbd_constraint_init_distance(out, id, bi, bj, ai, aj, dist) != FEM_SUCCESS) {
            return -1;
        }
    }

    return 1;
}

static int parse_revolute_line(const char *line, mbd_constraint2d_t *out)
{
    int id = 0;
    int bi = 0;
    int bj = 0;
    double aix = 0.0;
    double aiy = 0.0;
    double ajx = 0.0;
    double ajy = 0.0;
    int scanned;

    if (!line_starts_with_token(line, "MBD_REVOLUTE")) {
        return 0;
    }
    scanned = sscanf(line, "MBD_REVOLUTE %d %d %d %lf %lf %lf %lf",
                     &id, &bi, &bj, &aix, &aiy, &ajx, &ajy);
    if (scanned != 7) {
        return -1;
    }
    if (!isfinite(aix) || !isfinite(aiy) || !isfinite(ajx) || !isfinite(ajy)) {
        return -1;
    }

    {
        const double ai[2] = {aix, aiy};
        const double aj[2] = {ajx, ajy};
        if (mbd_constraint_init_revolute(out, id, bi, bj, ai, aj) != FEM_SUCCESS) {
            return -1;
        }
    }

    return 1;
}

static fem_error_t mbd_try_load_case_from_input(const char *input_filename,
                                                mbd_body_state2d_t *states,
                                                int *num_bodies,
                                                mbd_constraint2d_t *constraints,
                                                int *num_constraints,
                                                int *from_input)
{
    FILE *fp = NULL;
    char line[512];
    char excerpt[160];
    int body_seen[MBD_RUNTIME_MAX_BODIES];
    int body_defined_line[MBD_RUNTIME_MAX_BODIES];
    mbd_constraint_source_t constraint_sources[MBD_RUNTIME_MAX_CONSTRAINTS];
    int loaded_constraints = 0;
    int dropped_constraints = 0;
    int saw_mbd_entry = 0;
    int first_mbd_line = 0;
    int max_seen_defined_body = -1;
    int max_seen_referenced_body = -1;
    int line_no = 0;

    CHECK_NULL(states, "MBD body states");
    CHECK_NULL(num_bodies, "MBD num_bodies");
    CHECK_NULL(constraints, "MBD constraints");
    CHECK_NULL(num_constraints, "MBD num_constraints");
    CHECK_NULL(from_input, "MBD source flag");

    memset(body_seen, 0, sizeof(body_seen));
    for (int i = 0; i < MBD_RUNTIME_MAX_BODIES; ++i) {
        body_defined_line[i] = -1;
    }
    for (int i = 0; i < MBD_RUNTIME_MAX_CONSTRAINTS; ++i) {
        constraint_sources[i].id = -1;
        constraint_sources[i].body_i = -1;
        constraint_sources[i].body_j = -1;
        constraint_sources[i].line_no = -1;
    }
    *num_bodies = 0;
    *num_constraints = 0;
    *from_input = 0;

    fp = fopen(input_filename, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND,
                         "Cannot open MBD input file: %s",
                         input_filename);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        mbd_body_state2d_t parsed_state;
        mbd_constraint2d_t parsed_constraint;
        int body_index = -1;
        int parsed;
        ++line_no;

        if (line_starts_with_prefix(line, "MBD_")) {
            saw_mbd_entry = 1;
            if (first_mbd_line == 0) {
                first_mbd_line = line_no;
            }
        }

        parsed = parse_body_line(line, &parsed_state, &body_index);
        if (parsed == -1) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_BODY at line %d: '%s'",
                             MBD_DIAG_E_BODY_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            if (body_index >= MBD_RUNTIME_MAX_BODIES) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_BODY index %d at line %d exceeds supported range [0,%d)",
                                 MBD_DIAG_E_BODY_RANGE, body_index, line_no, MBD_RUNTIME_MAX_BODIES);
            }
            if (body_seen[body_index]) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Duplicate MBD_BODY id %d at line %d (first defined at line %d)",
                                 MBD_DIAG_E_DUP_BODY, body_index, line_no, body_defined_line[body_index]);
            }
            states[body_index] = parsed_state;
            body_seen[body_index] = 1;
            body_defined_line[body_index] = line_no;
            if (body_index > max_seen_defined_body) {
                max_seen_defined_body = body_index;
            }
            continue;
        }

        parsed = parse_distance_line(line, &parsed_constraint);
        if (parsed == -1) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_DISTANCE at line %d: '%s'",
                             MBD_DIAG_E_DISTANCE_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            if (parsed_constraint.body_i >= MBD_RUNTIME_MAX_BODIES ||
                parsed_constraint.body_j >= MBD_RUNTIME_MAX_BODIES) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_DISTANCE at line %d references body outside supported range [0,%d)",
                                 MBD_DIAG_E_DISTANCE_RANGE, line_no, MBD_RUNTIME_MAX_BODIES);
            }
            if (loaded_constraints >= MBD_RUNTIME_MAX_CONSTRAINTS) {
                ++dropped_constraints;
                continue;
            }
            constraints[loaded_constraints] = parsed_constraint;
            constraint_sources[loaded_constraints].id = parsed_constraint.id;
            constraint_sources[loaded_constraints].body_i = parsed_constraint.body_i;
            constraint_sources[loaded_constraints].body_j = parsed_constraint.body_j;
            constraint_sources[loaded_constraints].line_no = line_no;
            if (parsed_constraint.body_i > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_i;
            }
            if (parsed_constraint.body_j > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_j;
            }
            ++loaded_constraints;
            continue;
        }

        parsed = parse_revolute_line(line, &parsed_constraint);
        if (parsed == -1) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_REVOLUTE at line %d: '%s'",
                             MBD_DIAG_E_REVOLUTE_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            if (parsed_constraint.body_i >= MBD_RUNTIME_MAX_BODIES ||
                parsed_constraint.body_j >= MBD_RUNTIME_MAX_BODIES) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_REVOLUTE at line %d references body outside supported range [0,%d)",
                                 MBD_DIAG_E_REVOLUTE_RANGE, line_no, MBD_RUNTIME_MAX_BODIES);
            }
            if (loaded_constraints >= MBD_RUNTIME_MAX_CONSTRAINTS) {
                ++dropped_constraints;
                continue;
            }
            constraints[loaded_constraints] = parsed_constraint;
            constraint_sources[loaded_constraints].id = parsed_constraint.id;
            constraint_sources[loaded_constraints].body_i = parsed_constraint.body_i;
            constraint_sources[loaded_constraints].body_j = parsed_constraint.body_j;
            constraint_sources[loaded_constraints].line_no = line_no;
            if (parsed_constraint.body_i > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_i;
            }
            if (parsed_constraint.body_j > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_j;
            }
            ++loaded_constraints;
            continue;
        }

        if (line_starts_with_prefix(line, "MBD_")) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Unsupported MBD directive at line %d: '%s'",
                             MBD_DIAG_E_UNSUPPORTED_DIRECTIVE, line_no, excerpt);
        }
    }

    if (fclose(fp) != 0) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot close MBD input file after read: %s",
                         input_filename);
    }

    if (!saw_mbd_entry) {
        return FEM_SUCCESS;
    }
    if (loaded_constraints < 1) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[%s] MBD input is incomplete: no constraints found (first MBD line: %d)",
                         MBD_DIAG_E_INCOMPLETE_INPUT, first_mbd_line);
    }
    if (max_seen_referenced_body < 1) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[%s] MBD input is incomplete: constraints must reference at least body 0 and 1 (first MBD line: %d)",
                         MBD_DIAG_E_INCOMPLETE_INPUT, first_mbd_line);
    }

    for (int i = 0; i < loaded_constraints; ++i) {
        int bi = constraint_sources[i].body_i;
        int bj = constraint_sources[i].body_j;
        int cl = constraint_sources[i].line_no;
        int cid = constraint_sources[i].id;
        if (bi >= 0 && !body_seen[bi]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by constraint id %d at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF, bi, cid, cl);
        }
        if (bj >= 0 && !body_seen[bj]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by constraint id %d at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF, bj, cid, cl);
        }
    }

    if (dropped_constraints > 0) {
        printf("  mbd_constraints_dropped_by_cap: %d (cap=%d)\n",
               dropped_constraints, MBD_RUNTIME_MAX_CONSTRAINTS);
    }
    if (loaded_constraints >= 3) {
        printf("  mbd_constraint_lines_processed: %d (third+ constraints accepted)\n",
               loaded_constraints);
    }

    if (max_seen_referenced_body + 1 > MBD_RUNTIME_MAX_BODIES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[%s] MBD body count exceeds supported range [0,%d) (first MBD line: %d)",
                         MBD_DIAG_E_BODY_COUNT_RANGE, MBD_RUNTIME_MAX_BODIES, first_mbd_line);
    }

    if (max_seen_defined_body > max_seen_referenced_body) {
        *num_bodies = max_seen_defined_body + 1;
    } else {
        *num_bodies = max_seen_referenced_body + 1;
    }
    *num_constraints = loaded_constraints;
    *from_input = 1;
    return FEM_SUCCESS;
}

static fem_error_t mbd_analysis_minimal(const char *input_filename, const char *output_filename)
{
    int num_bodies = 0;
    int num_constraints = 0;
    mbd_constraint2d_t constraints[MBD_RUNTIME_MAX_CONSTRAINTS];
    mbd_body_state2d_t states[MBD_RUNTIME_MAX_BODIES];
    mbd_kkt_layout_t layout;
    coupled_integrator_t integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    double newmark_beta = 2.5e-1;
    double newmark_gamma = 5.0e-1;
    double hht_alpha = -5.0e-2;
    const char *newmark_beta_status = MBD_SOURCE_DEFAULT;
    const char *newmark_gamma_status = MBD_SOURCE_DEFAULT;
    const char *hht_alpha_status = MBD_SOURCE_DEFAULT;
    const char *newmark_beta_source_marker = NULL;
    const char *newmark_gamma_source_marker = NULL;
    const char *hht_alpha_source_marker = NULL;
    double dt = 1.0e-3;
    int num_steps = 1;
    int steps_requested = 0;
    int steps_executed = 0;
    const char *dt_status = MBD_SOURCE_DEFAULT;
    const char *steps_status = MBD_SOURCE_DEFAULT;
    const char *dt_source_marker = NULL;
    const char *steps_source_marker = NULL;
    double residual_norm_sq = 0.0;
    int num_equations = 0;
    int i;
    int from_input = 0;
    FILE *out = NULL;

    CHECK_NULL(input_filename, "MBD input filename");
    CHECK_NULL(output_filename, "MBD output filename");

    memset(states, 0, sizeof(states));
    memset(constraints, 0, sizeof(constraints));
    integrator = mbd_integrator_from_env();
    newmark_beta = parse_env_double_or_default_with_status("FEM4C_MBD_NEWMARK_BETA",
                                                           2.5e-1, 1.0e-12, 1.0,
                                                           &newmark_beta_status);
    newmark_gamma = parse_env_double_or_default_with_status("FEM4C_MBD_NEWMARK_GAMMA",
                                                            5.0e-1, 1.0e-12, 1.5,
                                                            &newmark_gamma_status);
    hht_alpha = parse_env_double_or_default_with_status("FEM4C_MBD_HHT_ALPHA",
                                                        -5.0e-2, -1.0 / 3.0, 0.0,
                                                        &hht_alpha_status);
    dt = parse_env_double_or_default_with_status("FEM4C_MBD_DT",
                                                 1.0e-3, 1.0e-12, 1.0e3,
                                                 &dt_status);
    num_steps = parse_env_int_or_default_with_status("FEM4C_MBD_STEPS",
                                                     1, 1, 1000000,
                                                     &steps_status);
    steps_requested = num_steps;
    newmark_beta_source_marker = getenv("FEM4C_MBD_NEWMARK_BETA_SOURCE");
    newmark_gamma_source_marker = getenv("FEM4C_MBD_NEWMARK_GAMMA_SOURCE");
    hht_alpha_source_marker = getenv("FEM4C_MBD_HHT_ALPHA_SOURCE");
    dt_source_marker = getenv("FEM4C_MBD_DT_SOURCE");
    steps_source_marker = getenv("FEM4C_MBD_STEPS_SOURCE");
    if (source_marker_is_cli(newmark_beta_source_marker) &&
        strcmp(newmark_beta_status, MBD_SOURCE_ENV) == 0) {
        newmark_beta_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(newmark_gamma_source_marker) &&
        strcmp(newmark_gamma_status, MBD_SOURCE_ENV) == 0) {
        newmark_gamma_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(hht_alpha_source_marker) &&
        strcmp(hht_alpha_status, MBD_SOURCE_ENV) == 0) {
        hht_alpha_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(dt_source_marker) &&
        strcmp(dt_status, MBD_SOURCE_ENV) == 0) {
        dt_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(steps_source_marker) &&
        strcmp(steps_status, MBD_SOURCE_ENV) == 0) {
        steps_status = MBD_SOURCE_CLI;
    }

    CHECK_ERROR(mbd_try_load_case_from_input(input_filename, states, &num_bodies,
                                             constraints, &num_constraints, &from_input));
    if (!from_input) {
        mbd_setup_builtin_case(states, &num_bodies, constraints, &num_constraints);
        printf("  mbd_source: builtin_fallback (input has no MBD_* entries)\n");
    } else {
        printf("  mbd_source: input_case (`MBD_BODY`/`MBD_DISTANCE`/`MBD_REVOLUTE`)\n");
        printf("  mbd_caps: max_bodies=%d max_constraints=%d\n",
               MBD_RUNTIME_MAX_BODIES, MBD_RUNTIME_MAX_CONSTRAINTS);
    }

    CHECK_ERROR(mbd_kkt_count_constraint_equations(constraints, num_constraints, &num_equations));
    CHECK_ERROR(mbd_kkt_compute_layout_from_constraints(num_bodies, constraints, num_constraints, &layout));

    for (i = 0; i < num_constraints; ++i) {
        double residual[MBD_CONSTRAINT2D_MAX_EQ];
        double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF];
        double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF];
        int eq = 0;
        int r;
        const mbd_constraint2d_t *c = &constraints[i];
        const mbd_body_state2d_t *state_i;
        const mbd_body_state2d_t *state_j;

        if (c->body_i < 0 || c->body_i >= num_bodies || c->body_j < 0 || c->body_j >= num_bodies) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Constraint body index out of range (id=%d, body_i=%d, body_j=%d)",
                             MBD_DIAG_E_CONSTRAINT_BODY_RANGE, c->id, c->body_i, c->body_j);
        }

        state_i = &states[c->body_i];
        state_j = &states[c->body_j];

        CHECK_ERROR(mbd_constraint_evaluate(c, state_i, state_j, residual, jac_i, jac_j, &eq));

        for (r = 0; r < eq; ++r) {
            residual_norm_sq += residual[r] * residual[r];
        }
    }

    printf("MBD minimal case summary:\n");
    printf("  Bodies: %d\n", num_bodies);
    printf("  Constraints: %d\n", num_constraints);
    printf("  Constraint equations: %d\n", num_equations);
    printf("  constraint_equations: %d\n", num_equations);
    printf("  KKT layout: body_dof=%d lambda_dof=%d total_dof=%d\n",
           layout.body_dof, layout.lambda_dof, layout.total_dof);
    printf("  integrator: %s\n", coupled_integrator_to_string(integrator));
    printf("  integrator_params: newmark_beta=%.6e newmark_gamma=%.6e hht_alpha=%.6e\n",
           newmark_beta, newmark_gamma, hht_alpha);
    printf("  integrator_fallback: newmark_beta=%s newmark_gamma=%s hht_alpha=%s\n",
           newmark_beta_status, newmark_gamma_status, hht_alpha_status);
    printf("  time_control: dt=%.6e steps=%d\n", dt, num_steps);
    printf("  time_fallback: dt=%s steps=%s\n", dt_status, steps_status);
    steps_executed = mbd_emit_step_trace(steps_requested, dt, integrator);
    printf("  steps_trace: requested=%d executed=%d\n",
           steps_requested, steps_executed);
    printf("  Constraint residual L2 norm: %.6e\n", sqrt(residual_norm_sq));
    printf("  residual_l2: %.6e\n", sqrt(residual_norm_sq));

    out = fopen(output_filename, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE, "Cannot open MBD output file: %s", output_filename);
    }

    fprintf(out, "# FEM4C MBD minimal output\n");
    fprintf(out, "bodies,%d\n", num_bodies);
    fprintf(out, "constraints,%d\n", num_constraints);
    fprintf(out, "constraint_equations,%d\n", num_equations);
    fprintf(out, "body_dof,%d\n", layout.body_dof);
    fprintf(out, "lambda_dof,%d\n", layout.lambda_dof);
    fprintf(out, "total_dof,%d\n", layout.total_dof);
    fprintf(out, "integrator,%s\n", coupled_integrator_to_string(integrator));
    fprintf(out, "newmark_beta,%.16e\n", newmark_beta);
    fprintf(out, "newmark_gamma,%.16e\n", newmark_gamma);
    fprintf(out, "hht_alpha,%.16e\n", hht_alpha);
    fprintf(out, "newmark_beta_source_status,%s\n", newmark_beta_status);
    fprintf(out, "newmark_gamma_source_status,%s\n", newmark_gamma_status);
    fprintf(out, "hht_alpha_source_status,%s\n", hht_alpha_status);
    fprintf(out, "dt,%.16e\n", dt);
    fprintf(out, "steps,%d\n", num_steps);
    fprintf(out, "steps_requested,%d\n", steps_requested);
    fprintf(out, "steps_executed,%d\n", steps_executed);
    fprintf(out, "dt_source_status,%s\n", dt_status);
    fprintf(out, "steps_source_status,%s\n", steps_status);
    fprintf(out, "source,%s\n", from_input ? "input" : "builtin");
    fprintf(out, "residual_l2,%.16e\n", sqrt(residual_norm_sq));

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE, "Cannot close MBD output file: %s", output_filename);
    }

    return FEM_SUCCESS;
}

static fem_error_t coupled_analysis_not_ready(const coupled_io_contract_t *io)
{
    const double dt = io ? io->time.dt : 0.0;
    const int num_steps = io ? io->time.num_steps : 0;
    const int max_iters = io ? io->time.max_coupling_iterations : 0;
    const double tol = io ? io->time.residual_tolerance : 0.0;
    const coupled_integrator_t integrator =
        io ? io->time.integrator : COUPLED_INTEGRATOR_NEWMARK_BETA;

    /*
     * TODO(A-2): wire runtime data into the coupled contract:
     *  - FEM path: io->fem.{analysis,nodes,elements,materials,num_nodes,num_elements,num_materials}
     *  - MBD path: io->mbd.{body_states,constraints,num_bodies,num_constraints}
     *  - Time path: io->time.{dt,num_steps,max_coupling_iterations,residual_tolerance,
     *                         integrator,newmark_beta,newmark_gamma,hht_alpha}
     */
    CHECK_NULL(io, "coupled io contract");

    printf("Coupled mode contract snapshot (stub):\n");
    printf("  fem: nodes=%d elements=%d materials=%d analysis_ptr=%p\n",
           io->fem.num_nodes, io->fem.num_elements, io->fem.num_materials,
           (const void *)io->fem.analysis);
    printf("  mbd: bodies=%d constraints=%d bodies_ptr=%p constraints_ptr=%p\n",
           io->mbd.num_bodies, io->mbd.num_constraints,
           (const void *)io->mbd.body_states, (const void *)io->mbd.constraints);
    printf("  time: dt=%.6e steps=%d max_iter=%d residual_tol=%.6e\n",
           dt, num_steps, max_iters, tol);
    printf("  integrator=%s\n", coupled_integrator_to_string(integrator));
    printf("  integrator_params: newmark_beta=%.6e newmark_gamma=%.6e hht_alpha=%.6e\n",
           io->time.newmark_beta, io->time.newmark_gamma, io->time.hht_alpha);

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Coupled FEM+MBD mode is not wired yet");
}

static fem_error_t coupled_seed_contract_from_input(coupled_io_contract_t *io,
                                                    const char *input_filename)
{
    fem_error_t err;
    static mbd_body_state2d_t mbd_states_snapshot[MBD_RUNTIME_MAX_BODIES];
    static mbd_constraint2d_t mbd_constraints_snapshot[MBD_RUNTIME_MAX_CONSTRAINTS];
    int mbd_num_bodies = 0;
    int mbd_num_constraints = 0;
    int mbd_from_input = 0;

    CHECK_NULL(io, "coupled io contract");
    CHECK_NULL(input_filename, "input filename");

    err = globals_initialize();
    CHECK_ERROR(err);

    err = input_read_data(input_filename);
    if (err != FEM_SUCCESS) {
        (void)globals_finalize();
        return err;
    }

    io->fem.analysis = &g_analysis;
    io->fem.num_nodes = g_num_nodes;
    io->fem.num_elements = g_num_elements;
    io->fem.num_materials = g_num_materials;

    err = mbd_try_load_case_from_input(input_filename,
                                       mbd_states_snapshot, &mbd_num_bodies,
                                       mbd_constraints_snapshot, &mbd_num_constraints,
                                       &mbd_from_input);
    if (err != FEM_SUCCESS) {
        (void)globals_finalize();
        return err;
    }

    if (!mbd_from_input) {
        mbd_setup_builtin_case(mbd_states_snapshot, &mbd_num_bodies,
                               mbd_constraints_snapshot, &mbd_num_constraints);
    }

    io->mbd.body_states = mbd_states_snapshot;
    io->mbd.constraints = mbd_constraints_snapshot;
    io->mbd.num_bodies = mbd_num_bodies;
    io->mbd.num_constraints = mbd_num_constraints;

    err = globals_finalize();
    CHECK_ERROR(err);

    return FEM_SUCCESS;
}

const char *analysis_mode_to_string(analysis_mode_t mode)
{
    if (mode == ANALYSIS_MODE_FEM) {
        return "fem";
    }
    if (mode == ANALYSIS_MODE_MBD) {
        return "mbd";
    }
    if (mode == ANALYSIS_MODE_COUPLED) {
        return "coupled";
    }
    return "unknown";
}

fem_error_t analysis_mode_parse(const char *text, analysis_mode_t *mode)
{
    CHECK_NULL(text, "analysis mode");
    CHECK_NULL(mode, "analysis mode out");

    if (string_equals_ignore_case(text, "fem") ||
        string_equals_ignore_case(text, "static")) {
        *mode = ANALYSIS_MODE_FEM;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "mbd")) {
        *mode = ANALYSIS_MODE_MBD;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "coupled")) {
        *mode = ANALYSIS_MODE_COUPLED;
        return FEM_SUCCESS;
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Unknown analysis mode '%s' (expected: fem|mbd|coupled)", text);
}

analysis_mode_t analysis_mode_from_env(void)
{
    analysis_mode_t mode = ANALYSIS_MODE_FEM;
    const char *env_mode = getenv("FEM4C_ANALYSIS_MODE");

    if (!env_mode || env_mode[0] == '\0') {
        return mode;
    }

    if (analysis_mode_parse(env_mode, &mode) != FEM_SUCCESS) {
        fprintf(stderr, "Warning: invalid FEM4C_ANALYSIS_MODE='%s', fallback to 'fem'\n", env_mode);
        mode = ANALYSIS_MODE_FEM;
    }

    return mode;
}

fem_error_t analysis_run(analysis_mode_t mode,
                         const char *input_filename,
                         const char *output_filename)
{
    CHECK_NULL(input_filename, "input filename");
    CHECK_NULL(output_filename, "output filename");

    printf("Analysis mode: %s\n\n", analysis_mode_to_string(mode));

    if (mode == ANALYSIS_MODE_FEM) {
        return static_analysis(input_filename, output_filename);
    }
    if (mode == ANALYSIS_MODE_MBD) {
        printf("MBD input adapter: enabled (input MBD_* with builtin fallback)\n");
        printf("  Received input file: %s\n", input_filename);
        printf("  Output file: %s\n", output_filename);
        return mbd_analysis_minimal(input_filename, output_filename);
    }
    if (mode == ANALYSIS_MODE_COUPLED) {
        coupled_io_contract_t io;
        fem_error_t seed_err;

        memset(&io, 0, sizeof(io));
        io.time.dt = 1.0e-3;
        io.time.num_steps = 1;
        io.time.max_coupling_iterations = 10;
        io.time.residual_tolerance = 1.0e-6;
        io.time.integrator = coupled_integrator_from_env();
        io.time.newmark_beta = parse_env_double_or_default("FEM4C_NEWMARK_BETA", 2.5e-1,
                                                           1.0e-12, 1.0);
        io.time.newmark_gamma = parse_env_double_or_default("FEM4C_NEWMARK_GAMMA", 5.0e-1,
                                                            1.0e-12, 1.5);
        io.time.hht_alpha = parse_env_double_or_default("FEM4C_HHT_ALPHA", -5.0e-2,
                                                        -1.0 / 3.0, 0.0);

        seed_err = coupled_seed_contract_from_input(&io, input_filename);
        CHECK_ERROR(seed_err);

        return coupled_analysis_not_ready(&io);
    }

    return error_set(FEM_ERROR_INVALID_INPUT, "Invalid analysis mode enum: %d", (int)mode);
}
