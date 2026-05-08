#ifndef FEM4C_MBD_FORCES2D_H
#define FEM4C_MBD_FORCES2D_H

#include "../kernel/integrator_hht2d.h"
#include "system2d.h"

fem_error_t mbd_forces2d_add_generalized_force(double generalized_force[MBD_BODY2D_DOF],
                                               const double increment[MBD_BODY2D_DOF]);

fem_error_t mbd_forces2d_apply_user_loads(const mbd_body2d_t *body,
                                          double generalized_force[MBD_BODY2D_DOF]);

fem_error_t mbd_forces2d_apply_gravity(const mbd_system2d_t *system,
                                       int body_index,
                                       double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_forces2d_apply_flexible_loads(const mbd_system2d_t *system,
                                              int body_index,
                                              double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_forces2d_build_body_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_forces2d_build_hht_effective_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    const mbd_hht2d_params_t *params,
    double effective_force[MBD_BODY2D_DOF]);

fem_error_t mbd_forces2d_build_rhs_vector(const mbd_system2d_t *system,
                                          double *rhs,
                                          int rhs_length);

#endif /* FEM4C_MBD_FORCES2D_H */
