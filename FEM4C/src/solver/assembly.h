#ifndef ASSEMBLY_H
#define ASSEMBLY_H

/* FEM4C - Global matrix assembly functions
 * Assembly of global stiffness matrix and force vector
 */

#include "../common/types.h"
#include "../common/constants.h"
#include "../elements/t3/t3_element.h"
#include "../elements/q4/q4_element.h"

/* Assembly functions */
fem_error_t assembly_global_stiffness_matrix(void);
fem_error_t assembly_global_force_vector(void);
fem_error_t assembly_clear_global_arrays(void);

/* Element assembly functions */
fem_error_t assembly_add_element_stiffness(int element_id,
                                          double ke[T6_TOTAL_DOF][T6_TOTAL_DOF]);
fem_error_t assembly_add_element_stiffness_t3(int element_id,
                                             double ke[T3_TOTAL_DOF][T3_TOTAL_DOF]);
fem_error_t assembly_add_element_stiffness_q4(int element_id,
                                             double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF]);
fem_error_t assembly_add_element_force(int element_id,
                                      double fe[T6_TOTAL_DOF]);

/* DOF mapping functions */
fem_error_t assembly_get_element_dof_map(int element_id, int dof_map[T6_TOTAL_DOF]);
fem_error_t assembly_get_global_dof_index(int node_id, int local_dof);

/* Utility functions */
fem_error_t assembly_apply_boundary_conditions(void);
fem_error_t assembly_check_matrix_properties(void);

/* OpenMP parallel assembly */
fem_error_t assembly_parallel_stiffness_matrix(void);

#endif /* ASSEMBLY_H */