#ifndef FLEX_SOLVER2D_H
#define FLEX_SOLVER2D_H

/* FEM4C - Minimal model-centric wrapper around the existing FEM globals kernel.
 * Uses fem_model_t until the coupled model type is unified across C/D tasks.
 */

#include "fem_model_copy.h"

typedef struct flex_bc2d_list flex_bc2d_list_t;

fem_error_t flex_solver2d_prepare_model(fem_model_t *model);
fem_error_t flex_solver2d_assemble_full_mesh(fem_model_t *model);
fem_error_t flex_solver2d_set_inertial_loads(fem_model_t *model,
                                             const flex_inertial_load2d_t *load);
fem_error_t flex_solver2d_clear_inertial_loads(fem_model_t *model);
fem_error_t flex_solver2d_apply_bc_entries(fem_model_t *model,
                                           const flex_bc2d_list_t *bc_list);
fem_error_t flex_solver2d_reassemble_and_solve(fem_model_t *model,
                                               fem_model_t *assembled_model);
fem_error_t flex_solver2d_compute_residual(const fem_model_t *assembled_model,
                                           const double *u,
                                           int u_size,
                                           double *residual,
                                           int residual_size);

#endif /* FLEX_SOLVER2D_H */
