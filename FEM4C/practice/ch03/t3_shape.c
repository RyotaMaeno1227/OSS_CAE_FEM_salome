#include <math.h>
#include <stdio.h>

void t3_shape_functions(double xi, double eta, double N[3]) {
    N[0] = 1.0 - xi - eta;
    N[1] = xi;
    N[2] = eta;
}

void t3_shape_derivatives_natural(double dN_dxi[3], double dN_deta[3]) {
    dN_dxi[0] = -1.0;
    dN_dxi[1] = 1.0;
    dN_dxi[2] = 0.0;

    dN_deta[0] = -1.0;
    dN_deta[1] = 0.0;
    dN_deta[2] = 1.0;
}

void t3_jacobian(const double coords[3][2], double xi, double eta,
                 double J[2][2], double *detJ) {
    (void)xi;
    (void)eta;

    double dN_dxi[3];
    double dN_deta[3];
    t3_shape_derivatives_natural(dN_dxi, dN_deta);

    J[0][0] = 0.0;
    J[0][1] = 0.0;
    J[1][0] = 0.0;
    J[1][1] = 0.0;

    for (int i = 0; i < 3; ++i) {
        J[0][0] += dN_dxi[i] * coords[i][0];
        J[0][1] += dN_deta[i] * coords[i][0];
        J[1][0] += dN_dxi[i] * coords[i][1];
        J[1][1] += dN_deta[i] * coords[i][1];
    }

    *detJ = J[0][0] * J[1][1] - J[0][1] * J[1][0];
}

int t3_shape_derivatives_global(const double coords[3][2], double xi, double eta,
                                double dNdx[3], double dNdy[3], double *detJ) {
    double J[2][2];
    t3_jacobian(coords, xi, eta, J, detJ);
    if (fabs(*detJ) < 1e-12) {
        return -1;
    }

    const double inv_det = 1.0 / (*detJ);
    const double invJ00 = J[1][1] * inv_det;
    const double invJ01 = -J[0][1] * inv_det;
    const double invJ10 = -J[1][0] * inv_det;
    const double invJ11 = J[0][0] * inv_det;

    double dN_dxi[3];
    double dN_deta[3];
    t3_shape_derivatives_natural(dN_dxi, dN_deta);

    for (int i = 0; i < 3; ++i) {
        dNdx[i] = invJ00 * dN_dxi[i] + invJ01 * dN_deta[i];
        dNdy[i] = invJ10 * dN_dxi[i] + invJ11 * dN_deta[i];
    }
    return 0;
}

static double max_abs_diff(const double *a, const double *b, int n) {
    double max_diff = 0.0;
    for (int i = 0; i < n; ++i) {
        double diff = fabs(a[i] - b[i]);
        if (diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

int main(void) {
    const double coords[3][2] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0},
    };

    double N[3];
    t3_shape_functions(1.0 / 3.0, 1.0 / 3.0, N);
    const double expected_equal[3] = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    printf("test1 |N - expected|_max = %.3e\n",
           max_abs_diff(N, expected_equal, 3));

    double detJ = 0.0;
    double J[2][2];
    t3_jacobian(coords, 0.2, 0.1, J, &detJ);
    printf("test2 detJ = %.6f (unit triangle => 1.000000)\n", detJ);

    double dNdx[3];
    double dNdy[3];
    if (t3_shape_derivatives_global(coords, 0.2, 0.1, dNdx, dNdy, &detJ) != 0) {
        printf("test3 failed: detJ nearly zero\n");
        return 1;
    }
    const double expected_dx[3] = {-1.0, 1.0, 0.0};
    const double expected_dy[3] = {-1.0, 0.0, 1.0};
    printf("test3 |dNdx - expected|_max = %.3e\n",
           max_abs_diff(dNdx, expected_dx, 3));
    printf("test3 |dNdy - expected|_max = %.3e\n",
           max_abs_diff(dNdy, expected_dy, 3));

    return 0;
}
