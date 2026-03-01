/* FEM4C - High Performance Finite Element Method in C
 * Global variable definitions
 */

#include "globals.h"
#include "error.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static fem_error_t globals_resize_nodes(int new_capacity);
static fem_error_t globals_resize_elements(int new_capacity);
static fem_error_t globals_resize_materials(int new_capacity);
static fem_error_t globals_resize_node_id_map(int new_capacity);
static fem_error_t globals_resize_element_id_map(int new_capacity);
static fem_error_t globals_resize_material_id_map(int new_capacity);
static void globals_free_mesh_arrays(void);

/* Global arrays - actual storage (dynamically sized) */
double (*g_node_coords)[3] = NULL;
double (*g_node_displ)[3] = NULL;
double (*g_node_force)[3] = NULL;
int (*g_node_bc_flags)[3] = NULL;
int *g_node_ids = NULL;
int *g_node_id_to_index = NULL;

int (*g_element_nodes)[MAX_NODES_PER_ELEMENT] = NULL;
int *g_element_type = NULL;
int *g_element_material = NULL;
int *g_element_ids = NULL;
int *g_element_id_to_index = NULL;

double (*g_material_props)[6] = NULL;
int *g_material_type = NULL;
int *g_material_ids = NULL;
int *g_material_id_to_index = NULL;

int g_node_capacity = 0;
int g_element_capacity = 0;
int g_material_capacity = 0;
int g_node_id_capacity = 0;
int g_element_id_capacity = 0;
int g_material_id_capacity = 0;

static fem_error_t globals_resize_nodes(int new_capacity)
{
    if (new_capacity <= g_node_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    double (*coords)[3] = realloc(g_node_coords, (size_t)new_capacity * sizeof(*g_node_coords));
    if (!coords) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node coordinates");
    }
    g_node_coords = coords;

    double (*displ)[3] = realloc(g_node_displ, (size_t)new_capacity * sizeof(*g_node_displ));
    if (!displ) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node displacements");
    }
    g_node_displ = displ;

    double (*force)[3] = realloc(g_node_force, (size_t)new_capacity * sizeof(*g_node_force));
    if (!force) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node force array");
    }
    g_node_force = force;

    int (*flags)[3] = realloc(g_node_bc_flags, (size_t)new_capacity * sizeof(*g_node_bc_flags));
    if (!flags) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node boundary flag array");
    }
    g_node_bc_flags = flags;

    int *ids = realloc(g_node_ids, (size_t)new_capacity * sizeof(*g_node_ids));
    if (!ids) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node ID array");
    }
    g_node_ids = ids;

    for (int i = g_node_capacity; i < new_capacity; ++i) {
        globals_initialize_node_entry(i);
    }

    g_node_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t globals_resize_elements(int new_capacity)
{
    if (new_capacity <= g_element_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    int (*nodes)[MAX_NODES_PER_ELEMENT] =
        realloc(g_element_nodes, (size_t)new_capacity * sizeof(*g_element_nodes));
    if (!nodes) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize element connectivity array");
    }
    g_element_nodes = nodes;

    int *types = realloc(g_element_type, (size_t)new_capacity * sizeof(*g_element_type));
    if (!types) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize element type array");
    }
    g_element_type = types;

    int *materials = realloc(g_element_material, (size_t)new_capacity * sizeof(*g_element_material));
    if (!materials) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize element material array");
    }
    g_element_material = materials;

    int *ids = realloc(g_element_ids, (size_t)new_capacity * sizeof(*g_element_ids));
    if (!ids) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize element ID array");
    }
    g_element_ids = ids;

    for (int i = g_element_capacity; i < new_capacity; ++i) {
        globals_initialize_element_entry(i);
    }

    g_element_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t globals_resize_materials(int new_capacity)
{
    if (new_capacity <= g_material_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    double (*props)[6] = realloc(g_material_props, (size_t)new_capacity * sizeof(*g_material_props));
    if (!props) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize material property array");
    }
    g_material_props = props;

    int *types = realloc(g_material_type, (size_t)new_capacity * sizeof(*g_material_type));
    if (!types) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize material type array");
    }
    g_material_type = types;

    int *ids = realloc(g_material_ids, (size_t)new_capacity * sizeof(*g_material_ids));
    if (!ids) {
        globals_free_mesh_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize material ID array");
    }
    g_material_ids = ids;

    for (int i = g_material_capacity; i < new_capacity; ++i) {
        globals_initialize_material_entry(i);
    }

    g_material_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t globals_resize_node_id_map(int new_capacity)
{
    if (new_capacity <= g_node_id_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    int *map = realloc(g_node_id_to_index, (size_t)new_capacity * sizeof(*g_node_id_to_index));
    if (!map) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize node ID map");
    }
    for (int i = g_node_id_capacity; i < new_capacity; ++i) {
        map[i] = -1;
    }
    g_node_id_to_index = map;
    g_node_id_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t globals_resize_element_id_map(int new_capacity)
{
    if (new_capacity <= g_element_id_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    int *map = realloc(g_element_id_to_index, (size_t)new_capacity * sizeof(*g_element_id_to_index));
    if (!map) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize element ID map");
    }
    for (int i = g_element_id_capacity; i < new_capacity; ++i) {
        map[i] = -1;
    }
    g_element_id_to_index = map;
    g_element_id_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t globals_resize_material_id_map(int new_capacity)
{
    if (new_capacity <= g_material_id_capacity) {
        return FEM_SUCCESS;
    }
    if (new_capacity <= 0) {
        return FEM_SUCCESS;
    }

    int *map = realloc(g_material_id_to_index, (size_t)new_capacity * sizeof(*g_material_id_to_index));
    if (!map) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize material ID map");
    }
    for (int i = g_material_id_capacity; i < new_capacity; ++i) {
        map[i] = -1;
    }
    g_material_id_to_index = map;
    g_material_id_capacity = new_capacity;
    return FEM_SUCCESS;
}

static void globals_free_mesh_arrays(void)
{
    free(g_node_coords);
    free(g_node_displ);
    free(g_node_force);
    free(g_node_bc_flags);
    free(g_node_ids);
    free(g_node_id_to_index);

    free(g_element_nodes);
    free(g_element_type);
    free(g_element_material);
    free(g_element_ids);
    free(g_element_id_to_index);

    free(g_material_props);
    free(g_material_type);
    free(g_material_ids);
    free(g_material_id_to_index);

    g_node_coords = NULL;
    g_node_displ = NULL;
    g_node_force = NULL;
    g_node_bc_flags = NULL;
    g_node_ids = NULL;
    g_node_id_to_index = NULL;

    g_element_nodes = NULL;
    g_element_type = NULL;
    g_element_material = NULL;
    g_element_ids = NULL;
    g_element_id_to_index = NULL;

    g_material_props = NULL;
    g_material_type = NULL;
    g_material_ids = NULL;
    g_material_id_to_index = NULL;

    g_node_capacity = 0;
    g_element_capacity = 0;
    g_material_capacity = 0;
    g_node_id_capacity = 0;
    g_element_id_capacity = 0;
    g_material_id_capacity = 0;
}

static int globals_next_capacity(int current, int required, int initial, int block_size)
{
    int new_capacity = current > 0 ? current : initial;
    if (new_capacity <= 0) {
        new_capacity = initial;
    }
    while (new_capacity < required) {
        if (block_size > 0) {
            if (new_capacity > INT_MAX - block_size) {
                new_capacity = required;
                break;
            }
            new_capacity += block_size;
        } else {
            if (new_capacity > INT_MAX / 2) {
                new_capacity = required;
                break;
            }
            new_capacity *= 2;
        }
    }
    if (new_capacity < required) {
        new_capacity = required;
    }
    return new_capacity;
}

fem_error_t globals_reserve_nodes(int required)
{
    if (required <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_node_capacity, required,
                                             INITIAL_NODE_CAPACITY, 0);
    return globals_resize_nodes(new_capacity);
}

fem_error_t globals_reserve_elements(int required)
{
    if (required <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_element_capacity, required,
                                             INITIAL_ELEMENT_CAPACITY, 0);
    return globals_resize_elements(new_capacity);
}

fem_error_t globals_reserve_materials(int required)
{
    if (required <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_material_capacity, required,
                                             INITIAL_MATERIAL_CAPACITY, 0);
    return globals_resize_materials(new_capacity);
}

fem_error_t globals_reserve_node_ids(int max_id_plus_one)
{
    if (max_id_plus_one <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_node_id_capacity, max_id_plus_one,
                                             NODE_ID_BLOCK_SIZE, NODE_ID_BLOCK_SIZE);
    return globals_resize_node_id_map(new_capacity);
}

fem_error_t globals_reserve_element_ids(int max_id_plus_one)
{
    if (max_id_plus_one <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_element_id_capacity, max_id_plus_one,
                                             ELEMENT_ID_BLOCK_SIZE, ELEMENT_ID_BLOCK_SIZE);
    return globals_resize_element_id_map(new_capacity);
}

fem_error_t globals_reserve_material_ids(int max_id_plus_one)
{
    if (max_id_plus_one <= 0) {
        return FEM_SUCCESS;
    }
    int new_capacity = globals_next_capacity(g_material_id_capacity, max_id_plus_one,
                                             MATERIAL_ID_BLOCK_SIZE, MATERIAL_ID_BLOCK_SIZE);
    return globals_resize_material_id_map(new_capacity);
}

void globals_initialize_node_entry(int node_index)
{
    if (!g_node_coords || node_index < 0 || node_index >= g_node_capacity) {
        return;
    }
    g_node_coords[node_index][0] = 0.0;
    g_node_coords[node_index][1] = 0.0;
    g_node_coords[node_index][2] = 0.0;
    g_node_displ[node_index][0] = 0.0;
    g_node_displ[node_index][1] = 0.0;
    g_node_displ[node_index][2] = 0.0;
    g_node_force[node_index][0] = 0.0;
    g_node_force[node_index][1] = 0.0;
    g_node_force[node_index][2] = 0.0;
    g_node_bc_flags[node_index][0] = 0;
    g_node_bc_flags[node_index][1] = 0;
    g_node_bc_flags[node_index][2] = 0;
    if (g_node_ids) {
        g_node_ids[node_index] = 0;
    }
}

void globals_initialize_element_entry(int element_index)
{
    if (!g_element_nodes || element_index < 0 || element_index >= g_element_capacity) {
        return;
    }
    for (int j = 0; j < MAX_NODES_PER_ELEMENT; ++j) {
        g_element_nodes[element_index][j] = -1;
    }
    if (g_element_type) {
        g_element_type[element_index] = 0;
    }
    if (g_element_material) {
        g_element_material[element_index] = -1;
    }
    if (g_element_ids) {
        g_element_ids[element_index] = 0;
    }
}

void globals_initialize_material_entry(int material_index)
{
    if (!g_material_props || material_index < 0 || material_index >= g_material_capacity) {
        return;
    }
    for (int j = 0; j < 6; ++j) {
        g_material_props[material_index][j] = 0.0;
    }
    g_material_props[material_index][2] = 1.0; /* default thickness */
    g_material_props[material_index][3] = 1.0; /* default density */
    if (g_material_type) {
        g_material_type[material_index] = MATERIAL_PLANE_STRESS;
    }
    if (g_material_ids) {
        g_material_ids[material_index] = 0;
    }
}

/* Global system arrays (allocated dynamically based on DOF count) */
double *g_global_force = NULL;
double *g_global_displ = NULL;

/* Skyline stiffness storage */
double *g_global_stiffness_values = NULL;
int *g_stiffness_profile = NULL;
int *g_stiffness_offsets = NULL;
int g_stiffness_value_count = 0;
int g_stiffness_bandwidth = 0;

/* Distributed load data */
double g_body_force[3];
double g_pressure_value = 0.0;
int g_has_body_force = 0;
int g_has_pressure = 0;
int g_num_tractions = 0;
int g_traction_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];
double g_traction_values[MAX_TRACTION_SURFACES][3];
int g_num_pressure_surfaces = 0;
int g_pressure_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];

/* Analysis control variables */
analysis_control_t g_analysis;
solver_info_t g_solver_info;

/* Problem size variables */
int g_num_nodes = 0;
int g_num_elements = 0;
int g_num_materials = 0;
int g_total_dof = 0;

/* File names */
char g_input_filename[MAX_FILENAME_LEN];
char g_output_filename[MAX_FILENAME_LEN];

/* OpenMP control */
int g_num_threads = 1;
/* T3 orientation policy: 0=auto-correct (default), 1=strict-fail */
int g_t3_strict_orientation = 0;

/* Initialize global variables */
fem_error_t globals_initialize(void)
{
    fem_error_t err;

    globals_reset();

    err = globals_reserve_nodes(INITIAL_NODE_CAPACITY);
    CHECK_ERROR(err);
    err = globals_reserve_elements(INITIAL_ELEMENT_CAPACITY);
    CHECK_ERROR(err);
    err = globals_reserve_materials(INITIAL_MATERIAL_CAPACITY);
    CHECK_ERROR(err);
    err = globals_reserve_node_ids(NODE_ID_BLOCK_SIZE);
    CHECK_ERROR(err);
    err = globals_reserve_element_ids(ELEMENT_ID_BLOCK_SIZE);
    CHECK_ERROR(err);
    err = globals_reserve_material_ids(MATERIAL_ID_BLOCK_SIZE);
    CHECK_ERROR(err);

    /* Initialize analysis control */
    g_analysis.analysis_type = 1;  /* Static analysis */
    g_analysis.num_nodes = 0;
    g_analysis.num_elements = 0;
    g_analysis.num_materials = 0;
    g_analysis.max_iterations = MAX_ITERATIONS;
    g_analysis.tolerance = TOLERANCE;
    strcpy(g_analysis.title, "FEM4C Analysis");
    g_analysis.spatial_dimension = 2;

    /* Initialize solver info */
    g_solver_info.iterations = 0;
    g_solver_info.residual = 0.0;
    g_solver_info.elapsed_time = 0.0;
    g_solver_info.status = FEM_SUCCESS;

    /* Initialize file names */
    strcpy(g_input_filename, "input.dat");
    strcpy(g_output_filename, "output.dat");

    return FEM_SUCCESS;
}

/* Finalize global variables */
fem_error_t globals_finalize(void)
{
    globals_free_system_arrays();
    globals_free_mesh_arrays();
    return FEM_SUCCESS;
}

/* Reset all global arrays to zero */
void globals_reset(void)
{
    if (g_node_ids && g_node_id_to_index) {
        for (int i = 0; i < g_num_nodes; ++i) {
            int id = g_node_ids[i];
            if (id >= 0 && id < g_node_id_capacity) {
                g_node_id_to_index[id] = -1;
            }
            globals_initialize_node_entry(i);
        }
    } else if (g_node_capacity > 0) {
        for (int i = 0; i < g_node_capacity; ++i) {
            globals_initialize_node_entry(i);
        }
    }

    if (g_element_ids && g_element_id_to_index) {
        for (int i = 0; i < g_num_elements; ++i) {
            int id = g_element_ids[i];
            if (id >= 0 && id < g_element_id_capacity) {
                g_element_id_to_index[id] = -1;
            }
            globals_initialize_element_entry(i);
        }
    } else if (g_element_capacity > 0) {
        for (int i = 0; i < g_element_capacity; ++i) {
            globals_initialize_element_entry(i);
        }
    }

    if (g_material_ids && g_material_id_to_index) {
        for (int i = 0; i < g_num_materials; ++i) {
            int id = g_material_ids[i];
            if (id >= 0 && id < g_material_id_capacity) {
                g_material_id_to_index[id] = -1;
            }
            globals_initialize_material_entry(i);
        }
    } else if (g_material_capacity > 0) {
        for (int i = 0; i < g_material_capacity; ++i) {
            globals_initialize_material_entry(i);
        }
    }

    /* Reset load data */
    g_body_force[0] = 0.0;
    g_body_force[1] = 0.0;
    g_body_force[2] = 0.0;
    g_pressure_value = 0.0;
    g_has_body_force = 0;
    g_has_pressure = 0;
    g_num_tractions = 0;
    g_num_pressure_surfaces = 0;
    for (int i = 0; i < MAX_TRACTION_SURFACES; i++) {
        for (int j = 0; j < MAX_SURFACE_NODES; j++) {
            g_traction_surfaces[i][j] = -1;
            g_pressure_surfaces[i][j] = -1;
        }
        g_traction_values[i][0] = 0.0;
        g_traction_values[i][1] = 0.0;
        g_traction_values[i][2] = 0.0;
    }

    globals_free_system_arrays();

    g_num_nodes = 0;
    g_num_elements = 0;
    g_num_materials = 0;
    g_total_dof = 0;
}

/* Allocate system arrays based on total DOF */
fem_error_t globals_allocate_system_arrays(int total_dof)
{
    if (total_dof <= 0) {
        return FEM_SUCCESS;
    }

    globals_free_system_arrays();

    g_global_force = (double *)calloc((size_t)total_dof, sizeof(double));
    g_global_displ = (double *)calloc((size_t)total_dof, sizeof(double));

    if (!g_global_force || !g_global_displ) {
        globals_free_system_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to allocate global system vectors");
    }

    g_total_dof = total_dof;
    return FEM_SUCCESS;
}

/* Free dynamically allocated system arrays */
void globals_free_system_arrays(void)
{
    if (g_global_force) {
        free(g_global_force);
        g_global_force = NULL;
    }
    if (g_global_displ) {
        free(g_global_displ);
        g_global_displ = NULL;
    }
    if (g_global_stiffness_values) {
        free(g_global_stiffness_values);
        g_global_stiffness_values = NULL;
    }
    if (g_stiffness_profile) {
        free(g_stiffness_profile);
        g_stiffness_profile = NULL;
    }
    if (g_stiffness_offsets) {
        free(g_stiffness_offsets);
        g_stiffness_offsets = NULL;
    }
    g_stiffness_value_count = 0;
    g_stiffness_bandwidth = 0;
    g_total_dof = 0;
}
