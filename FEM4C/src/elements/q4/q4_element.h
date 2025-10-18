#ifndef Q4_ELEMENT_H
#define Q4_ELEMENT_H

/* FEM4C - Q4 Element Implementation
 * 4-node quadrilateral element with linear shape functions
 */

#include "../../common/constants.h"
#include "../../common/types.h"

/* Q4 element specific constants */
#define Q4_NODES_PER_ELEMENT    4
#define Q4_DOF_PER_NODE         2
#define Q4_TOTAL_DOF            8
#define Q4_GAUSS_POINTS         4
#define Q4_STRAIN_COMPONENTS    3
#define Q4_STRESS_COMPONENTS    3

/* Gauss integration point data for Q4 quadrilateral */
extern double g_q4_gauss_points[Q4_GAUSS_POINTS][2];  /* xi, eta coordinates */
extern double g_q4_gauss_weights[Q4_GAUSS_POINTS];    /* integration weights */

/* Q4 element functions */
fem_error_t q4_initialize(void);
fem_error_t q4_register(void);

/* Shape function evaluation */
fem_error_t q4_shape_functions(double xi, double eta, double N[Q4_NODES_PER_ELEMENT]);
fem_error_t q4_shape_derivatives_natural(double xi, double eta,
                                         double dN_dxi[Q4_NODES_PER_ELEMENT],
                                         double dN_deta[Q4_NODES_PER_ELEMENT]);

/* Jacobian and coordinate transformation */
fem_error_t q4_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J);
fem_error_t q4_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[Q4_NODES_PER_ELEMENT],
                                       double dN_dy[Q4_NODES_PER_ELEMENT]);

/* B-matrix (strain-displacement) calculation */
fem_error_t q4_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[Q4_STRAIN_COMPONENTS][Q4_TOTAL_DOF]);

/* Element stiffness matrix */
fem_error_t q4_element_stiffness(int element_id,
                                double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF]);

/* Element stress calculation */
fem_error_t q4_element_stress(int element_id,
                             double stress[Q4_STRESS_COMPONENTS]);

/* Material matrix for plane stress/strain */
fem_error_t q4_material_matrix(int material_id,
                              double D[Q4_STRESS_COMPONENTS][Q4_STRAIN_COMPONENTS]);

/* Utility functions */
fem_error_t q4_get_element_coordinates(int element_id,
                                      double coords[Q4_NODES_PER_ELEMENT][2]);
fem_error_t q4_get_element_displacements(int element_id,
                                        double displ[Q4_TOTAL_DOF]);

/* Validation functions */
fem_error_t q4_validate_element(int element_id);
fem_error_t q4_check_element_geometry(int element_id);

#endif /* Q4_ELEMENT_H */