#include <math.h>
#include <stddef.h>
#include <string.h>

#include "../include/chrono_constraint_kkt_backend.h"

static ChronoKKTBackendStats_C g_kkt_stats = {0, 0};

int chrono_kkt_backend_invert_small(const double *src,
                                    int n,
                                    double pivot_epsilon,
                                    ChronoKKTBackendResult_C *result) {
    g_kkt_stats.calls += 1;
    if (!src || !result || n <= 0 || n > CHRONO_COUPLED_KKT_MAX_EQ) {
        g_kkt_stats.fallback_calls += 1;
        return 0;
    }

    ChronoKKTBackendResult_C backend_result;
    memset(&backend_result, 0, sizeof(backend_result));

    double a[CHRONO_COUPLED_KKT_MAX_EQ][CHRONO_COUPLED_KKT_MAX_EQ];
    double inv[CHRONO_COUPLED_KKT_MAX_EQ][CHRONO_COUPLED_KKT_MAX_EQ];
    double scale[CHRONO_COUPLED_KKT_MAX_EQ];
    backend_result.pivot_count = 0;

    for (int i = 0; i < n; ++i) {
        double max_row = 0.0;
        for (int j = 0; j < n; ++j) {
            double value = src[i * CHRONO_COUPLED_KKT_MAX_EQ + j];
            a[i][j] = value;
            inv[i][j] = (i == j) ? 1.0 : 0.0;
            if (fabs(value) > max_row) {
                max_row = fabs(value);
            }
        }
        scale[i] = (max_row < pivot_epsilon) ? 1.0 : max_row;
    }

    double min_pivot = 0.0;
    double max_pivot = 0.0;
    int rank = 0;

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
        if (backend_result.pivot_count < CHRONO_COUPLED_KKT_MAX_EQ) {
            backend_result.pivot_history[backend_result.pivot_count++] = fabs(pivot);
        }
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
        rank += 1;
    }

    if (rank < n) {
        g_kkt_stats.fallback_calls += 1;
        memset(result, 0, sizeof(*result));
        return 0;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result->inverse[i * CHRONO_COUPLED_KKT_MAX_EQ + j] = inv[i][j];
        }
    }
    result->min_pivot = min_pivot;
    result->max_pivot = max_pivot;
    result->pivot_count = backend_result.pivot_count;
    memcpy(result->pivot_history,
           backend_result.pivot_history,
           sizeof(result->pivot_history));
    result->rank = rank;
    result->success = 1;
    return 1;
}

const ChronoKKTBackendStats_C *chrono_kkt_backend_get_stats(void) {
    return &g_kkt_stats;
}
