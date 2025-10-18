#ifndef STATIC_H
#define STATIC_H

/* FEM4C - Static Analysis Functions
 * Linear static structural analysis
 */

#include "../common/types.h"

/* Main static analysis function */
fem_error_t static_analysis(const char* input_filename, const char* output_filename);

/* Analysis phases */
fem_error_t static_analysis_initialize(void);
fem_error_t static_analysis_preprocessing(const char* input_filename);
fem_error_t static_analysis_solve(void);
fem_error_t static_analysis_postprocessing(const char* output_filename);
fem_error_t static_analysis_finalize(void);

/* Analysis workflow functions */
fem_error_t static_assemble_system(void);
fem_error_t static_solve_equations(void);
fem_error_t static_calculate_stresses(void);
fem_error_t static_write_results(const char* output_filename);

/* Validation and verification */
fem_error_t static_validate_input(void);
fem_error_t static_check_equilibrium(void);

#endif /* STATIC_H */