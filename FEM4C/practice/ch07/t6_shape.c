#include <math.h>
#include <stdio.h>

void t6_shape_functions(double xi, double eta, double N[6]) {
    const double L1 = 1.0 - xi - eta;
    const double L2 = xi;
    const double L3 = eta;

    N[0] = L1 * (2.0 * L1 - 1.0);
    N[1] = L2 * (2.0 * L2 - 1.0);
    N[2] = L3 * (2.0 * L3 - 1.0);
    N[3] = 4.0 * L1 * L2;
    N[4] = 4.0 * L2 * L3;
    N[5] = 4.0 * L3 * L1;
}

void t6_shape_derivatives_natural(double xi, double eta,
                                  double dN_dxi[6], double dN_deta[6]) {
    const double L1 = 1.0 - xi - eta;
    const double L2 = xi;
    const double L3 = eta;

    dN_dxi[0] = -(4.0 * L1 - 1.0);
    dN_deta[0] = -(4.0 * L1 - 1.0);

    dN_dxi[1] = 4.0 * L2 - 1.0;
    dN_deta[1] = 0.0;

    dN_dxi[2] = 0.0;
    dN_deta[2] = 4.0 * L3 - 1.0;

    dN_dxi[3] = 4.0 * (L1 - L2);
    dN_deta[3] = -4.0 * L2;

    dN_dxi[4] = 4.0 * L3;
    dN_deta[4] = 4.0 * L2;

    dN_dxi[5] = -4.0 * L3;
    dN_deta[5] = 4.0 * (L1 - L3);
}

int t6_shape_derivatives_global(const double coords[6][2],
                                double xi, double eta,
                                double dNdx[6], double dNdy[6],
                                double *detJ) {
    double dN_dxi[6];
    double dN_deta[6];
    t6_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);

    double J00 = 0.0;
    double J01 = 0.0;
    double J10 = 0.0;
    double J11 = 0.0;
    for (int i = 0; i < 6; ++i) {
        J00 += dN_dxi[i] * coords[i][0];
        J01 += dN_deta[i] * coords[i][0];
        J10 += dN_dxi[i] * coords[i][1];
        J11 += dN_deta[i] * coords[i][1];
    }

    *detJ = J00 * J11 - J01 * J10;
    if (fabs(*detJ) < 1e-13) {
        return -1;
    }
    const double inv_det = 1.0 / (*detJ);
    const double invJ00 = J11 * inv_det;
    const double invJ01 = -J01 * inv_det;
    const double invJ10 = -J10 * inv_det;
    const double invJ11 = J00 * inv_det;

    for (int i = 0; i < 6; ++i) {
        dNdx[i] = invJ00 * dN_dxi[i] + invJ01 * dN_deta[i];
        dNdy[i] = invJ10 * dN_dxi[i] + invJ11 * dN_deta[i];
    }
    return 0;
}

int main(void) {
    double N[6];
    t6_shape_functions(1.0, 0.0, N);
    printf("N at node 2 (xi=1,eta=0): N2=%.1f N1=%.1f\n", N[1], N[0]);

    const double coords[6][2] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0},
        {0.5, 0.0},
        {0.5, 0.5},
        {0.0, 0.5},
    };
    double detJ = 0.0;
    double dNdx[6];
    double dNdy[6];
    int rc = t6_shape_derivatives_global(coords, 1.0 / 3.0, 1.0 / 3.0,
                                         dNdx, dNdy, &detJ);
    if (rc != 0) {
        printf("detJ too small\n");
        return 1;
    }
    double sumN = 0.0;
    t6_shape_functions(1.0 / 3.0, 1.0 / 3.0, N);
    for (int i = 0; i < 6; ++i) {
        sumN += N[i];
    }
    printf("sum N at centroid = %.6f (should be 1)\n", sumN);
    printf("detJ = %.6f\n", detJ);
    return 0;
}
