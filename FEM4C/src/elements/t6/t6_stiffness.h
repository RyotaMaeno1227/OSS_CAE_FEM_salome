#ifndef T6_STIFFNESS_H
#define T6_STIFFNESS_H

/* FEM4C - T6 Element Stiffness Matrix Calculation
 * Functions for calculating T6 element stiffness matrix
 */

#include "t6_element.h"

/* Material matrix calculation */
fem_error_t t6_material_matrix_plane_stress(double E, double nu, 
                                           double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS]);
fem_error_t t6_material_matrix_plane_strain(double E, double nu, 
                                           double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS]);

/* Element stiffness matrix calculation */
fem_error_t t6_element_stiffness_matrix(int element_id, 
                                       double ke[T6_TOTAL_DOF][T6_TOTAL_DOF]);

/* Integration utilities */
fem_error_t t6_integrate_stiffness(int element_id,
                                  double D[T6_STRESS_COMPONENTS][T6_STRAIN_COMPONENTS],
                                  double ke[T6_TOTAL_DOF][T6_TOTAL_DOF]);

/* Stress calculation */
fem_error_t t6_calculate_element_stress(int element_id, 
                                       double stress[T6_STRESS_COMPONENTS]);

#endif /* T6_STIFFNESS_H */