#ifndef CHRONO_SMALL_MATRIX_H
#define CHRONO_SMALL_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

int chrono_smallmat_invert_2x2(const double *matrix, double *inverse);
int chrono_smallmat_invert_3x3(const double *matrix, double *inverse);
int chrono_smallmat_invert_4x4(const double *matrix, double *inverse);
void chrono_smallmat_mul(const double *a, const double *b, double *out, int n);

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_SMALL_MATRIX_H */
