#ifndef FEM4C_MBD_ASSEMBLER2D_H
#define FEM4C_MBD_ASSEMBLER2D_H

#include "system2d.h"
#include "../numerics/dense/linear_solver_dense.h"

#define MBD_ASSEMBLER2D_MAX_BODY_DOF (MBD_SYSTEM2D_MAX_BODIES * MBD_BODY2D_DOF)
#define MBD_ASSEMBLER2D_MAX_LAMBDA_DOF (MBD_SYSTEM2D_MAX_CONSTRAINTS * MBD_CONSTRAINT2D_MAX_EQ)
#define MBD_ASSEMBLER2D_MAX_TOTAL_DOF MBD_LINEAR_SOLVER_DENSE_MAX_DIM

typedef struct {
    mbd_kkt_layout_t layout;
    int total_dof_capacity;
    int constraint_row_offset_capacity;
    int lambda_capacity;
    int *constraint_row_offsets;
    double *constraint_residual;
    double *constraint_phi_dot;
    double *constraint_gamma_rhs;
    double *matrix_storage;
    double **matrix;
    double *rhs;
} mbd_dense_kkt2d_t;

void mbd_dense_kkt2d_zero(mbd_dense_kkt2d_t *kkt);
void mbd_dense_kkt2d_free(mbd_dense_kkt2d_t *kkt);
fem_error_t mbd_dense_kkt2d_reserve_dense_storage(mbd_dense_kkt2d_t *kkt,
                                                  int total_dof);
fem_error_t mbd_dense_kkt2d_reserve_constraint_scratch(mbd_dense_kkt2d_t *kkt,
                                                       int num_constraints,
                                                       int lambda_dof);

fem_error_t mbd_dense_kkt2d_copy_compact(const mbd_dense_kkt2d_t *kkt,
                                         double *matrix_out);

fem_error_t mbd_dense_kkt2d_assemble(const mbd_system2d_t *system,
                                     mbd_dense_kkt2d_t *kkt);

#endif /* FEM4C_MBD_ASSEMBLER2D_H */
