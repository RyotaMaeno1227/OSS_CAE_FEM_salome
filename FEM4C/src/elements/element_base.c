/* FEM4C - Element Base Implementation
 * Common functionality for all element types
 */

#include "element_base.h"
#include "../common/error.h"
#include "../common/globals.h"
#include <string.h>
#include <math.h>

/* Element registry */
element_properties_t g_element_registry[10];
int g_num_registered_elements = 0;

/* Initialize element base system */
fem_error_t element_base_initialize(void)
{
    g_num_registered_elements = 0;
    memset(g_element_registry, 0, sizeof(g_element_registry));

    return FEM_SUCCESS;
}

/* Register a new element type */
fem_error_t element_register_type(const element_properties_t *properties)
{
    if (!properties) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_register_type",
                     "Properties pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    if (g_num_registered_elements >= 10) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_register_type",
                     "Maximum number of element types exceeded");
        return FEM_ERROR_INVALID_INPUT;
    }

    /* Check if element type already registered */
    for (int i = 0; i < g_num_registered_elements; i++) {
        if (g_element_registry[i].element_type == properties->element_type) {
            error_set(FEM_ERROR_INVALID_INPUT, "element_register_type",
                         "Element type already registered");
            return FEM_ERROR_INVALID_INPUT;
        }
    }

    /* Copy properties to registry */
    g_element_registry[g_num_registered_elements] = *properties;
    g_num_registered_elements++;

    return FEM_SUCCESS;
}

/* Get properties for an element type */
fem_error_t element_get_properties(int element_type, element_properties_t **properties)
{
    if (!properties) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_get_properties",
                     "Properties pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    for (int i = 0; i < g_num_registered_elements; i++) {
        if (g_element_registry[i].element_type == element_type) {
            *properties = &g_element_registry[i];
            return FEM_SUCCESS;
        }
    }

    error_set(FEM_ERROR_INVALID_ELEMENT_TYPE, "element_get_properties",
                 "Element type not found in registry");
    return FEM_ERROR_INVALID_ELEMENT_TYPE;
}

/* Check if element type is supported */
fem_error_t element_is_supported(int element_type)
{
    element_properties_t *properties;
    return element_get_properties(element_type, &properties);
}

/* Compute element stiffness matrix using registered function */
fem_error_t element_compute_stiffness(int element_id, double ke[][MAX_DOF_PER_NODE * MAX_NODES_PER_ELEMENT])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_compute_stiffness",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    int element_type = g_element_type[element_id];
    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    if (!properties->stiffness) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_compute_stiffness",
                     "Stiffness function not implemented for this element type");
        return FEM_ERROR_INVALID_INPUT;
    }

    return properties->stiffness(element_id, ke);
}

/* Compute element stress using registered function */
fem_error_t element_compute_stress(int element_id, double stress[])
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_compute_stress",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    int element_type = g_element_type[element_id];
    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    if (!properties->stress) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_compute_stress",
                     "Stress function not implemented for this element type");
        return FEM_ERROR_INVALID_INPUT;
    }

    return properties->stress(element_id, stress);
}

/* Validate element using registered function */
fem_error_t element_validate(int element_id)
{
    if (element_id < 0 || element_id >= g_num_elements) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_validate",
                     "Invalid element ID");
        return FEM_ERROR_INVALID_INPUT;
    }

    int element_type = g_element_type[element_id];
    element_properties_t *properties;
    fem_error_t error = element_get_properties(element_type, &properties);
    if (error != FEM_SUCCESS) return error;

    if (!properties->validate) {
        return FEM_SUCCESS;  /* Validation is optional */
    }

    return properties->validate(element_id);
}

/* Determine element type from number of nodes */
int element_determine_type_from_nodes(int num_nodes)
{
    switch (num_nodes) {
        case 3:  return ELEMENT_T3;
        case 4:  return ELEMENT_Q4;
        case 6:  return ELEMENT_T6;
        case 8:  return ELEMENT_Q8;
        case 9:  return ELEMENT_Q9;
        case 10: return ELEMENT_T10;
        case 20: return ELEMENT_H8;
        default: return -1;  /* Unknown element type */
    }
}

/* 2D plane stress material matrix */
fem_error_t element_2d_material_matrix_plane_stress(int material_id, double D[3][3])
{
    if (material_id < 0 || material_id >= g_num_materials) {
        error_set(FEM_ERROR_INVALID_MATERIAL, "element_2d_material_matrix_plane_stress",
                     "Invalid material ID");
        return FEM_ERROR_INVALID_MATERIAL;
    }

    double E = g_material_props[material_id][0];   /* Young's modulus */
    double nu = g_material_props[material_id][1];  /* Poisson's ratio */
    double factor = E / (1.0 - nu * nu);

    /* Initialize matrix to zero */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            D[i][j] = 0.0;
        }
    }

    /* Plane stress material matrix */
    D[0][0] = factor;
    D[0][1] = factor * nu;
    D[1][0] = factor * nu;
    D[1][1] = factor;
    D[2][2] = factor * (1.0 - nu) / 2.0;

    return FEM_SUCCESS;
}

/* 2D plane strain material matrix */
fem_error_t element_2d_material_matrix_plane_strain(int material_id, double D[3][3])
{
    if (material_id < 0 || material_id >= g_num_materials) {
        error_set(FEM_ERROR_INVALID_MATERIAL, "element_2d_material_matrix_plane_strain",
                     "Invalid material ID");
        return FEM_ERROR_INVALID_MATERIAL;
    }

    double E = g_material_props[material_id][0];   /* Young's modulus */
    double nu = g_material_props[material_id][1];  /* Poisson's ratio */
    double factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));

    /* Initialize matrix to zero */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            D[i][j] = 0.0;
        }
    }

    /* Plane strain material matrix */
    D[0][0] = factor * (1.0 - nu);
    D[0][1] = factor * nu;
    D[1][0] = factor * nu;
    D[1][1] = factor * (1.0 - nu);
    D[2][2] = factor * (1.0 - 2.0 * nu) / 2.0;

    return FEM_SUCCESS;
}

/* Get Gauss points for 2D triangle elements */
fem_error_t element_get_gauss_points_2d_triangle(int order, gauss_integration_t *gauss)
{
    if (!gauss) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_2d_triangle",
                     "Gauss integration pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    if (order == 1) {
        /* 1-point Gauss integration for triangle */
        gauss->num_points = 1;
        gauss->points[0].xi = THIRD;
        gauss->points[0].eta = THIRD;
        gauss->points[0].zeta = 0.0;
        gauss->points[0].weight = 0.5;  /* Area of reference triangle */
    } else if (order == 2 || order == 3) {
        /* 3-point Gauss integration for triangle */
        gauss->num_points = 3;
        gauss->points[0].xi = SIXTH;
        gauss->points[0].eta = SIXTH;
        gauss->points[0].zeta = 0.0;
        gauss->points[0].weight = SIXTH;

        gauss->points[1].xi = 2.0/3.0;
        gauss->points[1].eta = SIXTH;
        gauss->points[1].zeta = 0.0;
        gauss->points[1].weight = SIXTH;

        gauss->points[2].xi = SIXTH;
        gauss->points[2].eta = 2.0/3.0;
        gauss->points[2].zeta = 0.0;
        gauss->points[2].weight = SIXTH;
    } else {
        error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_2d_triangle",
                     "Unsupported Gauss integration order");
        return FEM_ERROR_INVALID_INPUT;
    }

    return FEM_SUCCESS;
}

/* Get Gauss points for 2D quadrilateral elements */
fem_error_t element_get_gauss_points_2d_quad(int order, gauss_integration_t *gauss)
{
    if (!gauss) {
        error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_2d_quad",
                     "Gauss integration pointer is NULL");
        return FEM_ERROR_INVALID_INPUT;
    }

    if (order == 2) {
        /* 2x2 Gauss integration for quadrilateral */
        gauss->num_points = 4;
        double coord = 1.0/sqrt(3.0);

        gauss->points[0].xi = -coord;
        gauss->points[0].eta = -coord;
        gauss->points[0].zeta = 0.0;
        gauss->points[0].weight = 1.0;

        gauss->points[1].xi = coord;
        gauss->points[1].eta = -coord;
        gauss->points[1].zeta = 0.0;
        gauss->points[1].weight = 1.0;

        gauss->points[2].xi = coord;
        gauss->points[2].eta = coord;
        gauss->points[2].zeta = 0.0;
        gauss->points[2].weight = 1.0;

        gauss->points[3].xi = -coord;
        gauss->points[3].eta = coord;
        gauss->points[3].zeta = 0.0;
        gauss->points[3].weight = 1.0;
    } else if (order == 3) {
        /* 3x3 Gauss integration for quadrilateral */
        gauss->num_points = 9;
        double coords[3] = {-sqrt(0.6), 0.0, sqrt(0.6)};
        double weights[3] = {5.0/9.0, 8.0/9.0, 5.0/9.0};

        int point = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                gauss->points[point].xi = coords[i];
                gauss->points[point].eta = coords[j];
                gauss->points[point].zeta = 0.0;
                gauss->points[point].weight = weights[i] * weights[j];
                point++;
            }
        }
    } else {
        error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_2d_quad",
                     "Unsupported Gauss integration order");
        return FEM_ERROR_INVALID_INPUT;
    }

    return FEM_SUCCESS;
}

/* Placeholder for 3D functions - to be implemented later */
fem_error_t element_get_gauss_points_3d_tetrahedron(int order, gauss_integration_t *gauss)
{
    (void)order;
    (void)gauss;
    error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_3d_tetrahedron",
                 "3D elements not implemented yet");
    return FEM_ERROR_INVALID_INPUT;
}

fem_error_t element_get_gauss_points_3d_hexahedron(int order, gauss_integration_t *gauss)
{
    (void)order;
    (void)gauss;
    error_set(FEM_ERROR_INVALID_INPUT, "element_get_gauss_points_3d_hexahedron",
                 "3D elements not implemented yet");
    return FEM_ERROR_INVALID_INPUT;
}