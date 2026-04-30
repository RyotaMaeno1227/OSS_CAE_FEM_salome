#include "forces2d.h"
#include "../common/error.h"
#include <math.h>

fem_error_t mbd_forces2d_add_generalized_force(double generalized_force[MBD_BODY2D_DOF],
                                               const double increment[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(generalized_force, "generalized force");
    CHECK_NULL(increment, "generalized force increment");

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        generalized_force[i] += increment[i];
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_forces2d_apply_user_loads(const mbd_body2d_t *body,
                                          double generalized_force[MBD_BODY2D_DOF])
{
    double body_force[MBD_BODY2D_DOF];

    CHECK_NULL(body, "mbd body");
    CHECK_NULL(generalized_force, "generalized force");
    CHECK_ERROR(mbd_body2d_get_generalized_force(body, body_force));

    return mbd_forces2d_add_generalized_force(generalized_force, body_force);
}

fem_error_t mbd_forces2d_apply_gravity(const mbd_system2d_t *system,
                                       int body_index,
                                       double generalized_force[MBD_BODY2D_DOF])
{
    double gravity_force[MBD_BODY2D_DOF] = {0.0, 0.0, 0.0};

    CHECK_NULL(system, "mbd system");
    CHECK_NULL(generalized_force, "generalized force");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    gravity_force[0] = system->bodies[body_index].mass * system->gravity[0];
    gravity_force[1] = system->bodies[body_index].mass * system->gravity[1];
    return mbd_forces2d_add_generalized_force(generalized_force, gravity_force);
}

fem_error_t mbd_forces2d_apply_flexible_loads(const mbd_system2d_t *system,
                                              int body_index,
                                              double generalized_force[MBD_BODY2D_DOF])
{
    CHECK_NULL(system, "mbd system");
    CHECK_NULL(generalized_force, "generalized force");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    return mbd_forces2d_add_generalized_force(generalized_force,
                                              system->flexible_force[body_index]);
}

fem_error_t mbd_forces2d_apply_contact_loads(const mbd_system2d_t *system,
                                             int body_index,
                                             double generalized_force[MBD_BODY2D_DOF])
{
    CHECK_NULL(system, "mbd system");
    CHECK_NULL(generalized_force, "generalized force");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    return mbd_forces2d_add_generalized_force(generalized_force,
                                              system->contact_force[body_index]);
}

fem_error_t mbd_forces2d_build_body_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF])
{
    CHECK_NULL(system, "mbd system");
    CHECK_NULL(generalized_force, "generalized force");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    generalized_force[0] = 0.0;
    generalized_force[1] = 0.0;
    generalized_force[2] = 0.0;
    CHECK_ERROR(mbd_forces2d_apply_user_loads(&system->bodies[body_index],
                                              generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_gravity(system, body_index, generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_flexible_loads(system,
                                                  body_index,
                                                  generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_contact_loads(system,
                                                 body_index,
                                                 generalized_force));
    return FEM_SUCCESS;
}

fem_error_t mbd_forces2d_build_hht_effective_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    const mbd_hht2d_params_t *params,
    double effective_force[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(system, "mbd system");
    CHECK_NULL(params, "hht params");
    CHECK_NULL(effective_force, "hht effective generalized force");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }
    if (!system->generalized_force_history_valid) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht effective generalized force requested before history refresh");
    }
    if (!isfinite(params->alpha)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "hht alpha must be finite for effective generalized force");
    }

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        effective_force[i] = (1.0 + params->alpha) * system->current_generalized_force[body_index][i]
                           - params->alpha * system->previous_generalized_force[body_index][i];
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_forces2d_build_rhs_vector(const mbd_system2d_t *system,
                                          double *rhs,
                                          int rhs_length)
{
    int body_index;

    CHECK_NULL(system, "mbd system");
    CHECK_NULL(rhs, "mbd rhs");

    if (rhs_length < system->num_bodies * MBD_BODY2D_DOF) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "rhs_length %d is smaller than required body dof %d",
                         rhs_length,
                         system->num_bodies * MBD_BODY2D_DOF);
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        double generalized_force[MBD_BODY2D_DOF] = {0.0, 0.0, 0.0};
        const int row = body_index * MBD_BODY2D_DOF;

        CHECK_ERROR(mbd_forces2d_build_body_generalized_force(system,
                                                              body_index,
                                                              generalized_force));

        rhs[row + 0] = generalized_force[0];
        rhs[row + 1] = generalized_force[1];
        rhs[row + 2] = generalized_force[2];
    }

    return FEM_SUCCESS;
}
