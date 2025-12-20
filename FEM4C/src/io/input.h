#ifndef INPUT_H
#define INPUT_H

/* FEM4C - Input module header
 * Data input functions
 */

#include "../common/types.h"
#include <stdio.h>

/* Input file format types */
typedef enum {
    INPUT_FORMAT_NATIVE = 0,    /* Native FEM4C format */
    INPUT_FORMAT_NASTRAN,       /* Nastran bulk format */
    INPUT_FORMAT_PARSER_PACKAGE,/* Output of parser (mesh/material/boundary dirs) */
    INPUT_FORMAT_AUTO          /* Auto-detect format */
} input_format_t;

/* Input control structure */
typedef struct {
    input_format_t format;
    char filename[MAX_FILENAME_LEN];
    FILE *file_ptr;
    int line_number;
    char current_line[256];
} input_control_t;

/* Main input functions */
fem_error_t input_read_data(const char *filename);
fem_error_t input_open_file(input_control_t *input, const char *filename);
fem_error_t input_close_file(input_control_t *input);
fem_error_t input_detect_format(input_control_t *input);

/* Native format readers */
fem_error_t input_read_header(input_control_t *input);
fem_error_t input_read_nodes(input_control_t *input);
fem_error_t input_read_elements(input_control_t *input);
fem_error_t input_read_materials(input_control_t *input);
fem_error_t input_read_boundary_conditions(input_control_t *input);
fem_error_t input_read_loads(input_control_t *input);

/* Utility functions */
fem_error_t input_skip_blank_lines(input_control_t *input);
fem_error_t input_read_line(input_control_t *input);
fem_error_t input_parse_integers(const char *line, int *values, int max_count, int *actual_count);
fem_error_t input_parse_doubles(const char *line, double *values, int max_count, int *actual_count);

/* Nastran format readers */
fem_error_t input_read_nastran_bulk(input_control_t *input);
fem_error_t input_parse_nastran_grid(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_ctria3(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_cquad4(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_ctria6(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_mat1(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_spc(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_force(input_control_t *input, const char *line);
fem_error_t input_parse_nastran_pshell(input_control_t *input, const char *line);
fem_error_t input_read_parser_package(const char *directory);

/* Nastran utility functions */
fem_error_t input_nastran_parse_fixed_format(const char *line, char fields[][9], int max_fields);
fem_error_t input_nastran_parse_free_format(const char *line, char fields[][256], int max_fields);
fem_error_t input_nastran_get_integer(const char *field, int *value);
fem_error_t input_nastran_get_double(const char *field, double *value);

/* Validation functions */
fem_error_t input_validate_nodes(void);
fem_error_t input_validate_elements(void);
fem_error_t input_validate_materials(void);

#endif /* INPUT_H */
