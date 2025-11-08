#include "../include/chrono_small_matrix.h"

#include <math.h>
#include <string.h>

static int chrono_smallmat_invert(double *tmp, double *inv, int n) {
    const double epsilon = 1e-12;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            inv[i * n + j] = (i == j) ? 1.0 : 0.0;
        }
    }
    for (int col = 0; col < n; ++col) {
        int pivot = col;
        double max_val = fabs(tmp[col * n + col]);
        for (int row = col + 1; row < n; ++row) {
            double val = fabs(tmp[row * n + col]);
            if (val > max_val) {
                max_val = val;
                pivot = row;
            }
        }
        if (max_val < epsilon) {
            return 0;
        }
        if (pivot != col) {
            for (int k = 0; k < n; ++k) {
                double tmp_val = tmp[col * n + k];
                tmp[col * n + k] = tmp[pivot * n + k];
                tmp[pivot * n + k] = tmp_val;
                tmp_val = inv[col * n + k];
                inv[col * n + k] = inv[pivot * n + k];
                inv[pivot * n + k] = tmp_val;
            }
        }
        double diag = tmp[col * n + col];
        double inv_diag = 1.0 / diag;
        for (int k = 0; k < n; ++k) {
            tmp[col * n + k] *= inv_diag;
            inv[col * n + k] *= inv_diag;
        }
        for (int row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            double factor = tmp[row * n + col];
            if (factor == 0.0) {
                continue;
            }
            for (int k = 0; k < n; ++k) {
                tmp[row * n + k] -= factor * tmp[col * n + k];
                inv[row * n + k] -= factor * inv[col * n + k];
            }
        }
    }
    return 1;
}

int chrono_smallmat_invert_2x2(const double *matrix, double *inverse) {
    double tmp[4];
    memcpy(tmp, matrix, sizeof(tmp));
    return chrono_smallmat_invert(tmp, inverse, 2);
}

int chrono_smallmat_invert_3x3(const double *matrix, double *inverse) {
    double tmp[9];
    memcpy(tmp, matrix, sizeof(tmp));
    return chrono_smallmat_invert(tmp, inverse, 3);
}

int chrono_smallmat_invert_4x4(const double *matrix, double *inverse) {
    double tmp[16];
    memcpy(tmp, matrix, sizeof(tmp));
    return chrono_smallmat_invert(tmp, inverse, 4);
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
