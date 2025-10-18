/* FEM4C - Conjugate Gradient Solver Implementation
 * Iterative solver for symmetric positive definite systems
 */

#include "cg_solver.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

/* Conjugate gradient solver implementation */
fem_error_t cg_solve(double *A, double *b, double *x, int n, 
                    double tolerance, int max_iterations, 
                    int *actual_iterations, double *final_residual)
{
    double *r = NULL, *p = NULL, *Ap = NULL;
    double alpha, beta, rsold, rsnew;
    double residual_norm;
    int iter;
    fem_error_t err = FEM_SUCCESS;

    (void)A;
    
    /* Allocate working vectors */
    r = malloc(n * sizeof(double));
    p = malloc(n * sizeof(double));
    Ap = malloc(n * sizeof(double));
    
    CHECK_NULL(r, "CG residual vector allocation failed");
    CHECK_NULL(p, "CG search direction vector allocation failed");
    CHECK_NULL(Ap, "CG matrix-vector product allocation failed");
    
    printf("Starting conjugate gradient solver...\n");
    printf("  Problem size: %d\n", n);
    printf("  Tolerance: %e\n", tolerance);
    printf("  Max iterations: %d\n", max_iterations);
    
    /* Initialize: r = b - A*x */
    err = cg_matrix_vector_multiply(A, x, Ap, n);
    CHECK_ERROR_CLEANUP(err, goto cleanup);
    
    for (int i = 0; i < n; i++) {
        r[i] = b[i] - Ap[i];
        p[i] = r[i];  /* Initial search direction */
    }
    
    err = cg_dot_product(r, r, n, &rsold);
    CHECK_ERROR_CLEANUP(err, goto cleanup);
    
    /* Check initial convergence */
    residual_norm = sqrt(rsold);
    if (residual_norm < tolerance) {
        *actual_iterations = 0;
        *final_residual = residual_norm;
        printf("  Initial guess already converged\n");
        goto cleanup;
    }
    
    /* CG iterations */
    for (iter = 0; iter < max_iterations; iter++) {
        /* Compute A*p */
        err = cg_matrix_vector_multiply(A, p, Ap, n);
        CHECK_ERROR_CLEANUP(err, goto cleanup);
        
        /* Compute alpha = (r^T * r) / (p^T * A * p) */
        double pAp;
        err = cg_dot_product(p, Ap, n, &pAp);
        CHECK_ERROR_CLEANUP(err, goto cleanup);
        
        if (fabs(pAp) < TOLERANCE) {
            printf("  CG Debug: iteration %d, pAp = %.6e, tolerance = %.6e\n", iter, pAp, TOLERANCE);
            printf("  Search direction p[0:5]: ");
            for (int i = 0; i < (n < 6 ? n : 6); i++) {
                printf("%.3e ", p[i]);
            }
            printf("\n  A*p[0:5]: ");
            for (int i = 0; i < (n < 6 ? n : 6); i++) {
                printf("%.3e ", Ap[i]);
            }
            printf("\n");
            err = error_set(FEM_ERROR_SINGULAR_MATRIX, "Zero curvature in CG iteration %d", iter);
            goto cleanup;
        }
        
        alpha = rsold / pAp;
        
        /* Update solution: x = x + alpha * p */
        err = cg_vector_axpy(alpha, p, x, n);
        CHECK_ERROR_CLEANUP(err, goto cleanup);
        
        /* Update residual: r = r - alpha * A * p */
        err = cg_vector_axpy(-alpha, Ap, r, n);
        CHECK_ERROR_CLEANUP(err, goto cleanup);
        
        /* Compute new residual norm */
        err = cg_dot_product(r, r, n, &rsnew);
        CHECK_ERROR_CLEANUP(err, goto cleanup);
        
        residual_norm = sqrt(rsnew);
        
        /* Print iteration info */
        if (iter % 10 == 0 || iter < 5) {
            cg_print_iteration_info(iter + 1, residual_norm, tolerance);
        }
        
        /* Check convergence */
        if (residual_norm < tolerance) {
            *actual_iterations = iter + 1;
            *final_residual = residual_norm;
            printf("  Converged in %d iterations\n", iter + 1);
            printf("  Final residual: %e\n", residual_norm);
            goto cleanup;
        }
        
        /* Compute beta = (r_new^T * r_new) / (r_old^T * r_old) */
        beta = rsnew / rsold;
        
        /* Update search direction: p = r + beta * p */
        for (int i = 0; i < n; i++) {
            p[i] = r[i] + beta * p[i];
        }
        
        rsold = rsnew;
    }
    
    /* Maximum iterations reached */
    *actual_iterations = max_iterations;
    *final_residual = residual_norm;
    err = error_set(FEM_ERROR_MAX_ITERATIONS, 
                   "CG solver failed to converge in %d iterations (residual = %e)", 
                   max_iterations, residual_norm);

cleanup:
    if (r) free(r);
    if (p) free(p);
    if (Ap) free(Ap);
    
    return err;
}

/* Solve the global FEM system using CG */
fem_error_t cg_solve_system(void)
{
    int iterations;
    double final_residual;
    fem_error_t err;
    
    if (g_total_dof <= 0) {
        g_solver_info.iterations = 0;
        g_solver_info.residual = 0.0;
        g_solver_info.status = FEM_SUCCESS;
        return FEM_SUCCESS;
    }

    /* Use global arrays directly */
    double *b = g_global_force;
    double *x = g_global_displ;

    if (!b || !x || !g_global_stiffness_values) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Global system arrays not initialized");
    }

    /* Solve system */
    err = cg_solve(NULL, b, x, g_total_dof, 
                  g_analysis.tolerance, g_analysis.max_iterations,
                  &iterations, &final_residual);
    
    /* Update solver info */
    g_solver_info.iterations = iterations;
    g_solver_info.residual = final_residual;
    g_solver_info.status = err;
    
    /* Copy solution back to nodal displacements */
    if (err == FEM_SUCCESS) {
        for (int node = 0; node < g_num_nodes; node++) {
            g_node_displ[node][0] = g_global_displ[node * 2];     /* u */
            g_node_displ[node][1] = g_global_displ[node * 2 + 1]; /* v */
            g_node_displ[node][2] = 0.0; /* w = 0 for 2D */
        }
        
        printf("Solution completed successfully\n");
        printf("  Nodal displacements updated\n");
    }
    
    return err;
}

/* Matrix-vector multiplication: result = A * x */
fem_error_t cg_matrix_vector_multiply(double *A, double *x, double *result, int n)
{
    (void)A;

    if (!g_global_stiffness_values || !g_stiffness_profile || !g_stiffness_offsets) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Global stiffness matrix not initialized");
    }

    for (int i = 0; i < n; i++) {
        result[i] = ZERO;
    }

    for (int col = 0; col < n; col++) {
        int first_row = g_stiffness_profile[col];
        int offset = g_stiffness_offsets[col];
        int last_row = col;

        if (first_row < 0) {
            first_row = 0;
        }
        if (first_row > last_row) {
            continue;
        }
        if (offset < 0 || offset >= g_stiffness_value_count) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Skyline offset out of range for column %d (offset=%d, count=%d)",
                             col, offset, g_stiffness_value_count);
        }

        for (int row = first_row; row <= last_row; row++) {
            int value_index = offset + (row - first_row);
            if (value_index < 0 || value_index >= g_stiffness_value_count) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Skyline index out of range (col=%d row=%d index=%d count=%d)",
                                 col, row, value_index, g_stiffness_value_count);
            }
            double value = g_global_stiffness_values[value_index];
            result[row] += value * x[col];
            if (row != col) {
                result[col] += value * x[row];
            }
        }
    }

    return FEM_SUCCESS;
}

/* Dot product: result = a^T * b */
fem_error_t cg_dot_product(double *a, double *b, int n, double *result)
{
    double sum = ZERO;
    int i;
    
#ifdef _OPENMP
    #pragma omp parallel for private(i) reduction(+:sum)
#endif
    for (i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    
    *result = sum;
    return FEM_SUCCESS;
}

/* Vector copy: dst = src */
fem_error_t cg_vector_copy(double *src, double *dst, int n)
{
    memcpy(dst, src, n * sizeof(double));
    return FEM_SUCCESS;
}

/* Vector scaling: vec = scale * vec */
fem_error_t cg_vector_scale(double *vec, double scale, int n)
{
    int i;
    
#ifdef _OPENMP
    #pragma omp parallel for private(i)
#endif
    for (i = 0; i < n; i++) {
        vec[i] *= scale;
    }
    
    return FEM_SUCCESS;
}

/* AXPY operation: y = a * x + y */
fem_error_t cg_vector_axpy(double a, double *x, double *y, int n)
{
    int i;
    
#ifdef _OPENMP
    #pragma omp parallel for private(i)
#endif
    for (i = 0; i < n; i++) {
        y[i] += a * x[i];
    }
    
    return FEM_SUCCESS;
}

/* Check convergence */
fem_error_t cg_check_convergence(double *r, int n, double tolerance, double *residual_norm)
{
    double norm_squared;
    fem_error_t err;
    
    err = cg_dot_product(r, r, n, &norm_squared);
    CHECK_ERROR(err);
    
    *residual_norm = sqrt(norm_squared);
    
    return (*residual_norm < tolerance) ? FEM_SUCCESS : FEM_ERROR_CONVERGENCE_FAILED;
}

/* Print iteration information */
void cg_print_iteration_info(int iter, double residual_norm, double tolerance)
{
    printf("  Iteration %4d: residual = %12.5e (target = %e)\n", 
           iter, residual_norm, tolerance);
}
