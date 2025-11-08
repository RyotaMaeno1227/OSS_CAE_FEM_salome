#ifndef CHRONO_SMALL_MATRIX_H
#define CHRONO_SMALL_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONO_SMALL_MAT_MAX_N 4

typedef struct ChronoSmallMatInversionResult_C {
    double inverse[CHRONO_SMALL_MAT_MAX_N * CHRONO_SMALL_MAT_MAX_N];
    double pivot_history[CHRONO_SMALL_MAT_MAX_N];
    double min_pivot;
    double max_pivot;
    int pivot_count;
    int rank;
} ChronoSmallMatInversionResult_C;

int chrono_smallmat_invert_2x2(const double *matrix, double *inverse);
int chrono_smallmat_invert_3x3(const double *matrix, double *inverse);
int chrono_smallmat_invert_4x4(const double *matrix, double *inverse);
int chrono_smallmat_invert_with_history(const double *matrix,
                                        int n,
                                        double pivot_epsilon,
                                        ChronoSmallMatInversionResult_C *result);
void chrono_smallmat_mul(const double *a, const double *b, double *out, int n);

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_SMALL_MATRIX_H */
