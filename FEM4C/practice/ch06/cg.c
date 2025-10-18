#include <math.h>
#include <stdio.h>

static void matvec(const double *A, const double *x, double *y, int n) {
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += A[i * n + j] * x[j];
        }
        y[i] = sum;
    }
}

static double dot(const double *a, const double *b, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

int cg_solve(const double *A, const double *b, double *x,
             int n, double tol, int max_iter) {
    double r[n];
    double p[n];
    double Ap[n];

    matvec(A, x, r, n);
    for (int i = 0; i < n; ++i) {
        r[i] = b[i] - r[i];
        p[i] = r[i];
    }

    double rsold = dot(r, r, n);
    if (sqrt(rsold) < tol) {
        return 0;
    }

    for (int iter = 0; iter < max_iter; ++iter) {
        matvec(A, p, Ap, n);
        double alpha = rsold / dot(p, Ap, n);
        for (int i = 0; i < n; ++i) {
            x[i] += alpha * p[i];
            r[i] -= alpha * Ap[i];
        }
        double rsnew = dot(r, r, n);
        if (sqrt(rsnew) < tol) {
            return iter + 1;
        }
        double beta = rsnew / rsold;
        for (int i = 0; i < n; ++i) {
            p[i] = r[i] + beta * p[i];
        }
        rsold = rsnew;
    }
    return -1;
}

int main(void) {
    const int n = 4;
    double A[16] = {
        4, 1, 0, 0,
        1, 3, 1, 0,
        0, 1, 3, 1,
        0, 0, 1, 2,
    };
    double b[4] = {1, 2, 3, 4};
    double x[4] = {0, 0, 0, 0};

    int iters = cg_solve(A, b, x, n, 1e-10, 100);
    if (iters < 0) {
        printf("CG did not converge\n");
        return 1;
    }
    printf("CG converged in %d iterations\n", iters);
    for (int i = 0; i < n; ++i) {
        printf("x[%d] = %.6e\n", i, x[i]);
    }
    return 0;
}
