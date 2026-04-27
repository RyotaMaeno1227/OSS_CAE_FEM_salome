#include "kinematics2d.h"
#include "../common/error.h"
#include <math.h>

fem_error_t mbd_kinematics2d_rotate_local_vector(const double local_vector[2],
                                                 double theta,
                                                 double world_vector[2])
{
    const double c = cos(theta);
    const double s = sin(theta);

    CHECK_NULL(local_vector, "local vector");
    CHECK_NULL(world_vector, "world vector");

    world_vector[0] = c * local_vector[0] - s * local_vector[1];
    world_vector[1] = s * local_vector[0] + c * local_vector[1];
    return FEM_SUCCESS;
}

fem_error_t mbd_kinematics2d_drotate_dtheta(const double local_vector[2],
                                            double theta,
                                            double d_world_dtheta[2])
{
    const double c = cos(theta);
    const double s = sin(theta);

    CHECK_NULL(local_vector, "local vector");
    CHECK_NULL(d_world_dtheta, "d_world_dtheta");

    d_world_dtheta[0] = -s * local_vector[0] - c * local_vector[1];
    d_world_dtheta[1] = c * local_vector[0] - s * local_vector[1];
    return FEM_SUCCESS;
}

fem_error_t mbd_kinematics2d_local_point_to_world(const mbd_body_state2d_t *state,
                                                  const double local_point[2],
                                                  double world_point[2])
{
    double rotated[2];

    CHECK_NULL(state, "body state");
    CHECK_NULL(local_point, "local point");
    CHECK_NULL(world_point, "world point");

    CHECK_ERROR(mbd_kinematics2d_rotate_local_vector(local_point, state->theta, rotated));
    world_point[0] = state->x + rotated[0];
    world_point[1] = state->y + rotated[1];
    return FEM_SUCCESS;
}

fem_error_t mbd_kinematics2d_world_point_jacobian(const mbd_body_state2d_t *state,
                                                  const double local_point[2],
                                                  double jacobian[2][MBD_BODY2D_DOF])
{
    double d_world_dtheta[2];

    CHECK_NULL(state, "body state");
    CHECK_NULL(local_point, "local point");
    CHECK_NULL(jacobian, "world point jacobian");

    CHECK_ERROR(mbd_kinematics2d_drotate_dtheta(local_point, state->theta, d_world_dtheta));

    jacobian[0][0] = 1.0;
    jacobian[0][1] = 0.0;
    jacobian[0][2] = d_world_dtheta[0];

    jacobian[1][0] = 0.0;
    jacobian[1][1] = 1.0;
    jacobian[1][2] = d_world_dtheta[1];
    return FEM_SUCCESS;
}

fem_error_t mbd_kinematics2d_self_check(void)
{
    const double local_point[2] = {1.0, 0.0};
    const double expected_world[2] = {2.0, 4.0};
    const double expected_dtheta[2] = {-1.0, 0.0};
    mbd_body_state2d_t state;
    double world_point[2];
    double jacobian[2][MBD_BODY2D_DOF];
    const double tol = 1.0e-12;

    state.x = 2.0;
    state.y = 3.0;
    state.theta = 0.5 * acos(-1.0);

    CHECK_ERROR(mbd_kinematics2d_local_point_to_world(&state, local_point, world_point));
    CHECK_ERROR(mbd_kinematics2d_world_point_jacobian(&state, local_point, jacobian));

    if (fabs(world_point[0] - expected_world[0]) > tol ||
        fabs(world_point[1] - expected_world[1]) > tol) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "kinematics2d self-check failed for local_point_to_world");
    }
    if (fabs(jacobian[0][2] - expected_dtheta[0]) > tol ||
        fabs(jacobian[1][2] - expected_dtheta[1]) > tol) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "kinematics2d self-check failed for theta derivative");
    }

    return FEM_SUCCESS;
}
