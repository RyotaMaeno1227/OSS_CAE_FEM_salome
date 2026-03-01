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

fem_error_t mbd_kkt_count_constraint_equations(const mbd_constraint2d_t *constraints,
                                               int num_constraints,
                                               int *num_equations)
{
    int total = 0;

    CHECK_NULL(num_equations, "num_equations");
    *num_equations = 0;

    if (num_constraints < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "num_constraints must be non-negative");
    }
    if (num_constraints == 0) {
        return FEM_SUCCESS;
    }

    CHECK_NULL(constraints, "constraints");

    for (int i = 0; i < num_constraints; ++i) {
        CHECK_ERROR(mbd_constraint_validate(&constraints[i]));
        total += mbd_constraint_equation_count(&constraints[i]);
    }

    *num_equations = total;
    return FEM_SUCCESS;
}

fem_error_t mbd_kkt_compute_layout_from_constraints(int num_bodies,
                                                    const mbd_constraint2d_t *constraints,
                                                    int num_constraints,
                                                    mbd_kkt_layout_t *layout)
{
    int num_equations = 0;

    CHECK_ERROR(mbd_kkt_count_constraint_equations(constraints, num_constraints, &num_equations));
    return mbd_kkt_compute_layout(num_bodies, num_equations, layout);
}
