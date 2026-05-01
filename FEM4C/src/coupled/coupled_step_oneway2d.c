#include "coupled_step_oneway2d.h"

#include "../common/error.h"
#include "coupled_step_common2d.h"

#include <stdio.h>

fem_error_t coupled_step_oneway2d_run(coupled_run2d_t *run,
                                      int step_index,
                                      coupled_step_history2d_t *history)
{
    double residual_l2_before = 0.0;
    double residual_l2_after = 0.0;
    mbd_integrator2d_t mbd_integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
    int num_equations_before = 0;
    int num_equations_after = 0;
    int defined_flex_bodies = 0;
    fem_error_t err = FEM_SUCCESS;
    int i;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled one-way history");

    CHECK_ERROR(coupled_step_common2d_resolve_mbd_integrator(run, &mbd_integrator));
    coupled_step_common2d_sync_mbd_time(run, mbd_integrator);

    history->step_index = step_index;
    history->time = run->time.dt * (double)step_index;
    history->constraint_residual_l2 = 0.0;
    history->coupling_residual_l2 = 0.0;
    history->flex_solves = 0;
    history->fixed_point_iterations = 0;
    history->coupling_converged = 0;

    CHECK_ERROR(mbd_system2d_clear_flexible_forces(&run->mbd_system));
    CHECK_ERROR(mbd_system2d_sync_body_states(&run->mbd_system));
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                            &residual_l2_before,
                                                            &num_equations_before));

    printf("  coupled_step=%d/%d t=%.6e scheme=oneway_snapshot path_class=official sequence=mbd_accept->fem_snapshot->record_no_feedback\n",
           step_index,
           run->time.num_steps,
           history->time);
    printf("    coupling_role: official one-way snapshot/replay baseline with no FEM-to-MBD feedback\n");
    printf("    oneway_state: integrator=%s bodies=%d constraints=%d equations=%d residual_l2_before=%.6e feedback_to_mbd=0\n",
           coupled_integrator_to_string(run->time.integrator),
           run->mbd_system.num_bodies,
           run->mbd_system.num_constraints,
           num_equations_before,
           residual_l2_before);

    defined_flex_bodies = coupled_step_common2d_count_defined_flex_bodies(run);
    if (defined_flex_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled one-way step requires at least one defined flexible body");
    }

    err = coupled_step_common2d_do_mbd_step(run);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    coupled_step_common2d_sync_mbd_time(run, mbd_integrator);
    CHECK_ERROR(mbd_system2d_sync_body_states(&run->mbd_system));

    CHECK_ERROR(coupled_step_history2d_reserve_flex_body_storage(
        history,
        run->case_data.num_flex_bodies));
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        err = coupled_step_common2d_solve_flex_snapshot_for_slot(run,
                                                                 i,
                                                                 NULL,
                                                                 0,
                                                                 "    ",
                                                                 "      ",
                                                                 "accepted",
                                                                 "snapshot_force_observed",
                                                                 NULL,
                                                                 history);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    err = mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                      &residual_l2_after,
                                                      &num_equations_after);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    history->constraint_residual_l2 = residual_l2_after;

    printf("    oneway_result: equations_after=%d residual_l2_after=%.6e flex_solves=%d feedback_to_mbd=0\n",
           num_equations_after,
           residual_l2_after,
           history->flex_solves);

cleanup:
    if (run) {
        fem_error_t cleanup_err = mbd_system2d_clear_flexible_forces(&run->mbd_system);

        if (err == FEM_SUCCESS && cleanup_err != FEM_SUCCESS) {
            err = cleanup_err;
        }
    }
    return err;
}
