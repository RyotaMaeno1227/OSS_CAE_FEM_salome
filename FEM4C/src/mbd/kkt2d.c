#include "kkt2d.h"
#include "../common/error.h"

/*
 * 2D rigid-body state:
 *   q = [x, y, theta] for each body -> 3 dof/body.
 * Lagrange multipliers are stored after body dof.
 */
fem_error_t mbd_kkt_compute_layout(int num_bodies,
                                   int num_constraints,
                                   mbd_kkt_layout_t *layout)
{
    CHECK_NULL(layout, "layout");

    if (num_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "num_bodies must be positive");
    }
    if (num_constraints < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "num_constraints must be non-negative");
    }

    layout->body_dof = num_bodies * 3;
    layout->lambda_dof = num_constraints;
    layout->total_dof = layout->body_dof + layout->lambda_dof;

    return FEM_SUCCESS;
}
