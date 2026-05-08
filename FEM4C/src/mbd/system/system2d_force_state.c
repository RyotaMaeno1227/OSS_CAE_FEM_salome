#include "system2d.h"
#include "forces2d.h"
#include "../../common/error.h"
#include <string.h>

/*
 * Owns narrow force/state/history helper implementations for mbd_system2d_t.
 * Inputs are system object pointers, body ids, generalized force values, and
 * force/state buffers; outputs and side effects are force clearing, force
 * accumulation, history access, body force capture/restore, and related error
 * handling. Lifecycle/default config, body/constraint storage, contact
 * registration, snapshot helpers, clone helper, solver orchestration, contact
 * runtime refresh, input parsing, contact-load bridge, output bridge,
 * static/coupled bridge, and file IO remain in other files. This extraction is
 * for readability and maintainability, not behavior change.
 */

static size_t mbd_system2d_force_state_body_force_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(double[MBD_BODY2D_DOF]);
}

static fem_error_t mbd_system2d_validate_body_index(const mbd_system2d_t *system,
                                                    int body_index)
{
    CHECK_NULL(system, "mbd_system2d");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_clear_flexible_forces(mbd_system2d_t *system)
{
    CHECK_NULL(system, "mbd_system2d");

    if (system->num_bodies > 0 && system->flexible_force) {
        memset(system->flexible_force,
               0,
               mbd_system2d_force_state_body_force_bytes(system->num_bodies));
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_clear_contact_forces(mbd_system2d_t *system)
{
    CHECK_NULL(system, "mbd_system2d");

    if (system->num_bodies > 0 && system->contact_force) {
        memset(system->contact_force,
               0,
               mbd_system2d_force_state_body_force_bytes(system->num_bodies));
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_add_flexible_generalized_force(
    mbd_system2d_t *system,
    int body_id,
    const double generalized_force[MBD_BODY2D_DOF])
{
    int body_index = -1;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(generalized_force, "flexible generalized force");
    CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, body_id, &body_index));

    return mbd_forces2d_add_generalized_force(system->flexible_force[body_index],
                                              generalized_force);
}

fem_error_t mbd_system2d_get_current_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF])
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(generalized_force, "current generalized force");
    CHECK_ERROR(mbd_system2d_validate_body_index(system, body_index));

    if (!system->generalized_force_history_valid) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "current generalized force requested before history refresh");
    }

    memcpy(generalized_force,
           system->current_generalized_force[body_index],
           sizeof(system->current_generalized_force[body_index]));
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_get_previous_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF])
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(generalized_force, "previous generalized force");
    CHECK_ERROR(mbd_system2d_validate_body_index(system, body_index));

    if (!system->generalized_force_history_valid) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "previous generalized force requested before history refresh");
    }

    memcpy(generalized_force,
           system->previous_generalized_force[body_index],
           sizeof(system->previous_generalized_force[body_index]));
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_capture_body_forces(
    const mbd_system2d_t *system,
    double (*body_force)[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body_force, "mbd_system2d body force baseline");

    if (system->num_bodies > 0) {
        memset(body_force,
               0,
               mbd_system2d_force_state_body_force_bytes(system->num_bodies));
    }
    for (i = 0; i < system->num_bodies; ++i) {
        CHECK_ERROR(mbd_body2d_get_generalized_force(&system->bodies[i],
                                                     body_force[i]));
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_restore_body_forces(
    mbd_system2d_t *system,
    const double (*body_force)[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body_force, "mbd_system2d body force baseline");

    for (i = 0; i < system->num_bodies; ++i) {
        CHECK_ERROR(mbd_body2d_set_generalized_force(&system->bodies[i],
                                                     body_force[i]));
    }
    return FEM_SUCCESS;
}
