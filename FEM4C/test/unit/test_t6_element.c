/* FEM4C - T6 Element Unit Tests
 * Test functions for T6 element implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../../src/common/constants.h"
#include "../../src/common/types.h"
#include "../../src/common/globals.h"
#include "../../src/common/error.h"
#include "../../src/elements/t6/t6_element.h"
#include "../../src/elements/t6/t6_stiffness.h"

/* Test tolerance */
#define TEST_TOL 1.0e-10

/* Test counter */
static int tests_passed = 0;
static int tests_total = 0;

/* Test macros */
#define ASSERT_DOUBLE_EQ(expected, actual, tol) \
    do { \
        tests_total++; \
        if (fabs((expected) - (actual)) < (tol)) { \
            tests_passed++; \
            printf("  PASS: %s\n", #actual); \
        } else { \
            printf("  FAIL: %s - Expected %g, got %g\n", #actual, (double)(expected), (double)(actual)); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        tests_total++; \
        if (condition) { \
            tests_passed++; \
            printf("  PASS: %s\n", #condition); \
        } else { \
            printf("  FAIL: %s\n", #condition); \
        } \
    } while(0)

#define ASSERT_SUCCESS(call) \
    do { \
        tests_total++; \
        fem_error_t result = (call); \
        if (result == FEM_SUCCESS) { \
            tests_passed++; \
            printf("  PASS: %s\n", #call); \
        } else { \
            printf("  FAIL: %s - Error: %s\n", #call, error_get_string(result)); \
        } \
    } while(0)

/* Test functions */
void test_t6_shape_functions(void);
void test_t6_shape_derivatives(void);
void test_t6_jacobian_matrix(void);
void test_t6_material_matrix(void);
void test_t6_stiffness_matrix(void);
void setup_test_element(void);

int main(void)
{
    printf("FEM4C T6 Element Unit Tests\n");
    printf("============================\n\n");
    
    /* Initialize global variables */
    globals_initialize();
    t6_initialize();
    
    /* Setup test element */
    setup_test_element();
    
    /* Run tests */
    test_t6_shape_functions();
    test_t6_shape_derivatives();
    test_t6_jacobian_matrix();
    test_t6_material_matrix();
    test_t6_stiffness_matrix();
    test_t6_shape_function_unity();
    test_t6_derivatives_consistency();
    
    /* Print results */
    printf("\nTest Results:\n");
    printf("=============\n");
    printf("Tests passed: %d / %d\n", tests_passed, tests_total);
    printf("Success rate: %.1f%%\n", (double)tests_passed / tests_total * 100.0);
    
    return (tests_passed == tests_total) ? 0 : 1;
}

/* Setup a simple test element */
void setup_test_element(void)
{
    /* Create a simple T6 triangle */
    g_num_nodes = 6;
    g_num_elements = 1;
    g_num_materials = 1;
    
    /* Node coordinates - unit triangle with midside nodes */
    g_node_coords[0][0] = 0.0; g_node_coords[0][1] = 0.0; /* Node 1 */
    g_node_coords[1][0] = 1.0; g_node_coords[1][1] = 0.0; /* Node 2 */
    g_node_coords[2][0] = 0.0; g_node_coords[2][1] = 1.0; /* Node 3 */
    g_node_coords[3][0] = 0.5; g_node_coords[3][1] = 0.0; /* Node 4 */
    g_node_coords[4][0] = 0.5; g_node_coords[4][1] = 0.5; /* Node 5 */
    g_node_coords[5][0] = 0.0; g_node_coords[5][1] = 0.5; /* Node 6 */
    
    /* Element connectivity (0-based indexing) */
    g_element_nodes[0][0] = 0; /* Node 1 */
    g_element_nodes[0][1] = 1; /* Node 2 */
    g_element_nodes[0][2] = 2; /* Node 3 */
    g_element_nodes[0][3] = 3; /* Node 4 */
    g_element_nodes[0][4] = 4; /* Node 5 */
    g_element_nodes[0][5] = 5; /* Node 6 */
    
    /* Element type */
    g_element_type[0] = ELEMENT_T6;
    g_element_material[0] = 0;
    
    /* Material properties */
    g_material_props[0][0] = 200000.0; /* E */
    g_material_props[0][1] = 0.3;      /* nu */
    g_material_props[0][2] = 1.0;      /* thickness */
    g_material_type[0] = MATERIAL_PLANE_STRESS;
}

/* Test T6 shape functions */
void test_t6_shape_functions(void)
{
    printf("Testing T6 shape functions...\n");
    
    double N[T6_NODES_PER_ELEMENT];
    double xi, eta;
    double sum;
    int i;
    
    /* Test at element center */
    xi = THIRD;
    eta = THIRD;
    
    ASSERT_SUCCESS(t6_shape_functions(xi, eta, N));
    
    /* Check partition of unity */
    sum = 0.0;
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        sum += N[i];
    }
    ASSERT_DOUBLE_EQ(1.0, sum, TEST_TOL);
    
    /* Test at corner nodes */
    xi = 1.0; eta = 0.0; /* Node 2 */
    ASSERT_SUCCESS(t6_shape_functions(xi, eta, N));
    ASSERT_DOUBLE_EQ(1.0, N[1], TEST_TOL); /* N2 = 1 */
    ASSERT_DOUBLE_EQ(0.0, N[0], TEST_TOL); /* N1 = 0 */
    ASSERT_DOUBLE_EQ(0.0, N[2], TEST_TOL); /* N3 = 0 */
    
    xi = 0.0; eta = 1.0; /* Node 3 */
    ASSERT_SUCCESS(t6_shape_functions(xi, eta, N));
    ASSERT_DOUBLE_EQ(1.0, N[2], TEST_TOL); /* N3 = 1 */
    ASSERT_DOUBLE_EQ(0.0, N[0], TEST_TOL); /* N1 = 0 */
    ASSERT_DOUBLE_EQ(0.0, N[1], TEST_TOL); /* N2 = 0 */
    
    printf("\n");
}

/* Test T6 shape function derivatives */
void test_t6_shape_derivatives(void)
{
    printf("Testing T6 shape function derivatives...\n");
    
    double dN_dxi[T6_NODES_PER_ELEMENT], dN_deta[T6_NODES_PER_ELEMENT];
    double xi, eta;
    
    /* Test at element center */
    xi = THIRD;
    eta = THIRD;
    
    ASSERT_SUCCESS(t6_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta));
    
    /* Check that derivatives are reasonable (non-zero for most) */
    ASSERT_TRUE(fabs(dN_dxi[0]) > TEST_TOL);
    ASSERT_TRUE(fabs(dN_deta[0]) > TEST_TOL);
    
    printf("\n");
}

/* Test Jacobian matrix calculation */
void test_t6_jacobian_matrix(void)
{
    printf("Testing T6 Jacobian matrix...\n");
    
    double J[2][2], det_J;
    double xi, eta;
    
    /* Test at element center */
    xi = THIRD;
    eta = THIRD;
    
    ASSERT_SUCCESS(t6_jacobian_matrix(0, xi, eta, J, &det_J));
    
    /* For unit right triangle, determinant should be positive */
    ASSERT_TRUE(det_J > 0.0);
    
    /* Check that Jacobian components are reasonable */
    ASSERT_TRUE(fabs(J[0][0]) > TEST_TOL);
    ASSERT_TRUE(fabs(J[1][1]) > TEST_TOL);
    
    printf("\n");
}

/* Test material matrix */
void test_t6_material_matrix(void)
{
    printf("Testing T6 material matrix...\n");
    
    double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS];
    double E = 200000.0;
    double nu = 0.3;
    
    /* Test plane stress */
    ASSERT_SUCCESS(t6_material_matrix_plane_stress(E, nu, D));
    
    /* Check diagonal terms */
    double expected_D11 = E / (1.0 - nu * nu);
    ASSERT_DOUBLE_EQ(expected_D11, D[0][0], TEST_TOL);
    ASSERT_DOUBLE_EQ(expected_D11, D[1][1], TEST_TOL);
    
    /* Check off-diagonal terms */
    double expected_D12 = expected_D11 * nu;
    ASSERT_DOUBLE_EQ(expected_D12, D[0][1], TEST_TOL);
    ASSERT_DOUBLE_EQ(expected_D12, D[1][0], TEST_TOL);
    
    /* Check shear term */
    double expected_D33 = expected_D11 * (1.0 - nu) * 0.5;
    ASSERT_DOUBLE_EQ(expected_D33, D[2][2], TEST_TOL);
    
    /* Test plane strain */
    ASSERT_SUCCESS(t6_material_matrix_plane_strain(E, nu, D));
    
    /* Plane strain D11 should be different */
    double expected_D11_strain = E * (1.0 - nu) / ((1.0 + nu) * (1.0 - 2.0 * nu));
    ASSERT_DOUBLE_EQ(expected_D11_strain, D[0][0], TEST_TOL);
    
    printf("\n");
}

/* Test stiffness matrix calculation */
void test_t6_stiffness_matrix(void)
{
    printf("Testing T6 stiffness matrix...\n");
    
    double ke[T6_TOTAL_DOF][T6_TOTAL_DOF];
    
    ASSERT_SUCCESS(t6_element_stiffness_matrix(0, ke));
    
    /* Check symmetry */
    for (int i = 0; i < T6_TOTAL_DOF; i++) {
        for (int j = 0; j < T6_TOTAL_DOF; j++) {
            ASSERT_DOUBLE_EQ(ke[i][j], ke[j][i], TEST_TOL);
        }
    }
    
    /* Check that diagonal terms are positive */
    for (int i = 0; i < T6_TOTAL_DOF; i++) {
        ASSERT_TRUE(ke[i][i] > 0.0);
    }
    
    /* Check that stiffness matrix is not zero */
    double max_stiff = 0.0;
    for (int i = 0; i < T6_TOTAL_DOF; i++) {
        for (int j = 0; j < T6_TOTAL_DOF; j++) {
            if (fabs(ke[i][j]) > max_stiff) {
                max_stiff = fabs(ke[i][j]);
            }
        }
    }
    ASSERT_TRUE(max_stiff > 1.0); /* Should be significant for given E */

    printf("\n");
}

/* Test shape function partition of unity */
void test_t6_shape_function_unity(void)
{
    printf("Testing T6 shape function partition of unity...\n");

    double N[T6_NODES_PER_ELEMENT];
    double xi_vals[] = {0.0, 0.5, 0.25, 0.16667, 0.33333};
    double eta_vals[] = {0.0, 0.25, 0.5, 0.16667, 0.33333};
    int num_tests = 5;

    for (int test = 0; test < num_tests; test++) {
        double xi = xi_vals[test];
        double eta = eta_vals[test];

        /* Skip invalid points outside triangle */
        if (xi + eta > 1.0) continue;

        ASSERT_SUCCESS(t6_shape_functions(xi, eta, N));

        /* Sum of shape functions should equal 1 */
        double sum = 0.0;
        for (int i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            sum += N[i];
        }

        printf("  Point (%.3f, %.3f): sum = %.6f\n", xi, eta, sum);
        ASSERT_DOUBLE_EQ(1.0, sum, TEST_TOL);
    }

    printf("\n");
}

/* Test shape function derivatives consistency */
void test_t6_derivatives_consistency(void)
{
    printf("Testing T6 shape function derivatives consistency...\n");

    double dN_dxi[T6_NODES_PER_ELEMENT], dN_deta[T6_NODES_PER_ELEMENT];
    double xi = 0.25, eta = 0.25;

    ASSERT_SUCCESS(t6_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta));

    /* Sum of derivatives should be zero (constant strain condition) */
    double sum_xi = 0.0, sum_eta = 0.0;
    for (int i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        sum_xi += dN_dxi[i];
        sum_eta += dN_deta[i];
    }

    printf("  Sum of dN/dxi  = %.6f (should be 0)\n", sum_xi);
    printf("  Sum of dN/deta = %.6f (should be 0)\n", sum_eta);

    ASSERT_DOUBLE_EQ(0.0, sum_xi, TEST_TOL);
    ASSERT_DOUBLE_EQ(0.0, sum_eta, TEST_TOL);

    printf("\n");
}