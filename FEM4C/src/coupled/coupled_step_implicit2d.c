#include "coupled_step_implicit2d.h"

#include "../common/error.h"
#include "coupled_step_common2d.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void coupled_step_relax_marker_guess(double marker_guess[3],
                                            const double next_pose[3],
                                            double relaxation)
{
    int i;

    if (!marker_guess || !next_pose) {
        return;
    }

    if (relaxation <= 0.0) {
        relaxation = 1.0;
    } else if (relaxation > 1.0) {
        relaxation = 1.0;
    }

    for (i = 0; i < 3; ++i) {
        marker_guess[i] = (1.0 - relaxation) * marker_guess[i]
                        + relaxation * next_pose[i];
    }
}

static double coupled_step_compute_qflex_residual_l2(
    const coupled_run2d_t *run,
    const double (*current_force)[MBD_BODY2D_DOF],
    const double (*previous_force)[MBD_BODY2D_DOF])
{
    double residual_sq = 0.0;
    int i;
    int j;

    if (!run || !current_force || !previous_force) {
        return 0.0;
    }

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        for (j = 0; j < MBD_BODY2D_DOF; ++j) {
            const double diff = current_force[i][j] - previous_force[i][j];
            residual_sq += diff * diff;
        }
    }

    return sqrt(residual_sq);
}

static double coupled_step_compute_qflex_norm_l2(
    const coupled_run2d_t *run,
    const double (*current_force)[MBD_BODY2D_DOF])
{
    double norm_sq = 0.0;
    int i;
    int j;

    if (!run || !current_force) {
        return 0.0;
    }

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        for (j = 0; j < MBD_BODY2D_DOF; ++j) {
            norm_sq += current_force[i][j] * current_force[i][j];
        }
    }

    return sqrt(norm_sq);
}

static mbd_integrator2d_t coupled_step_select_mbd_integrator(
    const coupled_run2d_t *run)
{
    if (run && run->time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        return MBD_INTEGRATOR2D_HHT_ALPHA;
    }
    return MBD_INTEGRATOR2D_NEWMARK_BETA;
}

static fem_error_t coupled_step_do_implicit_mbd_step(coupled_run2d_t *run)
{
    CHECK_NULL(run, "coupled_run2d");

    if (run->time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        return mbd_system2d_do_hht_step(&run->mbd_system);
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_NEWMARK_BETA) {
        return mbd_system2d_do_newmark_step(&run->mbd_system);
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Coupled implicit step requires newmark_beta or hht_alpha integrator");
}

fem_error_t coupled_step_implicit2d_run(coupled_run2d_t *run,
                                        int step_index,
                                        coupled_step_history2d_t *history)
{
    double (*base_force)[MBD_BODY2D_DOF] = NULL;
    double (*marker_guess)[MBD_BODY2D_DOF] = NULL;
    double (*current_force)[MBD_BODY2D_DOF] = NULL;
    double (*previous_force)[MBD_BODY2D_DOF] = NULL;
    double residual_l2_before = 0.0;
    double residual_l2_after = 0.0;
    double coupling_residual_l2 = 0.0;
    double coupling_force_norm_l2 = 0.0;
    double coupling_residual_limit = 0.0;
    mbd_system2d_snapshot_t *step_start_snapshot = NULL;
    int num_equations_before = 0;
    int num_equations_after = 0;
    int defined_flex_bodies = 0;
    int converged = 0;
    int max_iterations = 0;
    int iteration = 0;
    fem_error_t err = FEM_SUCCESS;
    int i;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled implicit history");

    step_start_snapshot = calloc(1, sizeof(*step_start_snapshot));
    if (!step_start_snapshot) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate implicit coupled step snapshot");
    }
    marker_guess = calloc((size_t)run->case_data.num_flex_bodies, sizeof(*marker_guess));
    current_force = calloc((size_t)run->case_data.num_flex_bodies, sizeof(*current_force));
    previous_force = calloc((size_t)run->case_data.num_flex_bodies, sizeof(*previous_force));
    base_force = calloc((size_t)run->mbd_system.num_bodies, sizeof(*base_force));
    if (!marker_guess || !current_force || !previous_force || !base_force) {
        err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                        "Failed to allocate implicit coupled local scratch");
        goto cleanup;
    }

    coupled_step_common2d_sync_mbd_time(run,
                                        coupled_step_select_mbd_integrator(run));

    history->step_index = step_index;
    history->time = run->time.dt * (double)step_index;
    history->constraint_residual_l2 = 0.0;
    history->coupling_residual_l2 = 0.0;
    history->flex_solves = 0;
    history->fixed_point_iterations = 0;
    history->coupling_converged = 0;

    CHECK_ERROR(mbd_system2d_sync_body_states(&run->mbd_system));
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                            &residual_l2_before,
                                                            &num_equations_before));

    printf("  coupled_step=%d/%d t=%.6e scheme=fixed_point_strong path_class=experimental sequence=newmark_fixed_point->flex_loop->reaction_map\n",
           step_index,
           run->time.num_steps,
           history->time);
    printf("    coupling_role: experimental strong same-step path preserved outside official mainline acceptance\n");
    printf("    newmark_state: bodies=%d constraints=%d equations=%d residual_l2_before=%.6e beta=%.6e gamma=%.6e\n",
           run->mbd_system.num_bodies,
           run->mbd_system.num_constraints,
           num_equations_before,
           residual_l2_before,
           run->time.newmark_beta,
           run->time.newmark_gamma);

    defined_flex_bodies = coupled_step_common2d_count_defined_flex_bodies(run);
    if (defined_flex_bodies <= 0) {
        err = error_set(FEM_ERROR_INVALID_INPUT,
                        "Coupled implicit step requires at least one defined flexible body");
        goto cleanup;
    }
    CHECK_ERROR(coupled_step_history2d_reserve_flex_body_storage(
        history,
        run->case_data.num_flex_bodies));

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        err = coupled_step_common2d_capture_current_pose_for_slot(run,
                                                                  i,
                                                                  marker_guess[i]);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    CHECK_ERROR(mbd_system2d_capture_body_forces(&run->mbd_system, base_force));
    err = mbd_system2d_snapshot_capture(step_start_snapshot, &run->mbd_system);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    max_iterations = run->time.max_coupling_iterations > 0
        ? run->time.max_coupling_iterations
        : 1;

    printf("    same_step_loop: max_iter=%d residual_metric=qflex_l2 residual_tol=%.6e marker_relaxation=%.6e\n",
           max_iterations,
           run->time.residual_tolerance,
           run->time.marker_relaxation);
    for (iteration = 0; iteration < max_iterations; ++iteration) {
        err = mbd_system2d_snapshot_restore(&run->mbd_system, step_start_snapshot);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        coupled_step_common2d_sync_mbd_time(run,
                                            coupled_step_select_mbd_integrator(run));
        memset(current_force,
               0,
               (size_t)run->case_data.num_flex_bodies * sizeof(*current_force));

        printf("    same_step_iteration=%d/%d\n", iteration + 1, max_iterations);
        for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
            err = coupled_step_common2d_solve_flex_snapshot_for_slot(run,
                                                                     i,
                                                                     marker_guess[i],
                                                                     1,
                                                                     "      ",
                                                                     "        ",
                                                                     NULL,
                                                                     NULL,
                                                                     current_force[i],
                                                                     history);
            if (err != FEM_SUCCESS) {
                goto cleanup;
            }
        }

        coupled_step_common2d_sync_mbd_time(run,
                                            coupled_step_select_mbd_integrator(run));
        err = coupled_step_do_implicit_mbd_step(run);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }

        coupling_residual_l2 = coupled_step_compute_qflex_residual_l2(run,
                                                                      current_force,
                                                                      previous_force);
        coupling_force_norm_l2 = coupled_step_compute_qflex_norm_l2(run,
                                                                    current_force);
        coupling_residual_limit = run->time.residual_tolerance
            * fmax(coupling_force_norm_l2, 1.0);
        history->fixed_point_iterations = iteration + 1;
        history->coupling_residual_l2 = coupling_residual_l2;

        if (iteration == 0 && max_iterations <= 1) {
            converged = 1;
            history->coupling_converged = 1;
            printf("      residual_qflex_l2=%.6e limit=%.6e status=single_pass_accepted\n",
                   coupling_residual_l2,
                   coupling_residual_limit);
            break;
        } else if (iteration == 0) {
            printf("      residual_qflex_l2=%.6e limit=%.6e status=bootstrap\n",
                   coupling_residual_l2,
                   coupling_residual_limit);
        } else if (coupling_residual_l2 <= coupling_residual_limit) {
            converged = 1;
            history->coupling_converged = 1;
            printf("      residual_qflex_l2=%.6e limit=%.6e status=converged\n",
                   coupling_residual_l2,
                   coupling_residual_limit);
            break;
        } else if (iteration + 1 >= max_iterations) {
            printf("      residual_qflex_l2=%.6e limit=%.6e status=max_iter_reached\n",
                   coupling_residual_l2,
                   coupling_residual_limit);
        } else {
            printf("      residual_qflex_l2=%.6e limit=%.6e status=continue\n",
                   coupling_residual_l2,
                   coupling_residual_limit);
        }

        memcpy(previous_force,
               current_force,
               (size_t)run->case_data.num_flex_bodies * sizeof(*previous_force));
        for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
            double next_pose[3] = {0.0, 0.0, 0.0};

            err = coupled_step_common2d_capture_current_pose_for_slot(run,
                                                                      i,
                                                                      next_pose);
            if (err != FEM_SUCCESS) {
                goto cleanup;
            }
            coupled_step_relax_marker_guess(marker_guess[i],
                                            next_pose,
                                            run->time.marker_relaxation);
        }
    }

    if (!converged) {
        printf("    same_step_status: converged=0 iterations=%d final_residual_qflex_l2=%.6e\n",
               history->fixed_point_iterations,
               history->coupling_residual_l2);
    } else {
        printf("    same_step_status: converged=1 iterations=%d final_residual_qflex_l2=%.6e\n",
               history->fixed_point_iterations,
               history->coupling_residual_l2);
    }

    err = mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                      &residual_l2_after,
                                                      &num_equations_after);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    history->constraint_residual_l2 = residual_l2_after;

    printf("    newmark_result: equations_after=%d residual_l2_after=%.6e coupling_residual_l2=%.6e fixed_point_iters=%d converged=%d\n",
           num_equations_after,
           residual_l2_after,
           history->coupling_residual_l2,
           history->fixed_point_iterations,
           history->coupling_converged);

cleanup:
    free(previous_force);
    free(current_force);
    free(marker_guess);
    if (step_start_snapshot) {
        mbd_system2d_snapshot_free(step_start_snapshot);
    }
    free(step_start_snapshot);
    if (run) {
        fem_error_t cleanup_err = mbd_system2d_clear_flexible_forces(&run->mbd_system);

        if (err == FEM_SUCCESS && cleanup_err != FEM_SUCCESS) {
            err = cleanup_err;
        }
    }
    if (base_force) {
        fem_error_t restore_err = mbd_system2d_restore_body_forces(&run->mbd_system,
                                                                   base_force);

        if (err == FEM_SUCCESS && restore_err != FEM_SUCCESS) {
            err = restore_err;
        }
    }
    free(base_force);
    return err;
}
