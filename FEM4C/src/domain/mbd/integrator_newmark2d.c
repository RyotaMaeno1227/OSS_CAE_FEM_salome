#include "integrator_newmark2d.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

static fem_error_t mbd_newmark2d_compute_unconstrained_acceleration(
    const mbd_body2d_t *body,
    const double generalized_force[MBD_BODY2D_DOF],
    double acceleration[MBD_BODY2D_DOF])
{
    CHECK_NULL(body, "newmark body");
    CHECK_NULL(generalized_force, "newmark generalized force");
    CHECK_NULL(acceleration, "newmark acceleration");

    if (!isfinite(body->mass) || body->mass <= 0.0 ||
        !isfinite(body->inertia) || body->inertia <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "newmark body requires positive finite mass/inertia");
    }

    acceleration[0] = generalized_force[0] / body->mass;
    acceleration[1] = generalized_force[1] / body->mass;
    acceleration[2] = generalized_force[2] / body->inertia;
    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_validate_params(const mbd_newmark2d_params_t *params)
{
    CHECK_NULL(params, "newmark params");

    if (!isfinite(params->dt) || params->dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "newmark dt %.6e must be finite and positive",
                         params->dt);
    }
    if (!isfinite(params->beta) || params->beta <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "newmark beta %.6e must be finite and positive",
                         params->beta);
    }
    if (!isfinite(params->gamma) || params->gamma <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "newmark gamma %.6e must be finite and positive",
                         params->gamma);
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_predict(const mbd_body2d_t *current,
                                  const mbd_newmark2d_params_t *params,
                                  double q_pred[MBD_BODY2D_DOF],
                                  double v_pred[MBD_BODY2D_DOF])
{
    int dof;

    CHECK_NULL(current, "newmark current body");
    CHECK_NULL(q_pred, "newmark predicted position");
    CHECK_NULL(v_pred, "newmark predicted velocity");
    CHECK_ERROR(mbd_newmark2d_validate_params(params));

    for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
        q_pred[dof] = current->q[dof]
                    + params->dt * current->v[dof]
                    + params->dt * params->dt * (0.5 - params->beta) * current->a[dof];
        v_pred[dof] = current->v[dof]
                    + params->dt * (1.0 - params->gamma) * current->a[dof];
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_predict_state(const mbd_body2d_t *current,
                                        const mbd_newmark2d_params_t *params,
                                        mbd_body2d_t *predicted)
{
    CHECK_NULL(current, "newmark current body");
    CHECK_NULL(predicted, "newmark predicted body");
    CHECK_ERROR(mbd_newmark2d_validate_params(params));

    *predicted = *current;
    CHECK_ERROR(mbd_newmark2d_predict(current,
                                      params,
                                      predicted->q,
                                      predicted->v));
    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_correct(const mbd_body2d_t *current,
                                  const double acceleration_next[MBD_BODY2D_DOF],
                                  const mbd_newmark2d_params_t *params,
                                  double q_next[MBD_BODY2D_DOF],
                                  double v_next[MBD_BODY2D_DOF])
{
    double q_pred[MBD_BODY2D_DOF];
    double v_pred[MBD_BODY2D_DOF];
    int dof;

    CHECK_NULL(acceleration_next, "newmark next acceleration");
    CHECK_NULL(q_next, "newmark next position");
    CHECK_NULL(v_next, "newmark next velocity");
    CHECK_ERROR(mbd_newmark2d_predict(current, params, q_pred, v_pred));

    for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
        q_next[dof] = q_pred[dof]
                    + params->beta * params->dt * params->dt * acceleration_next[dof];
        v_next[dof] = v_pred[dof]
                    + params->gamma * params->dt * acceleration_next[dof];
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_update_state(
    const mbd_body2d_t *current,
    const double acceleration_next[MBD_BODY2D_DOF],
    const mbd_newmark2d_params_t *params,
    mbd_body2d_t *next)
{
    CHECK_NULL(current, "newmark current body");
    CHECK_NULL(next, "newmark next body");
    CHECK_NULL(acceleration_next, "newmark next acceleration");
    CHECK_ERROR(mbd_newmark2d_validate_params(params));

    *next = *current;
    memcpy(next->a, acceleration_next, sizeof(next->a));
    CHECK_ERROR(mbd_newmark2d_correct(current,
                                      acceleration_next,
                                      params,
                                      next->q,
                                      next->v));
    return FEM_SUCCESS;
}

fem_error_t mbd_newmark2d_step_unconstrained(
    const mbd_body2d_t *current,
    const double generalized_force[MBD_BODY2D_DOF],
    const mbd_newmark2d_params_t *params,
    mbd_body2d_t *next)
{
    double acceleration_next[MBD_BODY2D_DOF];

    CHECK_NULL(current, "newmark current body");
    CHECK_NULL(next, "newmark next body");
    CHECK_ERROR(mbd_newmark2d_validate_params(params));
    CHECK_ERROR(mbd_newmark2d_compute_unconstrained_acceleration(current,
                                                                 generalized_force,
                                                                 acceleration_next));

    CHECK_ERROR(mbd_newmark2d_update_state(current,
                                           acceleration_next,
                                           params,
                                           next));
    return FEM_SUCCESS;
}
