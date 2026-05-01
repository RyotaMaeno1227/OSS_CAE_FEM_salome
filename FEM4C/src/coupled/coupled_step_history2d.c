#include "coupled_run2d.h"

#include "../common/error.h"
#include <stdlib.h>
#include <string.h>

/* Owns per-step history dynamic buffers for coupled routes.
 * Inputs are coupled_step_history2d_t records and requested storage sizes.
 * Outputs are allocated or reset dynamic arrays inside the history object.
 * Side effects are allocation, freeing, copying existing entries, and zero/reset
 * of released pointers/capacities. Route execution and output writing remain in
 * their route/run modules; this extraction is for readability, not behavior.
 */

void coupled_step_history2d_free_dynamic_buffers(coupled_step_history2d_t *history)
{
    if (!history) {
        return;
    }

    free(history->flex_body_ids);
    free(history->flex_body_full_reassembly_count);
    free(history->flex_body_static_solve_count);
    free(history->flex_body_reaction_root);
    free(history->flex_body_reaction_tip);
    free(history->flex_body_root_force);
    free(history->flex_body_tip_force);
    free(history->flex_body_total_force);
    free(history->snapshot_body_ids);
    free(history->snapshot_iteration_indices);
    free(history->snapshot_paths);

    history->flex_body_ids = NULL;
    history->flex_body_full_reassembly_count = NULL;
    history->flex_body_static_solve_count = NULL;
    history->flex_body_reaction_root = NULL;
    history->flex_body_reaction_tip = NULL;
    history->flex_body_root_force = NULL;
    history->flex_body_tip_force = NULL;
    history->flex_body_total_force = NULL;
    history->snapshot_body_ids = NULL;
    history->snapshot_iteration_indices = NULL;
    history->snapshot_paths = NULL;
    history->flex_body_storage_capacity = 0;
    history->snapshot_record_capacity = 0;
}

fem_error_t coupled_step_history2d_reserve_flex_body_storage(
    coupled_step_history2d_t *history,
    int required_capacity)
{
    int *flex_body_ids = NULL;
    int *flex_body_full_reassembly_count = NULL;
    int *flex_body_static_solve_count = NULL;
    double (*flex_body_reaction_root)[3] = NULL;
    double (*flex_body_reaction_tip)[3] = NULL;
    double (*flex_body_root_force)[3] = NULL;
    double (*flex_body_tip_force)[3] = NULL;
    double (*flex_body_total_force)[3] = NULL;

    CHECK_NULL(history, "coupled step history");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled step flex-body capacity %d must be positive",
                         required_capacity);
    }
    if (history->flex_body_storage_capacity >= required_capacity &&
        history->flex_body_ids &&
        history->flex_body_full_reassembly_count &&
        history->flex_body_static_solve_count &&
        history->flex_body_reaction_root &&
        history->flex_body_reaction_tip &&
        history->flex_body_root_force &&
        history->flex_body_tip_force &&
        history->flex_body_total_force) {
        return FEM_SUCCESS;
    }

    flex_body_ids = calloc((size_t)required_capacity, sizeof(*flex_body_ids));
    flex_body_full_reassembly_count =
        calloc((size_t)required_capacity, sizeof(*flex_body_full_reassembly_count));
    flex_body_static_solve_count =
        calloc((size_t)required_capacity, sizeof(*flex_body_static_solve_count));
    flex_body_reaction_root =
        calloc((size_t)required_capacity, sizeof(*flex_body_reaction_root));
    flex_body_reaction_tip =
        calloc((size_t)required_capacity, sizeof(*flex_body_reaction_tip));
    flex_body_root_force =
        calloc((size_t)required_capacity, sizeof(*flex_body_root_force));
    flex_body_tip_force =
        calloc((size_t)required_capacity, sizeof(*flex_body_tip_force));
    flex_body_total_force =
        calloc((size_t)required_capacity, sizeof(*flex_body_total_force));
    if (!flex_body_ids ||
        !flex_body_full_reassembly_count ||
        !flex_body_static_solve_count ||
        !flex_body_reaction_root ||
        !flex_body_reaction_tip ||
        !flex_body_root_force ||
        !flex_body_tip_force ||
        !flex_body_total_force) {
        free(flex_body_ids);
        free(flex_body_full_reassembly_count);
        free(flex_body_static_solve_count);
        free(flex_body_reaction_root);
        free(flex_body_reaction_tip);
        free(flex_body_root_force);
        free(flex_body_tip_force);
        free(flex_body_total_force);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate coupled step flex-body storage");
    }

    if (history->flex_body_count > 0) {
        memcpy(flex_body_ids,
               history->flex_body_ids,
               (size_t)history->flex_body_count * sizeof(*flex_body_ids));
        memcpy(flex_body_full_reassembly_count,
               history->flex_body_full_reassembly_count,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_full_reassembly_count));
        memcpy(flex_body_static_solve_count,
               history->flex_body_static_solve_count,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_static_solve_count));
        memcpy(flex_body_reaction_root,
               history->flex_body_reaction_root,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_reaction_root));
        memcpy(flex_body_reaction_tip,
               history->flex_body_reaction_tip,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_reaction_tip));
        memcpy(flex_body_root_force,
               history->flex_body_root_force,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_root_force));
        memcpy(flex_body_tip_force,
               history->flex_body_tip_force,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_tip_force));
        memcpy(flex_body_total_force,
               history->flex_body_total_force,
               (size_t)history->flex_body_count
                   * sizeof(*flex_body_total_force));
    }

    free(history->flex_body_ids);
    free(history->flex_body_full_reassembly_count);
    free(history->flex_body_static_solve_count);
    free(history->flex_body_reaction_root);
    free(history->flex_body_reaction_tip);
    free(history->flex_body_root_force);
    free(history->flex_body_tip_force);
    free(history->flex_body_total_force);

    history->flex_body_ids = flex_body_ids;
    history->flex_body_full_reassembly_count = flex_body_full_reassembly_count;
    history->flex_body_static_solve_count = flex_body_static_solve_count;
    history->flex_body_reaction_root = flex_body_reaction_root;
    history->flex_body_reaction_tip = flex_body_reaction_tip;
    history->flex_body_root_force = flex_body_root_force;
    history->flex_body_tip_force = flex_body_tip_force;
    history->flex_body_total_force = flex_body_total_force;
    history->flex_body_storage_capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t coupled_step_history2d_reserve_snapshot_storage(
    coupled_step_history2d_t *history,
    int required_capacity)
{
    int *snapshot_body_ids = NULL;
    int *snapshot_iteration_indices = NULL;
    char (*snapshot_paths)[MAX_FILENAME_LEN] = NULL;

    CHECK_NULL(history, "coupled step history");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled snapshot capacity %d must be positive",
                         required_capacity);
    }
    if (history->snapshot_record_capacity >= required_capacity &&
        history->snapshot_body_ids &&
        history->snapshot_iteration_indices &&
        history->snapshot_paths) {
        return FEM_SUCCESS;
    }

    snapshot_body_ids = calloc((size_t)required_capacity, sizeof(*snapshot_body_ids));
    snapshot_iteration_indices =
        calloc((size_t)required_capacity, sizeof(*snapshot_iteration_indices));
    snapshot_paths = calloc((size_t)required_capacity, sizeof(*snapshot_paths));
    if (!snapshot_body_ids || !snapshot_iteration_indices || !snapshot_paths) {
        free(snapshot_body_ids);
        free(snapshot_iteration_indices);
        free(snapshot_paths);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate coupled snapshot storage");
    }

    if (history->snapshot_record_count > 0) {
        memcpy(snapshot_body_ids,
               history->snapshot_body_ids,
               (size_t)history->snapshot_record_count * sizeof(*snapshot_body_ids));
        memcpy(snapshot_iteration_indices,
               history->snapshot_iteration_indices,
               (size_t)history->snapshot_record_count
                   * sizeof(*snapshot_iteration_indices));
        memcpy(snapshot_paths,
               history->snapshot_paths,
               (size_t)history->snapshot_record_count * sizeof(*snapshot_paths));
    }

    free(history->snapshot_body_ids);
    free(history->snapshot_iteration_indices);
    free(history->snapshot_paths);

    history->snapshot_body_ids = snapshot_body_ids;
    history->snapshot_iteration_indices = snapshot_iteration_indices;
    history->snapshot_paths = snapshot_paths;
    history->snapshot_record_capacity = required_capacity;
    return FEM_SUCCESS;
}
