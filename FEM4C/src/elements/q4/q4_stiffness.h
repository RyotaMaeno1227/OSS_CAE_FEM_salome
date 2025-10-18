#ifndef Q4_STIFFNESS_H
#define Q4_STIFFNESS_H

/* FEM4C - Q4 Element Stiffness Matrix Computation
 * 4-node quadrilateral element stiffness matrix calculation
 */

#include "q4_element.h"

/* Q4 element stiffness matrix computation */
fem_error_t q4_element_stiffness(int element_id,
                                double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF]);

/* Internal functions for stiffness computation */
fem_error_t q4_integrate_stiffness(int element_id,
                                  double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF]);

fem_error_t q4_stiffness_integrand(int element_id, double xi, double eta,
                                  double integrand[Q4_TOTAL_DOF][Q4_TOTAL_DOF]);

#endif /* Q4_STIFFNESS_H */