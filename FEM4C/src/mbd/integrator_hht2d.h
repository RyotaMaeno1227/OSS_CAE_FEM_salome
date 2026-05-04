#ifndef FEM4C_MBD_INTEGRATOR_HHT2D_H
#define FEM4C_MBD_INTEGRATOR_HHT2D_H

#include "body2d.h"
#include "../domain/mbd/integrator_newmark2d.h"

typedef struct {
    double dt;
    double alpha;
    double beta;
    double gamma;
} mbd_hht2d_params_t;

fem_error_t mbd_hht2d_validate_alpha(double alpha);

fem_error_t mbd_hht2d_init_params(mbd_hht2d_params_t *params,
                                  double dt,
                                  double alpha);

fem_error_t mbd_hht2d_validate_params(const mbd_hht2d_params_t *params);

fem_error_t mbd_hht2d_predict(const mbd_body2d_t *current,
                              const mbd_hht2d_params_t *params,
                              double q_pred[MBD_BODY2D_DOF],
                              double v_pred[MBD_BODY2D_DOF]);

fem_error_t mbd_hht2d_predict_state(const mbd_body2d_t *current,
                                    const mbd_hht2d_params_t *params,
                                    mbd_body2d_t *predicted);

fem_error_t mbd_hht2d_to_newmark_params(const mbd_hht2d_params_t *params,
                                        mbd_newmark2d_params_t *newmark_params);

fem_error_t mbd_hht2d_blend_vector(const double current[MBD_BODY2D_DOF],
                                   const double previous[MBD_BODY2D_DOF],
                                   const mbd_hht2d_params_t *params,
                                   double blended[MBD_BODY2D_DOF]);

fem_error_t mbd_hht2d_blend_scalar(double current,
                                   double previous,
                                   const mbd_hht2d_params_t *params,
                                   double *blended);

fem_error_t mbd_hht2d_compute_effective_force(const double current[MBD_BODY2D_DOF],
                                              const double previous[MBD_BODY2D_DOF],
                                              const mbd_hht2d_params_t *params,
                                              double effective_force[MBD_BODY2D_DOF]);

fem_error_t mbd_hht2d_compute_effective_residual(const double current[MBD_BODY2D_DOF],
                                                 const double previous[MBD_BODY2D_DOF],
                                                 const mbd_hht2d_params_t *params,
                                                 double effective_residual[MBD_BODY2D_DOF]);

fem_error_t mbd_hht2d_step_unconstrained(
    const mbd_body2d_t *current,
    const double generalized_force[MBD_BODY2D_DOF],
    const double previous_generalized_force[MBD_BODY2D_DOF],
    const mbd_hht2d_params_t *params,
    mbd_body2d_t *next);

#endif /* FEM4C_MBD_INTEGRATOR_HHT2D_H */
