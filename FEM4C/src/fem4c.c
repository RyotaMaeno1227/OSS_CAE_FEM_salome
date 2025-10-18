/* FEM4C - High Performance Finite Element Method in C
 * Main program entry point
 * Based on "High Performance Finite Element Method" by Takahiro Yamada
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "common/constants.h"
#include "common/types.h"
#include "common/globals.h"
#include "common/error.h"
#include "analysis/static.h"

int main(int argc, char *argv[])
{
    fem_error_t err;
    
    printf("FEM4C - High Performance Finite Element Method in C\n");
    printf("Based on \"High Performance Finite Element Method\" by Takahiro Yamada\n");
    printf("Version 1.0 - Complete Implementation\n");
    printf("=====================================\n\n");
    
    /* Parse command line arguments */
    if (argc > 1) {
        strncpy(g_input_filename, argv[1], MAX_FILENAME_LEN-1);
        g_input_filename[MAX_FILENAME_LEN-1] = '\0';
    } else {
        strcpy(g_input_filename, "input.dat");
    }
    
    if (argc > 2) {
        strncpy(g_output_filename, argv[2], MAX_FILENAME_LEN-1);
        g_output_filename[MAX_FILENAME_LEN-1] = '\0';
    } else {
        strcpy(g_output_filename, "output.dat");
    }
    
    /* Store filenames before initialization overwrites them */
    char input_file[MAX_FILENAME_LEN];
    char output_file[MAX_FILENAME_LEN];
    strcpy(input_file, g_input_filename);
    strcpy(output_file, g_output_filename);
    
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