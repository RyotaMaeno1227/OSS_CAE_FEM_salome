#include "system2d.h"
#include "system2d_internal.h"
#include "../../common/error.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * Owns narrow configuration/default/string helper implementations for
 * mbd_system2d_t. Inputs are enum values, time-control values, system object
 * pointers, and configuration values; outputs and side effects are
 * configuration/default assignment and string conversion results. Lifecycle,
 * storage, solver orchestration, input parsing, contact-load bridge, output
 * bridge, static/coupled bridge, and file IO remain in other files. This
 * extraction is for readability and maintainability, not behavior change.
 */

const char *mbd_integrator2d_to_string(mbd_integrator2d_t integrator)
{
    if (integrator == MBD_INTEGRATOR2D_EXPLICIT) {
        return "explicit";
    }
    if (integrator == MBD_INTEGRATOR2D_NEWMARK_BETA) {
        return "newmark_beta";
    }
    if (integrator == MBD_INTEGRATOR2D_HHT_ALPHA) {
        return "hht_alpha";
    }
    return "unknown";
}

const char *mbd_contact_coupling_mode_to_string(mbd_contact_coupling_mode_t mode)
{
    if (mode == MBD_CONTACT_COUPLING_MODE_ONE_WAY) {
        return "ONE_WAY";
    }
    if (mode == MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS) {
        return "LAGGED_STIFFNESS";
    }
    return "UNKNOWN";
}

const char *mbd_local_feedback_mode_to_string(mbd_local_feedback_mode_t mode)
{
    if (mode == MBD_LOCAL_FEEDBACK_MODE_NONE) {
        return "NONE";
    }
    if (mode == MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED) {
        return "LAGGED_REDUCED";
    }
    if (mode == MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED) {
        return "SAME_TIME_REDUCED";
    }
    return "UNKNOWN";
}

const char *mbd_local_contact_monolithic_mode_to_string(
    mbd_local_contact_monolithic_mode_t mode)
{
    if (mode == MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE) {
        return "NONE";
    }
    if (mode == MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE) {
        return "PATCH_MVP_CIRCLE";
    }
    return "UNKNOWN";
}

const char *mbd_monolithic_proper_mode_to_string(
    mbd_monolithic_proper_mode_t mode)
{
    if (mode == MBD_MONOLITHIC_PROPER_MODE_NONE) {
        return "NONE";
    }
    if (mode == MBD_MONOLITHIC_PROPER_MODE_BLOCK_NEWTON_V1) {
        return "BLOCK_NEWTON_V1";
    }
    return "UNKNOWN";
}

void mbd_time_control2d_set_defaults(mbd_time_control2d_t *time)
{
    if (!time) {
        return;
    }

    memset(time, 0, sizeof(*time));
    time->dt = 1.0e-3;
    time->num_steps = 1;
    time->steps_requested = 1;
    time->history_stride = MBD_HISTORY_STRIDE_DEFAULT;
    time->integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
    time->integrator_source_status = MBD_SOURCE_DEFAULT;
    time->newmark_beta = 2.5e-1;
    time->newmark_gamma = 5.0e-1;
    time->hht_alpha = -5.0e-2;
    time->implicit_max_iterations = 1;
    time->implicit_iterations_last = 0;
    time->dt_source_status = MBD_SOURCE_DEFAULT;
    time->steps_source_status = MBD_SOURCE_DEFAULT;
    time->history_stride_source_status = MBD_SOURCE_DEFAULT;
    time->newmark_beta_source_status = MBD_SOURCE_DEFAULT;
    time->newmark_gamma_source_status = MBD_SOURCE_DEFAULT;
    time->hht_alpha_source_status = MBD_SOURCE_DEFAULT;
    time->implicit_max_iterations_source_status = MBD_SOURCE_DEFAULT;
}

fem_error_t mbd_system2d_set_gravity(mbd_system2d_t *system,
                                     double gravity_x,
                                     double gravity_y)
{
    CHECK_NULL(system, "mbd_system2d");

    system->gravity[0] = gravity_x;
    system->gravity[1] = gravity_y;
    system->has_gravity = 1;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_contact_coupling_mode(
    mbd_system2d_t *system,
    mbd_contact_coupling_mode_t mode)
{
    CHECK_NULL(system, "mbd_system2d");

    if (mode != MBD_CONTACT_COUPLING_MODE_ONE_WAY &&
        mode != MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "unsupported contact coupling mode %d",
                         (int)mode);
    }

    system->contact_coupling_mode = mode;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_local_feedback_mode(
    mbd_system2d_t *system,
    mbd_local_feedback_mode_t mode)
{
    CHECK_NULL(system, "mbd_system2d");

    if (mode != MBD_LOCAL_FEEDBACK_MODE_NONE &&
        mode != MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED &&
        mode != MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "unsupported local feedback mode %d",
                         (int)mode);
    }

    system->local_feedback_mode = mode;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_local_contact_monolithic_mode(
    mbd_system2d_t *system,
    mbd_local_contact_monolithic_mode_t mode)
{
    CHECK_NULL(system, "mbd_system2d");

    if (mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE &&
        mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "unsupported local contact monolithic mode %d",
                         (int)mode);
    }

    system->local_contact_monolithic_mode = mode;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_monolithic_proper_mode(
    mbd_system2d_t *system,
    mbd_monolithic_proper_mode_t mode)
{
    CHECK_NULL(system, "mbd_system2d");

    if (mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
        mode != MBD_MONOLITHIC_PROPER_MODE_BLOCK_NEWTON_V1) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "unsupported monolithic proper mode %d",
                         (int)mode);
    }

    system->monolithic_proper_mode = mode;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_monolithic_proper_context(
    mbd_system2d_t *system,
    const mbd_monolithic_proper_context2d_t *context)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(context, "monolithic proper context");

    if (!isfinite(context->k_contact_eff) || context->k_contact_eff < 0.0 ||
        !isfinite(context->mu_eff) || context->mu_eff < 0.0 ||
        !isfinite(context->stress_residual) || context->stress_residual < 0.0 ||
        !isfinite(context->displacement_residual) || context->displacement_residual < 0.0 ||
        !isfinite(context->contact_parameter_residual) ||
        context->contact_parameter_residual < 0.0 ||
        !isfinite(context->fem_residual) || context->fem_residual < 0.0 ||
        context->context_step < 0 || context->iter_index < 0 ||
        (context->converged_flag != 0 && context->converged_flag != 1)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "invalid monolithic proper context values");
    }

    system->monolithic_proper_context = *context;
    system->monolithic_proper_context.is_defined = 1;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_local_feedback_file(
    mbd_system2d_t *system,
    const char *path)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(path, "local feedback file path");

    if (snprintf(system->local_feedback_filename,
                 sizeof(system->local_feedback_filename),
                 "%s",
                 path) >= (int)sizeof(system->local_feedback_filename)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "local feedback file path is too long");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_local_contact_file(
    mbd_system2d_t *system,
    const char *path)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(path, "local contact file path");

    if (snprintf(system->local_contact_filename,
                 sizeof(system->local_contact_filename),
                 "%s",
                 path) >= (int)sizeof(system->local_contact_filename)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "local contact file path is too long");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_ehl_file(
    mbd_system2d_t *system,
    const char *path)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(path, "ehl file path");

    if (snprintf(system->ehl_filename,
                 sizeof(system->ehl_filename),
                 "%s",
                 path) >= (int)sizeof(system->ehl_filename)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "ehl file path is too long");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_set_time_control(mbd_system2d_t *system,
                                          const mbd_time_control2d_t *time)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(time, "mbd_time_control2d");

    system->time = *time;
    return FEM_SUCCESS;
}
