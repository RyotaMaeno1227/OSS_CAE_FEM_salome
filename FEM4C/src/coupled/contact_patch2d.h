#ifndef FEM4C_COUPLED_CONTACT_PATCH2D_H
#define FEM4C_COUPLED_CONTACT_PATCH2D_H

#include "../common/constants.h"
#include "../common/types.h"

typedef struct {
    int step;
    double time;
    int pair_id;
    int active;
    double gap_macro;
    double penetration;
    double normal_world[2];
    double fn_macro;
    double cp1_world[2];
    double cp2_world[2];
} contact_patch2d_macro_trace_row_t;

typedef struct {
    int body_id;
    int pair_id;
    int step;
    double time;
    double contact_point_world[2];
    double normal_world[2];
    double tangent_world[2];
    double radius_body;
    double gap_macro;
    double fn_macro;
    double thickness;
    double patch_size;
    char mesh_path[MAX_FILENAME_LEN];
    char output_path[MAX_FILENAME_LEN];
} contact_patch2d_t;

void contact_patch2d_zero(contact_patch2d_t *patch);
void contact_patch2d_macro_trace_row_zero(contact_patch2d_macro_trace_row_t *row);

fem_error_t contact_patch2d_parse_macro_trace_csv_row(
    const char *line,
    contact_patch2d_macro_trace_row_t *row);
fem_error_t contact_patch2d_build(
    contact_patch2d_t *patch,
    int body_id,
    int pair_id,
    int step,
    double time,
    const double contact_point_world[2],
    const double normal_world[2],
    double radius_body,
    double gap_macro,
    double fn_macro,
    double thickness,
    double patch_size,
    const char *artifact_dir);
fem_error_t contact_patch2d_build_pair_from_trace_row(
    const contact_patch2d_macro_trace_row_t *row,
    int body_small_id,
    int body_large_id,
    double radius_small,
    double radius_large,
    double thickness,
    double patch_size,
    const char *artifact_dir,
    contact_patch2d_t patches[2]);
fem_error_t contact_patch2d_build_metadata_path(
    const contact_patch2d_t *patch,
    const char *artifact_dir,
    char metadata_path[MAX_FILENAME_LEN]);
fem_error_t contact_patch2d_write_metadata_json(
    const contact_patch2d_t *patch,
    const char *metadata_path);
fem_error_t contact_patch2d_world_to_local(
    const contact_patch2d_t *patch,
    const double point_world[2],
    double *s_out,
    double *u_out);

#endif /* FEM4C_COUPLED_CONTACT_PATCH2D_H */
