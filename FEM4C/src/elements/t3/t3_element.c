/* FEM4C - T3 Element Implementation
 * 3-node triangular element with linear shape functions
 */

#include "t3_element.h"
#include "../element_base.h"
#include "../../common/globals.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

/* Initialize T3 element module */
fem_error_t t3_initialize(void)
{
    return FEM_SUCCESS;
}

/* Register T3 element with element base system */
fem_error_t t3_register(void)
{
    element_properties_t t3_props = {
        .element_type = ELEMENT_T3,
        .nodes_per_element = T3_NODES_PER_ELEMENT,
        .dof_per_node = T3_DOF_PER_NODE,
        .total_dof = T3_TOTAL_DOF,
        .gauss_points = T3_GAUSS_POINTS,
        .spatial_dimension = 2,
        .strain_components = T3_STRAIN_COMPONENTS,
        .stress_components = T3_STRESS_COMPONENTS,
        .name = "T3",
        .init = t3_initialize,
        .shape_functions = NULL,  /* Complex signature, handled directly */
        .jacobian = NULL,         /* Complex signature, handled directly */
        .stiffness = t3_element_stiffness,
        .stress = t3_element_stress,
        .validate = t3_validate_element
    };

    return element_register_type(&t3_props);
}

/* Calculate T3 shape functions (linear) */
fem_error_t t3_shape_functions(double xi, double eta, double N[T3_NODES_PER_ELEMENT])
{
    /* Linear shape functions for T3 element */
    N[0] = 1.0 - xi - eta;  /* N1 at node (0,0) */
    N[1] = xi;              /* N2 at node (1,0) */
    N[2] = eta;             /* N3 at node (0,1) */

    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to natural coordinates */
fem_error_t t3_shape_derivatives_natural(double xi, double eta,
                                         double dN_dxi[T3_NODES_PER_ELEMENT],
                                         double dN_deta[T3_NODES_PER_ELEMENT])
{
    (void)xi;   /* Not used for linear triangles */
    (void)eta;  /* Not used for linear triangles */

    /* Derivatives of shape functions with respect to xi */
    dN_dxi[0] = -1.0;  /* dN1/dxi */
    dN_dxi[1] =  1.0;  /* dN2/dxi */
    dN_dxi[2] =  0.0;  /* dN3/dxi */

    /* Derivatives of shape functions with respect to eta */
    dN_deta[0] = -1.0;  /* dN1/deta */
    dN_deta[1] =  0.0;  /* dN2/deta */
    dN_deta[2] =  1.0;  /* dN3/deta */

    return FEM_SUCCESS;
}

/* Calculate Jacobian matrix and determinant */
fem_error_t t3_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_jacobian_matrix",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Get element coordinates */
    double coords[T3_NODES_PER_ELEMENT][2];
    fem_error_t error = t3_get_element_coordinates(element_id, coords);
    if (error != FEM_SUCCESS) return error;

    /* Calculate shape function derivatives */
    double dN_dxi[T3_NODES_PER_ELEMENT], dN_deta[T3_NODES_PER_ELEMENT];
    error = t3_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    if (error != FEM_SUCCESS) return error;

    /* Calculate Jacobian matrix components */
    J[0][0] = J[0][1] = J[1][0] = J[1][1] = 0.0;

    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        J[0][0] += dN_dxi[i]  * coords[i][0];  /* dx/dxi */
        J[0][1] += dN_dxi[i]  * coords[i][1];  /* dy/dxi */
        J[1][0] += dN_deta[i] * coords[i][0];  /* dx/deta */
        J[1][1] += dN_deta[i] * coords[i][1];  /* dy/deta */
    }

    /* Calculate determinant */
    *det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];

    if (fabs(*det_J) < TOLERANCE) {
        error_set(FEM_ERROR_SINGULAR_MATRIX, "t3_jacobian_matrix",
                     "Singular Jacobian matrix detected");
        return FEM_ERROR_SINGULAR_MATRIX;
    }

    return FEM_SUCCESS;
}

/* Calculate shape function derivatives with respect to global coordinates */
fem_error_t t3_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[T3_NODES_PER_ELEMENT],
                                       double dN_dy[T3_NODES_PER_ELEMENT])
{
    /* Calculate Jacobian matrix */
    double J[2][2], det_J;
    fem_error_t error = t3_jacobian_matrix(element_id, xi, eta, J, &det_J);
    if (error != FEM_SUCCESS) return error;

    /* Calculate inverse Jacobian */
    double J_inv[2][2];
    J_inv[0][0] =  J[1][1] / det_J;
    J_inv[0][1] = -J[0][1] / det_J;
    J_inv[1][0] = -J[1][0] / det_J;
    J_inv[1][1] =  J[0][0] / det_J;

    /* Calculate natural derivatives */
    double dN_dxi[T3_NODES_PER_ELEMENT], dN_deta[T3_NODES_PER_ELEMENT];
    error = t3_shape_derivatives_natural(xi, eta, dN_dxi, dN_deta);
    if (error != FEM_SUCCESS) return error;

    /* Transform to global derivatives */
    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        dN_dx[i] = J_inv[0][0] * dN_dxi[i] + J_inv[0][1] * dN_deta[i];
        dN_dy[i] = J_inv[1][0] * dN_dxi[i] + J_inv[1][1] * dN_deta[i];
    }

    return FEM_SUCCESS;
}

/* Calculate strain-displacement matrix B */
fem_error_t t3_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[T3_STRAIN_COMPONENTS][T3_TOTAL_DOF])
{
    /* Get global shape function derivatives */
    double dN_dx[T3_NODES_PER_ELEMENT], dN_dy[T3_NODES_PER_ELEMENT];
    fem_error_t error = t3_shape_derivatives_global(element_id, xi, eta, dN_dx, dN_dy);
    if (error != FEM_SUCCESS) return error;

    /* Initialize B matrix to zero */
    for (int i = 0; i < T3_STRAIN_COMPONENTS; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            B[i][j] = 0.0;
        }
    }

    /* Fill B matrix for 2D plane stress/strain */
    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
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

/* Calculate T3 element stiffness matrix */
fem_error_t t3_element_stiffness(int element_id, double ke[T3_TOTAL_DOF][T3_TOTAL_DOF])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_element_stiffness",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Initialize stiffness matrix to zero */
    for (int i = 0; i < T3_TOTAL_DOF; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            ke[i][j] = 0.0;
        }
    }

    /* Get material properties */
    int material_id = g_element_material[element_id];
    if (material_id < 0 || material_id >= g_num_materials) {
        error_set(FEM_ERROR_INVALID_MATERIAL, "t3_element_stiffness",
                     "Invalid material ID");
        return FEM_ERROR_INVALID_MATERIAL;
    }

    double thickness = g_material_props[material_id][2];

    /* For T3, use 1-point Gauss integration at centroid */
    double xi = 1.0/3.0, eta = 1.0/3.0;

    /* Get material matrix */
    double D[T3_STRAIN_COMPONENTS][T3_STRAIN_COMPONENTS];
    fem_error_t error = element_2d_material_matrix_plane_stress(material_id, D);
    if (error != FEM_SUCCESS) return error;

    /* Calculate B matrix */
    double B[T3_STRAIN_COMPONENTS][T3_TOTAL_DOF];
    error = t3_strain_displacement_matrix(element_id, xi, eta, B);
    if (error != FEM_SUCCESS) return error;

    /* Calculate Jacobian determinant */
    double J[2][2], det_J;
    error = t3_jacobian_matrix(element_id, xi, eta, J, &det_J);
    if (error != FEM_SUCCESS) return error;

    /* Calculate stiffness matrix: K = B^T * D * B * det_J * thickness */
    double factor = det_J * thickness * 0.5;  /* 0.5 is the weight for triangle integration */

    for (int i = 0; i < T3_TOTAL_DOF; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            for (int k = 0; k < T3_STRAIN_COMPONENTS; k++) {
                for (int l = 0; l < T3_STRAIN_COMPONENTS; l++) {
                    ke[i][j] += factor * B[k][i] * D[k][l] * B[l][j];
                }
            }
        }
    }

    return FEM_SUCCESS;
}

/* Calculate element stress */
fem_error_t t3_element_stress(int element_id, double stress[T3_STRESS_COMPONENTS])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_element_stress",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Get element displacements */
    double displ[T3_TOTAL_DOF];
    fem_error_t error = t3_get_element_displacements(element_id, displ);
    if (error != FEM_SUCCESS) return error;

    /* Calculate stress at element center */
    double xi = 1.0/3.0, eta = 1.0/3.0;

    /* Get material matrix */
    int material_id = g_element_material[element_id];
    double D[T3_STRAIN_COMPONENTS][T3_STRAIN_COMPONENTS];
    error = element_2d_material_matrix_plane_stress(material_id, D);
    if (error != FEM_SUCCESS) return error;

    /* Calculate B matrix */
    double B[T3_STRAIN_COMPONENTS][T3_TOTAL_DOF];
    error = t3_strain_displacement_matrix(element_id, xi, eta, B);
    if (error != FEM_SUCCESS) return error;

    /* Calculate strain: ε = B * u */
    double strain[T3_STRAIN_COMPONENTS] = {0.0, 0.0, 0.0};
    for (int i = 0; i < T3_STRAIN_COMPONENTS; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            strain[i] += B[i][j] * displ[j];
        }
    }

    /* Calculate stress: σ = D * ε */
    for (int i = 0; i < T3_STRESS_COMPONENTS; i++) {
        stress[i] = 0.0;
        for (int j = 0; j < T3_STRAIN_COMPONENTS; j++) {
            stress[i] += D[i][j] * strain[j];
        }
    }

    return FEM_SUCCESS;
}

/* Get element coordinates */
fem_error_t t3_get_element_coordinates(int element_id,
                                      double coords[T3_NODES_PER_ELEMENT][2])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_get_element_coordinates",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        if (node_id < 0 || node_id >= g_num_nodes) {
            error_set(FEM_ERROR_INVALID_NODE, "t3_get_element_coordinates",
                         "Invalid node ID in element connectivity");
            return FEM_ERROR_INVALID_NODE;
        }
        coords[i][0] = g_node_coords[node_id][0];  /* x coordinate */
        coords[i][1] = g_node_coords[node_id][1];  /* y coordinate */
    }

    return FEM_SUCCESS;
}

/* Get element displacements */
fem_error_t t3_get_element_displacements(int element_id, double displ[T3_TOTAL_DOF])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_get_element_displacements",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        if (node_id < 0 || node_id >= g_num_nodes) {
            error_set(FEM_ERROR_INVALID_NODE, "t3_get_element_displacements",
                         "Invalid node ID in element connectivity");
            return FEM_ERROR_INVALID_NODE;
        }
        displ[2*i]     = g_node_displ[node_id][0];  /* u displacement */
        displ[2*i + 1] = g_node_displ[node_id][1];  /* v displacement */
    }

    return FEM_SUCCESS;
}

/* Validate T3 element */
fem_error_t t3_validate_element(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_validate_element",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Basic geometric validity check */
    double coords[T3_NODES_PER_ELEMENT][2];
    fem_error_t error = t3_get_element_coordinates(element_id, coords);
    if (error != FEM_SUCCESS) return error;

    /* Check if element is not degenerate by testing area */
    double J[2][2], det_J;
    error = t3_jacobian_matrix(element_id, 1.0/3.0, 1.0/3.0, J, &det_J);
    if (error != FEM_SUCCESS) return error;

    /* Area magnitude should be non-zero; fix orientation if needed */
    if (fabs(det_J) <= TOLERANCE) {
        error_set(FEM_ERROR_INVALID_INPUT, "t3_validate_element",
                     "Element has invalid geometry (near-zero area)");
        return FEM_ERROR_INVALID_INPUT;
    }
    if (det_J < 0.0) {
        if (g_t3_strict_orientation) {
            error_set(FEM_ERROR_INVALID_INPUT, "t3_validate_element",
                      "Element %d has clockwise orientation (strict mode)", element_id + 1);
            return FEM_ERROR_INVALID_INPUT;
        }
        int tmp = g_element_nodes[element_id][0];
        g_element_nodes[element_id][0] = g_element_nodes[element_id][1];
        g_element_nodes[element_id][1] = tmp;
        static int warned = 0;
        if (!warned) {
            printf("  Warning: T3 element orientation corrected (clockwise -> CCW). ");
            printf("Use --strict-t3-orientation to fail instead.\n");
            warned = 1;
        }
        error = t3_jacobian_matrix(element_id, 1.0/3.0, 1.0/3.0, J, &det_J);
        if (error != FEM_SUCCESS) return error;
        if (fabs(det_J) <= TOLERANCE) {
            error_set(FEM_ERROR_INVALID_INPUT, "t3_validate_element",
                         "Element has invalid geometry after orientation fix");
            return FEM_ERROR_INVALID_INPUT;
        }
    }

    return FEM_SUCCESS;
}
