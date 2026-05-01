#include "coupled_step_monolithic2d.h"

#include "coupled_monolithic_assembly2d.h"
#include "../common/error.h"

#include <math.h>
#include <stdio.h>

static const char *coupled_step_monolithic2d_reason_code(int converged)
{
    return converged
        ? "residual_below_tolerance"
        : "max_iterations_reached";
}

fem_error_t coupled_step_monolithic2d_run(coupled_run2d_t *run,
                                          int step_index,
                                          coupled_step_history2d_t *history)
{
    double constraint_residual_l2 = 0.0;
    double residual_tolerance = 0.0;
    int constraint_equations = 0;
    int body_slot = 0;
    int max_iterations = 0;
    int iteration = 0;
    int converged = 0;
    coupled_monolithic_state2d_t state;

    coupled_monolithic_state2d_zero(&state);
    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled monolithic history");
    CHECK_ERROR_CLEANUP(mbd_system2d_sync_body_states(&run->mbd_system),
                        coupled_monolithic_state2d_free(&state));
    CHECK_ERROR_CLEANUP(mbd_system2d_compute_constraint_residual_l2(&run->mbd_system,
                                                                    &constraint_residual_l2,
                                                                    &constraint_equations),
                        coupled_monolithic_state2d_free(&state));
    CHECK_ERROR_CLEANUP(coupled_monolithic_assembly2d_prepare(&state, run),
                        coupled_monolithic_state2d_free(&state));

    history->step_index = step_index;
    history->time = run->time.dt * (double)step_index;
    history->constraint_residual_l2 = constraint_residual_l2;
    history->coupling_residual_l2 = 0.0;
    history->flex_solves = 0;
    history->fixed_point_iterations = 0;
    history->coupling_converged = 0;
    history->coupling_reason[0] = '\0';

    printf("  coupled_step=%d/%d t=%.6e scheme=monolithic_strong_v1 path_class=experimental sequence=%s\n",
           step_index,
           run->time.num_steps,
           history->time,
           coupled_monolithic_state2d_sequence_name(&state));
    printf("    coupling_role: year1 experimental monolithic strong comparison lane %s\n",
           coupled_monolithic_state2d_body_count(&state) >= 2
               ? "dedicated 2-link body/interface skeleton"
               : "dedicated 1-link Newton minimal proof");
    printf("    comparison_role=monolithic_strong year1_experimental_only=1 fixed_point_strong!=monolithic_strong_v1\n");
    printf("    monolithic_layout: step_runner=coupled_step_monolithic2d_run solver_route_class=%s integrator=%s body_count=%d interface_count=%d coupled_unknowns=%d rigid_unknowns=%d flex_unknowns=%d joint_multipliers=%d interface_multipliers=%d constraint_equations=%d constraint_residual_l2=%.6e\n",
           coupled_monolithic_state2d_solver_route_class(&state),
           coupled_integrator_to_string(run->time.integrator),
           coupled_monolithic_state2d_body_count(&state),
           coupled_monolithic_state2d_interface_count(&state),
           coupled_monolithic_state2d_coupled_unknowns(&state),
           state.layout.num_rigid_unknowns,
           state.layout.num_flex_unknowns,
           state.layout.num_joint_multipliers,
           state.layout.num_interface_multipliers,
           constraint_equations,
           constraint_residual_l2);
    for (body_slot = 0; body_slot < state.layout.body_count; ++body_slot) {
        printf("    monolithic_body_loop: slot=%d/%d body_id=%d rigid_unknown_offset=%d flex_unknown_offset=%d\n",
               body_slot + 1,
               state.layout.body_count,
               state.active_flex_body_ids[body_slot],
               3 * body_slot,
               state.layout.num_rigid_unknowns + 3 * body_slot);
    }
    for (body_slot = 0; body_slot < state.layout.interface_count; ++body_slot) {
        printf("    monolithic_interface_loop: slot=%d/%d body_id=%d interface_unknown_offset=%d\n",
               body_slot + 1,
               state.layout.interface_count,
               state.active_flex_body_ids[body_slot],
               state.layout.num_rigid_unknowns +
                   state.layout.num_flex_unknowns +
                   state.layout.num_joint_multipliers +
                   3 * body_slot);
    }

    residual_tolerance = run->time.residual_tolerance;
    if (!isfinite(residual_tolerance) || residual_tolerance <= 0.0) {
        residual_tolerance = 1.0e-6;
    }
    max_iterations = state.iteration_limit > 0 ? state.iteration_limit : 1;
    printf("    monolithic_newton: max_iter=%d residual_tol=%.6e reduced_flex_bodies=%d reason_surface=convergence_reason|divergence_reason\n",
           max_iterations,
           residual_tolerance,
           state.layout.reduced_flex_body_count);

    for (iteration = 0; iteration < max_iterations; ++iteration) {
        CHECK_ERROR_CLEANUP(coupled_monolithic_assembly2d_assemble(&state,
                                                                   run,
                                                                   iteration + 1),
                            coupled_monolithic_state2d_free(&state));
        history->fixed_point_iterations = iteration + 1;
        history->coupling_residual_l2 = state.last_iteration.residual_norm;

        if (state.last_iteration.residual_norm <= residual_tolerance) {
            converged = 1;
            history->coupling_converged = 1;
            printf("      monolithic_iteration=%d/%d residual_norm=%.6e physical_residual_l2=%.6e coupled_unknowns=%d linear_solve_residual_inf=%.6e status=converged convergence_reason=%s\n",
                   iteration + 1,
                   max_iterations,
                   state.last_iteration.residual_norm,
                   history->coupling_residual_l2,
                   state.last_iteration.coupled_unknowns,
                   state.last_iteration.linear_solve_residual_inf,
                   coupled_step_monolithic2d_reason_code(1));
            break;
        }

        CHECK_ERROR_CLEANUP(coupled_monolithic_assembly2d_solve(&state),
                            coupled_monolithic_state2d_free(&state));
        printf("      monolithic_iteration=%d/%d residual_norm=%.6e physical_residual_l2=%.6e coupled_unknowns=%d linear_solve_residual_inf=%.6e status=update_applied\n",
               iteration + 1,
               max_iterations,
               state.last_iteration.residual_norm,
               history->coupling_residual_l2,
               state.last_iteration.coupled_unknowns,
               state.last_iteration.linear_solve_residual_inf);
        coupled_monolithic_assembly2d_apply_update(&state);
    }

    if (!converged) {
        snprintf(history->coupling_reason,
                 sizeof(history->coupling_reason),
                 "%s",
                 coupled_step_monolithic2d_reason_code(0));
        printf("    monolithic_status: converged=0 iterations=%d final_residual_norm=%.6e physical_residual_l2=%.6e constraint_residual_l2=%.6e divergence_reason=%s\n",
               history->fixed_point_iterations,
               history->coupling_residual_l2,
               history->coupling_residual_l2,
               history->constraint_residual_l2,
               coupled_step_monolithic2d_reason_code(0));
    } else {
        snprintf(history->coupling_reason,
                 sizeof(history->coupling_reason),
                 "%s",
                 coupled_step_monolithic2d_reason_code(1));
        printf("    monolithic_status: converged=1 iterations=%d final_residual_norm=%.6e physical_residual_l2=%.6e constraint_residual_l2=%.6e convergence_reason=%s\n",
               history->fixed_point_iterations,
               history->coupling_residual_l2,
               history->coupling_residual_l2,
               history->constraint_residual_l2,
               coupled_step_monolithic2d_reason_code(1));
    }

    coupled_monolithic_state2d_free(&state);
    return FEM_SUCCESS;
}
