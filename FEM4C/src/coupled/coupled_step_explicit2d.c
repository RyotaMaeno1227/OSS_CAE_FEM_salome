#include "coupled_step_explicit2d.h"

#include "../common/error.h"
#include "coupled_step_common2d.h"
#include <stdio.h>
#include <string.h>

fem_error_t coupled_step_explicit2d_run(coupled_run2d_t *run,
                                        int step_index,
                                        coupled_step_history2d_t *history)
{
    double base_force[MBD_SYSTEM2D_MAX_BODIES][MBD_BODY2D_DOF] = {{0.0}};
    double residual_l2_before = 0.0;
    double residual_l2_after = 0.0;
    int num_equations_before = 0;
    int num_equations_after = 0;
    int defined_flex_bodies = 0;
    int advanced = 0;
    fem_error_t err = FEM_SUCCESS;
    int i;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled explicit history");

    coupled_step_common2d_sync_mbd_time(run, MBD_INTEGRATOR2D_EXPLICIT);

    history->step_index = step_index;
    history->time = run->time.dt * (double)step_index;
    history->constraint_residual_l2 = 0.0;
    history->coupling_residual_l2 = 0.0;
    history->flex_solves = 0;
    history->fixed_point_iterations = 1;
    history->coupling_converged = 1;

    CHECK_ERROR(mbd_system2d_sync_body_states(&run->mbd_system));
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                            &residual_l2_before,
                                                            &num_equations_before));

    printf("  coupled_step=%d/%d t=%.6e scheme=staggered_explicit path_class=experimental sequence=flex_loop->reaction_map->mbd_explicit\n",
           step_index,
           run->time.num_steps,
           history->time);
    printf("    coupling_role: experimental two-way staggered path preserved outside official mainline acceptance\n");
    printf("    mbd_explicit_state: bodies=%d constraints=%d equations=%d residual_l2_before=%.6e\n",
           run->mbd_system.num_bodies,
           run->mbd_system.num_constraints,
           num_equations_before,
           residual_l2_before);

    defined_flex_bodies = coupled_step_common2d_count_defined_flex_bodies(run);
    if (defined_flex_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled explicit step requires at least one defined flexible body");
    }

    CHECK_ERROR(mbd_system2d_capture_body_forces(&run->mbd_system, base_force));

    for (i = 0; i < COUPLED_CASE2D_MAX_FLEX_BODIES; ++i) {
        if (!run->case_data.flex_bodies[i].is_defined) {
            continue;
        }
        err = coupled_step_common2d_solve_flex_snapshot_for_slot(run,
                                                                 i,
                                                                 NULL,
                                                                 1,
                                                                 "    ",
                                                                 "      ",
                                                                 NULL,
                                                                 NULL,
                                                                 NULL,
                                                                 history);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    coupled_step_common2d_sync_mbd_time(run, MBD_INTEGRATOR2D_EXPLICIT);
    err = mbd_system2d_do_explicit_step(&run->mbd_system);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    advanced = 1;

    err = mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                      &residual_l2_after,
                                                      &num_equations_after);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    history->constraint_residual_l2 = residual_l2_after;

    printf("    mbd_explicit_result: advanced=%d equations_after=%d residual_l2_after=%.6e\n",
           advanced,
           num_equations_after,
           residual_l2_after);

cleanup:
    if (run) {
        fem_error_t cleanup_err = mbd_system2d_clear_flexible_forces(&run->mbd_system);

        if (err == FEM_SUCCESS && cleanup_err != FEM_SUCCESS) {
            err = cleanup_err;
        }
    }
    CHECK_ERROR(mbd_system2d_restore_body_forces(&run->mbd_system, base_force));
    return err;
}
