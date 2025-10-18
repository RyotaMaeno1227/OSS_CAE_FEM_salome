/* FEM4C - Element Management and Registry Implementation
 * Element type determination and management functions
 */

#include "elements.h"
#include "../common/error.h"
#include "../common/globals.h"
#include <stdio.h>
#include <math.h>

/* Initialize element management system */
fem_error_t elements_initialize(void)
{
    fem_error_t error;

    /* Initialize element base system */
    error = element_base_initialize();
    if (error != FEM_SUCCESS) return error;

    /* Register all supported element types */
    error = elements_register_all_types();
    if (error != FEM_SUCCESS) return error;

    return FEM_SUCCESS;
}

/* Finalize element management system */
fem_error_t elements_finalize(void)
{
    /* Currently no cleanup needed */
    return FEM_SUCCESS;
}

/* Register all supported element types */
fem_error_t elements_register_all_types(void)
{
    fem_error_t error;

    /* Register T6 element (already implemented) */
    error = t6_register();
    if (error != FEM_SUCCESS) {
        error_set(error, "elements_register_all_types", "Failed to register T6 element");
        return error;
    }

    /* Register other elements as they are implemented */
    /* Note: These will return success for now, actual implementation will be added later */

    /* Register T3 element */
    error = t3_register();
    if (error != FEM_SUCCESS) {
        error_set(error, "elements_register_all_types", "Failed to register T3 element");
        return error;
    }

    /* Register Q4 element */
    error = q4_register();
    if (error != FEM_SUCCESS) {
        error_set(error, "elements_register_all_types", "Failed to register Q4 element");
        return error;
    }

    return FEM_SUCCESS;
}

/* Determine element type for a specific element */
fem_error_t elements_determine_type(int element_id, int *element_type)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_determine_type",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    if (!element_type) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_determine_type",
                     "Element type pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Get element type from global element array */
    *element_type = g_element_type[element_id];

    /* If type is not set, try to auto-detect */
    if (*element_type == 0) {
        fem_error_t error = elements_auto_detect_type(element_id);
        if (error != FEM_SUCCESS) return error;
        *element_type = g_element_type[element_id];
    }

    return FEM_SUCCESS;
}

/* Auto-detect element type based on node count */
fem_error_t elements_auto_detect_type(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_auto_detect_type",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Number of nodes per element needs to be determined from element connectivity */
    int num_nodes = 0;
    for (int i = 0; i < MAX_NODES_PER_ELEMENT; i++) {
        if (g_element_nodes[element_id][i] > 0) {
            num_nodes++;
        } else {
            break;
        }
    }
    int detected_type = element_determine_type_from_nodes(num_nodes);

    if (detected_type == -1) {
        error_set(FEM_ERROR_INVALID_ELEMENT_TYPE, "elements_auto_detect_type",
                     "Cannot determine element type from node count");
        return FEM_ERROR_INVALID_ELEMENT_TYPE;
    }

    /* Check if detected type is supported */
    fem_error_t error = element_is_supported(detected_type);
    if (error != FEM_SUCCESS) {
        error_set(FEM_ERROR_INVALID_ELEMENT_TYPE, "elements_auto_detect_type",
                     "Detected element type is not supported");
        return FEM_ERROR_INVALID_ELEMENT_TYPE;
    }

    /* Set the detected type */
    g_element_type[element_id] = detected_type;

    return FEM_SUCCESS;
}

/* Validate element data */
fem_error_t elements_validate_element_data(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_validate_element_data",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    fem_error_t error;

    /* Check material compatibility */
    error = elements_check_material_compatibility(element_id);
    if (error != FEM_SUCCESS) return error;

    /* Check node connectivity */
    error = elements_check_node_connectivity(element_id);
    if (error != FEM_SUCCESS) return error;

    /* Check geometric validity */
    error = elements_check_geometric_validity(element_id);
    if (error != FEM_SUCCESS) return error;

    return FEM_SUCCESS;
}

/* Get element information */
fem_error_t elements_get_info(int element_type, element_properties_t **info)
{
    return element_get_properties(element_type, info);
}

/* Get DOF count for element type */
fem_error_t elements_get_dof_count(int element_type, int *total_dof)
{
    if (!total_dof) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_get_dof_count",
                     "DOF count pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    *total_dof = properties->total_dof;
    return FEM_SUCCESS;
}

/* Get node count for element type */
fem_error_t elements_get_node_count(int element_type, int *node_count)
{
    if (!node_count) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_get_node_count",
                     "Node count pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    *node_count = properties->nodes_per_element;
    return FEM_SUCCESS;
}

/* Get spatial dimension for element type */
fem_error_t elements_get_dimension(int element_type, int *dimension)
{
    if (!dimension) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_get_dimension",
                     "Dimension pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    *dimension = properties->spatial_dimension;
    return FEM_SUCCESS;
}

/* Check material compatibility */
fem_error_t elements_check_material_compatibility(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_check_material_compatibility",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    int material_id = g_element_material[element_id];
    if (material_id < 0 || material_id >= g_num_materials) {
        error_set(FEM_ERROR_INVALID_MATERIAL, "elements_check_material_compatibility",
                     "Invalid material ID for element");
        return FEM_ERROR_INVALID_MATERIAL;
    }

    /* Additional compatibility checks can be added here */
    return FEM_SUCCESS;
}

/* Check node connectivity */
fem_error_t elements_check_node_connectivity(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_check_node_connectivity",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Number of nodes per element needs to be determined from element connectivity */
    int num_nodes = 0;
    for (int i = 0; i < MAX_NODES_PER_ELEMENT; i++) {
        if (g_element_nodes[element_id][i] > 0) {
            num_nodes++;
        } else {
            break;
        }
    }
    for (int i = 0; i < num_nodes; i++) {
        int node_id = g_element_nodes[element_id][i];
        if (node_id < 0 || node_id >= g_num_nodes) {
            error_set(FEM_ERROR_INVALID_NODE, "elements_check_node_connectivity",
                         "Invalid node ID in element connectivity");
            return FEM_ERROR_INVALID_NODE;
        }
    }

    return FEM_SUCCESS;
}

/* Check geometric validity */
fem_error_t elements_check_geometric_validity(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_check_geometric_validity",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Basic geometric checks - can be enhanced later */
    int element_type = g_element_type[element_id];

    /* For now, just use the element's validate function if available */
    return element_validate(element_id);
}

/* Count elements by type */
fem_error_t elements_count_by_type(int element_type, int *count)
{
    if (!count) {
        error_set(FEM_ERROR_INVALID_INPUT, "elements_count_by_type",
                     "Count pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    *count = 0;
    for (int i = 0; i < g_num_elements; i++) {
        if (g_element_type[i] == element_type) {
            (*count)++;
        }
    }

    return FEM_SUCCESS;
}

/* Print element registry information */
fem_error_t elements_print_registry(void)
{
    printf("\n=== Element Registry ===\n");
    printf("Number of registered element types: %d\n", g_num_registered_elements);

    for (int i = 0; i < g_num_registered_elements; i++) {
        element_properties_t *props = &g_element_registry[i];
        printf("Type %d: %s - %dD, %d nodes, %d DOF\n",
               props->element_type, props->name,
               props->spatial_dimension, props->nodes_per_element, props->total_dof);
    }

    return FEM_SUCCESS;
}

/* Print element summary */
fem_error_t elements_print_summary(void)
{
    printf("\n=== Element Summary ===\n");
    printf("Total elements: %d\n", g_num_elements);

    /* Count elements by type */
    for (int i = 0; i < g_num_registered_elements; i++) {
        int element_type = g_element_registry[i].element_type;
        int count;
        elements_count_by_type(element_type, &count);
        if (count > 0) {
            printf("%s elements: %d\n", g_element_registry[i].name, count);
        }
    }

    return FEM_SUCCESS;
}

/* Forward declaration for T3 register function */
extern fem_error_t t3_register(void);

/* Forward declaration for Q4 register function */
extern fem_error_t q4_register(void);

fem_error_t q8_register(void)
{
    /* Q8 element - future implementation */
    return FEM_SUCCESS;
}

fem_error_t q9_register(void)
{
    /* Q9 element - future implementation */
    return FEM_SUCCESS;
}