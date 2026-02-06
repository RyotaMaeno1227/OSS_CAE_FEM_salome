#ifndef FEM4C_MBD_CONSTRAINT2D_H
#define FEM4C_MBD_CONSTRAINT2D_H

#include "../common/types.h"

typedef enum {
    MBD_CONSTRAINT_DISTANCE = 1,
    MBD_CONSTRAINT_REVOLUTE = 2
} mbd_constraint_type_t;

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

#endif /* FEM4C_MBD_CONSTRAINT2D_H */
