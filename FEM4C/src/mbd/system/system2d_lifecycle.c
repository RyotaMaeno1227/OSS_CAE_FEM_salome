#include "system2d.h"
#include "system2d_internal.h"
#include "projection2d.h"
#include "../../common/error.h"
#include <stdio.h>
#include <string.h>

/*
 * Owns narrow zero/init/free lifecycle helper implementations for mbd_system2d_t.
 * Inputs are system object pointers; outputs and side effects are zero
 * initialization, default initialization, storage cleanup, snapshot cleanup, and
 * related lifecycle ownership handling. Body/constraint storage, default/config
 * setters, contact registration, snapshot helpers, clone helper, force/state
 * helpers, solver orchestration, contact runtime refresh, input parsing,
 * contact-load bridge, output bridge, static/coupled bridge, and file IO remain
 * in other files. This extraction is for readability and maintainability, not
 * behavior change.
 */

void mbd_system2d_zero(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }

    memset(system, 0, sizeof(*system));
    system->bodies = NULL;
    system->body_states = NULL;
    system->flexible_force = NULL;
    system->contact_force = NULL;
    system->current_generalized_force = NULL;
    system->previous_generalized_force = NULL;
    system->constraints = NULL;
    system->body_capacity = 0;
    system->constraint_capacity = 0;
    system->implicit_residual_l2_last = 0.0;
    system->implicit_residual_tolerance_last = 0.0;
    system->implicit_residual_num_equations_last = 0;
    system->implicit_converged = 0;
    system->hht_force_history_mode = MBD_HHT_FORCE_HISTORY_MODE_NOT_APPLICABLE;
    system->implicit_residual_mode = MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT;
    system->implicit_scheme_mode = MBD_IMPLICIT_SCHEME_NOT_APPLICABLE;
    system->implicit_convergence_reason = MBD_IMPLICIT_REASON_NOT_RUN;
    system->position_projection_source_status = MBD_SOURCE_DEFAULT;
    system->position_projection_target_reached = 0;
    system->position_projection_max_iterations = MBD_PROJECTION2D_DEFAULT_MAX_ITERS;
    system->position_projection_max_iterations_source_status = MBD_SOURCE_DEFAULT;
    system->position_projection_residual_tolerance = MBD_PROJECTION2D_DEFAULT_RESIDUAL_TOL;
    system->position_projection_residual_tolerance_source_status = MBD_SOURCE_DEFAULT;
    system->position_projection_stop_reason = MBD_PROJECTION_STOP_DISABLED;
    system->position_projection_velocity_residual_l2_before = 0.0;
    system->position_projection_velocity_residual_l2_after = 0.0;
    system->position_projection_velocity_reduction_ratio_last = 0.0;
    system->contact_coupling_mode = MBD_CONTACT_COUPLING_MODE_ONE_WAY;
    system->local_feedback_mode = MBD_LOCAL_FEEDBACK_MODE_NONE;
    system->local_contact_monolithic_mode = MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE;
    system->monolithic_proper_mode = MBD_MONOLITHIC_PROPER_MODE_NONE;
    memset(&system->monolithic_proper_context, 0, sizeof(system->monolithic_proper_context));
    memset(&system->monolithic_proper_internal, 0, sizeof(system->monolithic_proper_internal));
    memset(&system->monolithic_proper_runtime, 0, sizeof(system->monolithic_proper_runtime));
    snprintf(system->monolithic_proper_runtime.overall_status,
             sizeof(system->monolithic_proper_runtime.overall_status),
             "%s",
             "not_run");
    system->local_feedback_filename[0] = '\0';
    system->local_contact_filename[0] = '\0';
    system->ehl_filename[0] = '\0';
    system->local_contact_monolithic_artifact_root[0] = '\0';
    system->num_local_feedback_records = 0;
    system->monolithic_local_patch_row_count_total = 0;
    system->monolithic_local_patch_active_rows_total = 0;
    system->monolithic_local_patch_gamma_not_one_rows_total = 0;
    system->monolithic_local_patch_fn_positive_rows_total = 0;
    system->current_step_index = 0;
    mbd_time_control2d_set_defaults(&system->time);
}

fem_error_t mbd_system2d_init(mbd_system2d_t *system)
{
    fem_error_t err;

    CHECK_NULL(system, "mbd_system2d");
    mbd_system2d_zero(system);
    err = mbd_system2d_reserve_body_storage(system, MBD_SYSTEM2D_MAX_BODIES);
    if (err != FEM_SUCCESS) {
        mbd_system2d_free(system);
        return err;
    }
    err = mbd_system2d_reserve_constraint_storage(system,
                                                  MBD_SYSTEM2D_MAX_CONSTRAINTS);
    if (err != FEM_SUCCESS) {
        mbd_system2d_free(system);
        return err;
    }
    return FEM_SUCCESS;
}

void mbd_system2d_free(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }
    mbd_system2d_release_body_storage(system);
    mbd_system2d_release_constraint_storage(system);
    mbd_system2d_zero(system);
}
