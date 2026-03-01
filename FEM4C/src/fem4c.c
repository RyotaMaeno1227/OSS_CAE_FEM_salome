/* FEM4C - Finite Element Method in C
 * Main program entry point
 * 実行コマンド：./bin/fem4c.exe NastranBalkFile/3Dtria_example.dat run_out part_0001 output.dat
 * 　　　　　　(PWD:FEM4C)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
extern int setenv(const char *name, const char *value, int overwrite);
#else
#include <process.h>   /* system を使うだけなら不要だが将来spawn使うなら */
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include "common/constants.h"
#include "common/types.h"
#include "common/globals.h"
#include "common/error.h"
#include "analysis/runner.h"

static int path_is_file(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

static int has_suffix(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > text_len) {
        return 0;
    }
    return strcmp(text + text_len - suffix_len, suffix) == 0;
}

static int looks_like_nastran_input(const char *path)
{
    if (strstr(path, "NastranBalkFile") != NULL) {
        return 1;
    }
    if (has_suffix(path, ".nas") || has_suffix(path, ".bdf")) {
        return 1;
    }
    return 0;
}

static int text_equals_ignore_case(const char *lhs, const char *rhs)
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

static int parse_bool_option_value(const char *text, int *value_out)
{
    if (!text || !value_out) {
        return 0;
    }

    if (strcmp(text, "1") == 0 ||
        text_equals_ignore_case(text, "true") ||
        text_equals_ignore_case(text, "yes") ||
        text_equals_ignore_case(text, "on")) {
        *value_out = 1;
        return 1;
    }

    if (strcmp(text, "0") == 0 ||
        text_equals_ignore_case(text, "false") ||
        text_equals_ignore_case(text, "no") ||
        text_equals_ignore_case(text, "off")) {
        *value_out = 0;
        return 1;
    }

    return 0;
}

static int parse_finite_double_option(const char *text, double *value_out)
{
    char *end_ptr = NULL;
    double value = 0.0;

    if (!text || !value_out) {
        return 0;
    }
    if (isspace((unsigned char)text[0])) {
        return 0;
    }

    errno = 0;
    value = strtod(text, &end_ptr);
    if (end_ptr == text || *end_ptr != '\0' || errno == ERANGE) {
        return 0;
    }
    if (!isfinite(value)) {
        return 0;
    }

    *value_out = value;
    return 1;
}

static int parse_ranged_double_option(const char *text,
                                      double min_value,
                                      double max_value,
                                      double *value_out)
{
    double value = 0.0;
    if (!parse_finite_double_option(text, &value)) {
        return 0;
    }
    if (value < min_value || value > max_value) {
        return 0;
    }
    *value_out = value;
    return 1;
}

static int parse_ranged_int_option(const char *text,
                                   int min_value,
                                   int max_value,
                                   int *value_out)
{
    char *end_ptr = NULL;
    long value = 0;

    if (!text || !value_out) {
        return 0;
    }
    if (isspace((unsigned char)text[0])) {
        return 0;
    }

    errno = 0;
    value = strtol(text, &end_ptr, 10);
    if (end_ptr == text || *end_ptr != '\0' || errno == ERANGE) {
        return 0;
    }
    if (value < min_value || value > max_value) {
        return 0;
    }
    *value_out = (int)value;
    return 1;
}

static int parse_coupled_integrator_option(const char *text)
{
    if (!text) {
        return 0;
    }

    return text_equals_ignore_case(text, "newmark_beta") ||
           text_equals_ignore_case(text, "newmark-beta") ||
           text_equals_ignore_case(text, "newmark") ||
           text_equals_ignore_case(text, "hht_alpha") ||
           text_equals_ignore_case(text, "hht-alpha") ||
           text_equals_ignore_case(text, "hht");
}

static int set_named_env(const char *name, const char *value)
{
    if (!name || !value || value[0] == '\0') {
        return 0;
    }
#ifdef _WIN32
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

static int set_coupled_integrator_env(const char *integrator)
{
    return set_named_env("FEM4C_COUPLED_INTEGRATOR", integrator);
}

static int set_mbd_integrator_env(const char *integrator)
{
    return set_named_env("FEM4C_MBD_INTEGRATOR", integrator);
}

static int set_mbd_param_env(const char *mbd_name, const char *value)
{
    return set_named_env(mbd_name, value);
}

static int set_mbd_and_coupled_param_env(const char *mbd_name,
                                         const char *coupled_name,
                                         const char *value)
{
    return set_named_env(mbd_name, value) &&
           set_named_env(coupled_name, value);
}

static int run_parser(const char *input_path, const char *outroot, const char *part)
{
#ifdef _WIN32
    const char *parser_path = "parser\\parser.exe"; /* Windowsはバックスラッシュが安全 */
    char cmd[4096];

    /* cmd.exe に確実に通すため、/c を付けて丸ごとクォート */
    snprintf(cmd, sizeof(cmd),
             "cmd /c \"\"%s\" \"%s\" \"%s\" \"%s\"\"",
             parser_path, input_path, outroot, part);

    fprintf(stderr, "[DEBUG] run_parser cmd: %s\n", cmd);

    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Parser execution failed: rc=%d\n", rc);
        return 1;
    }
    return 0;

#else
    /* POSIX path (Linux/macOS):
       - Prefer ./parser/parser if executable.
       - Use fork/execv + waitpid for robust status handling.
    */
    const char *parser_path = "./parser/parser";

    /* If not executable, try without "./" */
    if (access(parser_path, X_OK) != 0) {
        parser_path = "parser/parser";
        if (access(parser_path, X_OK) != 0) {
            fprintf(stderr, "Parser executable not found: ./parser/parser\n");
            return 1;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child process: exec parser */
        char *const args[] = {
            (char *)parser_path,
            (char *)input_path,
            (char *)outroot,
            (char *)part,
            NULL
        };
        execv(parser_path, args);
        perror("execv");
        _exit(127);
    }

    /* Parent: wait for child */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Parser failed (status=%d)\n", status);
        return 1;
    }

    return 0;
#endif
}

int main(int argc, char *argv[])
{
    fem_error_t err;
    int needs_parser = 0;
    analysis_mode_t analysis_mode = analysis_mode_from_env();
    const char *strict_t3_prefix = "--strict-t3-orientation=";
    const char *coupled_integrator_prefix = "--coupled-integrator=";
    const char *mbd_integrator_prefix = "--mbd-integrator=";
    const char *newmark_beta_prefix = "--newmark-beta=";
    const char *newmark_gamma_prefix = "--newmark-gamma=";
    const char *hht_alpha_prefix = "--hht-alpha=";
    const char *mbd_newmark_beta_prefix = "--mbd-newmark-beta=";
    const char *mbd_newmark_gamma_prefix = "--mbd-newmark-gamma=";
    const char *mbd_hht_alpha_prefix = "--mbd-hht-alpha=";
    const char *mbd_dt_prefix = "--mbd-dt=";
    const char *mbd_steps_prefix = "--mbd-steps=";
    const char *strict_t3_env = getenv("FEM4C_STRICT_T3_ORIENTATION");
    const char *coupled_integrator_cli = NULL;
    const char *mbd_integrator_cli = NULL;
    int coupled_integrator_from_cli = 0;
    int mbd_integrator_from_cli = 0;
    int newmark_beta_from_cli = 0;
    int newmark_gamma_from_cli = 0;
    int hht_alpha_from_cli = 0;
    int mbd_dt_from_cli = 0;
    int mbd_steps_from_cli = 0;
    int argi = 1;
    int positional_count;
    
    printf("FEM4C - High Performance Finite Element Method in C\n");
    printf("Based on \"Finite Element Method\"\n");
    printf("Version 1.0\n");
    printf("=====================================\n\n");
    
    if (strict_t3_env && strict_t3_env[0] != '\0') {
        if (!parse_bool_option_value(strict_t3_env, &g_t3_strict_orientation)) {
            printf("Invalid FEM4C_STRICT_T3_ORIENTATION value: %s (use 0/1/true/false)\n",
                   strict_t3_env);
            return EXIT_FAILURE;
        }
    }

    /* Parse optional CLI flags before positional args. */
    while (argc > argi && strncmp(argv[argi], "--", 2) == 0) {
        if (strcmp(argv[argi], "--mode") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mode (expected fem|mbd|coupled)\n");
                return EXIT_FAILURE;
            }
            err = analysis_mode_parse(argv[argi + 1], &analysis_mode);
            if (err != FEM_SUCCESS) {
                error_print(err);
                return EXIT_FAILURE;
            }
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], "--mode=", 7) == 0) {
            err = analysis_mode_parse(argv[argi] + 7, &analysis_mode);
            if (err != FEM_SUCCESS) {
                error_print(err);
                return EXIT_FAILURE;
            }
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--strict-t3-orientation") == 0) {
            g_t3_strict_orientation = 1;
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--no-strict-t3-orientation") == 0) {
            g_t3_strict_orientation = 0;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], strict_t3_prefix, strlen(strict_t3_prefix)) == 0) {
            if (!parse_bool_option_value(argv[argi] + strlen(strict_t3_prefix),
                                         &g_t3_strict_orientation)) {
                printf("Invalid value for --strict-t3-orientation (use 0/1/true/false)\n");
                return EXIT_FAILURE;
            }
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--coupled-integrator") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --coupled-integrator (expected newmark_beta|hht_alpha)\n");
                return EXIT_FAILURE;
            }
            if (!parse_coupled_integrator_option(argv[argi + 1])) {
                printf("Invalid --coupled-integrator value: %s (expected newmark_beta|hht_alpha)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            coupled_integrator_cli = argv[argi + 1];
            coupled_integrator_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-integrator") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-integrator (expected newmark_beta|hht_alpha)\n");
                return EXIT_FAILURE;
            }
            if (!parse_coupled_integrator_option(argv[argi + 1])) {
                printf("Invalid --mbd-integrator value: %s (expected newmark_beta|hht_alpha)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            mbd_integrator_cli = argv[argi + 1];
            mbd_integrator_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], coupled_integrator_prefix, strlen(coupled_integrator_prefix)) == 0) {
            const char *integrator_value = argv[argi] + strlen(coupled_integrator_prefix);
            if (!parse_coupled_integrator_option(integrator_value)) {
                printf("Invalid --coupled-integrator value: %s (expected newmark_beta|hht_alpha)\n",
                       integrator_value);
                return EXIT_FAILURE;
            }
            coupled_integrator_cli = integrator_value;
            coupled_integrator_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], mbd_integrator_prefix, strlen(mbd_integrator_prefix)) == 0) {
            const char *integrator_value = argv[argi] + strlen(mbd_integrator_prefix);
            if (!parse_coupled_integrator_option(integrator_value)) {
                printf("Invalid --mbd-integrator value: %s (expected newmark_beta|hht_alpha)\n",
                       integrator_value);
                return EXIT_FAILURE;
            }
            mbd_integrator_cli = integrator_value;
            mbd_integrator_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--newmark-beta") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --newmark-beta\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], 1.0e-12, 1.0, &(double){0.0})) {
                printf("Invalid value for --newmark-beta: %s (allowed range: 1e-12..1.0)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_NEWMARK_BETA",
                                               "FEM4C_NEWMARK_BETA",
                                               argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_NEWMARK_BETA/FEM4C_NEWMARK_BETA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_beta_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-newmark-beta") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-newmark-beta\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], 1.0e-12, 1.0, &(double){0.0})) {
                printf("Invalid value for --mbd-newmark-beta: %s (allowed range: 1e-12..1.0)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            /* PM-3 (2026-02-08): mbd-prefixed options must not mutate coupled env keys. */
            if (!set_mbd_param_env("FEM4C_MBD_NEWMARK_BETA", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_NEWMARK_BETA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_beta_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--newmark-gamma") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --newmark-gamma\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], 1.0e-12, 1.5, &(double){0.0})) {
                printf("Invalid value for --newmark-gamma: %s (allowed range: 1e-12..1.5)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_NEWMARK_GAMMA",
                                               "FEM4C_NEWMARK_GAMMA",
                                               argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_NEWMARK_GAMMA/FEM4C_NEWMARK_GAMMA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_gamma_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-newmark-gamma") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-newmark-gamma\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], 1.0e-12, 1.5, &(double){0.0})) {
                printf("Invalid value for --mbd-newmark-gamma: %s (allowed range: 1e-12..1.5)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_mbd_param_env("FEM4C_MBD_NEWMARK_GAMMA", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_NEWMARK_GAMMA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_gamma_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--hht-alpha") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --hht-alpha\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], -1.0 / 3.0, 0.0, &(double){0.0})) {
                printf("Invalid value for --hht-alpha: %s (allowed range: -1/3..0)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_HHT_ALPHA",
                                               "FEM4C_HHT_ALPHA",
                                               argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_HHT_ALPHA/FEM4C_HHT_ALPHA from CLI option\n");
                return EXIT_FAILURE;
            }
            hht_alpha_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-hht-alpha") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-hht-alpha\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], -1.0 / 3.0, 0.0, &(double){0.0})) {
                printf("Invalid value for --mbd-hht-alpha: %s (allowed range: -1/3..0)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_mbd_param_env("FEM4C_MBD_HHT_ALPHA", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_HHT_ALPHA from CLI option\n");
                return EXIT_FAILURE;
            }
            hht_alpha_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], newmark_beta_prefix, strlen(newmark_beta_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(newmark_beta_prefix);
            if (!parse_ranged_double_option(value_text, 1.0e-12, 1.0, &(double){0.0})) {
                printf("Invalid value for --newmark-beta: %s (allowed range: 1e-12..1.0)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_NEWMARK_BETA",
                                               "FEM4C_NEWMARK_BETA",
                                               value_text)) {
                printf("Failed to set FEM4C_MBD_NEWMARK_BETA/FEM4C_NEWMARK_BETA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_beta_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], mbd_newmark_beta_prefix, strlen(mbd_newmark_beta_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_newmark_beta_prefix);
            if (!parse_ranged_double_option(value_text, 1.0e-12, 1.0, &(double){0.0})) {
                printf("Invalid value for --mbd-newmark-beta: %s (allowed range: 1e-12..1.0)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_param_env("FEM4C_MBD_NEWMARK_BETA", value_text)) {
                printf("Failed to set FEM4C_MBD_NEWMARK_BETA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_beta_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], newmark_gamma_prefix, strlen(newmark_gamma_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(newmark_gamma_prefix);
            if (!parse_ranged_double_option(value_text, 1.0e-12, 1.5, &(double){0.0})) {
                printf("Invalid value for --newmark-gamma: %s (allowed range: 1e-12..1.5)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_NEWMARK_GAMMA",
                                               "FEM4C_NEWMARK_GAMMA",
                                               value_text)) {
                printf("Failed to set FEM4C_MBD_NEWMARK_GAMMA/FEM4C_NEWMARK_GAMMA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_gamma_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], mbd_newmark_gamma_prefix, strlen(mbd_newmark_gamma_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_newmark_gamma_prefix);
            if (!parse_ranged_double_option(value_text, 1.0e-12, 1.5, &(double){0.0})) {
                printf("Invalid value for --mbd-newmark-gamma: %s (allowed range: 1e-12..1.5)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_param_env("FEM4C_MBD_NEWMARK_GAMMA", value_text)) {
                printf("Failed to set FEM4C_MBD_NEWMARK_GAMMA from CLI option\n");
                return EXIT_FAILURE;
            }
            newmark_gamma_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], hht_alpha_prefix, strlen(hht_alpha_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(hht_alpha_prefix);
            if (!parse_ranged_double_option(value_text, -1.0 / 3.0, 0.0, &(double){0.0})) {
                printf("Invalid value for --hht-alpha: %s (allowed range: -1/3..0)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_and_coupled_param_env("FEM4C_MBD_HHT_ALPHA",
                                               "FEM4C_HHT_ALPHA",
                                               value_text)) {
                printf("Failed to set FEM4C_MBD_HHT_ALPHA/FEM4C_HHT_ALPHA from CLI option\n");
                return EXIT_FAILURE;
            }
            hht_alpha_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], mbd_hht_alpha_prefix, strlen(mbd_hht_alpha_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_hht_alpha_prefix);
            if (!parse_ranged_double_option(value_text, -1.0 / 3.0, 0.0, &(double){0.0})) {
                printf("Invalid value for --mbd-hht-alpha: %s (allowed range: -1/3..0)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_mbd_param_env("FEM4C_MBD_HHT_ALPHA", value_text)) {
                printf("Failed to set FEM4C_MBD_HHT_ALPHA from CLI option\n");
                return EXIT_FAILURE;
            }
            hht_alpha_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-dt") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-dt\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_double_option(argv[argi + 1], 1.0e-12, 1.0e3, &(double){0.0})) {
                printf("Invalid value for --mbd-dt: %s (allowed range: 1e-12..1e3)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_DT", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_DT from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_dt_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], mbd_dt_prefix, strlen(mbd_dt_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_dt_prefix);
            if (!parse_ranged_double_option(value_text, 1.0e-12, 1.0e3, &(double){0.0})) {
                printf("Invalid value for --mbd-dt: %s (allowed range: 1e-12..1e3)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_DT", value_text)) {
                printf("Failed to set FEM4C_MBD_DT from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_dt_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-steps") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-steps\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_int_option(argv[argi + 1], 1, 1000000, &(int){0})) {
                printf("Invalid value for --mbd-steps: %s (allowed range: 1..1000000)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_STEPS", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_STEPS from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_steps_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], mbd_steps_prefix, strlen(mbd_steps_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_steps_prefix);
            if (!parse_ranged_int_option(value_text, 1, 1000000, &(int){0})) {
                printf("Invalid value for --mbd-steps: %s (allowed range: 1..1000000)\n",
                       value_text);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_STEPS", value_text)) {
                printf("Failed to set FEM4C_MBD_STEPS from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_steps_from_cli = 1;
            argi += 1;
            continue;
        }

        break;
    }

    if (coupled_integrator_cli) {
        if (!set_coupled_integrator_env(coupled_integrator_cli)) {
            printf("Failed to set FEM4C_COUPLED_INTEGRATOR from CLI option\n");
            return EXIT_FAILURE;
        }
    }
    if (mbd_integrator_cli) {
        if (!set_mbd_integrator_env(mbd_integrator_cli)) {
            printf("Failed to set FEM4C_MBD_INTEGRATOR from CLI option\n");
            return EXIT_FAILURE;
        }
    }
    if (analysis_mode == ANALYSIS_MODE_MBD) {
        const char *mbd_integrator_env = getenv("FEM4C_MBD_INTEGRATOR");
        const char *mbd_newmark_beta = getenv("FEM4C_MBD_NEWMARK_BETA");
        const char *mbd_newmark_gamma = getenv("FEM4C_MBD_NEWMARK_GAMMA");
        const char *mbd_hht_alpha = getenv("FEM4C_MBD_HHT_ALPHA");
        const char *mbd_dt = getenv("FEM4C_MBD_DT");
        const char *mbd_steps = getenv("FEM4C_MBD_STEPS");
        const char *integrator_source = mbd_integrator_from_cli ? "cli" :
            ((mbd_integrator_env && mbd_integrator_env[0] != '\0') ? "env" : "default");
        const char *newmark_beta_source = newmark_beta_from_cli ? "cli" :
            ((mbd_newmark_beta && mbd_newmark_beta[0] != '\0') ? "env" : "default");
        const char *newmark_gamma_source = newmark_gamma_from_cli ? "cli" :
            ((mbd_newmark_gamma && mbd_newmark_gamma[0] != '\0') ? "env" : "default");
        const char *hht_alpha_source = hht_alpha_from_cli ? "cli" :
            ((mbd_hht_alpha && mbd_hht_alpha[0] != '\0') ? "env" : "default");
        const char *dt_source = mbd_dt_from_cli ? "cli" :
            ((mbd_dt && mbd_dt[0] != '\0') ? "env" : "default");
        const char *steps_source = mbd_steps_from_cli ? "cli" :
            ((mbd_steps && mbd_steps[0] != '\0') ? "env" : "default");

        if (!set_named_env("FEM4C_MBD_INTEGRATOR_SOURCE", integrator_source) ||
            !set_named_env("FEM4C_MBD_NEWMARK_BETA_SOURCE", newmark_beta_source) ||
            !set_named_env("FEM4C_MBD_NEWMARK_GAMMA_SOURCE", newmark_gamma_source) ||
            !set_named_env("FEM4C_MBD_HHT_ALPHA_SOURCE", hht_alpha_source) ||
            !set_named_env("FEM4C_MBD_DT_SOURCE", dt_source) ||
            !set_named_env("FEM4C_MBD_STEPS_SOURCE", steps_source)) {
            printf("Failed to set MBD source metadata environment keys from CLI context\n");
            return EXIT_FAILURE;
        }
    }

    positional_count = argc - argi;

    /* Parse positional arguments */
    if (positional_count > 0) {
        strncpy(g_input_filename, argv[argi], MAX_FILENAME_LEN - 1);
        g_input_filename[MAX_FILENAME_LEN - 1] = '\0';
    } else {
        strcpy(g_input_filename, "input.dat");
    }

    if (positional_count > 1) {
        strncpy(g_output_filename, argv[argi + 1], MAX_FILENAME_LEN - 1);
        g_output_filename[MAX_FILENAME_LEN - 1] = '\0';
    } else {
        strcpy(g_output_filename, "output.dat");
    }

    if (path_is_file(g_input_filename)) {
        const char *force_parser = getenv("FEM4C_FORCE_PARSER");
        if ((force_parser && strcmp(force_parser, "1") == 0) ||
            positional_count >= 3 ||
            looks_like_nastran_input(g_input_filename)) {
            needs_parser = 1;
        }
    }

    /* Store filenames before initialization overwrites them */
    char input_file[MAX_FILENAME_LEN];
    char output_file[MAX_FILENAME_LEN];
    strcpy(input_file, g_input_filename);
    strcpy(output_file, g_output_filename);

    if (needs_parser) {
        const char *outroot = getenv("FEM4C_PARSE_OUTROOT");
        const char *part = getenv("FEM4C_PARSE_PART");

        if (positional_count > 1) {
            outroot = argv[argi + 1];
            if (positional_count > 2) {
                part = argv[argi + 2];
            }
            if (positional_count > 3) {
                strncpy(output_file, argv[argi + 3], MAX_FILENAME_LEN - 1);
                output_file[MAX_FILENAME_LEN - 1] = '\0';
            }
        }

        if (!outroot) {
            outroot = "run_out";
        }
        if (!part) {
            part = "part_0001";
        }

        printf("Detected Nastran input: %s\n", input_file);
        printf("Running parser: outroot=%s part=%s\n", outroot, part);
        if (run_parser(input_file, outroot, part) != 0) {
            printf("Parser execution failed.\n");
            return EXIT_FAILURE;
        }

        snprintf(input_file, sizeof(input_file), "%s/%s", outroot, part);
    }
    
    printf("Input file:  %s\n", input_file);
    printf("Output file: %s\n\n", output_file);
    if (analysis_mode == ANALYSIS_MODE_MBD) {
        const char *integrator = getenv("FEM4C_MBD_INTEGRATOR");
        const char *newmark_beta = getenv("FEM4C_MBD_NEWMARK_BETA");
        const char *newmark_gamma = getenv("FEM4C_MBD_NEWMARK_GAMMA");
        const char *hht_alpha = getenv("FEM4C_MBD_HHT_ALPHA");
        const char *mbd_dt = getenv("FEM4C_MBD_DT");
        const char *mbd_steps = getenv("FEM4C_MBD_STEPS");

        printf("MBD integrator: %s\n\n",
               (integrator && integrator[0] != '\0') ? integrator : "newmark_beta (default)");
        printf("MBD integrator source: %s\n",
               mbd_integrator_from_cli ? "cli" :
               ((integrator && integrator[0] != '\0') ? "env" : "default"));
        printf("MBD parameter source: newmark_beta=%s newmark_gamma=%s hht_alpha=%s\n\n",
               newmark_beta_from_cli ? "cli" :
               ((newmark_beta && newmark_beta[0] != '\0') ? "env" : "default"),
               newmark_gamma_from_cli ? "cli" :
               ((newmark_gamma && newmark_gamma[0] != '\0') ? "env" : "default"),
               hht_alpha_from_cli ? "cli" :
               ((hht_alpha && hht_alpha[0] != '\0') ? "env" : "default"));
        printf("MBD time source: dt=%s steps=%s\n\n",
               mbd_dt_from_cli ? "cli" :
               ((mbd_dt && mbd_dt[0] != '\0') ? "env" : "default"),
               mbd_steps_from_cli ? "cli" :
               ((mbd_steps && mbd_steps[0] != '\0') ? "env" : "default"));
    }
    if (analysis_mode == ANALYSIS_MODE_COUPLED) {
        const char *integrator = getenv("FEM4C_COUPLED_INTEGRATOR");
        const char *newmark_beta = getenv("FEM4C_NEWMARK_BETA");
        const char *newmark_gamma = getenv("FEM4C_NEWMARK_GAMMA");
        const char *hht_alpha = getenv("FEM4C_HHT_ALPHA");

        printf("Coupled integrator: %s\n\n",
               (integrator && integrator[0] != '\0') ? integrator : "newmark_beta (default)");
        printf("Coupled integrator source: %s\n",
               coupled_integrator_from_cli ? "cli" :
               ((integrator && integrator[0] != '\0') ? "env" : "default"));
        printf("Coupled parameter source: newmark_beta=%s newmark_gamma=%s hht_alpha=%s\n\n",
               newmark_beta_from_cli ? "cli" :
               ((newmark_beta && newmark_beta[0] != '\0') ? "env" : "default"),
               newmark_gamma_from_cli ? "cli" :
               ((newmark_gamma && newmark_gamma[0] != '\0') ? "env" : "default"),
               hht_alpha_from_cli ? "cli" :
               ((hht_alpha && hht_alpha[0] != '\0') ? "env" : "default"));
    }
    printf("T3 orientation strict mode: %s\n\n",
           g_t3_strict_orientation ? "Enabled" : "Disabled");
    
#ifdef _OPENMP
    printf("OpenMP support: Enabled\n");
    printf("Max threads: %d\n\n", omp_get_max_threads());
#else
    printf("OpenMP support: Disabled\n\n");
#endif
    
    /* Run selected analysis mode */
    err = analysis_run(analysis_mode, input_file, output_file);
    
    if (err != FEM_SUCCESS) {
        error_print(err);
        printf("\nAnalysis failed with error code: %d\n", err);
        return EXIT_FAILURE;
    }
    
    printf("Program completed successfully.\n");
    return EXIT_SUCCESS;
}
