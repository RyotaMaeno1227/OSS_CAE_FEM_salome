#ifndef FEM4C_MBD_INTEGRATOR_NEWMARK2D_H
#define FEM4C_MBD_INTEGRATOR_NEWMARK2D_H

#include "body2d.h"

typedef struct {
    double dt;
    double beta;
    double gamma;
} mbd_newmark2d_params_t;

fem_error_t mbd_newmark2d_validate_params(const mbd_newmark2d_params_t *params);

fem_error_t mbd_newmark2d_predict(const mbd_body2d_t *current,
                                  const mbd_newmark2d_params_t *params,
                                  double q_pred[MBD_BODY2D_DOF],
                                  double v_pred[MBD_BODY2D_DOF]);

fem_error_t mbd_newmark2d_predict_state(const mbd_body2d_t *current,
                                        const mbd_newmark2d_params_t *params,
                                        mbd_body2d_t *predicted);

fem_error_t mbd_newmark2d_correct(const mbd_body2d_t *current,
                                  const double acceleration_next[MBD_BODY2D_DOF],
                                  const mbd_newmark2d_params_t *params,
                                  double q_next[MBD_BODY2D_DOF],
                                  double v_next[MBD_BODY2D_DOF]);

fem_error_t mbd_newmark2d_update_state(
    const mbd_body2d_t *current,
    const double acceleration_next[MBD_BODY2D_DOF],
    const mbd_newmark2d_params_t *params,
    mbd_body2d_t *next);

fem_error_t mbd_newmark2d_step_unconstrained(
    const mbd_body2d_t *current,
    const double generalized_force[MBD_BODY2D_DOF],
    const mbd_newmark2d_params_t *params,
    mbd_body2d_t *next);

#endif /* FEM4C_MBD_INTEGRATOR_NEWMARK2D_H */
