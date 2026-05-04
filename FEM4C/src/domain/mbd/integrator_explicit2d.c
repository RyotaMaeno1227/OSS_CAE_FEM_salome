#include "integrator_explicit2d.h"
#include "../../common/error.h"
#include <math.h>

static fem_error_t mbd_explicit2d_validate_dt(double dt)
{
    if (!isfinite(dt) || dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "explicit integrator dt must be positive and finite");
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_explicit2d_predict(const mbd_body2d_t *body,
                                   double dt,
                                   double q_pred[MBD_BODY2D_DOF],
                                   double v_pred[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(body, "mbd body");
    CHECK_NULL(q_pred, "predicted position");
    CHECK_NULL(v_pred, "predicted velocity");
    CHECK_ERROR(mbd_explicit2d_validate_dt(dt));

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        v_pred[i] = body->v[i] + dt * body->a[i];
        q_pred[i] = body->q[i] + dt * v_pred[i];
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_explicit2d_update_velocity(mbd_body2d_t *body,
                                           double dt,
                                           const double acceleration[MBD_BODY2D_DOF])
{
    int i;

    CHECK_NULL(body, "mbd body");
    CHECK_NULL(acceleration, "body acceleration");
    CHECK_ERROR(mbd_explicit2d_validate_dt(dt));

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        if (!isfinite(acceleration[i])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "explicit integrator acceleration must be finite");
        }
    }

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        body->a[i] = acceleration[i];
        body->v[i] += dt * acceleration[i];
    }
    return FEM_SUCCESS;
}

fem_error_t mbd_explicit2d_update_position(mbd_body2d_t *body,
                                           double dt)
{
    int i;

    CHECK_NULL(body, "mbd body");
    CHECK_ERROR(mbd_explicit2d_validate_dt(dt));

    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        body->q[i] += dt * body->v[i];
    }
    return FEM_SUCCESS;
}
