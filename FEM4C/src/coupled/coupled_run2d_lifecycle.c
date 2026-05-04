#include "coupled_run2d.h"

#include "../common/error.h"

#include <stdlib.h>
#include <string.h>

/*
 * Owns coupled-run lifecycle and dynamic storage helpers. Inputs are the
 * coupled run object and requested flex model storage capacity; outputs are
 * zeroed, freed, or reserved run storage state. Side effects are memory
 * allocation/free/zeroing and dynamic buffer cleanup. Top-level run
 * orchestration, input loading, route dispatch, solvers, writers, snapshots,
 * validation, and history capture remain outside this file. This extraction is
 * for readability and maintainability, not behavior change.
 */

void coupled_time_control_set_defaults(coupled_time_control_t *time);

void coupled_run2d_zero(coupled_run2d_t *run)
{
    if (!run) {
        return;
    }

    memset(run, 0, sizeof(*run));
    coupled_case2d_zero(&run->case_data);
    mbd_system2d_zero(&run->mbd_system);
    coupled_time_control_set_defaults(&run->time);
}

void coupled_run2d_free_dynamic_buffers(coupled_run2d_t *run)
{
    int i;

    if (!run) {
        return;
    }

    for (i = 0; i < run->flex_model_capacity; ++i) {
        fem_model_free(&run->flex_models[i]);
    }
    free(run->flex_models);
    run->flex_models = NULL;
    run->flex_model_capacity = 0;
}

fem_error_t coupled_run2d_reserve_flex_model_storage(coupled_run2d_t *run,
                                                     int required_capacity)
{
    fem_model_t *new_models = NULL;

    CHECK_NULL(run, "coupled_run2d");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled flex model capacity %d must be positive",
                         required_capacity);
    }
    if (run->flex_model_capacity >= required_capacity &&
        run->flex_models) {
        return FEM_SUCCESS;
    }

    new_models = calloc((size_t)required_capacity, sizeof(*new_models));
    CHECK_NULL(new_models, "coupled flex model storage");
    if (run->flex_model_count > 0 && run->flex_models) {
        memcpy(new_models,
               run->flex_models,
               (size_t)run->flex_model_count * sizeof(*new_models));
    }

    free(run->flex_models);
    run->flex_models = new_models;
    run->flex_model_capacity = required_capacity;
    return FEM_SUCCESS;
}

void coupled_run2d_free(coupled_run2d_t *run)
{
    if (!run) {
        return;
    }

    coupled_run2d_free_dynamic_buffers(run);
    coupled_case2d_free(&run->case_data);
    coupled_run2d_zero(run);
}
