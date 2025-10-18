#ifndef OUTPUT_H
#define OUTPUT_H

/* FEM4C - Output module header
 * Result output functions
 */

#include "../common/types.h"
#include <stdio.h>

/* Output format types */
typedef enum {
    OUTPUT_FORMAT_NATIVE = 0,   /* Native FEM4C format */
    OUTPUT_FORMAT_VTK,          /* VTK format for visualization */
    OUTPUT_FORMAT_TECPLOT,      /* Tecplot format */
    OUTPUT_FORMAT_NASTRAN_F06   /* Nastran .f06 format */
} output_format_t;

/* Output control structure */
typedef struct {
    output_format_t format;
    char filename[MAX_FILENAME_LEN];
    FILE *file_ptr;
    int write_displacements;
    int write_stresses;
    int write_reactions;
} output_control_t;

/* Main output functions */
fem_error_t output_write_results(const char *filename);
fem_error_t output_write_vtk_file(const char *filename);
fem_error_t output_write_nastran_f06_file(const char *filename);
fem_error_t output_export_csv(const char *filename);
fem_error_t output_open_file(output_control_t *output, const char *filename, output_format_t format);
fem_error_t output_close_file(output_control_t *output);

/* Native format writers */
fem_error_t output_write_header(output_control_t *output);
fem_error_t output_write_displacements(output_control_t *output);
fem_error_t output_write_stresses(output_control_t *output);
fem_error_t output_write_reactions(output_control_t *output);
fem_error_t output_write_summary(output_control_t *output);

/* VTK format writers */
fem_error_t output_write_vtk(output_control_t *output);
fem_error_t output_write_vtk_header(output_control_t *output);
fem_error_t output_write_vtk_nodes(output_control_t *output);
fem_error_t output_write_vtk_elements(output_control_t *output);
fem_error_t output_write_vtk_displacement_field(output_control_t *output);

/* Nastran F06 format writers */
fem_error_t output_write_nastran_f06(output_control_t *output);
fem_error_t output_write_nastran_f06_header(output_control_t *output);
fem_error_t output_write_nastran_f06_displacements(output_control_t *output);
fem_error_t output_write_nastran_f06_stresses(output_control_t *output);
fem_error_t output_write_nastran_f06_forces(output_control_t *output);

/* Utility functions */
fem_error_t output_calculate_element_stresses(void);
fem_error_t output_calculate_reactions(void);

/* Console output functions */
void output_print_summary(void);
void output_print_convergence_info(void);
void output_print_timing_info(void);

#endif /* OUTPUT_H */
