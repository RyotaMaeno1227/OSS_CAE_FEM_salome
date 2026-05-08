#include "runner.h"
#include "static.h"
#include "../coupled/coupled_run2d.h"
#include "../mbd/system/system2d.h"
#include "../common/error.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int string_equals_ignore_case(const char *lhs, const char *rhs);
static void analysis_log_coupled_runtime_request(void);

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

static void analysis_log_coupled_runtime_request(void)
{
    const char *integrator = getenv("FEM4C_COUPLED_INTEGRATOR");
    const char *scheme = getenv("FEM4C_COUPLED_SCHEME");

    printf("Coupled runtime request:\n");
    printf("  mbd_integrator=%s\n",
           (integrator && integrator[0] != '\0') ? integrator : "newmark_beta (default)");
    printf("  coupling_scheme=%s\n",
           (scheme && scheme[0] != '\0') ? scheme : "legacy_default");
    if (!scheme || scheme[0] == '\0') {
        printf("  coupling_scheme_note=legacy_default resolves from mbd_integrator\n");
    } else if (string_equals_ignore_case(scheme, "legacy_default")) {
        printf("  coupling_scheme_note=requested legacy_default resolves from mbd_integrator\n");
    } else {
        printf("  coupling_scheme_note=explicitly requested coupling scheme\n");
    }
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
                     "Unknown analysis mode '%s' (expected: fem|mbd|coupled)",
                     text);
}

analysis_mode_t analysis_mode_from_env(void)
{
    analysis_mode_t mode = ANALYSIS_MODE_FEM;
    const char *env_mode = getenv("FEM4C_ANALYSIS_MODE");

    if (!env_mode || env_mode[0] == '\0') {
        return mode;
    }

    if (analysis_mode_parse(env_mode, &mode) != FEM_SUCCESS) {
        fprintf(stderr,
                "Warning: invalid FEM4C_ANALYSIS_MODE='%s', fallback to 'fem'\n",
                env_mode);
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
        return mbd_system2d_run(input_filename, output_filename);
    }
    if (mode == ANALYSIS_MODE_COUPLED) {
        analysis_log_coupled_runtime_request();
        return coupled_run2d(input_filename, output_filename);
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Invalid analysis mode enum: %d",
                     (int)mode);
}
