#include "coupled_monolithic_state2d.h"

#include "../common/error.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void coupled_monolithic_state2d_zero(coupled_monolithic_state2d_t *state)
{
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
}

void coupled_monolithic_state2d_free(coupled_monolithic_state2d_t *state)
{
    if (!state) {
        return;
    }

    free(state->active_flex_body_ids);
    free(state->target_unknowns);
    free(state->reduced_unknowns);
    free(state->reduced_residual);
    free(state->tangent);
    free(state->linear_rhs);
    free(state->delta_unknowns);

    coupled_monolithic_state2d_zero(state);
}

fem_error_t coupled_monolithic_state2d_reserve_active_flex_body_ids(
    coupled_monolithic_state2d_t *state,
    int required_capacity)
{
    int *active_flex_body_ids = NULL;
    size_t copy_count = 0;

    CHECK_NULL(state, "monolithic state");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic active flex body capacity %d must be positive",
                         required_capacity);
    }
    if (state->active_flex_body_capacity >= required_capacity &&
        state->active_flex_body_ids) {
        return FEM_SUCCESS;
    }

    active_flex_body_ids = calloc((size_t)required_capacity,
                                  sizeof(*active_flex_body_ids));
    if (!active_flex_body_ids) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate monolithic active flex body ids for %d bodies",
                         required_capacity);
    }

    if (state->active_flex_body_ids && state->active_flex_body_capacity > 0) {
        copy_count = (size_t)(state->active_flex_body_capacity < required_capacity
                                  ? state->active_flex_body_capacity
                                  : required_capacity);
        memcpy(active_flex_body_ids,
               state->active_flex_body_ids,
               copy_count * sizeof(*active_flex_body_ids));
    }

    free(state->active_flex_body_ids);
    state->active_flex_body_ids = active_flex_body_ids;
    state->active_flex_body_capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t coupled_monolithic_state2d_reserve_unknown_storage(
    coupled_monolithic_state2d_t *state,
    int required_unknowns)
{
    size_t n = 0;
    size_t matrix_entries = 0;
    double *target_unknowns = NULL;
    double *reduced_unknowns = NULL;
    double *reduced_residual = NULL;
    double *tangent = NULL;
    double *linear_rhs = NULL;
    double *delta_unknowns = NULL;

    CHECK_NULL(state, "monolithic state");
    if (required_unknowns <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic unknown count %d must be positive",
                         required_unknowns);
    }
    if (state->unknown_capacity >= required_unknowns &&
        state->target_unknowns &&
        state->reduced_unknowns &&
        state->reduced_residual &&
        state->tangent &&
        state->linear_rhs &&
        state->delta_unknowns) {
        return FEM_SUCCESS;
    }

    n = (size_t) required_unknowns;
    if (n > SIZE_MAX / n) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "monolithic tangent size overflow for unknown count %d",
                         required_unknowns);
    }
    matrix_entries = n * n;
    if (n > SIZE_MAX / sizeof(double) ||
        matrix_entries > SIZE_MAX / sizeof(double)) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "monolithic dense workspace overflow for unknown count %d",
                         required_unknowns);
    }

    target_unknowns = calloc(n, sizeof(double));
    reduced_unknowns = calloc(n, sizeof(double));
    reduced_residual = calloc(n, sizeof(double));
    tangent = calloc(matrix_entries, sizeof(double));
    linear_rhs = calloc(n, sizeof(double));
    delta_unknowns = calloc(n, sizeof(double));

    if (!target_unknowns || !reduced_unknowns || !reduced_residual ||
        !tangent || !linear_rhs || !delta_unknowns) {
        free(target_unknowns);
        free(reduced_unknowns);
        free(reduced_residual);
        free(tangent);
        free(linear_rhs);
        free(delta_unknowns);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate monolithic dense workspace for %d unknowns",
                         required_unknowns);
    }

    free(state->target_unknowns);
    free(state->reduced_unknowns);
    free(state->reduced_residual);
    free(state->tangent);
    free(state->linear_rhs);
    free(state->delta_unknowns);

    state->target_unknowns = target_unknowns;
    state->reduced_unknowns = reduced_unknowns;
    state->reduced_residual = reduced_residual;
    state->tangent = tangent;
    state->linear_rhs = linear_rhs;
    state->delta_unknowns = delta_unknowns;
    state->unknown_capacity = required_unknowns;

    return FEM_SUCCESS;
}

int coupled_monolithic_state2d_coupled_unknowns(
    const coupled_monolithic_state2d_t *state)
{
    if (!state) {
        return 0;
    }

    return state->layout.coupled_unknowns;
}

int coupled_monolithic_state2d_body_count(
    const coupled_monolithic_state2d_t *state)
{
    if (!state) {
        return 0;
    }

    return state->layout.body_count;
}

int coupled_monolithic_state2d_interface_count(
    const coupled_monolithic_state2d_t *state)
{
    if (!state) {
        return 0;
    }

    return state->layout.interface_count;
}

const char *coupled_monolithic_state2d_solver_route_class(
    const coupled_monolithic_state2d_t *state)
{
    if (coupled_monolithic_state2d_body_count(state) >= 2) {
        return "single_coupled_system_2link_body_interface_skeleton";
    }

    return "single_coupled_system_1link_newton_minimal_proof";
}

const char *coupled_monolithic_state2d_sequence_name(
    const coupled_monolithic_state2d_t *state)
{
    if (coupled_monolithic_state2d_body_count(state) >= 2) {
        return "dedicated_2link_body_interface_skeleton";
    }

    return "dedicated_1link_newton_minimal_proof";
}
