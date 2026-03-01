#include "constraint2d.h"
#include "../common/error.h"
#include <math.h>
#include <string.h>

static void rotate_local_point(const double local[2], double theta, double rotated[2])
{
    const double c = cos(theta);
    const double s = sin(theta);
    rotated[0] = c * local[0] - s * local[1];
    rotated[1] = s * local[0] + c * local[1];
}

static void d_rotate_dtheta(const double rotated[2], double d_rotated[2])
{
    d_rotated[0] = -rotated[1];
    d_rotated[1] = rotated[0];
}

static void build_anchor_world(const mbd_body_state2d_t *state,
                               const double local_anchor[2],
                               double world_anchor[2],
                               double d_world_dtheta[2])
{
    double rotated[2];
    rotate_local_point(local_anchor, state->theta, rotated);
    d_rotate_dtheta(rotated, d_world_dtheta);

    world_anchor[0] = state->x + rotated[0];
    world_anchor[1] = state->y + rotated[1];
}

static void zero_linearization(double residual[MBD_CONSTRAINT2D_MAX_EQ],
                               double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                               double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF])
{
    memset(residual, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ);
    memset(jac_i, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ * MBD_BODY2D_DOF);
    memset(jac_j, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ * MBD_BODY2D_DOF);
}

static fem_error_t evaluate_distance_constraint(const mbd_constraint2d_t *c,
                                                const mbd_body_state2d_t *state_i,
                                                const mbd_body_state2d_t *state_j,
                                                double residual[MBD_CONSTRAINT2D_MAX_EQ],
                                                double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                                double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                                int *num_equations)
{
    double pi[2];
    double pj[2];
    double dpi_dtheta[2];
    double dpj_dtheta[2];
    double delta[2];
    double target_sq;

    build_anchor_world(state_i, c->anchor_i, pi, dpi_dtheta);
    build_anchor_world(state_j, c->anchor_j, pj, dpj_dtheta);

    delta[0] = pi[0] - pj[0];
    delta[1] = pi[1] - pj[1];
    target_sq = c->target_value * c->target_value;

    /* C = 0.5 * (||pi - pj||^2 - d^2) */
    residual[0] = 0.5 * ((delta[0] * delta[0] + delta[1] * delta[1]) - target_sq);

    jac_i[0][0] = delta[0];
    jac_i[0][1] = delta[1];
    jac_i[0][2] = delta[0] * dpi_dtheta[0] + delta[1] * dpi_dtheta[1];

    jac_j[0][0] = -delta[0];
    jac_j[0][1] = -delta[1];
    jac_j[0][2] = -(delta[0] * dpj_dtheta[0] + delta[1] * dpj_dtheta[1]);

    *num_equations = 1;
    return FEM_SUCCESS;
}

static fem_error_t evaluate_revolute_constraint(const mbd_constraint2d_t *c,
                                                const mbd_body_state2d_t *state_i,
                                                const mbd_body_state2d_t *state_j,
                                                double residual[MBD_CONSTRAINT2D_MAX_EQ],
                                                double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                                double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                                int *num_equations)
{
    double pi[2];
    double pj[2];
    double dpi_dtheta[2];
    double dpj_dtheta[2];

    build_anchor_world(state_i, c->anchor_i, pi, dpi_dtheta);
    build_anchor_world(state_j, c->anchor_j, pj, dpj_dtheta);

    /* C = pi - pj (2 equations for x/y coincidence) */
    residual[0] = pi[0] - pj[0];
    residual[1] = pi[1] - pj[1];

    jac_i[0][0] = 1.0;
    jac_i[0][2] = dpi_dtheta[0];
    jac_i[1][1] = 1.0;
    jac_i[1][2] = dpi_dtheta[1];

    jac_j[0][0] = -1.0;
    jac_j[0][2] = -dpj_dtheta[0];
    jac_j[1][1] = -1.0;
    jac_j[1][2] = -dpj_dtheta[1];

    *num_equations = 2;
    return FEM_SUCCESS;
}

fem_error_t mbd_constraint_validate(const mbd_constraint2d_t *c)
{
    CHECK_NULL(c, "constraint");

    if (c->id <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD constraint id must be positive");
    }
    if (c->body_i < 0 || c->body_j < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD body index must be non-negative");
    }
    if (c->body_i == c->body_j) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD constraint requires two distinct bodies");
    }
    if (c->type != MBD_CONSTRAINT_DISTANCE && c->type != MBD_CONSTRAINT_REVOLUTE) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Unknown MBD constraint type: %d", c->type);
    }
    if (c->type == MBD_CONSTRAINT_DISTANCE && c->target_value <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Distance constraint target must be positive");
    }

    return FEM_SUCCESS;
}

int mbd_constraint_equation_count(const mbd_constraint2d_t *c)
{
    if (!c) {
        return 0;
    }
    if (c->type == MBD_CONSTRAINT_DISTANCE) {
        return 1;
    }
    if (c->type == MBD_CONSTRAINT_REVOLUTE) {
        return 2;
    }
    return 0;
}

fem_error_t mbd_constraint_init_distance(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2],
                                         double distance)
{
    CHECK_NULL(out, "out");
    CHECK_NULL(anchor_i, "anchor_i");
    CHECK_NULL(anchor_j, "anchor_j");

    out->id = id;
    out->type = MBD_CONSTRAINT_DISTANCE;
    out->body_i = body_i;
    out->body_j = body_j;
    out->anchor_i[0] = anchor_i[0];
    out->anchor_i[1] = anchor_i[1];
    out->anchor_j[0] = anchor_j[0];
    out->anchor_j[1] = anchor_j[1];
    out->target_value = distance;

    return mbd_constraint_validate(out);
}

fem_error_t mbd_constraint_evaluate(const mbd_constraint2d_t *c,
                                    const mbd_body_state2d_t *state_i,
                                    const mbd_body_state2d_t *state_j,
                                    double residual[MBD_CONSTRAINT2D_MAX_EQ],
                                    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                    int *num_equations)
{
    CHECK_NULL(c, "constraint");
    CHECK_NULL(state_i, "state_i");
    CHECK_NULL(state_j, "state_j");
    CHECK_NULL(residual, "residual");
    CHECK_NULL(jac_i, "jac_i");
    CHECK_NULL(jac_j, "jac_j");
    CHECK_NULL(num_equations, "num_equations");

    CHECK_ERROR(mbd_constraint_validate(c));

    zero_linearization(residual, jac_i, jac_j);

    if (c->type == MBD_CONSTRAINT_DISTANCE) {
        return evaluate_distance_constraint(c, state_i, state_j, residual, jac_i, jac_j, num_equations);
    }
    if (c->type == MBD_CONSTRAINT_REVOLUTE) {
        return evaluate_revolute_constraint(c, state_i, state_j, residual, jac_i, jac_j, num_equations);
    }

    return error_set(FEM_ERROR_INVALID_INPUT, "Unsupported MBD constraint type: %d", c->type);
}

fem_error_t mbd_constraint_init_revolute(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2])
{
    CHECK_NULL(out, "out");
    CHECK_NULL(anchor_i, "anchor_i");
    CHECK_NULL(anchor_j, "anchor_j");

    out->id = id;
    out->type = MBD_CONSTRAINT_REVOLUTE;
    out->body_i = body_i;
    out->body_j = body_j;
    out->anchor_i[0] = anchor_i[0];
    out->anchor_i[1] = anchor_i[1];
    out->anchor_j[0] = anchor_j[0];
    out->anchor_j[1] = anchor_j[1];
    out->target_value = 0.0;

    return mbd_constraint_validate(out);
}
