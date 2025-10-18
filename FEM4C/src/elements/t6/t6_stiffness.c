/* FEM4C - T6 Element Stiffness Matrix Calculation
 * Implementation of T6 element stiffness matrix computation
 */

#include "t6_stiffness.h"
#include "../../common/globals.h"
#include "../../common/error.h"
#include <string.h>

/* Calculate material matrix for plane stress */
fem_error_t t6_material_matrix_plane_stress(double E, double nu, 
                                           double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS])
{
    double factor;
    int i, j;
    
    CHECK_POSITIVE(E, "Young's modulus");
    if (nu >= 0.5 || nu < -1.0) {
        return error_set(FEM_ERROR_INVALID_MATERIAL, 
                        "Invalid Poisson's ratio: %f (must be < 0.5 and >= -1.0)", nu);
    }
    
    /* Initialize matrix */
    for (i = 0; i < T6_STRESS_COMPONENTS; i++) {
        for (j = 0; j < T6_STRAIN_COMPONENTS; j++) {
            D[i][j] = ZERO;
        }
    }
    
    /* Calculate factor */
    factor = E / (ONE - nu * nu);
    
    /* Fill material matrix for plane stress */
    D[0][0] = factor;           /* sigma_xx / epsilon_xx */
    D[0][1] = factor * nu;      /* sigma_xx / epsilon_yy */
    D[1][0] = factor * nu;      /* sigma_yy / epsilon_xx */
    D[1][1] = factor;           /* sigma_yy / epsilon_yy */
    D[2][2] = factor * (ONE - nu) * HALF;  /* tau_xy / gamma_xy */
    
    return FEM_SUCCESS;
}

/* Calculate material matrix for plane strain */
fem_error_t t6_material_matrix_plane_strain(double E, double nu, 
                                           double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS])
{
    double factor;
    int i, j;
    
    CHECK_POSITIVE(E, "Young's modulus");
    if (nu >= 0.5 || nu < -1.0) {
        return error_set(FEM_ERROR_INVALID_MATERIAL, 
                        "Invalid Poisson's ratio: %f (must be < 0.5 and >= -1.0)", nu);
    }
    
    /* Initialize matrix */
    for (i = 0; i < T6_STRESS_COMPONENTS; i++) {
        for (j = 0; j < T6_STRAIN_COMPONENTS; j++) {
            D[i][j] = ZERO;
        }
    }
    
    /* Calculate factor */
    factor = E / ((ONE + nu) * (ONE - TWO * nu));
    
    /* Fill material matrix for plane strain */
    D[0][0] = factor * (ONE - nu);     /* sigma_xx / epsilon_xx */
    D[0][1] = factor * nu;             /* sigma_xx / epsilon_yy */
    D[1][0] = factor * nu;             /* sigma_yy / epsilon_xx */
    D[1][1] = factor * (ONE - nu);     /* sigma_yy / epsilon_yy */
    D[2][2] = factor * (ONE - TWO * nu) * HALF;  /* tau_xy / gamma_xy */
    
    return FEM_SUCCESS;
}

/* Main function to calculate T6 element stiffness matrix */
fem_error_t t6_element_stiffness_matrix(int element_id, 
                                       double ke[T6_TOTAL_DOF][T6_TOTAL_DOF])
{
    double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS];
    int material_id;
    double E, nu;
    fem_error_t err;
    
    /* Validate element */
    err = t6_validate_element(element_id);
    CHECK_ERROR(err);
    
    /* Get material properties */
    material_id = g_element_material[element_id];
    CHECK_BOUNDS(material_id, g_num_materials, "Material ID");
    
    E = g_material_props[material_id][0];   /* Young's modulus */
    nu = g_material_props[material_id][1];  /* Poisson's ratio */
    
    /* Calculate material matrix based on material type */
    if (g_material_type[material_id] == MATERIAL_PLANE_STRESS) {
        err = t6_material_matrix_plane_stress(E, nu, D);
    } else {
        err = t6_material_matrix_plane_strain(E, nu, D);
    }
    CHECK_ERROR(err);
    
    /* Integrate stiffness matrix */
    err = t6_integrate_stiffness(element_id, D, ke);
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Integrate stiffness matrix using Gauss quadrature */
fem_error_t t6_integrate_stiffness(int element_id,
                                  double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS],
                                  double ke[T6_TOTAL_DOF][T6_TOTAL_DOF])
{
    double B[T6_STRAIN_COMPONENTS][T6_TOTAL_DOF];
    double BT_D[T6_TOTAL_DOF][T6_STRAIN_COMPONENTS];
    double xi, eta, weight, det_J, J[2][2];
    double thickness;
    int material_id;
    int gp, i, j, k;
    fem_error_t err;
    
    /* Initialize stiffness matrix */
    for (i = 0; i < T6_TOTAL_DOF; i++) {
        for (j = 0; j < T6_TOTAL_DOF; j++) {
            ke[i][j] = ZERO;
        }
    }
    
    /* Get material thickness */
    material_id = g_element_material[element_id];
    thickness = g_material_props[material_id][2];
    if (thickness <= ZERO) thickness = ONE; /* Default thickness */
    
    /* Gauss integration loop */
    for (gp = 0; gp < T6_GAUSS_POINTS; gp++) {
        /* Get Gauss point coordinates and weight */
        xi = g_t6_gauss_points[gp][0];
        eta = g_t6_gauss_points[gp][1];
        weight = g_t6_gauss_weights[gp];
        
        /* Calculate B-matrix at this Gauss point */
        err = t6_strain_displacement_matrix(element_id, xi, eta, B);
        CHECK_ERROR(err);

        /* Calculate Jacobian determinant */
        err = t6_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        /* Debug output for first Gauss point */
        if (gp == 0) {
            printf("  Debug: Gauss point 1 (xi=%.3f, eta=%.3f):\n", xi, eta);
            printf("    Jacobian det = %.6e\n", det_J);
            printf("    B-matrix sample: B[0][0]=%.6e, B[1][1]=%.6e, B[2][0]=%.6e\n",
                   B[0][0], B[1][1], B[2][0]);
        }
        
        /* Calculate B^T * D */
        for (i = 0; i < T6_TOTAL_DOF; i++) {
            for (j = 0; j < T6_STRAIN_COMPONENTS; j++) {
                BT_D[i][j] = ZERO;
                for (k = 0; k < T6_STRAIN_COMPONENTS; k++) {
                    BT_D[i][j] += B[k][i] * D[k][j];
                }
            }
        }
        
        /* Calculate (B^T * D) * B and add to stiffness matrix */
        double integration_factor = weight * det_J * thickness;
        
        for (i = 0; i < T6_TOTAL_DOF; i++) {
            for (j = 0; j < T6_TOTAL_DOF; j++) {
                for (k = 0; k < T6_STRAIN_COMPONENTS; k++) {
                    ke[i][j] += BT_D[i][k] * B[k][j] * integration_factor;
                }
            }
        }
    }
    
    return FEM_SUCCESS;
}

/* Calculate stress at element centroid */
fem_error_t t6_calculate_element_stress(int element_id, 
                                       double stress[T6_STRESS_COMPONENTS])
{
    double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS];
    double B[T6_STRAIN_COMPONENTS][T6_TOTAL_DOF];
    double displ[T6_TOTAL_DOF];
    double strain[T6_STRAIN_COMPONENTS];
    double xi, eta;
    int material_id;
    double E, nu;
    int i, j;
    fem_error_t err;
    
    /* Validate element */
    err = t6_validate_element(element_id);
    CHECK_ERROR(err);
    
    /* Get material properties */
    material_id = g_element_material[element_id];
    E = g_material_props[material_id][0];
    nu = g_material_props[material_id][1];
    
    /* Calculate material matrix */
    if (g_material_type[material_id] == MATERIAL_PLANE_STRESS) {
        err = t6_material_matrix_plane_stress(E, nu, D);
    } else {
        err = t6_material_matrix_plane_strain(E, nu, D);
    }
    CHECK_ERROR(err);
    
    /* Get element displacements */
    err = t6_get_element_displacements(element_id, displ);
    CHECK_ERROR(err);
    
    /* Calculate stress at element centroid (1/3, 1/3) */
    xi = THIRD;
    eta = THIRD;
    
    /* Calculate B-matrix at centroid */
    err = t6_strain_displacement_matrix(element_id, xi, eta, B);
    CHECK_ERROR(err);
    
    /* Calculate strain: {strain} = [B] * {displacement} */
    for (i = 0; i < T6_STRAIN_COMPONENTS; i++) {
        strain[i] = ZERO;
        for (j = 0; j < T6_TOTAL_DOF; j++) {
            strain[i] += B[i][j] * displ[j];
        }
    }
    
    /* Calculate stress: {stress} = [D] * {strain} */
    for (i = 0; i < T6_STRESS_COMPONENTS; i++) {
        stress[i] = ZERO;
        for (j = 0; j < T6_STRAIN_COMPONENTS; j++) {
            stress[i] += D[i][j] * strain[j];
        }
    }
    
    return FEM_SUCCESS;
}
