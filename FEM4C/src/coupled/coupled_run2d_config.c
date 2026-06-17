#include "coupled_run2d.h"

#include "../common/constants.h"
#include "../common/error.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_env_int_or_default(const char *name,
                                    int default_value,
                                    int min_value,
                                    int max_value);
static double parse_env_double_or_default(const char *name,
                                          double default_value,
                                          double min_value,
                                          double max_value);
static int string_equals_ignore_case(const char *lhs, const char *rhs);
static coupled_scheme_t coupled_scheme_legacy_default_from_integrator(
    coupled_integrator_t integrator);
static fem_error_t coupled_time_control_validate_contract(
    const coupled_time_control_t *time);

void coupled_time_control_set_defaults(coupled_time_control_t *time)
{
    if (!time) {
        return;
    }

    memset(time, 0, sizeof(*time));
    time->dt = 1.0e-3;
    time->num_steps = 1;
    time->max_coupling_iterations = 12;
    time->residual_tolerance = 1.0e-6;
    time->integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    time->scheme = COUPLED_SCHEME_FIXED_POINT_STRONG;
    time->scheme_is_legacy_default = 1;
    time->newmark_beta = 2.5e-1;
    time->newmark_gamma = 5.0e-1;
    time->hht_alpha = -5.0e-2;
    time->marker_relaxation = 6.2e-1;
}

static int parse_env_int_or_default(const char *name,
                                    int default_value,
                                    int min_value,
                                    int max_value)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    long parsed = 0;

    if (!env_value || env_value[0] == '\0') {
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        return default_value;
    }

    errno = 0;
    parsed = strtol(env_value, &end_ptr, 10);
    if (end_ptr == env_value || *end_ptr != '\0' || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %d..%d), fallback to %d\n",
                name, env_value, min_value, max_value, default_value);
        return default_value;
    }

    return (int)parsed;
}

static double parse_env_double_or_default(const char *name,
                                          double default_value,
                                          double min_value,
                                          double max_value)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    double parsed = 0.0;

    if (!env_value || env_value[0] == '\0') {
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        return default_value;
    }

    errno = 0;
    parsed = strtod(env_value, &end_ptr);
    if (end_ptr == env_value || *end_ptr != '\0' || !isfinite(parsed) || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %.6e..%.6e), fallback to %.6e\n",
                name, env_value, min_value, max_value, default_value);
        return default_value;
    }
    return parsed;
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

const char *coupled_integrator_to_string(coupled_integrator_t integrator)
{
    if (integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        return "explicit";
    }
    if (integrator == COUPLED_INTEGRATOR_NEWMARK_BETA) {
        return "newmark_beta";
    }
    if (integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        return "hht_alpha";
    }
    return "unknown";
}

const char *coupled_scheme_to_string(coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "oneway_snapshot";
    }
    if (scheme == COUPLED_SCHEME_STAGGERED_EXPLICIT) {
        return "staggered_explicit";
    }
    if (scheme == COUPLED_SCHEME_FIXED_POINT_STRONG) {
        return "fixed_point_strong";
    }
    if (scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        return "monolithic_strong_v1";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "delayed_cosim_v1_5";
    }
    return "unknown";
}

fem_error_t coupled_integrator_parse(const char *text,
                                     coupled_integrator_t *integrator)
{
    CHECK_NULL(text, "coupled integrator");
    CHECK_NULL(integrator, "coupled integrator out");

    if (string_equals_ignore_case(text, "explicit")) {
        *integrator = COUPLED_INTEGRATOR_EXPLICIT;
        return FEM_SUCCESS;
    }
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
                     "Unknown coupled integrator '%s' (expected: explicit|newmark_beta|hht_alpha)",
                     text);
}

fem_error_t coupled_scheme_parse(const char *text,
                                 coupled_scheme_t *scheme)
{
    CHECK_NULL(text, "coupled scheme");
    CHECK_NULL(scheme, "coupled scheme out");

    if (string_equals_ignore_case(text, "oneway_snapshot") ||
        string_equals_ignore_case(text, "oneway_replay_v1") ||
        string_equals_ignore_case(text, "oneway-replay-v1") ||
        string_equals_ignore_case(text, "oneway_replay") ||
        string_equals_ignore_case(text, "oneway-replay") ||
        string_equals_ignore_case(text, "oneway-snapshot") ||
        string_equals_ignore_case(text, "oneway")) {
        *scheme = COUPLED_SCHEME_ONEWAY_SNAPSHOT;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "staggered_explicit") ||
        string_equals_ignore_case(text, "staggered-explicit") ||
        string_equals_ignore_case(text, "staggered")) {
        *scheme = COUPLED_SCHEME_STAGGERED_EXPLICIT;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "fixed_point_strong") ||
        string_equals_ignore_case(text, "fixed-point-strong") ||
        string_equals_ignore_case(text, "strong")) {
        *scheme = COUPLED_SCHEME_FIXED_POINT_STRONG;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "monolithic_strong_v1") ||
        string_equals_ignore_case(text, "monolithic-strong-v1") ||
        string_equals_ignore_case(text, "monolithic_strong")) {
        *scheme = COUPLED_SCHEME_MONOLITHIC_STRONG_V1;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "delayed_cosim_v1_5") ||
        string_equals_ignore_case(text, "delayed-cosim-v1-5") ||
        string_equals_ignore_case(text, "delayed_cosim") ||
        string_equals_ignore_case(text, "delayed_co_sim_v1_5")) {
        *scheme = COUPLED_SCHEME_DELAYED_COSIM_V1_5;
        return FEM_SUCCESS;
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Unknown coupled scheme '%s' (expected: oneway_snapshot|oneway_replay_v1|staggered_explicit|fixed_point_strong|monolithic_strong_v1|delayed_cosim_v1_5|legacy_default)",
                     text);
}

coupled_integrator_t coupled_integrator_from_env(void)
{
    coupled_integrator_t integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    const char *env_integrator = getenv("FEM4C_COUPLED_INTEGRATOR");

    if (!env_integrator || env_integrator[0] == '\0') {
        return integrator;
    }

    if (coupled_integrator_parse(env_integrator, &integrator) != FEM_SUCCESS) {
        fprintf(stderr,
                "Warning: invalid FEM4C_COUPLED_INTEGRATOR='%s', fallback to 'newmark_beta'\n",
                env_integrator);
        integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
    }

    return integrator;
}

static coupled_scheme_t coupled_scheme_legacy_default_from_integrator(
    coupled_integrator_t integrator)
{
    if (integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        return COUPLED_SCHEME_STAGGERED_EXPLICIT;
    }
    return COUPLED_SCHEME_FIXED_POINT_STRONG;
}

fem_error_t coupled_time_control_from_env(coupled_time_control_t *time)
{
    const char *env_scheme = NULL;
    fem_error_t scheme_err = FEM_SUCCESS;

    CHECK_NULL(time, "coupled time control");

    coupled_time_control_set_defaults(time);
    time->integrator = coupled_integrator_from_env();
    time->scheme = coupled_scheme_legacy_default_from_integrator(time->integrator);
    time->scheme_is_legacy_default = 1;

    env_scheme = getenv("FEM4C_COUPLED_SCHEME");
    if (env_scheme && env_scheme[0] != '\0') {
        if (string_equals_ignore_case(env_scheme, "legacy_default")) {
            time->scheme = coupled_scheme_legacy_default_from_integrator(time->integrator);
            time->scheme_is_legacy_default = 1;
        } else if ((scheme_err = coupled_scheme_parse(env_scheme, &time->scheme)) != FEM_SUCCESS) {
            fprintf(stderr,
                    "Warning: invalid FEM4C_COUPLED_SCHEME='%s', fallback to legacy_default (%s via integrator=%s)\n",
                    env_scheme,
                    coupled_scheme_to_string(coupled_scheme_legacy_default_from_integrator(time->integrator)),
                    coupled_integrator_to_string(time->integrator));
            time->scheme = coupled_scheme_legacy_default_from_integrator(time->integrator);
            time->scheme_is_legacy_default = 1;
        } else {
            time->scheme_is_legacy_default = 0;
        }
    }
    time->dt = parse_env_double_or_default("FEM4C_MBD_DT",
                                           1.0e-3, 1.0e-12, 1.0e+3);
    time->num_steps = parse_env_int_or_default("FEM4C_MBD_STEPS",
                                               1, 1, MBD_MAX_STEPS);
    time->max_coupling_iterations = parse_env_int_or_default(
        "FEM4C_COUPLED_MAX_ITERATIONS",
        12, 1, 1000);
    time->residual_tolerance = parse_env_double_or_default(
        "FEM4C_COUPLED_RESIDUAL_TOLERANCE",
        1.0e-6, 1.0e-16, 1.0e+6);
    time->newmark_beta = parse_env_double_or_default("FEM4C_NEWMARK_BETA",
                                                     2.5e-1, 1.0e-12, 1.0);
    time->newmark_gamma = parse_env_double_or_default("FEM4C_NEWMARK_GAMMA",
                                                      5.0e-1, 1.0e-12, 1.5);
    time->hht_alpha = parse_env_double_or_default("FEM4C_HHT_ALPHA",
                                                  -5.0e-2, -1.0 / 3.0, 0.0);
    time->marker_relaxation = parse_env_double_or_default("FEM4C_COUPLED_MARKER_RELAXATION",
                                                          6.2e-1, 1.0e-3, 1.0);

    return coupled_time_control_validate_contract(time);
}

static fem_error_t coupled_time_control_validate_contract(
    const coupled_time_control_t *time)
{
    CHECK_NULL(time, "coupled time control");

    if (time->scheme == COUPLED_SCHEME_STAGGERED_EXPLICIT &&
        time->integrator != COUPLED_INTEGRATOR_EXPLICIT) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled scheme 'staggered_explicit' currently requires integrator=explicit");
    }
    if (time->scheme == COUPLED_SCHEME_FIXED_POINT_STRONG &&
        time->integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled scheme 'fixed_point_strong' currently requires integrator=newmark_beta or hht_alpha");
    }
    if (time->scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1 &&
        time->integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled scheme 'monolithic_strong_v1' currently supports integrator=newmark_beta or hht_alpha only");
    }

    return FEM_SUCCESS;
}
