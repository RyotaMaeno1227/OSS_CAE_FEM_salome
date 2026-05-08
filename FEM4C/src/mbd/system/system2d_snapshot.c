#include "system2d.h"
#include "../../common/error.h"
#include <stdlib.h>
#include <string.h>

/*
 * Owns narrow snapshot/state helper implementations for mbd_system2d_t.
 * Inputs are system object pointers and snapshot state data; outputs and side
 * effects are snapshot storage allocation/release, capture, restore,
 * count/state copy, and related error handling. Lifecycle/default config,
 * body/constraint storage, contact registration, solver orchestration,
 * contact runtime refresh, input parsing, contact-load bridge, output bridge,
 * static/coupled bridge, and file IO remain in other files. This extraction
 * is for readability and maintainability, not behavior change.
 */

static size_t mbd_system2d_snapshot_body_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(mbd_body2d_t);
}

static size_t mbd_system2d_snapshot_body_force_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(double[MBD_BODY2D_DOF]);
}

void mbd_system2d_snapshot_release_body_storage(mbd_system2d_snapshot_t *snapshot)
{
    if (!snapshot) {
        return;
    }

    free(snapshot->bodies);
    free(snapshot->body_states);
    free(snapshot->flexible_force);
    free(snapshot->contact_force);
    free(snapshot->current_generalized_force);
    free(snapshot->previous_generalized_force);
    snapshot->bodies = NULL;
    snapshot->body_states = NULL;
    snapshot->flexible_force = NULL;
    snapshot->contact_force = NULL;
    snapshot->current_generalized_force = NULL;
    snapshot->previous_generalized_force = NULL;
    snapshot->body_capacity = 0;
}

fem_error_t mbd_system2d_snapshot_reserve_body_storage(mbd_system2d_snapshot_t *snapshot,
                                                       int required_capacity)
{
    mbd_body2d_t *new_bodies = NULL;
    mbd_body_state2d_t *new_body_states = NULL;
    double (*new_flexible_force)[MBD_BODY2D_DOF] = NULL;
    double (*new_contact_force)[MBD_BODY2D_DOF] = NULL;
    double (*new_current_generalized_force)[MBD_BODY2D_DOF] = NULL;
    double (*new_previous_generalized_force)[MBD_BODY2D_DOF] = NULL;
    int old_capacity;
    int body_index;

    CHECK_NULL(snapshot, "mbd_system2d snapshot");

    if (required_capacity < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "mbd snapshot body capacity %d must be non-negative",
                         required_capacity);
    }
    if (required_capacity <= snapshot->body_capacity) {
        return FEM_SUCCESS;
    }
    if (required_capacity == 0) {
        return FEM_SUCCESS;
    }

    old_capacity = snapshot->body_capacity;
    new_bodies = (mbd_body2d_t *) calloc((size_t) required_capacity, sizeof(*new_bodies));
    new_body_states = (mbd_body_state2d_t *) calloc((size_t) required_capacity,
                                                    sizeof(*new_body_states));
    new_flexible_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_flexible_force));
    new_contact_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_contact_force));
    new_current_generalized_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_current_generalized_force));
    new_previous_generalized_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_previous_generalized_force));
    if (!new_bodies || !new_body_states || !new_flexible_force ||
        !new_contact_force || !new_current_generalized_force ||
        !new_previous_generalized_force) {
        free(new_bodies);
        free(new_body_states);
        free(new_flexible_force);
        free(new_contact_force);
        free(new_current_generalized_force);
        free(new_previous_generalized_force);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate mbd snapshot body storage for capacity %d",
                         required_capacity);
    }

    if (old_capacity > 0) {
        memcpy(new_bodies,
               snapshot->bodies,
               mbd_system2d_snapshot_body_bytes(old_capacity));
        memcpy(new_body_states,
               snapshot->body_states,
               (size_t) old_capacity * sizeof(*new_body_states));
        memcpy(new_flexible_force,
               snapshot->flexible_force,
               mbd_system2d_snapshot_body_force_bytes(old_capacity));
        memcpy(new_contact_force,
               snapshot->contact_force,
               mbd_system2d_snapshot_body_force_bytes(old_capacity));
        memcpy(new_current_generalized_force,
               snapshot->current_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(old_capacity));
        memcpy(new_previous_generalized_force,
               snapshot->previous_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(old_capacity));
    }
    for (body_index = old_capacity; body_index < required_capacity; ++body_index) {
        mbd_body2d_zero(&new_bodies[body_index]);
    }

    free(snapshot->bodies);
    free(snapshot->body_states);
    free(snapshot->flexible_force);
    free(snapshot->contact_force);
    free(snapshot->current_generalized_force);
    free(snapshot->previous_generalized_force);
    snapshot->bodies = new_bodies;
    snapshot->body_states = new_body_states;
    snapshot->flexible_force = new_flexible_force;
    snapshot->contact_force = new_contact_force;
    snapshot->current_generalized_force = new_current_generalized_force;
    snapshot->previous_generalized_force = new_previous_generalized_force;
    snapshot->body_capacity = required_capacity;
    return FEM_SUCCESS;
}

void mbd_system2d_snapshot_free(mbd_system2d_snapshot_t *snapshot)
{
    if (!snapshot) {
        return;
    }
    mbd_system2d_snapshot_release_body_storage(snapshot);
    memset(snapshot, 0, sizeof(*snapshot));
}

fem_error_t mbd_system2d_snapshot_capture(mbd_system2d_snapshot_t *snapshot,
                                          const mbd_system2d_t *src)
{
    int pair_index;

    CHECK_NULL(snapshot, "mbd_system2d snapshot");
    CHECK_NULL(src, "mbd_system2d snapshot src");

    mbd_system2d_snapshot_free(snapshot);
    CHECK_ERROR(mbd_system2d_snapshot_reserve_body_storage(snapshot, src->num_bodies));
    snapshot->body_capacity = src->num_bodies;
    snapshot->num_bodies = src->num_bodies;
    if (src->num_bodies > 0) {
        memcpy(snapshot->bodies,
               src->bodies,
               mbd_system2d_snapshot_body_bytes(src->num_bodies));
        memcpy(snapshot->body_states,
               src->body_states,
               (size_t) src->num_bodies * sizeof(*snapshot->body_states));
        memcpy(snapshot->flexible_force,
               src->flexible_force,
               mbd_system2d_snapshot_body_force_bytes(src->num_bodies));
        memcpy(snapshot->contact_force,
               src->contact_force,
               mbd_system2d_snapshot_body_force_bytes(src->num_bodies));
        memcpy(snapshot->current_generalized_force,
               src->current_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(src->num_bodies));
        memcpy(snapshot->previous_generalized_force,
               src->previous_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(src->num_bodies));
    }
    for (pair_index = 0; pair_index < MBD_CONTACT2D_MAX_PAIRS; ++pair_index) {
        memcpy(snapshot->contact_pair_last_normal[pair_index],
               src->contact_pairs[pair_index].last_normal,
               sizeof(snapshot->contact_pair_last_normal[pair_index]));
        snapshot->contact_pair_has_last_normal[pair_index] =
            src->contact_pairs[pair_index].has_last_normal;
    }
    snapshot->generalized_force_history_valid =
        src->generalized_force_history_valid;
    snapshot->steps_executed = src->time.steps_executed;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_snapshot_restore(mbd_system2d_t *dst,
                                          const mbd_system2d_snapshot_t *snapshot)
{
    int pair_index;

    CHECK_NULL(dst, "mbd_system2d snapshot dst");
    CHECK_NULL(snapshot, "mbd_system2d snapshot");

    CHECK_ERROR(mbd_system2d_reserve_body_storage(dst, snapshot->num_bodies));
    dst->num_bodies = snapshot->num_bodies;
    if (snapshot->num_bodies > 0) {
        memcpy(dst->bodies,
               snapshot->bodies,
               mbd_system2d_snapshot_body_bytes(snapshot->num_bodies));
        memcpy(dst->body_states,
               snapshot->body_states,
               (size_t) snapshot->num_bodies * sizeof(*dst->body_states));
        memcpy(dst->flexible_force,
               snapshot->flexible_force,
               mbd_system2d_snapshot_body_force_bytes(snapshot->num_bodies));
        memcpy(dst->contact_force,
               snapshot->contact_force,
               mbd_system2d_snapshot_body_force_bytes(snapshot->num_bodies));
        memcpy(dst->current_generalized_force,
               snapshot->current_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(snapshot->num_bodies));
        memcpy(dst->previous_generalized_force,
               snapshot->previous_generalized_force,
               mbd_system2d_snapshot_body_force_bytes(snapshot->num_bodies));
    }
    for (pair_index = 0; pair_index < MBD_CONTACT2D_MAX_PAIRS; ++pair_index) {
        memcpy(dst->contact_pairs[pair_index].last_normal,
               snapshot->contact_pair_last_normal[pair_index],
               sizeof(dst->contact_pairs[pair_index].last_normal));
        dst->contact_pairs[pair_index].has_last_normal =
            snapshot->contact_pair_has_last_normal[pair_index];
    }
    dst->generalized_force_history_valid =
        snapshot->generalized_force_history_valid;
    dst->time.steps_executed = snapshot->steps_executed;

    return mbd_system2d_sync_body_states(dst);
}
