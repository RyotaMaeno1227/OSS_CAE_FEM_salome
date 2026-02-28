#include "../include/chrono_small_matrix.h"

#include <math.h>
#include <string.h>

static int chrono_smallmat_gaussjordan(const double *matrix,
                                       int n,
                                       double pivot_epsilon,
                                       double *inverse,
                                       double *min_pivot_out,
                                       double *max_pivot_out,
                                       double *pivot_history,
                                       int *pivot_count_out,
                                       int *rank_out) {
    if (!matrix || !inverse || n <= 0 || n > CHRONO_SMALL_MAT_MAX_N) {
        return 0;
    }

    double a[CHRONO_SMALL_MAT_MAX_N][CHRONO_SMALL_MAT_MAX_N];
    double inv[CHRONO_SMALL_MAT_MAX_N][CHRONO_SMALL_MAT_MAX_N];
    double scale[CHRONO_SMALL_MAT_MAX_N];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[i][j] = matrix[i * CHRONO_SMALL_MAT_MAX_N + j];
            inv[i][j] = (i == j) ? 1.0 : 0.0;
        }
        double max_row = 0.0;
        for (int j = 0; j < n; ++j) {
            double value = fabs(a[i][j]);
            if (value > max_row) {
                max_row = value;
            }
        }
        scale[i] = (max_row < pivot_epsilon) ? 1.0 : max_row;
    }

    double min_pivot = 0.0;
    double max_pivot = 0.0;
    int rank = 0;
    int pivot_count = 0;

    for (int col = 0; col < n; ++col) {
        int pivot_row = col;
        double best_metric = -1.0;
        for (int row = col; row < n; ++row) {
            double metric = fabs(a[row][col]) / scale[row];
            if (metric > best_metric) {
                best_metric = metric;
                pivot_row = row;
            }
        }
        double pivot_val = fabs(a[pivot_row][col]);
        if (pivot_val < pivot_epsilon) {
            continue;
        }
        if (pivot_row != col) {
            for (int k = 0; k < n; ++k) {
                double tmp = a[col][k];
                a[col][k] = a[pivot_row][k];
                a[pivot_row][k] = tmp;
                tmp = inv[col][k];
                inv[col][k] = inv[pivot_row][k];
                inv[pivot_row][k] = tmp;
            }
            double tmp_scale = scale[col];
            scale[col] = scale[pivot_row];
            scale[pivot_row] = tmp_scale;
        }
        double pivot = a[col][col];
        double inv_pivot = 1.0 / pivot;
        for (int k = 0; k < n; ++k) {
            a[col][k] *= inv_pivot;
            inv[col][k] *= inv_pivot;
        }
        for (int row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            double factor = a[row][col];
            if (factor == 0.0) {
                continue;
            }
            for (int k = 0; k < n; ++k) {
                a[row][k] -= factor * a[col][k];
                inv[row][k] -= factor * inv[col][k];
            }
        }
        if (rank == 0) {
            min_pivot = fabs(pivot);
            max_pivot = fabs(pivot);
        } else {
            if (fabs(pivot) < min_pivot) {
                min_pivot = fabs(pivot);
            }
            if (fabs(pivot) > max_pivot) {
                max_pivot = fabs(pivot);
            }
        }
        if (pivot_history && pivot_count < CHRONO_SMALL_MAT_MAX_N) {
            pivot_history[pivot_count] = fabs(pivot);
        }
        pivot_count += 1;
        rank += 1;
    }

    if (rank < n) {
        return 0;
    }

    if (min_pivot_out) {
        *min_pivot_out = min_pivot;
    }
    if (max_pivot_out) {
        *max_pivot_out = max_pivot;
    }
    if (pivot_count_out) {
        *pivot_count_out = pivot_count;
    }
    if (rank_out) {
        *rank_out = rank;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            inverse[i * CHRONO_SMALL_MAT_MAX_N + j] = inv[i][j];
        }
    }
    return 1;
}

int chrono_smallmat_invert_with_history(const double *matrix,
                                        int n,
                                        double pivot_epsilon,
                                        ChronoSmallMatInversionResult_C *result) {
    if (!result) {
        return 0;
    }
    double min_pivot = 0.0;
    double max_pivot = 0.0;
    int pivot_count = 0;
    int rank = 0;
    if (!chrono_smallmat_gaussjordan(matrix,
                                     n,
                                     pivot_epsilon,
                                     result->inverse,
                                     &min_pivot,
                                     &max_pivot,
                                     result->pivot_history,
                                     &pivot_count,
                                     &rank)) {
        memset(result, 0, sizeof(*result));
        return 0;
    }
    result->min_pivot = min_pivot;
    result->max_pivot = max_pivot;
    result->pivot_count = pivot_count;
    result->rank = rank;
    return 1;
}

int chrono_smallmat_invert_2x2(const double *matrix, double *inverse) {
    ChronoSmallMatInversionResult_C result;
    if (!chrono_smallmat_invert_with_history(matrix, 2, 1e-12, &result)) {
        return 0;
    }
    memcpy(inverse, result.inverse, sizeof(double) * 4);
    return 1;
}

int chrono_smallmat_invert_3x3(const double *matrix, double *inverse) {
    ChronoSmallMatInversionResult_C result;
    if (!chrono_smallmat_invert_with_history(matrix, 3, 1e-12, &result)) {
        return 0;
    }
    memcpy(inverse, result.inverse, sizeof(double) * 9);
    return 1;
}

int chrono_smallmat_invert_4x4(const double *matrix, double *inverse) {
    ChronoSmallMatInversionResult_C result;
    if (!chrono_smallmat_invert_with_history(matrix, 4, 1e-12, &result)) {
        return 0;
    }
    memcpy(inverse, result.inverse, sizeof(double) * 16);
    return 1;
}

void chrono_smallmat_mul(const double *a, const double *b, double *out, int n) {
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += a[row * n + k] * b[k * n + col];
            }
            out[row * n + col] = sum;
        }
    }
}
