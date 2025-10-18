#include <math.h>
#include <stdio.h>

static void t3_shape_derivatives_natural(double dN_dxi[3], double dN_deta[3]) {
    dN_dxi[0] = -1.0;
    dN_dxi[1] = 1.0;
    dN_dxi[2] = 0.0;

    dN_deta[0] = -1.0;
    dN_deta[1] = 0.0;
    dN_deta[2] = 1.0;
}

static int t3_shape_derivatives_global(const double coords[3][2],
                                       double dNdx[3], double dNdy[3],
                                       double *detJ) {
    double dN_dxi[3];
    double dN_deta[3];
    t3_shape_derivatives_natural(dN_dxi, dN_deta);

    double J00 = 0.0;
    double J01 = 0.0;
    double J10 = 0.0;
    double J11 = 0.0;

    for (int i = 0; i < 3; ++i) {
        J00 += dN_dxi[i] * coords[i][0];
        J01 += dN_deta[i] * coords[i][0];
        J10 += dN_dxi[i] * coords[i][1];
        J11 += dN_deta[i] * coords[i][1];
    }

    *detJ = J00 * J11 - J01 * J10;
    if (fabs(*detJ) < 1e-16) {
        return -1;
    }

    const double inv_det = 1.0 / (*detJ);
    const double invJ00 = J11 * inv_det;
    const double invJ01 = -J01 * inv_det;
    const double invJ10 = -J10 * inv_det;
    const double invJ11 = J00 * inv_det;

    for (int i = 0; i < 3; ++i) {
        dNdx[i] = invJ00 * dN_dxi[i] + invJ01 * dN_deta[i];
        dNdy[i] = invJ10 * dN_dxi[i] + invJ11 * dN_deta[i];
    }
    return 0;
}

void t3_build_B(const double dNdx[3], const double dNdy[3], double B[3][6]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            B[i][j] = 0.0;
        }
    }

    for (int a = 0; a < 3; ++a) {
        const int ux = 2 * a;
        const int uy = 2 * a + 1;
        B[0][ux] = dNdx[a];
        B[1][uy] = dNdy[a];
        B[2][ux] = dNdy[a];
        B[2][uy] = dNdx[a];
    }
}

void plane_stress_D(double E, double nu, double D[3][3]) {
    const double scale = E / (1.0 - nu * nu);
    D[0][0] = scale;
    D[0][1] = scale * nu;
    D[0][2] = 0.0;

    D[1][0] = scale * nu;
    D[1][1] = scale;
    D[1][2] = 0.0;

    D[2][0] = 0.0;
    D[2][1] = 0.0;
    D[2][2] = scale * (1.0 - nu) * 0.5;
}

int t3_element_stiffness(const double coords[3][2], double E, double nu,
                         double thickness, double Ke[6][6]) {
    double dNdx[3];
    double dNdy[3];
    double detJ = 0.0;
    if (t3_shape_derivatives_global(coords, dNdx, dNdy, &detJ) != 0) {
        return -1;
    }
    if (detJ <= 0.0) {
        return -2;
    }

    double B[3][6];
    t3_build_B(dNdx, dNdy, B);

    double D[3][3];
    plane_stress_D(E, nu, D);

    double DB[3][6];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            DB[i][j] = 0.0;
            for (int k = 0; k < 3; ++k) {
                DB[i][j] += D[i][k] * B[k][j];
            }
        }
    }

    const double weight = 0.5;

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            double sum = 0.0;
            for (int k = 0; k < 3; ++k) {
                sum += B[k][i] * DB[k][j];
            }
            Ke[i][j] = sum * detJ * thickness * weight;
        }
    }
    return 0;
}

static double stiffness_vector_check(const double Ke[6][6], const double u[6]) {
    double v[6] = {0};
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            v[i] += Ke[i][j] * u[j];
        }
    }
    double max_abs = 0.0;
    for (int i = 0; i < 6; ++i) {
        double val = fabs(v[i]);
        if (val > max_abs) {
            max_abs = val;
        }
    }
    return max_abs;
}

int main(void) {
    const double coords[3][2] = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0},
    };

    double Ke[6][6];
    int rc = t3_element_stiffness(coords, 210e9, 0.3, 1.0, Ke);
    if (rc != 0) {
        printf("element stiffness failed with code %d\n", rc);
        return 1;
    }

    double max_asym = 0.0;
    for (int i = 0; i < 6; ++i) {
        for (int j = i + 1; j < 6; ++j) {
            double diff = fabs(Ke[i][j] - Ke[j][i]);
            if (diff > max_asym) {
                max_asym = diff;
            }
        }
    }
    printf("symmetry check: %.3e\n", max_asym);

    double rigid[6] = {1, 0, 1, 0, 1, 0};
    double rig_max = stiffness_vector_check(Ke, rigid);
    printf("rigid body x-translation residual: %.3e\n", rig_max);

    double rigid_y[6] = {0, 1, 0, 1, 0, 1};
    double rig_y = stiffness_vector_check(Ke, rigid_y);
    printf("rigid body y-translation residual: %.3e\n", rig_y);

    printf("K(0,0)=%.6e K(1,1)=%.6e\n", Ke[0][0], Ke[1][1]);

    return 0;
}
