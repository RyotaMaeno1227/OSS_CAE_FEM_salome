#include "coupled_step_delayed_cosim2d.h"

#include "../common/error.h"
#include "coupled_step_common2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int valid;
    int source_step;
    double total_body_force[3];
} delayed_cosim_force_buffer2d_t;

static int coupled_step_delayed_cosim2d_collect_active_slots(
    const coupled_run2d_t *run,
    int *active_slots,
    int active_slot_capacity)
{
    int i;
    int count = 0;

    if (!run || !active_slots) {
        return 0;
    }

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        if (!run->case_data.flex_bodies[i].is_defined) {
            continue;
        }
        if (count >= active_slot_capacity) {
            break;
        }
        active_slots[count] = i;
        ++count;
    }

    return count;
}

static void coupled_step_delayed_cosim2d_reset_buffers(
    delayed_cosim_force_buffer2d_t *buffers,
    int buffer_count)
{
    if (!buffers) {
        return;
    }

    memset(buffers,
           0,
           (size_t)buffer_count * sizeof(*buffers));
}

static fem_error_t coupled_step_delayed_cosim2d_reserve_buffers(
    delayed_cosim_force_buffer2d_t **buffers,
    int *buffer_capacity,
    int required_capacity)
{
    delayed_cosim_force_buffer2d_t *new_buffers = NULL;

    CHECK_NULL(buffers, "delayed cosim buffers");
    CHECK_NULL(buffer_capacity, "delayed cosim buffer capacity");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Delayed cosim buffer capacity %d must be positive",
                         required_capacity);
    }
    if (*buffer_capacity >= required_capacity && *buffers) {
        return FEM_SUCCESS;
    }

    new_buffers = calloc((size_t)required_capacity, sizeof(*new_buffers));
    CHECK_NULL(new_buffers, "delayed cosim buffers");
    if (*buffers && *buffer_capacity > 0) {
        memcpy(new_buffers,
               *buffers,
               (size_t)(*buffer_capacity) * sizeof(*new_buffers));
    }

    free(*buffers);
    *buffers = new_buffers;
    *buffer_capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t coupled_step_delayed_cosim2d_run(coupled_run2d_t *run,
                                             int step_index,
                                             coupled_step_history2d_t *history)
{
    static delayed_cosim_force_buffer2d_t *delay_buffers = NULL;
    static int delay_buffer_capacity = 0;
    double residual_l2_before = 0.0;
    double residual_l2_after = 0.0;
    mbd_integrator2d_t mbd_integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
    double delayed_total_body_force_sum[3] = {0.0, 0.0, 0.0};
    double buffered_total_body_force_sum[3] = {0.0, 0.0, 0.0};
    double (*buffered_total_body_force)[3] = NULL;
    double delayed_total_body_force[3] = {0.0, 0.0, 0.0};
    double (*base_force)[MBD_BODY2D_DOF] = NULL;
    int num_equations_before = 0;
    int num_equations_after = 0;
    int defined_flex_bodies = 0;
    int *active_slots = NULL;
    const char *solver_route_class =
        "partitioned_delayed_cosim_sample_hold_1link_skeleton";
    int base_force_valid = 0;
    fem_error_t err = FEM_SUCCESS;
    int active_body_index = 0;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled delayed cosim history");
    CHECK_ERROR(coupled_step_common2d_resolve_mbd_integrator(run, &mbd_integrator));
    coupled_step_common2d_sync_mbd_time(run, mbd_integrator);

    defined_flex_bodies = coupled_step_common2d_count_defined_flex_bodies(run);
    if (defined_flex_bodies <= 0) {
        err = error_set(FEM_ERROR_INVALID_INPUT,
                        "Coupled delayed cosim step requires at least one defined flexible body");
        goto cleanup;
    }
    CHECK_ERROR(coupled_step_history2d_reserve_flex_body_storage(
        history,
        run->case_data.num_flex_bodies));
    CHECK_ERROR(coupled_step_delayed_cosim2d_reserve_buffers(&delay_buffers,
                                                             &delay_buffer_capacity,
                                                             run->case_data.num_flex_bodies));
    buffered_total_body_force = calloc((size_t)defined_flex_bodies,
                                       sizeof(*buffered_total_body_force));
    active_slots = calloc((size_t)defined_flex_bodies, sizeof(*active_slots));
    base_force = calloc((size_t)run->mbd_system.num_bodies, sizeof(*base_force));
    if (!buffered_total_body_force || !active_slots || !base_force) {
        err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                        "Failed to allocate delayed cosim local scratch");
        goto cleanup;
    }

    if (step_index <= 1) {
        coupled_step_delayed_cosim2d_reset_buffers(delay_buffers, delay_buffer_capacity);
    }

    history->step_index = step_index;
    history->time = run->time.dt * (double)step_index;
    history->constraint_residual_l2 = 0.0;
    history->coupling_residual_l2 = 0.0;
    history->flex_solves = 0;
    history->fixed_point_iterations = 1;
    history->coupling_converged = 1;
    history->exchange_lag_steps = 1;
    history->sample_hold_active = 0;
    history->delayed_snapshot_step = 0;

    err = mbd_system2d_capture_body_forces(&run->mbd_system, base_force);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    base_force_valid = 1;
    err = mbd_system2d_clear_flexible_forces(&run->mbd_system);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = mbd_system2d_sync_body_states(&run->mbd_system);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                      &residual_l2_before,
                                                      &num_equations_before);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    defined_flex_bodies = coupled_step_delayed_cosim2d_collect_active_slots(run,
                                                                             active_slots,
                                                                             defined_flex_bodies);
    if (defined_flex_bodies <= 0) {
        err = error_set(FEM_ERROR_INVALID_INPUT,
                        "Coupled delayed cosim step could not resolve defined flexible bodies");
        goto cleanup;
    }
    if (defined_flex_bodies >= 2) {
        solver_route_class =
            "partitioned_delayed_cosim_sample_hold_2link_body_interface_skeleton";
    }

    for (active_body_index = 0; active_body_index < defined_flex_bodies;
         ++active_body_index) {
        const int flex_slot = active_slots[active_body_index];

        if (!delay_buffers[flex_slot].valid) {
            continue;
        }

        history->sample_hold_active = 1;
        if (history->delayed_snapshot_step <= 0 ||
            delay_buffers[flex_slot].source_step < history->delayed_snapshot_step) {
            history->delayed_snapshot_step = delay_buffers[flex_slot].source_step;
        }
        memcpy(delayed_total_body_force,
               delay_buffers[flex_slot].total_body_force,
               sizeof(delayed_total_body_force));
        delayed_total_body_force_sum[0] += delayed_total_body_force[0];
        delayed_total_body_force_sum[1] += delayed_total_body_force[1];
        delayed_total_body_force_sum[2] += delayed_total_body_force[2];
        err = mbd_system2d_add_flexible_generalized_force(
            &run->mbd_system,
            run->case_data.flex_bodies[flex_slot].body_id,
            delay_buffers[flex_slot].total_body_force);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    printf("  coupled_step=%d/%d t=%.6e scheme=delayed_cosim_v1_5 path_class=experimental sequence=sample_hold_apply->mbd_accept->snapshot_buffer\n",
           step_index,
           run->time.num_steps,
           history->time);
    printf("    coupling_role: year1 experimental delayed co-simulation comparison lane with minimal lag-1 sample-hold skeleton\n");
    printf("    comparison_role=co_simulation solver_route_class=%s delay_semantics_status=lag1_sample_hold_accepted_previous_step_skeleton v2_decision_state=undecided\n",
           solver_route_class);
    printf("    delayed_cosim_buffer: step_runner=coupled_step_delayed_cosim2d_run solver_route_class=%s integrator=%s feedback_to_mbd=1 body_count=%d interface_count=%d exchange_lag_steps=%d sample_hold_active=%d delayed_snapshot_step=%d snapshot_provenance=accepted_pose_end_of_step exchange_contract=accepted_previous_step_force_hold monolithic_strong_v1!=delayed_cosim_v1_5 residual_l2_before=%.6e equations_before=%d\n",
           solver_route_class,
           coupled_integrator_to_string(run->time.integrator),
           defined_flex_bodies,
           defined_flex_bodies,
           history->exchange_lag_steps,
           history->sample_hold_active,
           history->delayed_snapshot_step,
           residual_l2_before,
           num_equations_before);
    for (active_body_index = 0; active_body_index < defined_flex_bodies;
         ++active_body_index) {
        const int flex_slot = active_slots[active_body_index];

        printf("    delayed_cosim_body_loop: slot=%d/%d body_id=%d buffer_valid=%d sample_hold_force=(%.6e,%.6e,%.6e)\n",
               active_body_index + 1,
               defined_flex_bodies,
               run->case_data.flex_bodies[flex_slot].body_id,
               delay_buffers[flex_slot].valid ? 1 : 0,
               delay_buffers[flex_slot].total_body_force[0],
               delay_buffers[flex_slot].total_body_force[1],
               delay_buffers[flex_slot].total_body_force[2]);
    }
    for (active_body_index = 0; active_body_index < defined_flex_bodies;
         ++active_body_index) {
        const int flex_slot = active_slots[active_body_index];

        printf("    delayed_cosim_interface_loop: slot=%d/%d body_id=%d interface_id=%d exchange_lag_steps=%d delayed_snapshot_step=%d\n",
               active_body_index + 1,
               defined_flex_bodies,
               run->case_data.flex_bodies[flex_slot].body_id,
               active_body_index + 1,
               history->exchange_lag_steps,
               history->delayed_snapshot_step);
    }
    printf("      sample_hold_force_sum=(%.6e,%.6e,%.6e)\n",
           delayed_total_body_force_sum[0],
           delayed_total_body_force_sum[1],
           delayed_total_body_force_sum[2]);

    err = coupled_step_common2d_do_mbd_step(run);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    coupled_step_common2d_sync_mbd_time(run, mbd_integrator);
    err = mbd_system2d_sync_body_states(&run->mbd_system);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    for (active_body_index = 0; active_body_index < defined_flex_bodies;
         ++active_body_index) {
        const int flex_slot = active_slots[active_body_index];

        err = coupled_step_common2d_solve_flex_snapshot_for_slot(
            run,
            flex_slot,
            NULL,
            0,
            "    ",
            "      ",
            "accepted",
            "delayed_snapshot_force_buffered",
            buffered_total_body_force[active_body_index],
            history);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }

        delay_buffers[flex_slot].valid = 1;
        delay_buffers[flex_slot].source_step = step_index;
        memcpy(delay_buffers[flex_slot].total_body_force,
               buffered_total_body_force[active_body_index],
               sizeof(delay_buffers[flex_slot].total_body_force));
        buffered_total_body_force_sum[0] += buffered_total_body_force[active_body_index][0];
        buffered_total_body_force_sum[1] += buffered_total_body_force[active_body_index][1];
        buffered_total_body_force_sum[2] += buffered_total_body_force[active_body_index][2];
    }

    err = mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                      &residual_l2_after,
                                                      &num_equations_after);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    history->constraint_residual_l2 = residual_l2_after;

    printf("    delayed_cosim_result: equations_after=%d residual_l2_after=%.6e flex_solves=%d body_count=%d interface_count=%d buffered_snapshot_step=%d buffered_force_sum=(%.6e,%.6e,%.6e)\n",
           num_equations_after,
           residual_l2_after,
           history->flex_solves,
           defined_flex_bodies,
           defined_flex_bodies,
           step_index,
           buffered_total_body_force_sum[0],
           buffered_total_body_force_sum[1],
           buffered_total_body_force_sum[2]);

cleanup:
    if (run && base_force_valid) {
        fem_error_t restore_err = mbd_system2d_restore_body_forces(&run->mbd_system,
                                                                   base_force);

        if (err == FEM_SUCCESS && restore_err != FEM_SUCCESS) {
            err = restore_err;
        }
    }
    if (run) {
        fem_error_t cleanup_err = mbd_system2d_clear_flexible_forces(&run->mbd_system);

        if (err == FEM_SUCCESS && cleanup_err != FEM_SUCCESS) {
            err = cleanup_err;
        }
    }
    free(base_force);
    free(active_slots);
    free(buffered_total_body_force);
    return err;
}
