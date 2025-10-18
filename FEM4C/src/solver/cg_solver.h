#ifndef CG_SOLVER_H
#define CG_SOLVER_H

/* FEM4C - Conjugate Gradient Solver
 * Implementation of iterative solver for symmetric positive definite systems
 */

#include "../common/types.h"

/* Conjugate gradient solver */
fem_error_t cg_solve(double *A, double *b, double *x, int n, 
                    double tolerance, int max_iterations, 
                    int *actual_iterations, double *final_residual);

/* Preconditioned conjugate gradient solver */
fem_error_t pcg_solve(double *A, double *b, double *x, int n,
                     double tolerance, int max_iterations,
                     int *actual_iterations, double *final_residual);

/* Solver for FEM4C global system */
fem_error_t cg_solve_system(void);

/* Utility functions */
fem_error_t cg_matrix_vector_multiply(double *A, double *x, double *result, int n);
fem_error_t cg_dot_product(double *a, double *b, int n, double *result);
fem_error_t cg_vector_copy(double *src, double *dst, int n);
fem_error_t cg_vector_scale(double *vec, double scale, int n);
fem_error_t cg_vector_axpy(double a, double *x, double *y, int n); /* y = a*x + y */

/* Preconditioning functions */
fem_error_t cg_diagonal_preconditioner(double *A, double *M_inv, int n);
fem_error_t cg_apply_preconditioner(double *M_inv, double *r, double *z, int n);

/* Convergence checking */
fem_error_t cg_check_convergence(double *r, int n, double tolerance, double *residual_norm);

/* Performance monitoring */
void cg_print_iteration_info(int iter, double residual_norm, double tolerance);

#endif /* CG_SOLVER_H */