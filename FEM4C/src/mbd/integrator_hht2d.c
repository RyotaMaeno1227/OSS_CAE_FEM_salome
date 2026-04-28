#include "integrator_hht2d.h"
#include "../common/error.h"
#include <math.h>
#include <string.h>

fem_error_t mbd_hht2d_validate_alpha(double alpha)
{
    if (!isfinite(alpha) || alpha < (-1.0 / 3.0) || alpha > 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht alpha %.6e outside supported range [-1/3,0]",
                         alpha);
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_init_params(mbd_hht2d_params_t *params,
                                  double dt,
                                  double alpha)
{
    CHECK_NULL(params, "hht params");
    CHECK_ERROR(mbd_hht2d_validate_alpha(alpha));

    if (!isfinite(dt) || dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht dt %.6e must be finite and positive",
                         dt);
    }

    params->dt = dt;
    params->alpha = alpha;
    params->gamma = 0.5 - alpha;
    params->beta = 0.25 * (1.0 - alpha) * (1.0 - alpha);
    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_validate_params(const mbd_hht2d_params_t *params)
{
    CHECK_NULL(params, "hht params");
    CHECK_ERROR(mbd_hht2d_validate_alpha(params->alpha));

    if (!isfinite(params->dt) || params->dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht dt %.6e must be finite and positive",
                         params->dt);
    }
    if (!isfinite(params->beta) || params->beta <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht beta %.6e must be finite and positive",
                         params->beta);
    }
    if (!isfinite(params->gamma) || params->gamma <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht gamma %.6e must be finite and positive",
                         params->gamma);
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_predict(const mbd_body2d_t *current,
                              const mbd_hht2d_params_t *params,
                              double q_pred[MBD_BODY2D_DOF],
                              double v_pred[MBD_BODY2D_DOF])
{
    int dof;

    CHECK_NULL(current, "hht current body");
    CHECK_NULL(q_pred, "hht predicted position");
    CHECK_NULL(v_pred, "hht predicted velocity");
    CHECK_ERROR(mbd_hht2d_validate_params(params));

    for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
        q_pred[dof] = current->q[dof]
                    + params->dt * current->v[dof]
                    + params->dt * params->dt * (0.5 - params->beta) * current->a[dof];
        v_pred[dof] = current->v[dof]
                    + params->dt * (1.0 - params->gamma) * current->a[dof];
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_predict_state(const mbd_body2d_t *current,
                                    const mbd_hht2d_params_t *params,
                                    mbd_body2d_t *predicted)
{
    CHECK_NULL(current, "hht current body");
    CHECK_NULL(predicted, "hht predicted body");
    CHECK_ERROR(mbd_hht2d_validate_params(params));

    *predicted = *current;
    CHECK_ERROR(mbd_hht2d_predict(current,
                                  params,
                                  predicted->q,
                                  predicted->v));
    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_to_newmark_params(const mbd_hht2d_params_t *params,
                                        mbd_newmark2d_params_t *newmark_params)
{
    CHECK_NULL(newmark_params, "hht newmark params");
    CHECK_ERROR(mbd_hht2d_validate_params(params));

    newmark_params->dt = params->dt;
    newmark_params->beta = params->beta;
    newmark_params->gamma = params->gamma;
    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_blend_vector(const double current[MBD_BODY2D_DOF],
                                   const double previous[MBD_BODY2D_DOF],
                                   const mbd_hht2d_params_t *params,
                                   double blended[MBD_BODY2D_DOF])
{
    int dof;

    CHECK_NULL(current, "hht current vector");
    CHECK_NULL(previous, "hht previous vector");
    CHECK_NULL(blended, "hht blended vector");
    CHECK_ERROR(mbd_hht2d_validate_params(params));

    for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
        blended[dof] = (1.0 + params->alpha) * current[dof]
                     - params->alpha * previous[dof];
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_blend_scalar(double current,
                                   double previous,
                                   const mbd_hht2d_params_t *params,
                                   double *blended)
{
    CHECK_NULL(blended, "hht blended scalar");
    CHECK_ERROR(mbd_hht2d_validate_params(params));

    if (!isfinite(current) || !isfinite(previous)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht scalar blend requires finite values (current=%.6e previous=%.6e)",
                         current,
                         previous);
    }

    *blended = (1.0 + params->alpha) * current - params->alpha * previous;
    return FEM_SUCCESS;
}

fem_error_t mbd_hht2d_compute_effective_force(const double current[MBD_BODY2D_DOF],
                                              const double previous[MBD_BODY2D_DOF],
                                              const mbd_hht2d_params_t *params,
                                              double effective_force[MBD_BODY2D_DOF])
{
    return mbd_hht2d_blend_vector(current, previous, params, effective_force);
}

fem_error_t mbd_hht2d_compute_effective_residual(const double current[MBD_BODY2D_DOF],
                                                 const double previous[MBD_BODY2D_DOF],
                                                 const mbd_hht2d_params_t *params,
                                                 double effective_residual[MBD_BODY2D_DOF])
{
    return mbd_hht2d_blend_vector(current, previous, params, effective_residual);
}

fem_error_t mbd_hht2d_step_unconstrained(
    const mbd_body2d_t *current,
    const double generalized_force[MBD_BODY2D_DOF],
    const double previous_generalized_force[MBD_BODY2D_DOF],
    const mbd_hht2d_params_t *params,
    mbd_body2d_t *next)
{
    double effective_force[MBD_BODY2D_DOF];
    mbd_newmark2d_params_t corrected_params;

    CHECK_NULL(current, "hht current body");
    CHECK_NULL(generalized_force, "hht generalized force");
    CHECK_NULL(previous_generalized_force, "hht previous generalized force");
    CHECK_NULL(next, "hht next body");
    CHECK_ERROR(mbd_hht2d_validate_params(params));
    CHECK_ERROR(mbd_hht2d_compute_effective_force(generalized_force,
                                                  previous_generalized_force,
                                                  params,
                                                  effective_force));
    CHECK_ERROR(mbd_hht2d_to_newmark_params(params, &corrected_params));
    CHECK_ERROR(mbd_newmark2d_step_unconstrained(current,
                                                 effective_force,
                                                 &corrected_params,
                                                 next));
    return FEM_SUCCESS;
}
