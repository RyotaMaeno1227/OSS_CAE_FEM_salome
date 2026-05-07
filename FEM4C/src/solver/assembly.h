#ifndef ASSEMBLY_H
#define ASSEMBLY_H

/* FEM4C - Global matrix assembly functions
 * Assembly of global stiffness matrix and force vector
 */

#include "../common/types.h"
#include "../common/constants.h"
#include "../domain/fem/element/t3/t3_element.h"
#include "../domain/fem/element/q4/q4_element.h"

/* Assembly functions */
fem_error_t assembly_global_stiffness_matrix(void);
fem_error_t assembly_global_force_vector(void);
fem_error_t assembly_clear_global_arrays(void);
fem_error_t assembly_build_lumped_mass_diagonal(double *mass_diag);
fem_error_t assembly_add_lumped_mass_scaled_to_stiffness(double mass_scale);
fem_error_t assembly_add_lumped_mass_times_vector_to_force(const double *vector,
                                                          double scale);

/* Element assembly functions */
fem_error_t assembly_add_element_stiffness(int element_id,
                                          double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF]);
fem_error_t assembly_add_element_stiffness_t3(int element_id,
                                             double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF]);
fem_error_t assembly_add_element_stiffness_q4(int element_id,
                                             double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF]);
fem_error_t assembly_add_element_force(int element_id,
                                      double fe[MAX_ELEMENT_DOF]);

/* DOF mapping functions */
fem_error_t assembly_get_element_dof_map(int element_id, int dof_map[MAX_ELEMENT_DOF]);
fem_error_t assembly_get_global_dof_index(int node_id, int local_dof);

/* Utility functions */
fem_error_t assembly_apply_boundary_conditions(void);
fem_error_t assembly_check_matrix_properties(void);

/* OpenMP parallel assembly */
fem_error_t assembly_parallel_stiffness_matrix(void);

#endif /* ASSEMBLY_H */
