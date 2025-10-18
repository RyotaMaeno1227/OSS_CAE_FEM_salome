/* FEM4C - Q4 Element Stiffness Matrix Computation
 * 4-node quadrilateral element stiffness matrix calculation
 */

#include "q4_stiffness.h"
#include "q4_element.h"
#include "../../common/globals.h"
#include "../../common/error.h"
#include <math.h>
#include <string.h>

/* Calculate Q4 element stiffness matrix */
fem_error_t q4_element_stiffness(int element_id, double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "q4_element_stiffness",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Initialize stiffness matrix to zero */
    for (int i = 0; i < Q4_TOTAL_DOF; i++) {
        for (int j = 0; j < Q4_TOTAL_DOF; j++) {
            ke[i][j] = 0.0;
        }
    }

    /* Integrate stiffness matrix using Gauss quadrature */
    fem_error_t error = q4_integrate_stiffness(element_id, ke);
    if (error != FEM_SUCCESS) return error;

    return FEM_SUCCESS;
}

/* Integrate stiffness matrix using Gauss quadrature */
fem_error_t q4_integrate_stiffness(int element_id, double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF])
{
    /* Get material properties */
    int material_id = g_element_material[element_id];
    if (material_id < 0 || material_id >= g_num_materials) {
        error_set(FEM_ERROR_INVALID_MATERIAL, "q4_integrate_stiffness",
                     "Invalid material ID");
        return FEM_ERROR_INVALID_MATERIAL;
    }

    double thickness = g_material_props[material_id][2];

    /* Integrate over Gauss points */
    for (int igp = 0; igp < Q4_GAUSS_POINTS; igp++) {
        double xi = g_q4_gauss_points[igp][0];
        double eta = g_q4_gauss_points[igp][1];
        double weight = g_q4_gauss_weights[igp];

        /* Calculate integrand at this Gauss point */
        double integrand[Q4_TOTAL_DOF][Q4_TOTAL_DOF];
        fem_error_t error = q4_stiffness_integrand(element_id, xi, eta, integrand);
        if (error != FEM_SUCCESS) return error;

        /* Calculate Jacobian determinant */
        double J[2][2], det_J;
        error = q4_jacobian_matrix(element_id, xi, eta, J, &det_J);
        if (error != FEM_SUCCESS) return error;

        /* Add contribution to stiffness matrix */
        double factor = weight * det_J * thickness;
        for (int i = 0; i < Q4_TOTAL_DOF; i++) {
            for (int j = 0; j < Q4_TOTAL_DOF; j++) {
                ke[i][j] += factor * integrand[i][j];
            }
        }
    }

    return FEM_SUCCESS;
}

/* Calculate stiffness matrix integrand at a Gauss point */
fem_error_t q4_stiffness_integrand(int element_id, double xi, double eta,
                                  double integrand[Q4_TOTAL_DOF][Q4_TOTAL_DOF])
{
    /* Get material matrix */
    int material_id = g_element_material[element_id];
    double D[Q4_STRAIN_COMPONENTS][Q4_STRAIN_COMPONENTS];
    fem_error_t error = q4_material_matrix(material_id, D);
    if (error != FEM_SUCCESS) return error;

    /* Calculate B matrix (strain-displacement matrix) */
    double B[Q4_STRAIN_COMPONENTS][Q4_TOTAL_DOF];
    error = q4_strain_displacement_matrix(element_id, xi, eta, B);
    if (error != FEM_SUCCESS) return error;

    /* Initialize integrand to zero */
    for (int i = 0; i < Q4_TOTAL_DOF; i++) {
        for (int j = 0; j < Q4_TOTAL_DOF; j++) {
            integrand[i][j] = 0.0;
        }
    }

    /* Calculate B^T * D * B */
    for (int i = 0; i < Q4_TOTAL_DOF; i++) {
        for (int j = 0; j < Q4_TOTAL_DOF; j++) {
            for (int k = 0; k < Q4_STRAIN_COMPONENTS; k++) {
                for (int l = 0; l < Q4_STRAIN_COMPONENTS; l++) {
                    integrand[i][j] += B[k][i] * D[k][l] * B[l][j];
                }
            }
        }
    }

    return FEM_SUCCESS;
}