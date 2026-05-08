#ifndef FEM4C_MBD_BODY2D_H
#define FEM4C_MBD_BODY2D_H

#include "../../common/types.h"
#include "constraint2d.h"

#define MBD_BODY2D_ID_UNDEFINED (-1)
#define MBD_BODY2D_DEFAULT_MASS 1.0
#define MBD_BODY2D_DEFAULT_INERTIA 1.0

typedef struct {
    int id;
    double mass;
    double inertia;
    double q[3];
    double v[3];
    double a[3];
    double force[3];
    double reference_origin[2];
    double reference_theta;
    int is_ground;
} mbd_body2d_t;

void mbd_body2d_zero(mbd_body2d_t *body);

int mbd_body2d_is_defined(const mbd_body2d_t *body);

fem_error_t mbd_body2d_init_dyn(mbd_body2d_t *body,
                                int id,
                                double mass,
                                double inertia,
                                const double q[3],
                                const double v[3]);

fem_error_t mbd_body2d_init_pose(mbd_body2d_t *body,
                                 int id,
                                 double x,
                                 double y,
                                 double theta);

fem_error_t mbd_body2d_set_reference_frame(mbd_body2d_t *body,
                                           const double origin[2],
                                           double theta);

fem_error_t mbd_body2d_get_reference_frame(const mbd_body2d_t *body,
                                           double origin[2],
                                           double *theta);

fem_error_t mbd_body2d_get_current_pose(const mbd_body2d_t *body,
                                        double origin[2],
                                        double *theta);
fem_error_t mbd_body2d_get_generalized_state(const mbd_body2d_t *body,
                                             double q[3],
                                             double v[3],
                                             double a[3]);
fem_error_t mbd_body2d_get_generalized_force(const mbd_body2d_t *body,
                                             double force[3]);
fem_error_t mbd_body2d_set_generalized_force(mbd_body2d_t *body,
                                             const double force[3]);

void mbd_body2d_clear_force(mbd_body2d_t *body);

fem_error_t mbd_body2d_to_state_view(const mbd_body2d_t *body,
                                     mbd_body_state2d_t *state);

#endif /* FEM4C_MBD_BODY2D_H */
