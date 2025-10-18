#ifndef T6_ELEMENT_H
#define T6_ELEMENT_H

/* FEM4C - T6 Element Implementation
 * 6-node triangular element with quadratic shape functions
 */

#include "../../common/constants.h"
#include "../../common/types.h"

/* T6 element specific constants */
#define T6_GAUSS_POINTS 3
#define T6_STRAIN_COMPONENTS 3
#define T6_STRESS_COMPONENTS 3

/* Gauss integration point data for T6 triangle */
extern double g_t6_gauss_points[T6_GAUSS_POINTS][2];  /* xi, eta coordinates */
extern double g_t6_gauss_weights[T6_GAUSS_POINTS];    /* integration weights */

/* T6 element functions */
fem_error_t t6_initialize(void);
fem_error_t t6_register(void);

/* Shape function evaluation */
fem_error_t t6_shape_functions(double xi, double eta, double N[T6_NODES_PER_ELEMENT]);
fem_error_t t6_shape_derivatives_natural(double xi, double eta, 
                                         double dN_dxi[T6_NODES_PER_ELEMENT], 
                                         double dN_deta[T6_NODES_PER_ELEMENT]);

/* Jacobian and coordinate transformation */
fem_error_t t6_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J);
fem_error_t t6_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[T6_NODES_PER_ELEMENT],
                                       double dN_dy[T6_NODES_PER_ELEMENT]);

/* B-matrix (strain-displacement) calculation */
fem_error_t t6_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[T6_STRAIN_COMPONENTS][T6_TOTAL_DOF]);

/* Element stiffness matrix */
fem_error_t t6_element_stiffness(int element_id, 
                                double ke[T6_TOTAL_DOF][T6_TOTAL_DOF]);

/* Element stress calculation */
fem_error_t t6_element_stress(int element_id, 
                             double stress[T6_STRESS_COMPONENTS]);

/* Material matrix for plane stress/strain */
fem_error_t t6_material_matrix(int material_id, 
                              double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS]);

/* Utility functions */
fem_error_t t6_get_element_coordinates(int element_id, 
                                      double coords[T6_NODES_PER_ELEMENT][2]);
fem_error_t t6_get_element_displacements(int element_id,
                                        double displ[T6_TOTAL_DOF]);

/* Validation functions */
fem_error_t t6_validate_element(int element_id);
fem_error_t t6_check_element_geometry(int element_id);

#endif /* T6_ELEMENT_H */