#include <math.h>
#include <stdio.h>

static void gaussian_solve_3(double A[3][3], double b[3], double x[3]) {
    for (int k = 0; k < 3; ++k) {
        double pivot = A[k][k];
        for (int j = k; j < 3; ++j) {
            A[k][j] /= pivot;
        }
        b[k] /= pivot;
        for (int i = k + 1; i < 3; ++i) {
            double factor = A[i][k];
            for (int j = k; j < 3; ++j) {
                A[i][j] -= factor * A[k][j];
            }
            b[i] -= factor * b[k];
        }
    }

    for (int i = 2; i >= 0; --i) {
        double sum = b[i];
        for (int j = i + 1; j < 3; ++j) {
            sum -= A[i][j] * x[j];
        }
        x[i] = sum;
    }
}

static void solve_two_spring_system(double k1, double k2, double force,
                                    double penalty, double fixed_u0) {
    double K[3][3] = {
        {k1, -k1, 0.0},
        {-k1, k1 + k2, -k2},
        {0.0, -k2, k2},
    };
    double f[3] = {0.0, 0.0, force};

    K[0][0] += penalty;
    f[0] += penalty * fixed_u0;

    double x_penalty[3] = {0};
    double A_copy[3][3];
    double b_copy[3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            A_copy[i][j] = K[i][j];
        }
        b_copy[i] = f[i];
    }

    gaussian_solve_3(A_copy, b_copy, x_penalty);

    double A11 = k1 + k2;
    double A12 = -k2;
    double A22 = k2;
    double det = A11 * A22 - A12 * A12;
    double rhs1 = 0.0;
    double rhs2 = force;
    double u1_exact = (rhs1 * A22 - A12 * rhs2) / det;
    double u2_exact = (A11 * rhs2 - A12 * rhs1) / det;

    printf("penalty solution: u0=%.6e u1=%.6e u2=%.6e\n",
           x_penalty[0], x_penalty[1], x_penalty[2]);
    printf("exact solution  : u1=%.6e u2=%.6e\n",
           u1_exact, u2_exact);
    printf("|u1_err|=%.3e |u2_err|=%.3e\n",
           fabs(x_penalty[1] - u1_exact),
           fabs(x_penalty[2] - u2_exact));
}

int main(void) {
    const double k1 = 1000.0;
    const double k2 = 1500.0;
    const double force = 10.0;
    const double penalty = 1e12;

    solve_two_spring_system(k1, k2, force, penalty, 0.0);
    return 0;
}
