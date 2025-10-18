#include <stdio.h>

void map_t3_dofs(const int element_nodes[3], int dof_map[6]) {
    for (int a = 0; a < 3; ++a) {
        dof_map[2 * a] = element_nodes[a] * 2;
        dof_map[2 * a + 1] = element_nodes[a] * 2 + 1;
    }
}

void assemble(double *K, const double Ke[6][6],
              const int dof_map[6], int total_dof) {
    for (int i = 0; i < 6; ++i) {
        const int gi = dof_map[i];
        for (int j = 0; j < 6; ++j) {
            const int gj = dof_map[j];
            K[gi * total_dof + gj] += Ke[i][j];
        }
    }
}

void apply_dirichlet(double *K, double *f, int total_dof,
                     int fixed_dof, double value) {
    for (int j = 0; j < total_dof; ++j) {
        K[fixed_dof * total_dof + j] = 0.0;
    }
    for (int i = 0; i < total_dof; ++i) {
        K[i * total_dof + fixed_dof] = 0.0;
    }
    K[fixed_dof * total_dof + fixed_dof] = 1.0;
    f[fixed_dof] = value;
}

static void print_matrix(const double *K, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            printf("%8.2f ", K[i * n + j]);
        }
        printf("\n");
    }
}

int main(void) {
    const int element_nodes[3] = {0, 1, 2};
    int dof_map[6];
    map_t3_dofs(element_nodes, dof_map);

    double Ke[6][6] = {{0}};
    for (int i = 0; i < 6; ++i) {
        Ke[i][i] = 2.0;
    }
    Ke[0][2] = Ke[2][0] = -1.0;
    Ke[0][4] = Ke[4][0] = -1.0;

    const int total_dof = 6;
    double K[36] = {0};
    assemble(K, Ke, dof_map, total_dof);

    double f[6] = {0};
    f[5] = 1.0;
    apply_dirichlet(K, f, total_dof, 0, 0.0);

    printf("assembled K with Dirichlet at dof 0:\n");
    print_matrix(K, total_dof);
    printf("rhs:\n");
    for (int i = 0; i < total_dof; ++i) {
        printf("  f[%d] = %.2f\n", i, f[i]);
    }

    return 0;
}
