#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/chrono_small_matrix.h"

static void random_spd_matrix(double *m, int n) {
    for (int i = 0; i < n * n; ++i) {
        m[i] = ((double)rand() / (double)RAND_MAX) - 0.5;
    }
    for (int i = 0; i < n; ++i) {
        m[i * n + i] += (double)n + 0.5;
    }
}

static double timed_invert(int n,
                           int (*invert)(const double *, double *),
                           int samples) {
    double *mat = (double *)malloc((size_t)n * n * sizeof(double));
    double *inv = (double *)malloc((size_t)n * n * sizeof(double));
    int success = 0;
    clock_t start = clock();
    for (int i = 0; i < samples; ++i) {
        random_spd_matrix(mat, n);
        success += invert(mat, inv);
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    free(mat);
    free(inv);
    return (double)success / (double)samples / elapsed;
}

int main(void) {
    srand(0xC0FFEE);
    const int samples = 20000;
    double rate2 = timed_invert(2, chrono_smallmat_invert_2x2, samples);
    double rate3 = timed_invert(3, chrono_smallmat_invert_3x3, samples);
    double rate4 = timed_invert(4, chrono_smallmat_invert_4x4, samples);
    printf("Small-matrix inversion throughput (successes/s)\n");
    printf("2x2: %.2f\n", rate2);
    printf("3x3: %.2f\n", rate3);
    printf("4x4: %.2f\n", rate4);
    return 0;
}
