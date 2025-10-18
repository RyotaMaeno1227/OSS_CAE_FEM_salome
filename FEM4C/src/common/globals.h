#ifndef GLOBALS_H
#define GLOBALS_H

/* FEM4C - High Performance Finite Element Method in C
 * Global variable declarations
 */

#include "constants.h"
#include "types.h"

/* Global arrays - dynamically sized */
extern double (*g_node_coords)[3];           /* Node coordinates */
extern double (*g_node_displ)[3];            /* Node displacements */
extern double (*g_node_force)[3];            /* Node external forces */
extern int (*g_node_bc_flags)[3];            /* Boundary condition flags */
extern int *g_node_ids;                      /* Original node identifiers */
extern int *g_node_id_to_index;              /* Mapping from original ID to internal index */

extern int (*g_element_nodes)[MAX_NODES_PER_ELEMENT]; /* Element connectivity */
extern int *g_element_type;                   /* Element types */
extern int *g_element_material;               /* Element material IDs */
extern int *g_element_ids;                    /* Original element identifiers */
extern int *g_element_id_to_index;            /* Mapping from original ID to internal index */

extern double (*g_material_props)[6];         /* Material properties */
                                              /* [0]: Young's modulus */
                                              /* [1]: Poisson's ratio */
                                              /* [2]: thickness */
                                              /* [3]: density */
                                              /* [4]: reserved */
                                              /* [5]: reserved */

extern int *g_material_type;                 /* Material types */
extern int *g_material_ids;                  /* Original material identifiers */
extern int *g_material_id_to_index;          /* Mapping from material ID to index */

extern int g_node_capacity;
extern int g_element_capacity;
extern int g_material_capacity;
extern int g_node_id_capacity;
extern int g_element_id_capacity;
extern int g_material_id_capacity;

/* Global system arrays */
extern double *g_global_force;
extern double *g_global_displ;

/* Skyline global stiffness storage */
extern double *g_global_stiffness_values;
extern int *g_stiffness_profile;
extern int *g_stiffness_offsets;
extern int g_stiffness_value_count;
extern int g_stiffness_bandwidth;

/* Distributed load control */
extern double g_body_force[3];                       /* Uniform body force per unit volume */
extern double g_pressure_value;                      /* Uniform pressure value (if applicable) */
extern int g_has_body_force;
extern int g_has_pressure;
extern int g_num_tractions;
extern int g_traction_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];
extern double g_traction_values[MAX_TRACTION_SURFACES][3];
extern int g_num_pressure_surfaces;
extern int g_pressure_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];

/* Analysis control variables */
extern analysis_control_t g_analysis;
extern solver_info_t g_solver_info;

/* Problem size variables */
extern int g_num_nodes;
extern int g_num_elements;
extern int g_num_materials;
extern int g_total_dof;

/* File handles */
extern char g_input_filename[MAX_FILENAME_LEN];
extern char g_output_filename[MAX_FILENAME_LEN];

/* OpenMP control */
extern int g_num_threads;

/* Functions to initialize global variables */
fem_error_t globals_initialize(void);
fem_error_t globals_finalize(void);
void globals_reset(void);
fem_error_t globals_allocate_system_arrays(int total_dof);
void globals_free_system_arrays(void);
fem_error_t globals_reserve_nodes(int required);
fem_error_t globals_reserve_elements(int required);
fem_error_t globals_reserve_materials(int required);
fem_error_t globals_reserve_node_ids(int max_id_plus_one);
fem_error_t globals_reserve_element_ids(int max_id_plus_one);
fem_error_t globals_reserve_material_ids(int max_id_plus_one);
void globals_initialize_node_entry(int node_index);
void globals_initialize_element_entry(int element_index);
void globals_initialize_material_entry(int material_index);

#endif /* GLOBALS_H */
