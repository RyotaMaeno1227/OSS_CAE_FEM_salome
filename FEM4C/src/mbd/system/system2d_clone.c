#include "system2d.h"
#include "../../common/error.h"
#include <stdlib.h>
#include <string.h>

/*
 * Owns the narrow clone/copy helper implementation for mbd_system2d_t.
 * Inputs are source and destination system object pointers; outputs and side
 * effects are cloned system storage/state/config contents plus related
 * allocation and error handling. Lifecycle/default config, body/constraint
 * storage, contact registration, snapshot helpers, solver orchestration,
 * contact runtime refresh, input parsing, contact-load bridge, output bridge,
 * static/coupled bridge, and file IO remain in other files. This extraction is
 * for readability and maintainability, not behavior change.
 */

static size_t mbd_system2d_clone_body_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(mbd_body2d_t);
}

static size_t mbd_system2d_clone_body_force_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(double[MBD_BODY2D_DOF]);
}

static size_t mbd_system2d_clone_constraint_bytes(int constraint_capacity)
{
    if (constraint_capacity <= 0) {
        return 0;
    }
    return (size_t) constraint_capacity * sizeof(mbd_constraint2d_t);
}

fem_error_t mbd_system2d_clone(mbd_system2d_t *dst,
                               const mbd_system2d_t *src)
{
    mbd_system2d_t clone;
    int clone_capacity = 0;
    int clone_constraint_capacity = 0;
    fem_error_t err;

    CHECK_NULL(dst, "mbd_system2d clone dst");
    CHECK_NULL(src, "mbd_system2d clone src");

    mbd_system2d_zero(&clone);
    clone = *src;
    clone.bodies = NULL;
    clone.body_states = NULL;
    clone.flexible_force = NULL;
    clone.contact_force = NULL;
    clone.current_generalized_force = NULL;
    clone.previous_generalized_force = NULL;
    clone.body_capacity = 0;
    clone.constraints = NULL;
    clone.constraint_capacity = 0;

    clone_capacity = src->body_capacity > src->num_bodies
                         ? src->body_capacity
                         : src->num_bodies;
    clone_constraint_capacity = src->constraint_capacity > src->num_constraints
                                    ? src->constraint_capacity
                                    : src->num_constraints;
    err = mbd_system2d_reserve_body_storage(&clone, clone_capacity);
    if (err != FEM_SUCCESS) {
        mbd_system2d_free(&clone);
        return err;
    }
    if (clone_constraint_capacity > 0) {
        err = mbd_system2d_reserve_constraint_storage(&clone,
                                                      clone_constraint_capacity);
        if (err != FEM_SUCCESS) {
            mbd_system2d_free(&clone);
            return err;
        }
    }
    if (clone_capacity > 0) {
        memcpy(clone.bodies, src->bodies, mbd_system2d_clone_body_bytes(clone_capacity));
        memcpy(clone.body_states,
               src->body_states,
               (size_t) clone_capacity * sizeof(*clone.body_states));
        memcpy(clone.flexible_force,
               src->flexible_force,
               mbd_system2d_clone_body_force_bytes(clone_capacity));
        memcpy(clone.contact_force,
               src->contact_force,
               mbd_system2d_clone_body_force_bytes(clone_capacity));
        memcpy(clone.current_generalized_force,
               src->current_generalized_force,
               mbd_system2d_clone_body_force_bytes(clone_capacity));
        memcpy(clone.previous_generalized_force,
               src->previous_generalized_force,
               mbd_system2d_clone_body_force_bytes(clone_capacity));
    }
    if (src->num_constraints > 0) {
        memcpy(clone.constraints,
               src->constraints,
               mbd_system2d_clone_constraint_bytes(src->num_constraints));
    }
    err = mbd_system2d_sync_body_states(&clone);
    if (err != FEM_SUCCESS) {
        mbd_system2d_free(&clone);
        return err;
    }

    mbd_system2d_free(dst);
    *dst = clone;
    return FEM_SUCCESS;
}
