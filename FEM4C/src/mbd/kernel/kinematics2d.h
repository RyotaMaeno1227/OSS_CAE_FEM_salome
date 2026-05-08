#ifndef FEM4C_MBD_KINEMATICS2D_H
#define FEM4C_MBD_KINEMATICS2D_H

#include "constraint2d.h"

fem_error_t mbd_kinematics2d_rotate_local_vector(const double local_vector[2],
                                                 double theta,
                                                 double world_vector[2]);

fem_error_t mbd_kinematics2d_drotate_dtheta(const double local_vector[2],
                                            double theta,
                                            double d_world_dtheta[2]);

fem_error_t mbd_kinematics2d_local_point_to_world(const mbd_body_state2d_t *state,
                                                  const double local_point[2],
                                                  double world_point[2]);

fem_error_t mbd_kinematics2d_world_point_jacobian(const mbd_body_state2d_t *state,
                                                  const double local_point[2],
                                                  double jacobian[2][MBD_BODY2D_DOF]);

fem_error_t mbd_kinematics2d_self_check(void);

#endif /* FEM4C_MBD_KINEMATICS2D_H */
