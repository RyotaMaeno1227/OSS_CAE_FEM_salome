/* FEM4C - Finite Element Method in C
 * Main program entry point
 * 実行コマンド：./bin/fem4c.exe NastranBalkFile/3Dtria_example.dat run_out
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
#include "coupled/coupled_run2d.h"

static int path_is_file(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

static int has_suffix_ignore_case(const char *text, const char *suffix)
{
    size_t text_len = 0;
    size_t suffix_len = 0;
    size_t i = 0;

    if (!text || !suffix) {
        return 0;
    }

    text_len = strlen(text);
    suffix_len = strlen(suffix);
    if (suffix_len > text_len) {
        return 0;
    }

    for (i = 0; i < suffix_len; ++i) {
        unsigned char lhs = (unsigned char)text[text_len - suffix_len + i];
        unsigned char rhs = (unsigned char)suffix[i];
        if (tolower(lhs) != tolower(rhs)) {
            return 0;
        }
    }

    return 1;
}

static void uppercase_ascii_copy(char *dst, size_t dst_cap, const char *src)
{
    size_t i = 0;

    if (!dst || dst_cap == 0) {
        return;
    }

    if (!src) {
        dst[0] = '\0';
        return;
    }

    for (i = 0; i + 1 < dst_cap && src[i] != '\0'; ++i) {
        dst[i] = (char)toupper((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

static int line_starts_with_nastran_card(const char *line_upper, const char *card)
{
    size_t i = 0;
    size_t card_len = 0;

    if (!line_upper || !card) {
        return 0;
    }

    while (line_upper[i] == ' ' || line_upper[i] == '\t') {
        ++i;
    }
    if (line_upper[i] == '$' || line_upper[i] == '\0') {
        return 0;
    }

    card_len = strlen(card);
    if (strncmp(line_upper + i, card, card_len) != 0) {
        return 0;
    }

    return line_upper[i + card_len] == '\0' ||
           line_upper[i + card_len] == ' ' ||
           line_upper[i + card_len] == '\t' ||
           line_upper[i + card_len] == ',';
}

static int looks_like_nastran_bulk_content(const char *path)
{
    FILE *fp = NULL;
    char line[1024];
    int lines_read = 0;
    int begin_bulk_seen = 0;
    int score = 0;
    int saw_grid = 0;
    int saw_ctria3 = 0;
    int saw_ctria6 = 0;
    int saw_cquad4 = 0;
    int saw_pshell = 0;
    int saw_mat1 = 0;

    if (!path || path[0] == '\0') {
        return 0;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }

    while (lines_read < 160 && fgets(line, sizeof(line), fp)) {
        char upper[1024];

        ++lines_read;
        uppercase_ascii_copy(upper, sizeof(upper), line);

        if (!begin_bulk_seen && strstr(upper, "BEGIN BULK") != NULL) {
            begin_bulk_seen = 1;
            score += 3;
        }

        if (!saw_grid && (line_starts_with_nastran_card(upper, "GRID") ||
                          line_starts_with_nastran_card(upper, "GRID*"))) {
            saw_grid = 1;
            score += 1;
        }
        if (!saw_ctria3 && line_starts_with_nastran_card(upper, "CTRIA3")) {
            saw_ctria3 = 1;
            score += 1;
        }
        if (!saw_ctria6 && line_starts_with_nastran_card(upper, "CTRIA6")) {
            saw_ctria6 = 1;
            score += 1;
        }
        if (!saw_cquad4 &&
            (line_starts_with_nastran_card(upper, "CQUAD4") ||
             line_starts_with_nastran_card(upper, "CQUAD8"))) {
            saw_cquad4 = 1;
            score += 1;
        }
        if (!saw_pshell && line_starts_with_nastran_card(upper, "PSHELL")) {
            saw_pshell = 1;
            score += 1;
        }
        if (!saw_mat1 && line_starts_with_nastran_card(upper, "MAT1")) {
            saw_mat1 = 1;
            score += 1;
        }

        if (begin_bulk_seen && score >= 4) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return begin_bulk_seen ? score >= 4 : score >= 5;
}

static int looks_like_nastran_input(const char *path)
{
    if (!path || path[0] == '\0') {
        return 0;
    }

    if (strstr(path, "NastranBalkFile") != NULL) {
        return 1;
    }
    if (has_suffix_ignore_case(path, ".nas") || has_suffix_ignore_case(path, ".bdf")) {
        return 1;
    }
    if (has_suffix_ignore_case(path, ".txt") || has_suffix_ignore_case(path, ".dat")) {
        return looks_like_nastran_bulk_content(path);
    }
    return looks_like_nastran_bulk_content(path);
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
           text_equals_ignore_case(text, "explicit") ||
           text_equals_ignore_case(text, "newmark-beta") ||
           text_equals_ignore_case(text, "newmark") ||
           text_equals_ignore_case(text, "hht_alpha") ||
           text_equals_ignore_case(text, "hht-alpha") ||
           text_equals_ignore_case(text, "hht");
}

static int parse_coupled_scheme_option(const char *text)
{
    if (!text) {
        return 0;
    }

    return text_equals_ignore_case(text, "oneway_snapshot") ||
           text_equals_ignore_case(text, "oneway_replay_v1") ||
           text_equals_ignore_case(text, "oneway-replay-v1") ||
           text_equals_ignore_case(text, "oneway_replay") ||
           text_equals_ignore_case(text, "oneway-replay") ||
           text_equals_ignore_case(text, "oneway-snapshot") ||
           text_equals_ignore_case(text, "oneway") ||
           text_equals_ignore_case(text, "staggered_explicit") ||
           text_equals_ignore_case(text, "staggered-explicit") ||
           text_equals_ignore_case(text, "staggered") ||
           text_equals_ignore_case(text, "fixed_point_strong") ||
           text_equals_ignore_case(text, "fixed-point-strong") ||
           text_equals_ignore_case(text, "strong") ||
           text_equals_ignore_case(text, "monolithic_strong_v1") ||
           text_equals_ignore_case(text, "monolithic-strong-v1") ||
           text_equals_ignore_case(text, "monolithic_strong") ||
           text_equals_ignore_case(text, "delayed_cosim_v1_5") ||
           text_equals_ignore_case(text, "delayed-cosim-v1-5") ||
           text_equals_ignore_case(text, "delayed_cosim") ||
           text_equals_ignore_case(text, "delayed_co_sim_v1_5") ||
           text_equals_ignore_case(text, "legacy_default");
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

static int set_coupled_scheme_env(const char *scheme)
{
    return set_named_env("FEM4C_COUPLED_SCHEME", scheme);
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

static void print_usage(const char *program_name)
{
    const char *prog = program_name;

    if (!prog || prog[0] == '\0') {
        prog = "./bin/fem4c";
    }

    printf("Usage:\n");
    printf("  %s [options] <input.dat> [output.dat]\n", prog);
    printf("  %s [options] <nastran_bulk> [parser_out_root]\n", prog);
    printf("  %s [options] --parser-part=<collector> <nastran_bulk> [parser_out_root] [output.dat]\n\n", prog);

    printf("Modes:\n");
    printf("  --mode=<fem|mbd|coupled>      Analysis mode (env: FEM4C_ANALYSIS_MODE)\n\n");

    printf("Coupled surface contract:\n");
    printf("  --coupled-integrator=<explicit|newmark_beta|hht_alpha>\n");
    printf("      MBD integrator used inside coupled mode (env: FEM4C_COUPLED_INTEGRATOR)\n");
    printf("  --coupled-scheme=<oneway_snapshot|oneway_replay_v1|staggered_explicit|fixed_point_strong|monolithic_strong_v1|delayed_cosim_v1_5|legacy_default>\n");
    printf("      Coupling scheme between MBD and FEM (env: FEM4C_COUPLED_SCHEME)\n");
    printf("      official: oneway_snapshot (review alias: oneway_replay_v1)\n");
    printf("      experimental: staggered_explicit, fixed_point_strong, monolithic_strong_v1, delayed_cosim_v1_5\n");
    printf("      monolithic_strong_v1: year1 experimental comparison lane stub; fixed_point_strong != monolithic_strong_v1\n");
    printf("      delayed_cosim_v1_5: year1 experimental delayed co-simulation stub; delay semantics stay open and v2 is still undecided\n");
    printf("      legacy_default resolves explicit->staggered_explicit and newmark_beta/hht_alpha->fixed_point_strong\n");
    printf("      supported pairings: oneway_snapshot + explicit/newmark_beta/hht_alpha,\n");
    printf("                          staggered_explicit + explicit,\n");
    printf("                          fixed_point_strong + newmark_beta/hht_alpha,\n");
    printf("                          monolithic_strong_v1 + newmark_beta/hht_alpha (dedicated stub),\n");
    printf("                          delayed_cosim_v1_5 + explicit/newmark_beta/hht_alpha (dedicated stub)\n\n");

    printf("MBD mode surface contract:\n");
    printf("  --mbd-integrator=<explicit|newmark_beta|hht_alpha>\n");
    printf("      MBD-only integrator selection (env: FEM4C_MBD_INTEGRATOR)\n\n");

    printf("Shared time-integration parameters:\n");
    printf("  --newmark-beta=<value>        Coupled env: FEM4C_NEWMARK_BETA\n");
    printf("  --newmark-gamma=<value>       Coupled env: FEM4C_NEWMARK_GAMMA\n");
    printf("  --hht-alpha=<value>           Coupled env: FEM4C_HHT_ALPHA\n");
    printf("  --mbd-newmark-beta=<value>    MBD env: FEM4C_MBD_NEWMARK_BETA\n");
    printf("  --mbd-newmark-gamma=<value>   MBD env: FEM4C_MBD_NEWMARK_GAMMA\n");
    printf("  --mbd-hht-alpha=<value>       MBD env: FEM4C_MBD_HHT_ALPHA\n");
    printf("  --mbd-dt=<value>              MBD env: FEM4C_MBD_DT\n");
    printf("  --mbd-steps=<value>           MBD env: FEM4C_MBD_STEPS\n");
    printf("  --mbd-history-stride=<value>  MBD env: FEM4C_MBD_HISTORY_STRIDE\n\n");

    printf("Other options:\n");
    printf("  --parser-part=<collector>     Explicit single-part parser override; default bulk route exports all collectors\n");
    printf("  --strict-t3-orientation[=0|1] Strict T3 orientation checks (env: FEM4C_STRICT_T3_ORIENTATION)\n");
    printf("  -h, --help                    Show this help\n");
}

static int run_parser(const char *input_path, const char *outroot, const char *part)
{
#ifdef _WIN32
    const char *parser_path = "parser\\parser.exe"; /* Windowsはバックスラッシュが安全 */
    char cmd[4096];

    if (part && part[0] != '\0') {
        snprintf(cmd, sizeof(cmd),
                 "cmd /c \"\"%s\" \"%s\" \"%s\" \"--part=%s\"\"",
                 parser_path, input_path, outroot, part);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "cmd /c \"\"%s\" \"%s\" \"%s\"\"",
                 parser_path, input_path, outroot);
    }

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
        char part_arg[512];
        char *const args_without_part[] = {
            (char *)parser_path,
            (char *)input_path,
            (char *)outroot,
            NULL
        };
        char *const args_with_part[] = {
            (char *)parser_path,
            (char *)input_path,
            (char *)outroot,
            part_arg,
            NULL
        };

        /* Child process: exec parser */
        if (part && part[0] != '\0') {
            snprintf(part_arg, sizeof(part_arg), "--part=%s", part);
            execv(parser_path, args_with_part);
        } else {
            execv(parser_path, args_without_part);
        }
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
    const char *coupled_scheme_prefix = "--coupled-scheme=";
    const char *mbd_integrator_prefix = "--mbd-integrator=";
    const char *newmark_beta_prefix = "--newmark-beta=";
    const char *newmark_gamma_prefix = "--newmark-gamma=";
    const char *hht_alpha_prefix = "--hht-alpha=";
    const char *mbd_newmark_beta_prefix = "--mbd-newmark-beta=";
    const char *mbd_newmark_gamma_prefix = "--mbd-newmark-gamma=";
    const char *mbd_hht_alpha_prefix = "--mbd-hht-alpha=";
    const char *mbd_dt_prefix = "--mbd-dt=";
    const char *mbd_steps_prefix = "--mbd-steps=";
    const char *mbd_history_stride_prefix = "--mbd-history-stride=";
    const char *parser_part_prefix = "--parser-part=";
    const char *strict_t3_env = getenv("FEM4C_STRICT_T3_ORIENTATION");
    const char *coupled_integrator_cli = NULL;
    const char *coupled_scheme_cli = NULL;
    const char *mbd_integrator_cli = NULL;
    const char *parser_part_cli = NULL;
    int coupled_integrator_from_cli = 0;
    int coupled_scheme_from_cli = 0;
    int mbd_integrator_from_cli = 0;
    int newmark_beta_from_cli = 0;
    int newmark_gamma_from_cli = 0;
    int hht_alpha_from_cli = 0;
    int mbd_dt_from_cli = 0;
    int mbd_steps_from_cli = 0;
    int mbd_history_stride_from_cli = 0;
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
    while (argc > argi &&
           (strncmp(argv[argi], "--", 2) == 0 || strcmp(argv[argi], "-h") == 0)) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

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

        if (strcmp(argv[argi], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
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

        if (strcmp(argv[argi], "--parser-part") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --parser-part\n");
                return EXIT_FAILURE;
            }
            parser_part_cli = argv[argi + 1];
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], parser_part_prefix, strlen(parser_part_prefix)) == 0) {
            parser_part_cli = argv[argi] + strlen(parser_part_prefix);
            if (parser_part_cli[0] == '\0') {
                printf("Invalid empty value for --parser-part\n");
                return EXIT_FAILURE;
            }
            argi += 1;
            continue;
        }

        if (strcmp(argv[argi], "--coupled-integrator") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --coupled-integrator (expected explicit|newmark_beta|hht_alpha)\n");
                return EXIT_FAILURE;
            }
            if (!parse_coupled_integrator_option(argv[argi + 1])) {
                printf("Invalid --coupled-integrator value: %s (expected explicit|newmark_beta|hht_alpha)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            coupled_integrator_cli = argv[argi + 1];
            coupled_integrator_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--coupled-scheme") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --coupled-scheme (expected oneway_snapshot|oneway_replay_v1|staggered_explicit|fixed_point_strong|monolithic_strong_v1|delayed_cosim_v1_5|legacy_default)\n");
                return EXIT_FAILURE;
            }
            if (!parse_coupled_scheme_option(argv[argi + 1])) {
                printf("Invalid --coupled-scheme value: %s (expected oneway_snapshot|oneway_replay_v1|staggered_explicit|fixed_point_strong|monolithic_strong_v1|delayed_cosim_v1_5|legacy_default)\n",
                       argv[argi + 1]);
                return EXIT_FAILURE;
            }
            coupled_scheme_cli = argv[argi + 1];
            coupled_scheme_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strcmp(argv[argi], "--mbd-integrator") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-integrator (expected explicit|newmark_beta|hht_alpha)\n");
                return EXIT_FAILURE;
            }
            if (!parse_coupled_integrator_option(argv[argi + 1])) {
                printf("Invalid --mbd-integrator value: %s (expected explicit|newmark_beta|hht_alpha)\n",
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
                printf("Invalid --coupled-integrator value: %s (expected explicit|newmark_beta|hht_alpha)\n",
                       integrator_value);
                return EXIT_FAILURE;
            }
            coupled_integrator_cli = integrator_value;
            coupled_integrator_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], coupled_scheme_prefix, strlen(coupled_scheme_prefix)) == 0) {
            const char *scheme_value = argv[argi] + strlen(coupled_scheme_prefix);
            if (!parse_coupled_scheme_option(scheme_value)) {
                printf("Invalid --coupled-scheme value: %s (expected oneway_snapshot|oneway_replay_v1|staggered_explicit|fixed_point_strong|monolithic_strong_v1|delayed_cosim_v1_5|legacy_default)\n",
                       scheme_value);
                return EXIT_FAILURE;
            }
            coupled_scheme_cli = scheme_value;
            coupled_scheme_from_cli = 1;
            argi += 1;
            continue;
        }

        if (strncmp(argv[argi], mbd_integrator_prefix, strlen(mbd_integrator_prefix)) == 0) {
            const char *integrator_value = argv[argi] + strlen(mbd_integrator_prefix);
            if (!parse_coupled_integrator_option(integrator_value)) {
                printf("Invalid --mbd-integrator value: %s (expected explicit|newmark_beta|hht_alpha)\n",
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
            if (!parse_ranged_int_option(argv[argi + 1], 1, MBD_MAX_STEPS, &(int){0})) {
                printf("Invalid value for --mbd-steps: %s (allowed range: 1..%d)\n",
                       argv[argi + 1],
                       MBD_MAX_STEPS);
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
            if (!parse_ranged_int_option(value_text, 1, MBD_MAX_STEPS, &(int){0})) {
                printf("Invalid value for --mbd-steps: %s (allowed range: 1..%d)\n",
                       value_text,
                       MBD_MAX_STEPS);
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

        if (strcmp(argv[argi], "--mbd-history-stride") == 0) {
            if (argc <= argi + 1) {
                printf("Missing value after --mbd-history-stride\n");
                return EXIT_FAILURE;
            }
            if (!parse_ranged_int_option(argv[argi + 1], 1, MBD_MAX_STEPS, &(int){0})) {
                printf("Invalid value for --mbd-history-stride: %s (allowed range: 1..%d)\n",
                       argv[argi + 1],
                       MBD_MAX_STEPS);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_HISTORY_STRIDE", argv[argi + 1])) {
                printf("Failed to set FEM4C_MBD_HISTORY_STRIDE from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_history_stride_from_cli = 1;
            argi += 2;
            continue;
        }

        if (strncmp(argv[argi], mbd_history_stride_prefix, strlen(mbd_history_stride_prefix)) == 0) {
            const char *value_text = argv[argi] + strlen(mbd_history_stride_prefix);
            if (!parse_ranged_int_option(value_text, 1, MBD_MAX_STEPS, &(int){0})) {
                printf("Invalid value for --mbd-history-stride: %s (allowed range: 1..%d)\n",
                       value_text,
                       MBD_MAX_STEPS);
                return EXIT_FAILURE;
            }
            if (!set_named_env("FEM4C_MBD_HISTORY_STRIDE", value_text)) {
                printf("Failed to set FEM4C_MBD_HISTORY_STRIDE from CLI option\n");
                return EXIT_FAILURE;
            }
            mbd_history_stride_from_cli = 1;
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
    if (coupled_scheme_cli) {
        if (!set_coupled_scheme_env(coupled_scheme_cli)) {
            printf("Failed to set FEM4C_COUPLED_SCHEME from CLI option\n");
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
        const char *mbd_history_stride = getenv("FEM4C_MBD_HISTORY_STRIDE");
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
        const char *history_stride_source = mbd_history_stride_from_cli ? "cli" :
            ((mbd_history_stride && mbd_history_stride[0] != '\0') ? "env" : "default");

        if (!set_named_env("FEM4C_MBD_INTEGRATOR_SOURCE", integrator_source) ||
            !set_named_env("FEM4C_MBD_NEWMARK_BETA_SOURCE", newmark_beta_source) ||
            !set_named_env("FEM4C_MBD_NEWMARK_GAMMA_SOURCE", newmark_gamma_source) ||
            !set_named_env("FEM4C_MBD_HHT_ALPHA_SOURCE", hht_alpha_source) ||
            !set_named_env("FEM4C_MBD_DT_SOURCE", dt_source) ||
            !set_named_env("FEM4C_MBD_STEPS_SOURCE", steps_source) ||
            !set_named_env("FEM4C_MBD_HISTORY_STRIDE_SOURCE", history_stride_source)) {
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
        const char *part = parser_part_cli ? parser_part_cli : getenv("FEM4C_PARSE_PART");

        if (positional_count > 1) {
            outroot = argv[argi + 1];
        }

        if (part && part[0] == '\0') {
            part = NULL;
        }

        if (!part && positional_count > 2) {
            printf("Nastran bulk route no longer accepts positional part_name.\n");
            printf("Use --parser-part=<collector> for explicit single-part continuation.\n");
            return EXIT_FAILURE;
        }

        if (part && positional_count > 2) {
            strncpy(output_file, argv[argi + 2], MAX_FILENAME_LEN - 1);
            output_file[MAX_FILENAME_LEN - 1] = '\0';
        }

        if (!outroot) {
            outroot = "run_out";
        }

        printf("Detected Nastran input: %s\n", input_file);
        if (part) {
            printf("Running parser: outroot=%s part=%s\n", outroot, part);
        } else {
            printf("Running parser: outroot=%s route=multipart_manifest\n", outroot);
        }
        if (run_parser(input_file, outroot, part) != 0) {
            printf("Parser execution failed.\n");
            return EXIT_FAILURE;
        }

        if (!part) {
            printf("Parser export completed successfully.\n");
            printf("assembly_manifest=%s/assembly_manifest.json\n", outroot);
            return EXIT_SUCCESS;
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
        const char *mbd_history_stride = getenv("FEM4C_MBD_HISTORY_STRIDE");

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
        printf("MBD time source: dt=%s steps=%s history_stride=%s\n\n",
               mbd_dt_from_cli ? "cli" :
               ((mbd_dt && mbd_dt[0] != '\0') ? "env" : "default"),
               mbd_steps_from_cli ? "cli" :
               ((mbd_steps && mbd_steps[0] != '\0') ? "env" : "default"),
               mbd_history_stride_from_cli ? "cli" :
               ((mbd_history_stride && mbd_history_stride[0] != '\0') ? "env" : "default"));
    }
    if (analysis_mode == ANALYSIS_MODE_COUPLED) {
        const char *integrator = getenv("FEM4C_COUPLED_INTEGRATOR");
        const char *scheme = getenv("FEM4C_COUPLED_SCHEME");
        const char *newmark_beta = getenv("FEM4C_NEWMARK_BETA");
        const char *newmark_gamma = getenv("FEM4C_NEWMARK_GAMMA");
        const char *hht_alpha = getenv("FEM4C_HHT_ALPHA");
        const char *mbd_dt = getenv("FEM4C_MBD_DT");
        const char *mbd_steps = getenv("FEM4C_MBD_STEPS");
        coupled_integrator_t effective_integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
        coupled_scheme_t effective_scheme = COUPLED_SCHEME_FIXED_POINT_STRONG;
        const char *scheme_source = "legacy_default";
        int scheme_is_legacy_default = 1;

        if (integrator && integrator[0] != '\0' &&
            coupled_integrator_parse(integrator, &effective_integrator) != FEM_SUCCESS) {
            effective_integrator = COUPLED_INTEGRATOR_NEWMARK_BETA;
        }
        effective_scheme = (effective_integrator == COUPLED_INTEGRATOR_EXPLICIT)
            ? COUPLED_SCHEME_STAGGERED_EXPLICIT
            : COUPLED_SCHEME_FIXED_POINT_STRONG;
        if (scheme && scheme[0] != '\0') {
            if (text_equals_ignore_case(scheme, "legacy_default")) {
                scheme_source = coupled_scheme_from_cli ? "cli_legacy_default" : "env_legacy_default";
            } else if (coupled_scheme_parse(scheme, &effective_scheme) == FEM_SUCCESS) {
                scheme_source = coupled_scheme_from_cli ? "cli" : "env";
                scheme_is_legacy_default = 0;
            } else {
                scheme_source = coupled_scheme_from_cli ? "cli_invalid_fallback" : "env_invalid_fallback";
            }
        }

        printf("Coupled MBD integrator: %s\n\n",
               coupled_integrator_to_string(effective_integrator));
        printf("Coupled MBD integrator source: %s\n",
               coupled_integrator_from_cli ? "cli" :
               ((integrator && integrator[0] != '\0') ? "env" : "default"));
        printf("Coupled scheme: %s\n\n",
               coupled_scheme_to_string(effective_scheme));
        printf("Coupled scheme source: %s\n",
               scheme_source);
        if (scheme_is_legacy_default) {
            printf("Coupled scheme legacy default: explicit->staggered_explicit, newmark_beta/hht_alpha->fixed_point_strong\n");
            printf("Coupled scheme legacy resolution: integrator=%s -> scheme=%s\n",
                   coupled_integrator_to_string(effective_integrator),
                   coupled_scheme_to_string(effective_scheme));
        }
        printf("Coupled parameter source: newmark_beta=%s newmark_gamma=%s hht_alpha=%s\n\n",
               newmark_beta_from_cli ? "cli" :
               ((newmark_beta && newmark_beta[0] != '\0') ? "env" : "default"),
               newmark_gamma_from_cli ? "cli" :
               ((newmark_gamma && newmark_gamma[0] != '\0') ? "env" : "default"),
               hht_alpha_from_cli ? "cli" :
               ((hht_alpha && hht_alpha[0] != '\0') ? "env" : "default"));
        printf("Coupled time source: dt=%s steps=%s\n\n",
               mbd_dt_from_cli ? "cli" :
               ((mbd_dt && mbd_dt[0] != '\0') ? "env" : "default"),
               mbd_steps_from_cli ? "cli" :
               ((mbd_steps && mbd_steps[0] != '\0') ? "env" : "default"));
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
