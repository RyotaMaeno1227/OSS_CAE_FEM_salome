#ifndef FEM4C_MBD_KKT2D_H
#define FEM4C_MBD_KKT2D_H

#include "../common/types.h"

typedef struct {
    int body_dof;
    int lambda_dof;
    int total_dof;
} mbd_kkt_layout_t;

fem_error_t mbd_kkt_compute_layout(int num_bodies,
                                   int num_constraints,
                                   mbd_kkt_layout_t *layout);

#endif /* FEM4C_MBD_KKT2D_H */
