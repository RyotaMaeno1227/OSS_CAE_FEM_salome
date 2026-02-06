/* FEM4C - Finite Element Method in C
 * Main program entry point
 * 実行コマンド：./bin/fem4c.exe NastranBalkFile/3Dtria_example.dat run_out part_0001 output.dat
 * 　　　　　　(PWD:FEM4C)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
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
#include "analysis/static.h"

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
    
    printf("FEM4C - High Performance Finite Element Method in C\n");
    printf("Based on \"Finite Element Method\"\n");
    printf("Version 1.0\n");
    printf("=====================================\n\n");
    
    /* Parse command line arguments */
    if (argc > 1) {
        strncpy(g_input_filename, argv[1], MAX_FILENAME_LEN - 1);
        g_input_filename[MAX_FILENAME_LEN - 1] = '\0';
    } else {
        strcpy(g_input_filename, "input.dat");
    }

    if (argc > 2) {
        strncpy(g_output_filename, argv[2], MAX_FILENAME_LEN - 1);
        g_output_filename[MAX_FILENAME_LEN - 1] = '\0';
    } else {
        strcpy(g_output_filename, "output.dat");
    }

    if (path_is_file(g_input_filename)) {
        const char *force_parser = getenv("FEM4C_FORCE_PARSER");
        if ((force_parser && strcmp(force_parser, "1") == 0) ||
            argc >= 4 ||
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

        if (argc > 2) {
            outroot = argv[2];
            if (argc > 3) {
                part = argv[3];
            }
            if (argc > 4) {
                strncpy(output_file, argv[4], MAX_FILENAME_LEN - 1);
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
    
#ifdef _OPENMP
    printf("OpenMP support: Enabled\n");
    printf("Max threads: %d\n\n", omp_get_max_threads());
#else
    printf("OpenMP support: Disabled\n\n");
#endif
    
    /* Run static analysis */
    err = static_analysis(input_file, output_file);
    
    if (err != FEM_SUCCESS) {
        error_print(err);
        printf("\nAnalysis failed with error code: %d\n", err);
        return EXIT_FAILURE;
    }
    
    printf("Program completed successfully.\n");
    return EXIT_SUCCESS;
}
