/* FEM4C - Q4 Element Implementation
 * 4-node quadrilateral element with linear shape functions
 */

#include "q4_element.h"
#include "q4_stiffness.h"
#include "../element_base.h"
#include "../../common/globals.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

/* Q4 Gauss integration points and weights for quadrilateral (2x2 Gauss) */
double g_q4_gauss_points[Q4_GAUSS_POINTS][2] = {
    {-1.0/sqrt(3.0), -1.0/sqrt(3.0)},  /* Point 1: (-1/sqrt(3), -1/sqrt(3)) */
    { 1.0/sqrt(3.0), -1.0/sqrt(3.0)},  /* Point 2: ( 1/sqrt(3), -1/sqrt(3)) */
    { 1.0/sqrt(3.0),  1.0/sqrt(3.0)},  /* Point 3: ( 1/sqrt(3),  1/sqrt(3)) */
    {-1.0/sqrt(3.0),  1.0/sqrt(3.0)}   /* Point 4: (-1/sqrt(3),  1/sqrt(3)) */
};

double g_q4_gauss_weights[Q4_GAUSS_POINTS] = {
    1.0,  /* Weight 1 */
    1.0,  /* Weight 2 */
    1.0,  /* Weight 3 */
    1.0   /* Weight 4 */
};

/* Initialize Q4 element module */
fem_error_t q4_initialize(void)
{
    /* Gauss points and weights are already initialized as constants */
    return FEM_SUCCESS;
}

/* Register Q4 element with element base system */
fem_error_t q4_register(void)
{
    element_properties_t q4_props = {
        .element_type = ELEMENT_Q4,
        .nodes_per_element = Q4_NODES_PER_ELEMENT,
        .dof_per_node = Q4_DOF_PER_NODE,
        .total_dof = Q4_TOTAL_DOF,
        .gauss_points = Q4_GAUSS_POINTS,
        .spatial_dimension = 2,
        .strain_components = Q4_STRAIN_COMPONENTS,
        .stress_components = Q4_STRESS_COMPONENTS,
        .name = "Q4",
        .init = q4_initialize,
        .shape_functions = NULL,  /* Complex signature, handled directly */
        .jacobian = NULL,         /* Complex signature, handled directly */
        .stiffness = q4_element_stiffness,
        .stress = q4_element_stress,
        .validate = q4_validate_element
    };

    return element_register_type(&q4_props);
}

/* Calculate Q4 shape functions */
fem_error_t q4_shape_functions(double xi, double eta, double N[Q4_NODES_PER_ELEMENT])
{
    /* Linear shape functions for Q4 element (bilinear) */
    N[0] = 0.25 * (1.0 - xi) * (1.0 - eta);  /* N1: node at (-1,-1) */
    N[1] = 0.25 * (1.0 + xi) * (1.0 - eta);  /* N2: node at ( 1,-1) */
    N[2] = 0.25 * (1.0 + xi) * (1.0 + eta);  /* N3: node at ( 1, 1) */
    N[3] = 0.25 * (1.0 - xi) * (1.0 + eta);  /* N4: node at (-1, 1) */

    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to natural coordinates */
fem_error_t q4_shape_derivatives_natural(double xi, double eta,
                                         double dN_dxi[Q4_NODES_PER_ELEMENT],
                                         double dN_deta[Q4_NODES_PER_ELEMENT])
{
    /* Derivatives of shape functions with respect to xi */
    dN_dxi[0] = -0.25 * (1.0 - eta);  /* dN1/dxi */
    dN_dxi[1] =  0.25 * (1.0 - eta);  /* dN2/dxi */
    dN_dxi[2] =  0.25 * (1.0 + eta);  /* dN3/dxi */
    dN_dxi[3] = -0.25 * (1.0 + eta);  /* dN4/dxi */

    /* Derivatives of shape functions with respect to eta */
    dN_deta[0] = -0.25 * (1.0 - xi);  /* dN1/deta */
    dN_deta[1] = -0.25 * (1.0 + xi);  /* dN2/deta */
    dN_deta[2] =  0.25 * (1.0 + xi);  /* dN3/deta */
    dN_deta[3] =  0.25 * (1.0 - xi);  /* dN4/deta */

    return FEM_SUCCESS;
}

/* Calculate Jacobian matrix and determinant */
fem_error_t q4_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_jacobian_matrix",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Get element coordinates */
    double coords[Q4_NODES_PER_ELEMENT][2];
    fem_error_t error = q4_get_element_coordinates(element_id, coords);
    if (error != FEM_SUCCESS) return error;

    /* Calculate shape function derivatives */
    double dN_dxi[Q4_NODES_PER_ELEMENT], dN_deta[Q4_NODES_PER_ELEMENT];
    error = q4_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    if (error != FEM_SUCCESS) return error;

    /* Calculate Jacobian matrix components */
    J[0][0] = J[0][1] = J[1][0] = J[1][1] = 0.0;

    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        J[0][0] += dN_dxi[i]  * coords[i][0];  /* dx/dxi */
        J[0][1] += dN_dxi[i]  * coords[i][1];  /* dy/dxi */
        J[1][0] += dN_deta[i] * coords[i][0];  /* dx/deta */
        J[1][1] += dN_deta[i] * coords[i][1];  /* dy/deta */
    }

    /* Calculate determinant */
    *det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];

    if (fabs(*det_J) < TOLERANCE) {
        error_set(FEM_ERROR_SINGULAR_MATRIX, "q4_jacobian_matrix",
                     "Singular Jacobian matrix detected");
        return FEM_ERROR_SINGULAR_MATRIX;
    }

    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to global coordinates */
fem_error_t q4_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[Q4_NODES_PER_ELEMENT],
                                       double dN_dy[Q4_NODES_PER_ELEMENT])
{
    /* Calculate Jacobian matrix */
    double J[2][2], det_J;
    fem_error_t error = q4_jacobian_matrix(element_id, xi, eta, J, &det_J);
    if (error != FEM_SUCCESS) return error;

    /* Calculate inverse Jacobian */
    double J_inv[2][2];
    J_inv[0][0] =  J[1][1] / det_J;
    J_inv[0][1] = -J[0][1] / det_J;
    J_inv[1][0] = -J[1][0] / det_J;
    J_inv[1][1] =  J[0][0] / det_J;

    /* Calculate natural derivatives */
    double dN_dxi[Q4_NODES_PER_ELEMENT], dN_deta[Q4_NODES_PER_ELEMENT];
    error = q4_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    if (error != FEM_SUCCESS) return error;

    /* Transform to global derivatives */
    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        dN_dx[i] = J_inv[0][0] * dN_dxi[i] + J_inv[0][1] * dN_deta[i];
        dN_dy[i] = J_inv[1][0] * dN_dxi[i] + J_inv[1][1] * dN_deta[i];
    }

    return FEM_SUCCESS;
}

/* Calculate strain-displacement matrix B */
fem_error_t q4_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[Q4_STRAIN_COMPONENTS][Q4_TOTAL_DOF])
{
    /* Get global shape function derivatives */
    double dN_dx[Q4_NODES_PER_ELEMENT], dN_dy[Q4_NODES_PER_ELEMENT];
    fem_error_t error = q4_shape_derivatives_global(element_id, xi, eta, dN_dx, dN_dy);
    if (error != FEM_SUCCESS) return error;

    /* Initialize B matrix to zero */
    for (int i = 0; i < Q4_STRAIN_COMPONENTS; i++) {
        for (int j = 0; j < Q4_TOTAL_DOF; j++) {
            B[i][j] = 0.0;
        }
    }

    /* Fill B matrix for 2D plane stress/strain */
    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        int col_u = 2 * i;      /* u displacement column */
        int col_v = 2 * i + 1;  /* v displacement column */

        /* Strain component εxx */
        B[0][col_u] = dN_dx[i];
        B[0][col_v] = 0.0;

        /* Strain component εyy */
        B[1][col_u] = 0.0;
        B[1][col_v] = dN_dy[i];

        /* Strain component γxy (engineering shear strain) */
        B[2][col_u] = dN_dy[i];
        B[2][col_v] = dN_dx[i];
    }

    return FEM_SUCCESS;
}

/* Get element coordinates */
fem_error_t q4_get_element_coordinates(int element_id,
                                      double coords[Q4_NODES_PER_ELEMENT][2])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_get_element_coordinates",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        if (node_id < 0 || node_id >= g_num_nodes) {
            error_set(FEM_ERROR_INVALID_NODE, "q4_get_element_coordinates",
                         "Invalid node ID in element connectivity");
            return FEM_ERROR_INVALID_NODE;
        }
        coords[i][0] = g_node_coords[node_id][0];  /* x coordinate */
        coords[i][1] = g_node_coords[node_id][1];  /* y coordinate */
    }

    return FEM_SUCCESS;
}

/* Get element displacements */
fem_error_t q4_get_element_displacements(int element_id, double displ[Q4_TOTAL_DOF])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_get_element_displacements",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        if (node_id < 0 || node_id >= g_num_nodes) {
            error_set(FEM_ERROR_INVALID_NODE, "q4_get_element_displacements",
                         "Invalid node ID in element connectivity");
            return FEM_ERROR_INVALID_NODE;
        }
        displ[2*i]     = g_node_displ[node_id][0];  /* u displacement */
        displ[2*i + 1] = g_node_displ[node_id][1];  /* v displacement */
    }

    return FEM_SUCCESS;
}

/* Validate Q4 element */
fem_error_t q4_validate_element(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_validate_element",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Check number of nodes */
    if (Q4_NODES_PER_ELEMENT != Q4_NODES_PER_ELEMENT) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_validate_element",
                     "Invalid number of nodes for Q4 element");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Check geometric validity */
    return q4_check_element_geometry(element_id);
}

/* Check Q4 element geometry */
fem_error_t q4_check_element_geometry(int element_id)
{
    /* Get element coordinates */
    double coords[Q4_NODES_PER_ELEMENT][2];
    fem_error_t error = q4_get_element_coordinates(element_id, coords);
    if (error != FEM_SUCCESS) return error;

    /* Check if element is not degenerate by testing Jacobian at center */
    double J[2][2], det_J;
    error = q4_jacobian_matrix(element_id, 0.0, 0.0, J, &det_J);
    if (error != FEM_SUCCESS) return error;

    /* Jacobian determinant should be positive */
    if (det_J <= TOLERANCE) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_check_element_geometry",
                     "Element has invalid geometry (negative or zero Jacobian)");
        return FEM_ERROR_INVALID_INPUT;
    }

    return FEM_SUCCESS;
}

/* Calculate element stress */
fem_error_t q4_element_stress(int element_id, double stress[Q4_STRESS_COMPONENTS])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_element_stress",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Get element displacements */
    double displ[Q4_TOTAL_DOF];
    fem_error_t error = q4_get_element_displacements(element_id, displ);
    if (error != FEM_SUCCESS) return error;

    /* Calculate stress at element center (xi=0, eta=0) */
    double xi = 0.0, eta = 0.0;

    /* Get material matrix */
    int material_id = g_element_material[element_id];
    double D[Q4_STRAIN_COMPONENTS][Q4_STRAIN_COMPONENTS];
    error = q4_material_matrix(material_id, D);
    if (error != FEM_SUCCESS) return error;

    /* Calculate B matrix */
    double B[Q4_STRAIN_COMPONENTS][Q4_TOTAL_DOF];
    error = q4_strain_displacement_matrix(element_id, xi, eta, B);
    if (error != FEM_SUCCESS) return error;

    /* Calculate strain: ε = B * u */
    double strain[Q4_STRAIN_COMPONENTS] = {0.0, 0.0, 0.0};
    for (int i = 0; i < Q4_STRAIN_COMPONENTS; i++) {
        for (int j = 0; j < Q4_TOTAL_DOF; j++) {
            strain[i] += B[i][j] * displ[j];
        }
    }

    /* Calculate stress: σ = D * ε */
    for (int i = 0; i < Q4_STRESS_COMPONENTS; i++) {
        stress[i] = 0.0;
        for (int j = 0; j < Q4_STRAIN_COMPONENTS; j++) {
            stress[i] += D[i][j] * strain[j];
        }
    }

    return FEM_SUCCESS;
}

fem_error_t q4_material_matrix(int material_id, double D[Q4_STRESS_COMPONENTS][Q4_STRAIN_COMPONENTS])
{
    /* Use common 2D material matrix function */
    return element_2d_material_matrix_plane_stress(material_id, D);
}