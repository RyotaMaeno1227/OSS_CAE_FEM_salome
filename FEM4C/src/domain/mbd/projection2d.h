#ifndef FEM4C_MBD_PROJECTION2D_H
#define FEM4C_MBD_PROJECTION2D_H

#include "../../mbd/system2d.h"

#define MBD_PROJECTION2D_DEFAULT_MAX_ITERS 4
#define MBD_PROJECTION2D_DEFAULT_RESIDUAL_TOL 1.0e-12

typedef struct {
    int max_iterations;
    double residual_tolerance;
} mbd_projection2d_options_t;

typedef struct {
    mbd_kkt_layout_t layout;
    int num_equations;
    int applied;
    int target_reached;
    int iterations_applied;
    double residual_l2_before;
    double residual_l2_after;
    double residual_reduction_ratio;
    double correction_l2;
    double velocity_residual_l2_before;
    double velocity_residual_l2_after;
    double velocity_residual_reduction_ratio;
    const char *stop_reason;
} mbd_projection2d_report_t;

void mbd_projection2d_options_set_defaults(mbd_projection2d_options_t *options);
void mbd_projection2d_report_zero(mbd_projection2d_report_t *report);
fem_error_t mbd_projection2d_apply_with_options(
    mbd_system2d_t *system,
    const mbd_projection2d_options_t *options,
    mbd_projection2d_report_t *report);

fem_error_t mbd_projection2d_apply(mbd_system2d_t *system,
                                   mbd_projection2d_report_t *report);

#endif /* FEM4C_MBD_PROJECTION2D_H */
