#include "constraint2d.h"
#include "kinematics2d.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

static fem_error_t build_anchor_world(const mbd_body_state2d_t *state,
                                      const double local_anchor[2],
                                      double world_anchor[2],
                                      double d_world_dtheta[2])
{
    double jacobian[2][MBD_BODY2D_DOF];

    CHECK_ERROR(mbd_kinematics2d_local_point_to_world(state, local_anchor, world_anchor));
    CHECK_ERROR(mbd_kinematics2d_world_point_jacobian(state, local_anchor, jacobian));
    d_world_dtheta[0] = jacobian[0][2];
    d_world_dtheta[1] = jacobian[1][2];
    return FEM_SUCCESS;
}

static void zero_linearization(double residual[MBD_CONSTRAINT2D_MAX_EQ],
                               double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                               double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF])
{
    memset(residual, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ);
    memset(jac_i, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ * MBD_BODY2D_DOF);
    memset(jac_j, 0, sizeof(double) * MBD_CONSTRAINT2D_MAX_EQ * MBD_BODY2D_DOF);
}

static fem_error_t mbd_constraint_compute_phi_dot(
    const double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
    const double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
    const double velocity_i[MBD_BODY2D_DOF],
    const double velocity_j[MBD_BODY2D_DOF],
    int num_equations,
    double phi_dot[MBD_CONSTRAINT2D_MAX_EQ])
{
    int eq;
    int dof;

    CHECK_NULL(jac_i, "constraint jac_i");
    CHECK_NULL(jac_j, "constraint jac_j");
    CHECK_NULL(velocity_i, "constraint velocity_i");
    CHECK_NULL(velocity_j, "constraint velocity_j");
    CHECK_NULL(phi_dot, "constraint phi_dot");

    if (num_equations < 0 || num_equations > MBD_CONSTRAINT2D_MAX_EQ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "constraint equation count %d outside supported range [0,%d]",
                         num_equations, MBD_CONSTRAINT2D_MAX_EQ);
    }

    for (eq = 0; eq < num_equations; ++eq) {
        phi_dot[eq] = 0.0;
        for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
            phi_dot[eq] += jac_i[eq][dof] * velocity_i[dof];
            phi_dot[eq] += jac_j[eq][dof] * velocity_j[dof];
        }
    }
    for (eq = num_equations; eq < MBD_CONSTRAINT2D_MAX_EQ; ++eq) {
        phi_dot[eq] = 0.0;
    }

    return FEM_SUCCESS;
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

    CHECK_ERROR(build_anchor_world(state_i, c->anchor_i, pi, dpi_dtheta));
    CHECK_ERROR(build_anchor_world(state_j, c->anchor_j, pj, dpj_dtheta));

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

    CHECK_ERROR(build_anchor_world(state_i, c->anchor_i, pi, dpi_dtheta));
    CHECK_ERROR(build_anchor_world(state_j, c->anchor_j, pj, dpj_dtheta));

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

fem_error_t mbd_constraint_build_baumgarte_rhs(
    const double residual[MBD_CONSTRAINT2D_MAX_EQ],
    const double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
    const double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
    const double velocity_i[MBD_BODY2D_DOF],
    const double velocity_j[MBD_BODY2D_DOF],
    int num_equations,
    double alpha,
    double beta,
    double gamma_rhs[MBD_CONSTRAINT2D_MAX_EQ])
{
    int eq;

    CHECK_NULL(residual, "constraint residual");
    CHECK_NULL(jac_i, "constraint jac_i");
    CHECK_NULL(jac_j, "constraint jac_j");
    CHECK_NULL(velocity_i, "constraint velocity_i");
    CHECK_NULL(velocity_j, "constraint velocity_j");
    CHECK_NULL(gamma_rhs, "constraint gamma_rhs");

    if (num_equations < 0 || num_equations > MBD_CONSTRAINT2D_MAX_EQ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "constraint equation count %d outside supported range [0,%d]",
                         num_equations, MBD_CONSTRAINT2D_MAX_EQ);
    }
    if (!isfinite(alpha) || !isfinite(beta) || alpha < 0.0 || beta < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Baumgarte parameters must be finite and non-negative (alpha=%.6e, beta=%.6e)",
                         alpha, beta);
    }

    CHECK_ERROR(mbd_constraint_compute_phi_dot(jac_i,
                                               jac_j,
                                               velocity_i,
                                               velocity_j,
                                               num_equations,
                                               gamma_rhs));
    for (eq = 0; eq < num_equations; ++eq) {
        gamma_rhs[eq] = -2.0 * alpha * gamma_rhs[eq] - beta * beta * residual[eq];
    }

    for (eq = num_equations; eq < MBD_CONSTRAINT2D_MAX_EQ; ++eq) {
        gamma_rhs[eq] = 0.0;
    }

    return FEM_SUCCESS;
}

void mbd_constraint_eval2d_zero(mbd_constraint_eval2d_t *eval)
{
    if (!eval) {
        return;
    }

    memset(eval, 0, sizeof(*eval));
}

fem_error_t mbd_constraint_evaluate_accel_rhs(
    const mbd_constraint2d_t *c,
    const mbd_body_state2d_t *state_i,
    const mbd_body_state2d_t *state_j,
    const double velocity_i[MBD_BODY2D_DOF],
    const double velocity_j[MBD_BODY2D_DOF],
    double alpha,
    double beta,
    mbd_constraint_eval2d_t *eval)
{
    CHECK_NULL(eval, "constraint_eval2d");
    mbd_constraint_eval2d_zero(eval);

    CHECK_ERROR(mbd_constraint_evaluate(c,
                                        state_i,
                                        state_j,
                                        eval->residual,
                                        eval->jac_i,
                                        eval->jac_j,
                                        &eval->num_equations));
    CHECK_ERROR(mbd_constraint_compute_phi_dot(eval->jac_i,
                                               eval->jac_j,
                                               velocity_i,
                                               velocity_j,
                                               eval->num_equations,
                                               eval->phi_dot));
    CHECK_ERROR(mbd_constraint_build_baumgarte_rhs(eval->residual,
                                                   eval->jac_i,
                                                   eval->jac_j,
                                                   velocity_i,
                                                   velocity_j,
                                                   eval->num_equations,
                                                   alpha,
                                                   beta,
                                                   eval->gamma_rhs));

    return FEM_SUCCESS;
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
