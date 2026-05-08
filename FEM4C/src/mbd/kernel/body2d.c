#include "body2d.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

void mbd_body2d_zero(mbd_body2d_t *body)
{
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
    body->id = MBD_BODY2D_ID_UNDEFINED;
}

int mbd_body2d_is_defined(const mbd_body2d_t *body)
{
    return body != NULL && body->id >= 0;
}

static fem_error_t mbd_body2d_validate_reference_theta(double theta)
{
    if (!isfinite(theta)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD body reference theta must be finite");
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_init_dyn(mbd_body2d_t *body,
                                int id,
                                double mass,
                                double inertia,
                                const double q[3],
                                const double v[3])
{
    int i;

    CHECK_NULL(body, "MBD dynamic body");
    CHECK_NULL(q, "MBD body coordinates");

    if (id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD body id must be non-negative");
    }
    if (!isfinite(mass) || mass <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD body mass must be positive and finite");
    }
    if (!isfinite(inertia) || inertia <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD body inertia must be positive and finite");
    }
    for (i = 0; i < 3; ++i) {
        if (!isfinite(q[i])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD body coordinates must be finite");
        }
    }
    if (v) {
        for (i = 0; i < 3; ++i) {
            if (!isfinite(v[i])) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD body velocity must be finite");
            }
        }
    }

    mbd_body2d_zero(body);
    body->id = id;
    body->mass = mass;
    body->inertia = inertia;
    body->is_ground = 0;

    if (q) {
        memcpy(body->q, q, sizeof(body->q));
        body->reference_origin[0] = q[0];
        body->reference_origin[1] = q[1];
        body->reference_theta = q[2];
    }
    if (v) {
        memcpy(body->v, v, sizeof(body->v));
    }

    mbd_body2d_clear_force(body);
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_init_pose(mbd_body2d_t *body,
                                 int id,
                                 double x,
                                 double y,
                                 double theta)
{
    const double q[3] = {x, y, theta};

    return mbd_body2d_init_dyn(body, id,
                               MBD_BODY2D_DEFAULT_MASS,
                               MBD_BODY2D_DEFAULT_INERTIA,
                               q, NULL);
}

fem_error_t mbd_body2d_set_reference_frame(mbd_body2d_t *body,
                                           const double origin[2],
                                           double theta)
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(origin, "MBD body reference origin");
    CHECK_ERROR(mbd_body2d_validate_reference_theta(theta));

    if (!isfinite(origin[0]) || !isfinite(origin[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD body reference origin must be finite");
    }

    body->reference_origin[0] = origin[0];
    body->reference_origin[1] = origin[1];
    body->reference_theta = theta;
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_get_reference_frame(const mbd_body2d_t *body,
                                           double origin[2],
                                           double *theta)
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(origin, "MBD body reference origin");
    CHECK_NULL(theta, "MBD body reference theta");

    origin[0] = body->reference_origin[0];
    origin[1] = body->reference_origin[1];
    *theta = body->reference_theta;
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_get_current_pose(const mbd_body2d_t *body,
                                        double origin[2],
                                        double *theta)
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(origin, "MBD body current origin");
    CHECK_NULL(theta, "MBD body current theta");

    origin[0] = body->q[0];
    origin[1] = body->q[1];
    *theta = body->q[2];
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_get_generalized_state(const mbd_body2d_t *body,
                                             double q[3],
                                             double v[3],
                                             double a[3])
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(q, "MBD body coordinates");
    CHECK_NULL(v, "MBD body velocity");
    CHECK_NULL(a, "MBD body acceleration");

    memcpy(q, body->q, sizeof(body->q));
    memcpy(v, body->v, sizeof(body->v));
    memcpy(a, body->a, sizeof(body->a));
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_get_generalized_force(const mbd_body2d_t *body,
                                             double force[3])
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(force, "MBD body force");

    memcpy(force, body->force, sizeof(body->force));
    return FEM_SUCCESS;
}

fem_error_t mbd_body2d_set_generalized_force(mbd_body2d_t *body,
                                             const double force[3])
{
    int i;

    CHECK_NULL(body, "MBD body");
    CHECK_NULL(force, "MBD body force");

    for (i = 0; i < 3; ++i) {
        if (!isfinite(force[i])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD body force must be finite");
        }
    }

    memcpy(body->force, force, sizeof(body->force));
    return FEM_SUCCESS;
}

void mbd_body2d_clear_force(mbd_body2d_t *body)
{
    if (!body) {
        return;
    }
    memset(body->force, 0, sizeof(body->force));
}

fem_error_t mbd_body2d_to_state_view(const mbd_body2d_t *body,
                                     mbd_body_state2d_t *state)
{
    CHECK_NULL(body, "MBD body");
    CHECK_NULL(state, "MBD body state");

    state->x = body->q[0];
    state->y = body->q[1];
    state->theta = body->q[2];
    return FEM_SUCCESS;
}
