#include "coupled_run2d.h"

#include "../common/error.h"

/* Owns per-step flex body counter capture into coupled run history.
 * Inputs are the coupled run state, target history record, and current flex
 * body state after step execution. Outputs are updated per-step history flex
 * body counter records. Side effects are reserve/storage update through the
 * existing history storage helpers and counter record writes. Storage
 * allocation helpers remain in coupled_step_history2d.c; dispatch, solver
 * steps, flex model loading, validation, snapshots, writers, and route
 * descriptors remain outside this file. This extraction is for readability and
 * maintainability, not behavior change.
 */

fem_error_t coupled_run2d_capture_step_flex_counters(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history)
{
    int i;

    if (!run || !history) {
        return FEM_SUCCESS;
    }

    CHECK_ERROR(coupled_step_history2d_reserve_flex_body_storage(
        history,
        run->case_data.num_flex_bodies));
    history->flex_body_count = 0;
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];
        const int slot = history->flex_body_count;

        history->flex_body_ids[slot] = body->body_id;
        history->flex_body_full_reassembly_count[slot] =
            run->flex_models[i].full_reassembly_count;
        history->flex_body_static_solve_count[slot] =
            run->flex_models[i].static_solve_count;
        history->flex_body_count += 1;
    }
    return FEM_SUCCESS;
}
