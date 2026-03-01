#ifndef FEM4C_MBD_KKT2D_H
#define FEM4C_MBD_KKT2D_H

#include "../common/types.h"
#include "constraint2d.h"

typedef struct {
    int body_dof;
    int lambda_dof;
    int total_dof;
} mbd_kkt_layout_t;

fem_error_t mbd_kkt_compute_layout(int num_bodies,
                                   int num_constraints,
                                   mbd_kkt_layout_t *layout);

fem_error_t mbd_kkt_count_constraint_equations(const mbd_constraint2d_t *constraints,
                                               int num_constraints,
                                               int *num_equations);

fem_error_t mbd_kkt_compute_layout_from_constraints(int num_bodies,
                                                    const mbd_constraint2d_t *constraints,
                                                    int num_constraints,
                                                    mbd_kkt_layout_t *layout);

#endif /* FEM4C_MBD_KKT2D_H */
