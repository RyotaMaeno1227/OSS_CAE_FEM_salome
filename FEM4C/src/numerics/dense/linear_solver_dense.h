#ifndef FEM4C_MBD_LINEAR_SOLVER_DENSE_H
#define FEM4C_MBD_LINEAR_SOLVER_DENSE_H

#include "../../common/types.h"

#define MBD_LINEAR_SOLVER_DENSE_MAX_DIM 128
#define MBD_LINEAR_SOLVER_DENSE_DEFAULT_PIVOT_TOL 1.0e-12

fem_error_t mbd_linear_solver_dense_solve(const double *matrix,
                                          const double *rhs,
                                          int n,
                                          double pivot_tol,
                                          double *solution);

double mbd_linear_solver_dense_residual_inf(const double *matrix,
                                            const double *rhs,
                                            const double *solution,
                                            int n);

#endif /* FEM4C_MBD_LINEAR_SOLVER_DENSE_H */
