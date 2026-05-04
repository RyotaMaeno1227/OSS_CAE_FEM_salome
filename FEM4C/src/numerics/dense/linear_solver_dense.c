#include "linear_solver_dense.h"
#include "../../common/error.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

fem_error_t mbd_linear_solver_dense_solve(const double *matrix,
                                          const double *rhs,
                                          int n,
                                          double pivot_tol,
                                          double *solution)
{
    double *augmented = NULL;
    double *swap_row = NULL;
    const size_t max_double_count = ((size_t) -1) / sizeof(double);
    size_t row_width = 0;
    size_t augmented_entries = 0;
    size_t swap_row_bytes = 0;
    int pivot = 0;
    int row = 0;
    int col = 0;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(matrix, "dense matrix");
    CHECK_NULL(rhs, "dense rhs");
    CHECK_NULL(solution, "dense solution");

    if (n <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense dimension %d must be positive",
                         n);
    }
    if (!isfinite(pivot_tol) || pivot_tol <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "pivot tolerance %.6e must be finite and positive",
                         pivot_tol);
    }
    for (row = 0; row < n; ++row) {
        if (!isfinite(rhs[row])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "dense rhs[%d]=%.16e must be finite",
                             row,
                             rhs[row]);
        }
        for (col = 0; col < n; ++col) {
            const double value = matrix[row * n + col];
            if (!isfinite(value)) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "dense matrix(%d,%d)=%.16e must be finite",
                                 row,
                                 col,
                                 value);
            }
        }
    }

    row_width = (size_t) n + 1U;
    if ((size_t) n > max_double_count ||
        row_width > max_double_count ||
        (size_t) n > max_double_count / row_width) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense dimension %d overflows workspace allocation",
                         n);
    }
    augmented_entries = (size_t) n * row_width;
    swap_row_bytes = row_width * sizeof(*swap_row);
    augmented = (double *) calloc(augmented_entries, sizeof(*augmented));
    swap_row = (double *) calloc(row_width, sizeof(*swap_row));
    if (!augmented || !swap_row) {
        free(swap_row);
        free(augmented);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate dense solver workspace for dimension %d",
                         n);
    }

    for (row = 0; row < n; ++row) {
        for (col = 0; col < n; ++col) {
            augmented[(size_t) row * row_width + (size_t) col] = matrix[row * n + col];
        }
        augmented[(size_t) row * row_width + (size_t) n] = rhs[row];
    }

    for (pivot = 0; pivot < n; ++pivot) {
        int pivot_row = pivot;
        double pivot_abs = fabs(augmented[(size_t) pivot * row_width + (size_t) pivot]);

        for (row = pivot + 1; row < n; ++row) {
            const double candidate_abs =
                fabs(augmented[(size_t) row * row_width + (size_t) pivot]);
            if (candidate_abs > pivot_abs) {
                pivot_abs = candidate_abs;
                pivot_row = row;
            }
        }

        if (pivot_abs <= pivot_tol) {
            err = error_set(FEM_ERROR_SINGULAR_MATRIX,
                            "near-singular dense matrix at pivot %d (|pivot|=%.6e, tol=%.6e)",
                            pivot, pivot_abs, pivot_tol);
            goto cleanup;
        }

        if (pivot_row != pivot) {
            memcpy(swap_row,
                   augmented + (size_t) pivot * row_width,
                   swap_row_bytes);
            memcpy(augmented + (size_t) pivot * row_width,
                   augmented + (size_t) pivot_row * row_width,
                   swap_row_bytes);
            memcpy(augmented + (size_t) pivot_row * row_width,
                   swap_row,
                   swap_row_bytes);
        }

        for (row = pivot + 1; row < n; ++row) {
            const double factor =
                augmented[(size_t) row * row_width + (size_t) pivot] /
                augmented[(size_t) pivot * row_width + (size_t) pivot];

            augmented[(size_t) row * row_width + (size_t) pivot] = 0.0;
            for (col = pivot + 1; col <= n; ++col) {
                augmented[(size_t) row * row_width + (size_t) col] -=
                    factor * augmented[(size_t) pivot * row_width + (size_t) col];
            }
        }
    }

    for (row = n - 1; row >= 0; --row) {
        double sum = augmented[(size_t) row * row_width + (size_t) n];

        for (col = row + 1; col < n; ++col) {
            sum -= augmented[(size_t) row * row_width + (size_t) col] * solution[col];
        }
        if (fabs(augmented[(size_t) row * row_width + (size_t) row]) <= pivot_tol) {
            err = error_set(FEM_ERROR_SINGULAR_MATRIX,
                            "near-singular diagonal during back substitution at row %d",
                            row);
            goto cleanup;
        }
        solution[row] = sum / augmented[(size_t) row * row_width + (size_t) row];
    }

cleanup:
    free(swap_row);
    free(augmented);
    return err;
}

double mbd_linear_solver_dense_residual_inf(const double *matrix,
                                            const double *rhs,
                                            const double *solution,
                                            int n)
{
    double residual_inf = 0.0;
    int row = 0;
    int col = 0;

    if (!matrix || !rhs || !solution || n <= 0) {
        return 0.0;
    }

    for (row = 0; row < n; ++row) {
        double residual = -rhs[row];

        for (col = 0; col < n; ++col) {
            residual += matrix[row * n + col] * solution[col];
        }
        if (fabs(residual) > residual_inf) {
            residual_inf = fabs(residual);
        }
    }

    return residual_inf;
}
