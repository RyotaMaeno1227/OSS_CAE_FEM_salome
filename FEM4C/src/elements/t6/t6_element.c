/* FEM4C - T6 Element Implementation
 * 6-node triangular element with quadratic shape functions
 * Based on the Fortran implementation from elemod.f90
 */

#include "t6_element.h"
#include "t6_stiffness.h"
#include "../element_base.h"
#include "../../common/globals.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

/* T6 Gauss integration points and weights for triangle */
double g_t6_gauss_points[T6_GAUSS_POINTS][2] = {
    {0.166666666666667, 0.166666666666667},  /* Point 1: (1/6, 1/6) */
    {0.666666666666667, 0.166666666666667},  /* Point 2: (2/3, 1/6) */
    {0.166666666666667, 0.666666666666667}   /* Point 3: (1/6, 2/3) */
};

double g_t6_gauss_weights[T6_GAUSS_POINTS] = {
    0.166666666666667,  /* Weight 1: 1/6 */
    0.166666666666667,  /* Weight 2: 1/6 */
    0.166666666666667   /* Weight 3: 1/6 */
};

/* Initialize T6 element module */
fem_error_t t6_initialize(void)
{
    /* Gauss points and weights are already initialized as constants */
    return FEM_SUCCESS;
}

/* Register T6 element with element base system */
fem_error_t t6_register(void)
{
    element_properties_t t6_props = {
        .element_type = ELEMENT_T6,
        .nodes_per_element = T6_NODES_PER_ELEMENT,
        .dof_per_node = T6_DOF_PER_NODE,
        .total_dof = T6_TOTAL_DOF,
        .gauss_points = T6_GAUSS_POINTS,
        .spatial_dimension = 2,
        .strain_components = T6_STRAIN_COMPONENTS,
        .stress_components = T6_STRESS_COMPONENTS,
        .name = "T6",
        .init = t6_initialize,
        .shape_functions = NULL,  /* Complex signature, handled directly */
        .jacobian = NULL,         /* Complex signature, handled directly */
        .stiffness = t6_element_stiffness_matrix,
        .stress = NULL,  /* To be implemented */
        .validate = t6_validate_element
    };

    return element_register_type(&t6_props);
}

/* Calculate T6 shape functions */
fem_error_t t6_shape_functions(double xi, double eta, double N[T6_NODES_PER_ELEMENT])
{
    double zeta = ONE - xi - eta;  /* Third natural coordinate */
    
    /* Quadratic shape functions for T6 element */
    N[0] = zeta * (TWO * zeta - ONE);          /* N1 */
    N[1] = xi   * (TWO * xi   - ONE);          /* N2 */
    N[2] = eta  * (TWO * eta  - ONE);          /* N3 */
    N[3] = FOUR * zeta * xi;                   /* N4 */
    N[4] = FOUR * xi   * eta;                  /* N5 */
    N[5] = FOUR * eta  * zeta;                 /* N6 */
    
    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to natural coordinates */
fem_error_t t6_shape_derivatives_natural(double xi, double eta,
                                         double dN_dxi[T6_NODES_PER_ELEMENT],
                                         double dN_deta[T6_NODES_PER_ELEMENT])
{
    double zeta = ONE - xi - eta;  /* Third natural coordinate */

    /* Standard T6 shape function derivatives */
    /* Derivatives with respect to xi */
    dN_dxi[0] = FOUR * xi + FOUR * eta - THREE;     /* dN1/dxi = 4*xi + 4*eta - 3 */
    dN_dxi[1] = FOUR * xi - ONE;                    /* dN2/dxi = 4*xi - 1 */
    dN_dxi[2] = ZERO;                               /* dN3/dxi = 0 */
    dN_dxi[3] = FOUR * (ONE - TWO * xi - eta);     /* dN4/dxi = 4*(1 - 2*xi - eta) */
    dN_dxi[4] = FOUR * eta;                         /* dN5/dxi = 4*eta */
    dN_dxi[5] = -FOUR * eta;                        /* dN6/dxi = -4*eta */

    /* Derivatives with respect to eta */
    dN_deta[0] = FOUR * xi + FOUR * eta - THREE;    /* dN1/deta = 4*xi + 4*eta - 3 */
    dN_deta[1] = ZERO;                              /* dN2/deta = 0 */
    dN_deta[2] = FOUR * eta - ONE;                  /* dN3/deta = 4*eta - 1 */
    dN_deta[3] = -FOUR * xi;                        /* dN4/deta = -4*xi */
    dN_deta[4] = FOUR * xi;                         /* dN5/deta = 4*xi */
    dN_deta[5] = FOUR * (ONE - xi - TWO * eta);    /* dN6/deta = 4*(1 - xi - 2*eta) */

    return FEM_SUCCESS;
}

/* Calculate Jacobian matrix and its determinant */
fem_error_t t6_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J)
{
    double coords[T6_NODES_PER_ELEMENT][2];
    double dN_dxi[T6_NODES_PER_ELEMENT], dN_deta[T6_NODES_PER_ELEMENT];
    fem_error_t err;
    int i;
    
    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");
    
    /* Get element node coordinates */
    err = t6_get_element_coordinates(element_id, coords);
    CHECK_ERROR(err);
    
    /* Get shape function derivatives */
    err = t6_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    CHECK_ERROR(err);
    
    /* Initialize Jacobian matrix */
    J[0][0] = J[0][1] = J[1][0] = J[1][1] = ZERO;
    
    /* Calculate Jacobian matrix components */
    static int jacobian_calc_debug = 0;
    if (!jacobian_calc_debug) {
        printf("    Jacobian calculation debug:\n");
        printf("      Node coordinates: ");
        for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            printf("(%g,%g) ", coords[i][0], coords[i][1]);
        }
        printf("\n      Natural derivatives: ");
        for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            printf("dN%d/dxi=%g,dN%d/deta=%g ", i+1, dN_dxi[i], i+1, dN_deta[i]);
        }
        printf("\n");
        jacobian_calc_debug = 1;
    }

    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        J[0][0] += dN_dxi[i]  * coords[i][0];  /* dx/dxi */
        J[0][1] += dN_dxi[i]  * coords[i][1];  /* dy/dxi */
        J[1][0] += dN_deta[i] * coords[i][0];  /* dx/deta */
        J[1][1] += dN_deta[i] * coords[i][1];  /* dy/deta */
    }
    
    /* Calculate determinant */
    *det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];
    
    /* Check for valid determinant */
    if (fabs(*det_J) < TOLERANCE) {
        return error_set(FEM_ERROR_SINGULAR_MATRIX, 
                        "Zero or negative Jacobian determinant in element %d", element_id + 1);
    }
    
    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to global coordinates */
fem_error_t t6_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[T6_NODES_PER_ELEMENT],
                                       double dN_dy[T6_NODES_PER_ELEMENT])
{
    double J[2][2], inv_J[2][2], det_J;
    double dN_dxi[T6_NODES_PER_ELEMENT], dN_deta[T6_NODES_PER_ELEMENT];
    fem_error_t err;
    int i;
    
    /* Calculate Jacobian matrix */
    err = t6_jacobian_matrix(element_id, xi, eta, J, &det_J);
    CHECK_ERROR(err);
    
    /* Calculate inverse Jacobian */
    inv_J[0][0] =  J[1][1] / det_J;
    inv_J[0][1] = -J[0][1] / det_J;
    inv_J[1][0] = -J[1][0] / det_J;
    inv_J[1][1] =  J[0][0] / det_J;

    /* Debug output for Jacobian */
    static int jacobian_debug = 0;
    if (!jacobian_debug) {
        printf("    Jacobian matrix at (%.3f, %.3f):\n", xi, eta);
        printf("      J = [%.3f  %.3f]\n", J[0][0], J[0][1]);
        printf("          [%.3f  %.3f]\n", J[1][0], J[1][1]);
        printf("      det(J) = %.3f\n", det_J);
        printf("      inv_J = [%.3f  %.3f]\n", inv_J[0][0], inv_J[0][1]);
        printf("              [%.3f  %.3f]\n", inv_J[1][0], inv_J[1][1]);
        jacobian_debug = 1;
    }
    
    /* Get natural derivatives */
    err = t6_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    CHECK_ERROR(err);
    
    /* Transform to global derivatives */
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        dN_dx[i] = inv_J[0][0] * dN_dxi[i] + inv_J[0][1] * dN_deta[i];
        dN_dy[i] = inv_J[1][0] * dN_dxi[i] + inv_J[1][1] * dN_deta[i];
    }

    /* Debug output for first call */
    static int debug_printed = 0;
    if (!debug_printed) {
        printf("    Natural derivatives at (%.3f, %.3f):\n", xi, eta);
        for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            printf("      dN%d: dxi=%.3f, deta=%.3f -> dx=%.3f, dy=%.3f\n",
                   i+1, dN_dxi[i], dN_deta[i], dN_dx[i], dN_dy[i]);
        }
        debug_printed = 1;
    }
    
    return FEM_SUCCESS;
}

/* Calculate strain-displacement matrix (B-matrix) */
fem_error_t t6_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[T6_STRAIN_COMPONENTS][T6_TOTAL_DOF])
{
    double dN_dx[T6_NODES_PER_ELEMENT], dN_dy[T6_NODES_PER_ELEMENT];
    fem_error_t err;
    int i, j;
    
    /* Initialize B matrix */
    for (i = 0; i < T6_STRAIN_COMPONENTS; i++) {
        for (j = 0; j < T6_TOTAL_DOF; j++) {
            B[i][j] = ZERO;
        }
    }
    
    /* Get shape function derivatives */
    err = t6_shape_derivatives_global(element_id, xi, eta, dN_dx, dN_dy);
    CHECK_ERROR(err);
    
    /* Fill B matrix for 2D plane stress/strain */
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        int u_dof = 2 * i;      /* u displacement DOF */
        int v_dof = 2 * i + 1;  /* v displacement DOF */
        
        /* Strain component: epsilon_xx */
        B[0][u_dof] = dN_dx[i];
        B[0][v_dof] = ZERO;
        
        /* Strain component: epsilon_yy */
        B[1][u_dof] = ZERO;
        B[1][v_dof] = dN_dy[i];
        
        /* Strain component: gamma_xy */
        B[2][u_dof] = dN_dy[i];
        B[2][v_dof] = dN_dx[i];
    }
    
    return FEM_SUCCESS;
}

/* Get element node coordinates */
fem_error_t t6_get_element_coordinates(int element_id, 
                                      double coords[T6_NODES_PER_ELEMENT][2])
{
    int i, node_id;
    
    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");
    
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        node_id = g_element_nodes[element_id][i];
        CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
        
        coords[i][0] = g_node_coords[node_id][0];
        coords[i][1] = g_node_coords[node_id][1];
    }
    
    return FEM_SUCCESS;
}

/* Get element nodal displacements */
fem_error_t t6_get_element_displacements(int element_id,
                                        double displ[T6_TOTAL_DOF])
{
    int i, node_id;
    
    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");
    
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        node_id = g_element_nodes[element_id][i];
        CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
        
        displ[2*i]     = g_node_displ[node_id][0];  /* u displacement */
        displ[2*i + 1] = g_node_displ[node_id][1];  /* v displacement */
    }
    
    return FEM_SUCCESS;
}

/* Validate T6 element */
fem_error_t t6_validate_element(int element_id)
{
    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");
    
    /* Check if element type is T6 */
    if (g_element_type[element_id] != ELEMENT_T6) {
        fprintf(stderr, "[t6_validate] element %d unexpected type %d\n",
                element_id, g_element_type[element_id]);
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE, 
                        "Element %d is not a T6 element", element_id + 1);
    }
    
    /* Check element geometry */
    return t6_check_element_geometry(element_id);
}

/* Check element geometry for validity */
fem_error_t t6_check_element_geometry(int element_id)
{
    double coords[T6_NODES_PER_ELEMENT][2];
    double area;
    fem_error_t err;
    
    /* Get element coordinates */
    err = t6_get_element_coordinates(element_id, coords);
    CHECK_ERROR(err);
    
    /* Calculate element area using the first 3 corner nodes */
    area = 0.5 * fabs((coords[1][0] - coords[0][0]) * (coords[2][1] - coords[0][1]) -
                     (coords[2][0] - coords[0][0]) * (coords[1][1] - coords[0][1]));
    
    /* Check for degenerate element */
    if (area < TOLERANCE) {
        return error_set(FEM_ERROR_INVALID_INPUT, 
                        "Element %d has zero or negative area", element_id + 1);
    }
    
    return FEM_SUCCESS;
}
