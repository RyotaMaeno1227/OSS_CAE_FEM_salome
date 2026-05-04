#ifndef FEM4C_MBD_INTEGRATOR_EXPLICIT2D_H
#define FEM4C_MBD_INTEGRATOR_EXPLICIT2D_H

#include "../../mbd/body2d.h"

fem_error_t mbd_explicit2d_predict(const mbd_body2d_t *body,
                                   double dt,
                                   double q_pred[MBD_BODY2D_DOF],
                                   double v_pred[MBD_BODY2D_DOF]);

fem_error_t mbd_explicit2d_update_velocity(mbd_body2d_t *body,
                                           double dt,
                                           const double acceleration[MBD_BODY2D_DOF]);

fem_error_t mbd_explicit2d_update_position(mbd_body2d_t *body,
                                           double dt);

#endif /* FEM4C_MBD_INTEGRATOR_EXPLICIT2D_H */
