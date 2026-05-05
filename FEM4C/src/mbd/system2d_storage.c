#include "system2d.h"
#include "../common/error.h"
#include <stdlib.h>
#include <string.h>

/*
 * Owns body/constraint storage helper implementations for mbd_system2d_t.
 * Inputs are the system object and body/constraint data; outputs and side
 * effects are body/constraint storage allocation, append, lookup, count
 * updates, and related error handling. Lifecycle defaults, input parsing,
 * solver orchestration, contact-load bridge, output bridge, static/coupled
 * bridge, and file IO remain in other files. This extraction is for
 * readability and maintainability, not behavior change.
 */

static size_t mbd_system2d_storage_body_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(mbd_body2d_t);
}

static size_t mbd_system2d_storage_body_force_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(double[MBD_BODY2D_DOF]);
}

static size_t mbd_system2d_storage_constraint_bytes(int constraint_capacity)
{
    if (constraint_capacity <= 0) {
        return 0;
    }
    return (size_t) constraint_capacity * sizeof(mbd_constraint2d_t);
}

void mbd_system2d_release_body_storage(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }

    free(system->bodies);
    free(system->body_states);
    free(system->flexible_force);
    free(system->contact_force);
    free(system->current_generalized_force);
    free(system->previous_generalized_force);
    system->bodies = NULL;
    system->body_states = NULL;
    system->flexible_force = NULL;
    system->contact_force = NULL;
    system->current_generalized_force = NULL;
    system->previous_generalized_force = NULL;
    system->body_capacity = 0;
}

void mbd_system2d_release_constraint_storage(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }

    free(system->constraints);
    system->constraints = NULL;
    system->constraint_capacity = 0;
}

fem_error_t mbd_system2d_reserve_body_storage(mbd_system2d_t *system,
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

    CHECK_NULL(system, "mbd_system2d");

    if (required_capacity < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "mbd body capacity %d must be non-negative",
                         required_capacity);
    }
    if (required_capacity <= system->body_capacity) {
        return FEM_SUCCESS;
    }
    if (required_capacity == 0) {
        return FEM_SUCCESS;
    }

    old_capacity = system->body_capacity;
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
                         "Failed to allocate mbd body storage for capacity %d",
                         required_capacity);
    }

    if (old_capacity > 0) {
        memcpy(new_bodies, system->bodies, mbd_system2d_storage_body_bytes(old_capacity));
        memcpy(new_body_states,
               system->body_states,
               (size_t) old_capacity * sizeof(*new_body_states));
        memcpy(new_flexible_force,
               system->flexible_force,
               mbd_system2d_storage_body_force_bytes(old_capacity));
        memcpy(new_contact_force,
               system->contact_force,
               mbd_system2d_storage_body_force_bytes(old_capacity));
        memcpy(new_current_generalized_force,
               system->current_generalized_force,
               mbd_system2d_storage_body_force_bytes(old_capacity));
        memcpy(new_previous_generalized_force,
               system->previous_generalized_force,
               mbd_system2d_storage_body_force_bytes(old_capacity));
    }
    for (body_index = old_capacity; body_index < required_capacity; ++body_index) {
        mbd_body2d_zero(&new_bodies[body_index]);
    }

    free(system->bodies);
    free(system->body_states);
    free(system->flexible_force);
    free(system->contact_force);
    free(system->current_generalized_force);
    free(system->previous_generalized_force);
    system->bodies = new_bodies;
    system->body_states = new_body_states;
    system->flexible_force = new_flexible_force;
    system->contact_force = new_contact_force;
    system->current_generalized_force = new_current_generalized_force;
    system->previous_generalized_force = new_previous_generalized_force;
    system->body_capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_reserve_constraint_storage(mbd_system2d_t *system,
                                                    int required_capacity)
{
    mbd_constraint2d_t *new_constraints = NULL;

    CHECK_NULL(system, "mbd_system2d");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "mbd constraint capacity %d must be positive",
                         required_capacity);
    }
    if (required_capacity <= system->constraint_capacity) {
        return FEM_SUCCESS;
    }

    new_constraints = (mbd_constraint2d_t *) calloc((size_t) required_capacity,
                                                    sizeof(*new_constraints));
    if (!new_constraints) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate mbd constraint storage for capacity %d",
                         required_capacity);
    }
    if (system->num_constraints > 0 && system->constraints) {
        memcpy(new_constraints,
               system->constraints,
               mbd_system2d_storage_constraint_bytes(system->num_constraints));
    }

    free(system->constraints);
    system->constraints = new_constraints;
    system->constraint_capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_find_body_index_by_id(const mbd_system2d_t *system,
                                               int body_id,
                                               int *body_index)
{
    int i;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body_index, "body_index");

    for (i = 0; i < system->num_bodies; ++i) {
        if (system->bodies[i].id == body_id) {
            *body_index = i;
            return FEM_SUCCESS;
        }
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "MBD flexible force references undefined body id %d",
                     body_id);
}

fem_error_t mbd_system2d_get_body_const(const mbd_system2d_t *system,
                                        int body_id,
                                        const mbd_body2d_t **body)
{
    int body_index = -1;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body, "mbd_body2d");
    CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, body_id, &body_index));

    *body = &system->bodies[body_index];
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_get_body_mut(mbd_system2d_t *system,
                                      int body_id,
                                      mbd_body2d_t **body)
{
    int body_index = -1;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body, "mbd_body2d");
    CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, body_id, &body_index));

    *body = &system->bodies[body_index];
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_add_body(mbd_system2d_t *system,
                                  int body_index,
                                  const mbd_body2d_t *body)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body, "mbd_body2d");

    if (body_index < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "mbd body index %d must be non-negative",
                         body_index);
    }
    CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));

    system->bodies[body_index] = *body;
    CHECK_ERROR(mbd_body2d_to_state_view(body, &system->body_states[body_index]));
    if (body_index + 1 > system->num_bodies) {
        system->num_bodies = body_index + 1;
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_add_body_state(mbd_system2d_t *system,
                                        int body_index,
                                        const mbd_body_state2d_t *state)
{
    double q[3];

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(state, "mbd_body_state2d");

    if (body_index < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "mbd body index %d must be non-negative",
                         body_index);
    }
    CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));

    system->body_states[body_index] = *state;
    q[0] = state->x;
    q[1] = state->y;
    q[2] = state->theta;
    CHECK_ERROR(mbd_body2d_init_dyn(&system->bodies[body_index], body_index,
                                    MBD_BODY2D_DEFAULT_MASS,
                                    MBD_BODY2D_DEFAULT_INERTIA,
                                    q, NULL));
    if (body_index + 1 > system->num_bodies) {
        system->num_bodies = body_index + 1;
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_sync_body_states(mbd_system2d_t *system)
{
    int i;

    CHECK_NULL(system, "mbd_system2d");

    for (i = 0; i < system->num_bodies; ++i) {
        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[i], &system->body_states[i]));
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_append_constraint(mbd_system2d_t *system,
                                           const mbd_constraint2d_t *constraint)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(constraint, "mbd_constraint2d");
    CHECK_ERROR(mbd_constraint_validate(constraint));
    CHECK_ERROR(mbd_system2d_reserve_constraint_storage(system,
                                                        system->num_constraints + 1));
    system->constraints[system->num_constraints] = *constraint;
    ++system->num_constraints;
    return FEM_SUCCESS;
}
