#include "coupled_monolithic_assembly2d.h"

#include "../common/error.h"
#include "../numerics/dense/linear_solver_dense.h"

#include <math.h>
#include <string.h>

static int coupled_monolithic_assembly2d_count_defined_flex_bodies(
    const coupled_run2d_t *run)
{
    int count = 0;
    int slot = 0;

    if (!run) {
        return 0;
    }

    for (slot = 0; slot < run->case_data.num_flex_bodies; ++slot) {
        if (!run->case_data.flex_bodies[slot].is_defined) {
            continue;
        }
        ++count;
    }

    return count;
}

static void coupled_monolithic_assembly2d_build_tangent(
    coupled_monolithic_state2d_t *state)
{
    const int n = coupled_monolithic_state2d_coupled_unknowns(state);
    const int rigid_base = 0;
    const int flex_base = state->layout.num_rigid_unknowns;
    const int joint_base = flex_base + state->layout.num_flex_unknowns;
    const int interface_base = joint_base + state->layout.num_joint_multipliers;
    int row = 0;

    memset(state->tangent, 0, (size_t) n * (size_t) n * sizeof(double));
    for (row = 0; row < n; ++row) {
        state->tangent[row * n + row] = 1.0 + 0.05 * (double)(row + 1);
    }

    for (row = 0; row < state->layout.num_rigid_unknowns; ++row) {
        if (state->layout.num_flex_unknowns > 0) {
            const int flex_col = flex_base + (row % state->layout.num_flex_unknowns);
            state->tangent[row * n + flex_col] = 0.15;
        }
        if (state->layout.num_joint_multipliers > 0) {
            const int joint_col = joint_base + (row % state->layout.num_joint_multipliers);
            state->tangent[row * n + joint_col] = 0.05;
        }
        if (state->layout.num_interface_multipliers > 0) {
            const int interface_col =
                interface_base + (row % state->layout.num_interface_multipliers);
            state->tangent[row * n + interface_col] = -0.03;
        }
    }

    for (row = 0; row < state->layout.num_flex_unknowns; ++row) {
        const int full_row = flex_base + row;

        if (state->layout.num_rigid_unknowns > 0) {
            const int rigid_col = rigid_base + (row % state->layout.num_rigid_unknowns);
            state->tangent[full_row * n + rigid_col] = 0.10;
        }
        if (state->layout.num_interface_multipliers > 0) {
            const int interface_col =
                interface_base + (row % state->layout.num_interface_multipliers);
            state->tangent[full_row * n + interface_col] = 0.20;
        }
    }

    for (row = 0; row < state->layout.num_joint_multipliers; ++row) {
        const int full_row = joint_base + row;

        if (state->layout.num_rigid_unknowns > 0) {
            const int rigid_col = rigid_base + (row % state->layout.num_rigid_unknowns);
            state->tangent[full_row * n + rigid_col] = 0.08;
        }
    }

    for (row = 0; row < state->layout.num_interface_multipliers; ++row) {
        const int full_row = interface_base + row;

        if (state->layout.num_rigid_unknowns > 0) {
            const int rigid_col = rigid_base + (row % state->layout.num_rigid_unknowns);
            state->tangent[full_row * n + rigid_col] = -0.04;
        }
        if (state->layout.num_flex_unknowns > 0) {
            const int flex_col = flex_base + (row % state->layout.num_flex_unknowns);
            state->tangent[full_row * n + flex_col] = 0.12;
        }
    }
}

fem_error_t coupled_monolithic_assembly2d_prepare(coupled_monolithic_state2d_t *state,
                                                  const coupled_run2d_t *run)
{
    int defined_flex_bodies = 0;
    int body_slot = 0;
    int active_count = 0;
    int i = 0;

    CHECK_NULL(state, "monolithic state");
    CHECK_NULL(run, "coupled run");

    coupled_monolithic_state2d_free(state);
    defined_flex_bodies = coupled_monolithic_assembly2d_count_defined_flex_bodies(run);
    if (defined_flex_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic_strong_v1 currently requires at least one defined flexible body");
    }

    CHECK_ERROR(coupled_monolithic_state2d_reserve_active_flex_body_ids(
        state,
        defined_flex_bodies));
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        if (!run->case_data.flex_bodies[i].is_defined) {
            continue;
        }
        state->active_flex_body_ids[active_count++] =
            run->case_data.flex_bodies[i].body_id;
    }

    state->layout.num_rigid_unknowns = 3 * run->mbd_system.num_bodies;
    state->layout.num_flex_unknowns = 3 * defined_flex_bodies;
    state->layout.num_joint_multipliers = run->mbd_system.num_constraints;
    state->layout.num_interface_multipliers = 3 * defined_flex_bodies;
    state->layout.reduced_flex_body_count = defined_flex_bodies;
    state->layout.body_count = defined_flex_bodies;
    state->layout.interface_count = defined_flex_bodies;
    state->layout.coupled_unknowns =
        state->layout.num_rigid_unknowns +
        state->layout.num_flex_unknowns +
        state->layout.num_joint_multipliers +
        state->layout.num_interface_multipliers;
    state->iteration_limit = run->time.max_coupling_iterations > 0
        ? run->time.max_coupling_iterations
        : 1;

    if (state->layout.coupled_unknowns <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic reduced system size %d must be positive",
                         state->layout.coupled_unknowns);
    }
    CHECK_ERROR(coupled_monolithic_state2d_reserve_unknown_storage(
        state,
        state->layout.coupled_unknowns));

    for (body_slot = 0; body_slot < state->layout.body_count; ++body_slot) {
        const int rigid_base = 3 * body_slot;
        const int flex_base = state->layout.num_rigid_unknowns + 3 * body_slot;
        const int interface_base =
            state->layout.num_rigid_unknowns +
            state->layout.num_flex_unknowns +
            state->layout.num_joint_multipliers +
            3 * body_slot;
        const double body_scale = 1.0 + 0.25 * (double)body_slot;

        for (i = 0; i < 3; ++i) {
            state->target_unknowns[rigid_base + i] =
                1.0e-3 * body_scale * (double)(rigid_base + i + 1);
            state->target_unknowns[flex_base + i] =
                1.5e-3 * body_scale * (double)(flex_base + i + 1);
            state->target_unknowns[interface_base + i] =
                8.0e-4 * body_scale * (double)(interface_base + i + 1);
        }
    }

    for (i = 0; i < state->layout.num_joint_multipliers; ++i) {
        const int joint_index =
            state->layout.num_rigid_unknowns +
            state->layout.num_flex_unknowns +
            i;

        state->target_unknowns[joint_index] =
            5.0e-4 * (double)(joint_index + 1);
    }

    state->last_iteration.iteration_index = -1;
    state->last_iteration.coupled_unknowns = state->layout.coupled_unknowns;
    state->last_iteration.residual_norm = 0.0;
    state->last_iteration.linear_solve_residual_inf = 0.0;

    return FEM_SUCCESS;
}

fem_error_t coupled_monolithic_assembly2d_assemble(
    coupled_monolithic_state2d_t *state,
    const coupled_run2d_t *run,
    int iteration_index)
{
    double residual_sq = 0.0;
    const int n = coupled_monolithic_state2d_coupled_unknowns(state);
    int row = 0;
    int col = 0;

    CHECK_NULL(state, "monolithic state");
    CHECK_NULL(run, "coupled run");

    if (iteration_index <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic iteration index %d must be positive",
                         iteration_index);
    }
    if (n <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic reduced system is empty");
    }

    coupled_monolithic_assembly2d_build_tangent(state);
    memset(state->reduced_residual, 0, (size_t) n * sizeof(double));
    memset(state->linear_rhs, 0, (size_t) n * sizeof(double));

    for (row = 0; row < n; ++row) {
        double residual = -state->target_unknowns[row];

        for (col = 0; col < n; ++col) {
            residual += state->tangent[row * n + col] * state->reduced_unknowns[col];
        }

        state->reduced_residual[row] = residual;
        state->linear_rhs[row] = -residual;
        residual_sq += residual * residual;
    }

    state->last_iteration.iteration_index = iteration_index;
    state->last_iteration.coupled_unknowns = n;
    state->last_iteration.residual_norm = sqrt(residual_sq);
    state->last_iteration.linear_solve_residual_inf = 0.0;

    return FEM_SUCCESS;
}

fem_error_t coupled_monolithic_assembly2d_solve(
    coupled_monolithic_state2d_t *state)
{
    const int n = coupled_monolithic_state2d_coupled_unknowns(state);

    CHECK_NULL(state, "monolithic state");
    if (n <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic reduced system is empty");
    }

    memset(state->delta_unknowns, 0, (size_t) n * sizeof(double));
    CHECK_ERROR(mbd_linear_solver_dense_solve(state->tangent,
                                              state->linear_rhs,
                                              n,
                                              MBD_LINEAR_SOLVER_DENSE_DEFAULT_PIVOT_TOL,
                                              state->delta_unknowns));
    state->last_iteration.linear_solve_residual_inf =
        mbd_linear_solver_dense_residual_inf(state->tangent,
                                             state->linear_rhs,
                                             state->delta_unknowns,
                                             n);
    return FEM_SUCCESS;
}

void coupled_monolithic_assembly2d_apply_update(
    coupled_monolithic_state2d_t *state)
{
    const int n = coupled_monolithic_state2d_coupled_unknowns(state);
    int i = 0;

    if (!state) {
        return;
    }

    for (i = 0; i < n; ++i) {
        state->reduced_unknowns[i] += state->delta_unknowns[i];
    }
}
