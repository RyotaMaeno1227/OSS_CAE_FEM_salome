#ifndef FEM4C_MBD_CONSTRAINT2D_H
#define FEM4C_MBD_CONSTRAINT2D_H

#include "../common/types.h"

#define MBD_BODY2D_DOF 3
#define MBD_CONSTRAINT2D_MAX_EQ 2

typedef enum {
    MBD_CONSTRAINT_DISTANCE = 1,
    MBD_CONSTRAINT_REVOLUTE = 2
} mbd_constraint_type_t;

typedef struct {
    double x;
    double y;
    double theta;
} mbd_body_state2d_t;

typedef struct {
    int id;
    mbd_constraint_type_t type;
    int body_i;
    int body_j;
    double anchor_i[2];
    double anchor_j[2];
    double target_value;
} mbd_constraint2d_t;

fem_error_t mbd_constraint_init_distance(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2],
                                         double distance);

fem_error_t mbd_constraint_init_revolute(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2]);

fem_error_t mbd_constraint_validate(const mbd_constraint2d_t *c);
int mbd_constraint_equation_count(const mbd_constraint2d_t *c);

fem_error_t mbd_constraint_evaluate(const mbd_constraint2d_t *c,
                                    const mbd_body_state2d_t *state_i,
                                    const mbd_body_state2d_t *state_j,
                                    double residual[MBD_CONSTRAINT2D_MAX_EQ],
                                    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF],
                                    int *num_equations);

#endif /* FEM4C_MBD_CONSTRAINT2D_H */
