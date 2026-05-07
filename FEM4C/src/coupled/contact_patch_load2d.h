#ifndef FEM4C_COUPLED_CONTACT_PATCH_LOAD2D_H
#define FEM4C_COUPLED_CONTACT_PATCH_LOAD2D_H

#include "../contact/kernel/contact_patch2d.h"
#include "fem_model_copy.h"

#define CONTACT_PATCH_LOAD2D_MAX_NODES 4

typedef struct {
    int node_count;
    int node_ids[CONTACT_PATCH_LOAD2D_MAX_NODES];
    double nodal_force_local[CONTACT_PATCH_LOAD2D_MAX_NODES][2];
    double total_force_local[2];
    double total_force_world[2];
    double application_point_local[2];
} contact_patch_load2d_t;

typedef struct {
    char summary_path[MAX_FILENAME_LEN];
    char deformed_output_path[MAX_FILENAME_LEN];
    double displacement_max_norm;
    double displacement_centroid_local[2];
    double applied_force_local[2];
    double applied_force_world[2];
    double reaction_resultant_local[3];
} contact_patch_load2d_result_t;

void contact_patch_load2d_zero(contact_patch_load2d_t *load);
void contact_patch_load2d_result_zero(contact_patch_load2d_result_t *result);

fem_error_t contact_patch_load2d_build_equivalent_nodal_load(
    const contact_patch2d_t *patch,
    const fem_model_t *model,
    contact_patch_load2d_t *load);
fem_error_t contact_patch_load2d_apply_to_model(
    fem_model_t *model,
    const contact_patch_load2d_t *load);
fem_error_t contact_patch_load2d_run_fixture_static(
    const contact_patch2d_t *patch,
    contact_patch_load2d_result_t *result_out);

#endif /* FEM4C_COUPLED_CONTACT_PATCH_LOAD2D_H */
