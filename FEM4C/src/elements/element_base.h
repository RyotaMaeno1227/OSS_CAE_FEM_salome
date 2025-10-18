#ifndef ELEMENT_BASE_H
#define ELEMENT_BASE_H

/* FEM4C - Element Base Interface
 * Common interface for all element types
 */

#include "../common/constants.h"
#include "../common/types.h"

/* Element interface function pointer types */
typedef fem_error_t (*element_init_func_t)(void);
typedef fem_error_t (*element_shape_func_t)(double xi, double eta, double zeta,
                                           double N[], double dN_dxi[],
                                           double dN_deta[], double dN_dzeta[]);
typedef fem_error_t (*element_jacobian_func_t)(int element_id, double xi, double eta, double zeta,
                                              double J[3][3], double *det_J);
typedef fem_error_t (*element_stiffness_func_t)(int element_id, double ke[][MAX_DOF_PER_NODE * MAX_NODES_PER_ELEMENT]);
typedef fem_error_t (*element_stress_func_t)(int element_id, double stress[]);
typedef fem_error_t (*element_validate_func_t)(int element_id);

/* Element properties structure */
typedef struct {
    int element_type;               /* Element type ID */
    int nodes_per_element;          /* Number of nodes per element */
    int dof_per_node;              /* DOF per node for this element type */
    int total_dof;                 /* Total DOF for element */
    int gauss_points;              /* Number of Gauss integration points */
    int spatial_dimension;         /* 1D, 2D, or 3D */
    int strain_components;         /* Number of strain components */
    int stress_components;         /* Number of stress components */
    char name[16];                 /* Element name (T3, Q4, T6, etc.) */

    /* Function pointers for element-specific operations */
    element_init_func_t init;
    element_shape_func_t shape_functions;
    element_jacobian_func_t jacobian;
    element_stiffness_func_t stiffness;
    element_stress_func_t stress;
    element_validate_func_t validate;
} element_properties_t;

/* Element registry - array of supported element types */
extern element_properties_t g_element_registry[10];  /* Support up to 10 element types */
extern int g_num_registered_elements;

/* Element interface functions */
fem_error_t element_base_initialize(void);
fem_error_t element_register_type(const element_properties_t *properties);
fem_error_t element_get_properties(int element_type, element_properties_t **properties);
fem_error_t element_is_supported(int element_type);

/* Generic element operations using function pointers */
fem_error_t element_compute_stiffness(int element_id, double ke[][MAX_DOF_PER_NODE * MAX_NODES_PER_ELEMENT]);
fem_error_t element_compute_stress(int element_id, double stress[]);
fem_error_t element_validate(int element_id);

/* Element utility functions */
fem_error_t element_get_coordinates(int element_id, double coords[][3]);
fem_error_t element_get_displacements(int element_id, double displ[]);
fem_error_t element_get_material_matrix(int element_id, double D[][6]);

/* Element type determination from node count */
int element_determine_type_from_nodes(int num_nodes);

/* 2D element specific functions */
fem_error_t element_2d_material_matrix_plane_stress(int material_id, double D[3][3]);
fem_error_t element_2d_material_matrix_plane_strain(int material_id, double D[3][3]);

/* Common Gauss integration point storage */
typedef struct {
    int num_points;
    gauss_point_t points[MAX_GAUSS_POINTS];
} gauss_integration_t;

/* Get Gauss points for different element types */
fem_error_t element_get_gauss_points_2d_triangle(int order, gauss_integration_t *gauss);
fem_error_t element_get_gauss_points_2d_quad(int order, gauss_integration_t *gauss);
fem_error_t element_get_gauss_points_3d_tetrahedron(int order, gauss_integration_t *gauss);
fem_error_t element_get_gauss_points_3d_hexahedron(int order, gauss_integration_t *gauss);

#endif /* ELEMENT_BASE_H */