#ifndef T3_ELEMENT_H
#define T3_ELEMENT_H

/* FEM4C - T3 Element Implementation
 * 3-node triangular element with linear shape functions
 */

#include "../../common/constants.h"
#include "../../common/types.h"

/* T3 element specific constants */
#define T3_NODES_PER_ELEMENT    3
#define T3_DOF_PER_NODE         2
#define T3_TOTAL_DOF            6
#define T3_GAUSS_POINTS         1
#define T3_STRAIN_COMPONENTS    3
#define T3_STRESS_COMPONENTS    3

/* T3 element functions */
fem_error_t t3_initialize(void);
fem_error_t t3_register(void);

/* Shape function evaluation */
fem_error_t t3_shape_functions(double xi, double eta, double N[T3_NODES_PER_ELEMENT]);
fem_error_t t3_shape_derivatives_natural(double xi, double eta,
                                         double dN_dxi[T3_NODES_PER_ELEMENT],
                                         double dN_deta[T3_NODES_PER_ELEMENT]);

/* Jacobian and coordinate transformation */
fem_error_t t3_jacobian_matrix(int element_id, double xi, double eta,
                              double J[2][2], double *det_J);
fem_error_t t3_shape_derivatives_global(int element_id, double xi, double eta,
                                       double dN_dx[T3_NODES_PER_ELEMENT],
                                       double dN_dy[T3_NODES_PER_ELEMENT]);

/* B-matrix (strain-displacement) calculation */
fem_error_t t3_strain_displacement_matrix(int element_id, double xi, double eta,
                                         double B[T3_STRAIN_COMPONENTS][T3_TOTAL_DOF]);

/* Element stiffness matrix */
fem_error_t t3_element_stiffness(int element_id,
                                double ke[T3_TOTAL_DOF][T3_TOTAL_DOF]);

/* Element stress calculation */
fem_error_t t3_element_stress(int element_id,
                             double stress[T3_STRESS_COMPONENTS]);

/* Utility functions */
fem_error_t t3_get_element_coordinates(int element_id,
                                      double coords[T3_NODES_PER_ELEMENT][2]);
fem_error_t t3_get_element_displacements(int element_id,
                                        double displ[T3_TOTAL_DOF]);

/* Validation functions */
fem_error_t t3_validate_element(int element_id);

#endif /* T3_ELEMENT_H */