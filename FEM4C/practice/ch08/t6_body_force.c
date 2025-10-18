#include <stdio.h>

void t6_shape_functions(double xi, double eta, double N[6]);
int t6_shape_derivatives_global(const double coords[6][2],
                                double xi, double eta,
                                double dNdx[6], double dNdy[6],
                                double *detJ);

void t6_body_force(const double coords[6][2], double thickness,
                   double bx, double by, double load[12]) {
    const double gauss[3][2] = {
        {1.0 / 6.0, 1.0 / 6.0},
        {2.0 / 3.0, 1.0 / 6.0},
        {1.0 / 6.0, 2.0 / 3.0},
    };
    const double weight = 1.0 / 6.0;

    for (int i = 0; i < 12; ++i) {
        load[i] = 0.0;
    }

    for (int gp = 0; gp < 3; ++gp) {
        const double xi = gauss[gp][0];
        const double eta = gauss[gp][1];
        double N[6];
        t6_shape_functions(xi, eta, N);
        double detJ = 0.0;
        double dNdx[6];
        double dNdy[6];
        if (t6_shape_derivatives_global(coords, xi, eta,
                                        dNdx, dNdy, &detJ) != 0) {
            continue;
        }
        const double coeff = detJ * thickness * weight;
        for (int a = 0; a < 6; ++a) {
            load[2 * a] += N[a] * bx * coeff;
            load[2 * a + 1] += N[a] * by * coeff;
        }
    }
}

/* Minimal copies of the t6 shape helpers so that this file is self-contained. */
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

static void t6_shape_derivatives_natural(double xi, double eta,
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
    if (*detJ <= 0.0) {
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
    const double coords[6][2] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0},
        {0.5, 0.0},
        {0.5, 0.5},
        {0.0, 0.5},
    };
    double load[12];
    t6_body_force(coords, 1.0, 0.0, -1000.0, load);

    double sum_y = 0.0;
    for (int i = 0; i < 6; ++i) {
        sum_y += load[2 * i + 1];
    }
    printf("total Fy = %.3f (expected -500.000)\n", sum_y);
    return 0;
}
