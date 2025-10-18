#ifndef ELEMENTS_H
#define ELEMENTS_H

/* FEM4C - Element Management and Registry
 * Element type determination and management functions
 */

#include "element_base.h"
#include "../common/constants.h"
#include "../common/types.h"

/* Element manager functions */
fem_error_t elements_initialize(void);
fem_error_t elements_finalize(void);
fem_error_t elements_register_all_types(void);

/* Element type determination */
fem_error_t elements_determine_type(int element_id, int *element_type);
fem_error_t elements_auto_detect_type(int element_id);
fem_error_t elements_validate_element_data(int element_id);

/* Element information functions */
fem_error_t elements_get_info(int element_type, element_properties_t **info);
fem_error_t elements_get_dof_count(int element_type, int *total_dof);
fem_error_t elements_get_node_count(int element_type, int *node_count);
fem_error_t elements_get_dimension(int element_type, int *dimension);

/* Element compatibility checks */
fem_error_t elements_check_material_compatibility(int element_id);
fem_error_t elements_check_node_connectivity(int element_id);
fem_error_t elements_check_geometric_validity(int element_id);

/* Element group operations */
fem_error_t elements_group_by_type(int **type_groups, int *group_counts, int *num_groups);
fem_error_t elements_count_by_type(int element_type, int *count);

/* Element utility functions */
fem_error_t elements_print_registry(void);
fem_error_t elements_print_summary(void);

/* Forward declarations for element-specific initialization */
fem_error_t t3_register(void);
fem_error_t q4_register(void);
fem_error_t t6_register(void);
fem_error_t q8_register(void);
fem_error_t q9_register(void);

#endif /* ELEMENTS_H */