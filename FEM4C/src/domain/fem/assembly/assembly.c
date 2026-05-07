/* FEM4C - Global matrix assembly implementation
 * Assembly of global stiffness matrix and force vector
 */

#include "assembly.h"
#include "../../../common/constants.h"
#include "../../../common/globals.h"
#include "../../../common/error.h"
#include "../element/t6/t6_element.h"
#include "../element/t6/t6_stiffness.h"
#include "../element/t3/t3_element.h"
#include "../element/q4/q4_element.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

/* Local quadrature definitions for distributed loads */
static const double t3_body_force_points[T3_GAUSS_POINTS][2] = {
    {1.0 / 3.0, 1.0 / 3.0}
};
static const double t3_body_force_weights[T3_GAUSS_POINTS] = {0.5};

static const double line_gauss_points[3] = {
    -sqrt(3.0 / 5.0), 0.0, sqrt(3.0 / 5.0)
};
static const double line_gauss_weights[3] = {
    5.0 / 9.0, 8.0 / 9.0, 5.0 / 9.0
};

static fem_error_t assembly_apply_body_force(void);
static fem_error_t assembly_apply_body_force_t6(int element_id);
static fem_error_t assembly_apply_body_force_t3(int element_id);
static fem_error_t assembly_apply_body_force_q4(int element_id);
static fem_error_t assembly_apply_traction_loads(void);
static fem_error_t assembly_apply_traction_surface(int surface_index);
static fem_error_t assembly_apply_pressure_loads(void);
static fem_error_t assembly_apply_pressure_surface(int surface_index);
static fem_error_t assembly_apply_pressure_surface_t3(int surface_index);
static fem_error_t assembly_apply_pressure_surface_t6(int surface_index);
static fem_error_t assembly_accumulate_quadratic_edge_pressure(
    const double edge_coords[3][2],
    double pressure,
    const int target_local_indices[3],
    double *fe_local);
static fem_error_t assembly_prepare_global_system(void);
static fem_error_t assembly_build_stiffness_profile(void);
static void assembly_zero_stiffness_matrix(void);
static int assembly_matrix_contains_entry(int row, int col);
static double assembly_matrix_get_value(int row, int col);
static fem_error_t assembly_matrix_set_value(int row, int col, double value);
static fem_error_t assembly_matrix_add_value(int row, int col, double value);
static fem_error_t assembly_collect_element_dofs(int element_id, int *dof_map, int *dof_count);
static int assembly_element_dof_per_node(int element_type);
static int assembly_element_node_count(int element_type);
static int assembly_element_total_dof(int element_type);
static fem_error_t assembly_fill_element_dof_map(int element_id, int *dof_map);
static double assembly_get_element_thickness(int element_id);
static fem_error_t assembly_compute_element_area(int element_id, double *area_out);
static fem_error_t assembly_compute_element_area_t6(int element_id, double *area_out);
static fem_error_t assembly_compute_element_area_t3(int element_id, double *area_out);
static fem_error_t assembly_compute_element_area_q4(int element_id, double *area_out);

static int assembly_element_dof_per_node(int element_type)
{
    switch (element_type) {
        case ELEMENT_T6:
            return (g_fem_dof_per_node == 3) ? T6_SHELL_DOF_PER_NODE : T6_DOF_PER_NODE;
        case ELEMENT_T3:
            return (g_fem_dof_per_node == 3) ? T3_SHELL_DOF_PER_NODE : T3_DOF_PER_NODE;
        case ELEMENT_Q4:
            return Q4_DOF_PER_NODE;
        default:
            return 0;
    }
}

static int assembly_element_node_count(int element_type)
{
    switch (element_type) {
        case ELEMENT_T6:
            return T6_NODES_PER_ELEMENT;
        case ELEMENT_T3:
            return T3_NODES_PER_ELEMENT;
        case ELEMENT_Q4:
            return Q4_NODES_PER_ELEMENT;
        default:
            return 0;
    }
}

static int assembly_element_total_dof(int element_type)
{
    return assembly_element_node_count(element_type) *
           assembly_element_dof_per_node(element_type);
}

static fem_error_t assembly_fill_element_dof_map(int element_id, int *dof_map)
{
    int element_type;
    int node_count;
    int dof_per_node;

    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");

    element_type = g_element_type[element_id];
    node_count = assembly_element_node_count(element_type);
    dof_per_node = assembly_element_dof_per_node(element_type);
    if (node_count <= 0 || dof_per_node <= 0) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Unsupported element type %d in DOF map",
                         element_type);
    }

    for (int i = 0; i < node_count; ++i) {
        int node_id = g_element_nodes[element_id][i];
        CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
        for (int j = 0; j < dof_per_node; ++j) {
            dof_map[i * dof_per_node + j] = node_id * g_fem_dof_per_node + j;
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t assembly_prepare_global_system(void)
{
    fem_error_t err;
    int expected_dof = g_total_dof > 0 ? g_total_dof : g_num_nodes * g_fem_dof_per_node;

    err = globals_allocate_system_arrays(expected_dof);
    CHECK_ERROR(err);

    if (g_total_dof <= 0) {
        return FEM_SUCCESS;
    }

    err = assembly_build_stiffness_profile();
    CHECK_ERROR(err);

    assembly_zero_stiffness_matrix();
    return FEM_SUCCESS;
}

static fem_error_t assembly_build_stiffness_profile(void)
{
    int dof = g_total_dof;
    fem_error_t err = FEM_SUCCESS;

    if (dof <= 0) {
        return FEM_SUCCESS;
    }

    g_stiffness_profile = (int *)malloc((size_t)dof * sizeof(int));
    g_stiffness_offsets = (int *)malloc((size_t)(dof + 1) * sizeof(int));
    if (!g_stiffness_profile || !g_stiffness_offsets) {
        globals_free_system_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate skyline index arrays for %d DOF", dof);
    }

    for (int i = 0; i < dof; i++) {
        g_stiffness_profile[i] = i;
    }

    int dof_map[MAX_ELEMENT_DOF];
    int dof_count = 0;
    for (int element_id = 0; element_id < g_num_elements; element_id++) {
        err = assembly_collect_element_dofs(element_id, dof_map, &dof_count);
        CHECK_ERROR(err);

        for (int i = 0; i < dof_count; i++) {
            int row_dof = dof_map[i];
            if (row_dof < 0 || row_dof >= dof) {
                continue;
            }
            for (int j = i; j < dof_count; j++) {
                int col_dof = dof_map[j];
                if (col_dof < 0 || col_dof >= dof) {
                    continue;
                }

                int row = row_dof;
                int col = col_dof;
                if (row > col) {
                    int tmp = row;
                    row = col;
                    col = tmp;
                }

                if (row < g_stiffness_profile[col]) {
                    g_stiffness_profile[col] = row;
                }
            }
        }
    }

    g_stiffness_offsets[0] = 0;
    g_stiffness_bandwidth = 0;
    for (int col = 0; col < dof; col++) {
        if (g_stiffness_profile[col] < 0 || g_stiffness_profile[col] > col) {
            g_stiffness_profile[col] = col;
        }
        int column_height = col - g_stiffness_profile[col];
        if (column_height > g_stiffness_bandwidth) {
            g_stiffness_bandwidth = column_height;
        }
        g_stiffness_offsets[col + 1] = g_stiffness_offsets[col] + column_height + 1;
    }

    g_stiffness_value_count = g_stiffness_offsets[dof];
    if (g_stiffness_value_count <= 0) {
        g_stiffness_value_count = dof;
    }

    g_global_stiffness_values = (double *)calloc((size_t)g_stiffness_value_count, sizeof(double));
    if (!g_global_stiffness_values) {
        globals_free_system_arrays();
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate skyline stiffness storage (%d entries)",
                         g_stiffness_value_count);
    }

    return FEM_SUCCESS;
}

static void assembly_zero_stiffness_matrix(void)
{
    if (g_global_stiffness_values && g_stiffness_value_count > 0) {
        memset(g_global_stiffness_values, 0, (size_t)g_stiffness_value_count * sizeof(double));
    }
}

static int assembly_matrix_contains_entry(int row, int col)
{
    if (row > col) {
        int tmp = row;
        row = col;
        col = tmp;
    }

    if (row < 0 || col < 0 || col >= g_total_dof) {
        return 0;
    }

    if (!g_stiffness_profile || !g_stiffness_offsets) {
        return 0;
    }

    return row >= g_stiffness_profile[col];
}

static double assembly_matrix_get_value(int row, int col)
{
    if (row > col) {
        int tmp = row;
        row = col;
        col = tmp;
    }

    if (!assembly_matrix_contains_entry(row, col)) {
        return 0.0;
    }

    int offset = g_stiffness_offsets[col] + (row - g_stiffness_profile[col]);
    if (offset < 0 || offset >= g_stiffness_value_count) {
        return 0.0;
    }
    return g_global_stiffness_values[offset];
}

static fem_error_t assembly_matrix_set_value(int row, int col, double value)
{
    if (row > col) {
        int tmp = row;
        row = col;
        col = tmp;
    }

    if (!assembly_matrix_contains_entry(row, col)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Stiffness profile missing entry for DOF pair (%d,%d)",
                         row + 1, col + 1);
    }

    int offset = g_stiffness_offsets[col] + (row - g_stiffness_profile[col]);
    if (offset < 0 || offset >= g_stiffness_value_count) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Skyline index out of range for (%d,%d)", row + 1, col + 1);
    }

    g_global_stiffness_values[offset] = value;
    return FEM_SUCCESS;
}

static fem_error_t assembly_matrix_add_value(int row, int col, double value)
{
    if (row > col) {
        int tmp = row;
        row = col;
        col = tmp;
    }

    if (!assembly_matrix_contains_entry(row, col)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Stiffness profile missing entry for DOF pair (%d,%d)",
                         row + 1, col + 1);
    }

    int offset = g_stiffness_offsets[col] + (row - g_stiffness_profile[col]);
    if (offset < 0 || offset >= g_stiffness_value_count) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Skyline index out of range for (%d,%d)", row + 1, col + 1);
    }

    g_global_stiffness_values[offset] += value;
    return FEM_SUCCESS;
}

static fem_error_t assembly_collect_element_dofs(int element_id, int *dof_map, int *dof_count)
{
    *dof_count = assembly_element_total_dof(g_element_type[element_id]);
    if (*dof_count <= 0) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Unsupported element type %d in skyline profile build",
                         g_element_type[element_id]);
    }
    return assembly_fill_element_dof_map(element_id, dof_map);
}

/* Clear global arrays */
fem_error_t assembly_clear_global_arrays(void)
{
    if (g_total_dof <= 0) {
        return FEM_SUCCESS;
    }
    if (!g_global_force || !g_global_displ || !g_global_stiffness_values) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Global system arrays are not initialized");
    }

    for (int i = 0; i < g_total_dof; i++) {
        g_global_force[i] = ZERO;
        g_global_displ[i] = ZERO;
    }
    assembly_zero_stiffness_matrix();

    return FEM_SUCCESS;
}

fem_error_t assembly_add_lumped_mass_scaled_to_stiffness(double mass_scale)
{
    fem_error_t err;
    double *mass_diag = NULL;

    if (!g_global_stiffness_values || g_total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Global stiffness matrix must be assembled before adding lumped mass");
    }

    mass_diag = calloc((size_t)g_total_dof, sizeof(double));
    if (!mass_diag) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate lumped mass diagonal buffer");
    }

    err = assembly_build_lumped_mass_diagonal(mass_diag);
    CHECK_ERROR_CLEANUP(err, free(mass_diag));

    for (int dof = 0; dof < g_total_dof; ++dof) {
        err = assembly_matrix_add_value(dof, dof, mass_scale * mass_diag[dof]);
        CHECK_ERROR_CLEANUP(err, free(mass_diag));
    }

    free(mass_diag);
    return FEM_SUCCESS;
}

fem_error_t assembly_add_lumped_mass_times_vector_to_force(const double *vector,
                                                          double scale)
{
    fem_error_t err;
    double *mass_diag = NULL;

    CHECK_NULL(vector, "lumped mass vector");

    if (!g_global_force || g_total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Global force vector must be assembled before adding lumped mass history RHS");
    }

    mass_diag = calloc((size_t)g_total_dof, sizeof(double));
    if (!mass_diag) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate lumped mass diagonal buffer");
    }

    err = assembly_build_lumped_mass_diagonal(mass_diag);
    CHECK_ERROR_CLEANUP(err, free(mass_diag));

    for (int dof = 0; dof < g_total_dof; ++dof) {
        g_global_force[dof] += scale * mass_diag[dof] * vector[dof];
    }

    free(mass_diag);
    return FEM_SUCCESS;
}

fem_error_t assembly_build_lumped_mass_diagonal(double *mass_diag)
{
    fem_error_t err;

    CHECK_NULL(mass_diag, "lumped mass diagonal");

    if (g_fem_dof_per_node != 2) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "lumped mass first cut: shell / 3 dof node is not supported yet; require 2 dof per node");
    }

    if (g_total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Global DOF must be initialized before building lumped mass diagonal");
    }

    for (int dof = 0; dof < g_total_dof; ++dof) {
        mass_diag[dof] = 0.0;
    }

    for (int element_id = 0; element_id < g_num_elements; ++element_id) {
        int element_type = g_element_type[element_id];
        int node_count = assembly_element_node_count(element_type);
        int dof_map[MAX_ELEMENT_DOF];
        double area = 0.0;
        double rho = 0.0;
        double thickness = 0.0;
        double total_mass = 0.0;
        double nodal_mass = 0.0;
        int material_id = g_element_material[element_id];

        switch (element_type) {
            case ELEMENT_T3:
            case ELEMENT_T6:
            case ELEMENT_Q4:
                break;
            default:
                return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                                 "lumped mass first cut: unsupported element type %d in element %d",
                                 element_type,
                                 element_id + 1);
        }

        if (node_count <= 0) {
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                             "lumped mass first cut: invalid node count for element %d",
                             element_id + 1);
        }

        err = assembly_compute_element_area(element_id, &area);
        CHECK_ERROR(err);

        if (material_id < 0 || material_id >= g_num_materials) {
            return error_set(FEM_ERROR_INVALID_MATERIAL,
                             "lumped mass first cut: invalid material index %d for element %d",
                             material_id + 1,
                             element_id + 1);
        }

        rho = g_material_props[material_id][3];
        thickness = assembly_get_element_thickness(element_id);
        total_mass = rho * thickness * area;
        nodal_mass = total_mass / (double)node_count;

        err = assembly_fill_element_dof_map(element_id, dof_map);
        CHECK_ERROR(err);

        for (int node_index = 0; node_index < node_count; ++node_index) {
            int ux_dof = dof_map[node_index * g_fem_dof_per_node];
            int uy_dof = dof_map[node_index * g_fem_dof_per_node + 1];

            mass_diag[ux_dof] += nodal_mass;
            mass_diag[uy_dof] += nodal_mass;
        }
    }

    return FEM_SUCCESS;
}

/* Assemble global stiffness matrix */
fem_error_t assembly_global_stiffness_matrix(void)
{
    double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
    int element_id;
    fem_error_t err;

    err = assembly_prepare_global_system();
    CHECK_ERROR(err);

    err = assembly_clear_global_arrays();
    CHECK_ERROR(err);

    printf("Assembling global stiffness matrix...\n");
    printf("  Number of elements: %d\n", g_num_elements);
    printf("  Global DOF: %d\n", g_total_dof);

    /* Loop over all elements */
    for (element_id = 0; element_id < g_num_elements; element_id++) {
        if (g_element_type[element_id] == ELEMENT_T6) {
            for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                    ke[i][j] = 0.0;
                }
            }
            if (g_fem_dof_per_node == 3) {
                err = t6_element_shell_stiffness_matrix(element_id, ke);
            } else {
                double ke_mem[T6_TOTAL_DOF][T6_TOTAL_DOF];
                err = t6_element_stiffness_matrix(element_id, ke_mem);
                if (err == FEM_SUCCESS) {
                    for (int i = 0; i < T6_TOTAL_DOF; ++i) {
                        for (int j = 0; j < T6_TOTAL_DOF; ++j) {
                            ke[i][j] = ke_mem[i][j];
                        }
                    }
                }
            }
            CHECK_ERROR(err);

            err = assembly_add_element_stiffness(element_id, ke);
            CHECK_ERROR(err);
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            double ke_t3[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
            for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                    ke_t3[i][j] = 0.0;
                }
            }
            if (g_fem_dof_per_node == 3) {
                err = t3_element_shell_stiffness(element_id, ke_t3);
            } else {
                double ke_mem[T3_TOTAL_DOF][T3_TOTAL_DOF];
                err = t3_element_stiffness(element_id, ke_mem);
                if (err == FEM_SUCCESS) {
                    for (int i = 0; i < T3_TOTAL_DOF; ++i) {
                        for (int j = 0; j < T3_TOTAL_DOF; ++j) {
                            ke_t3[i][j] = ke_mem[i][j];
                        }
                    }
                }
            }
            CHECK_ERROR(err);

            err = assembly_add_element_stiffness_t3(element_id, ke_t3);
            CHECK_ERROR(err);
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            double ke_q4[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
            double ke_mem[Q4_TOTAL_DOF][Q4_TOTAL_DOF];
            for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                    ke_q4[i][j] = 0.0;
                }
            }
            err = q4_element_stiffness(element_id, ke_mem);
            if (err == FEM_SUCCESS) {
                for (int i = 0; i < Q4_TOTAL_DOF; ++i) {
                    for (int j = 0; j < Q4_TOTAL_DOF; ++j) {
                        ke_q4[i][j] = ke_mem[i][j];
                    }
                }
            }
            CHECK_ERROR(err);

            err = assembly_add_element_stiffness_q4(element_id, ke_q4);
            CHECK_ERROR(err);
        } else {
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                           "Unsupported element type %d in element %d",
                           g_element_type[element_id], element_id + 1);
        }
    }

    printf("  Global stiffness matrix assembled successfully\n");
    return FEM_SUCCESS;
}

/* Assemble global force vector */
fem_error_t assembly_global_force_vector(void)
{
    int node_id, dof;
    double total_force = 0.0;
    double load_scale = g_fem_static_current_load_scale;

    printf("Assembling global force vector...\n");

    if (!g_global_force) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Global force vector not initialized");
    }

    /* Ensure vector is clean */
    for (int i = 0; i < g_total_dof; i++) {
        g_global_force[i] = 0.0;
    }

    /* Add nodal forces */
    for (node_id = 0; node_id < g_num_nodes; node_id++) {
        for (dof = 0; dof < g_fem_dof_per_node; dof++) {
            int global_dof = node_id * g_fem_dof_per_node + dof;
            if (global_dof < g_total_dof && fabs(g_node_force[node_id][dof]) > 0.0) {
                g_global_force[global_dof] += g_node_force[node_id][dof] * load_scale;
            }
        }
    }

    /* Distributed body force */
    if (g_has_body_force) {
        fem_error_t err = assembly_apply_body_force();
        CHECK_ERROR(err);
    }

    /* Surface tractions */
    if (g_num_tractions > 0) {
        fem_error_t err = assembly_apply_traction_loads();
        CHECK_ERROR(err);
    }

    if (g_has_pressure) {
        if (g_num_pressure_surfaces > 0) {
            fem_error_t err = assembly_apply_pressure_loads();
            CHECK_ERROR(err);
        } else {
            printf("  Warning: pressure value specified but no pressure surfaces defined.\n");
        }
    }

    for (int i = 0; i < g_total_dof; i++) {
        total_force += fabs(g_global_force[i]);
    }

    printf("  Total applied force magnitude: %.6e\n", total_force);
    printf("  Global force vector assembled successfully\n");
    return FEM_SUCCESS;
}

/* Add element stiffness matrix to global matrix */
fem_error_t assembly_add_element_stiffness(int element_id, 
                                          double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF])
{
    int dof_map[MAX_ELEMENT_DOF];
    int dof_count = assembly_element_total_dof(g_element_type[element_id]);
    fem_error_t err;

    err = assembly_get_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    static int assembly_debug = 0;
    if (!assembly_debug) {
        printf("  DOF mapping for element %d: ", element_id);
        for (int i = 0; i < dof_count; i++) {
            printf("%d ", dof_map[i]);
        }
        printf("\n  Element stiffness matrix sample:\n");
        for (int i = 0; i < 3; i++) {
            printf("    ");
            for (int j = 0; j < 3; j++) {
                printf("%.2e ", ke[i][j]);
            }
            printf("\n");
        }
        assembly_debug = 1;
    }

    for (int i = 0; i < dof_count; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < dof_count; j++) {
            int global_j = dof_map[j];
            if (global_j < 0 || global_j >= g_total_dof) {
                continue;
            }
            err = assembly_matrix_add_value(global_i, global_j, ke[i][j]);
            CHECK_ERROR(err);
        }
    }

    return FEM_SUCCESS;
}

/* Get DOF mapping for element */
fem_error_t assembly_get_element_dof_map(int element_id, int dof_map[MAX_ELEMENT_DOF])
{
    return assembly_fill_element_dof_map(element_id, dof_map);
}

/* Get global DOF index */
fem_error_t assembly_get_global_dof_index(int node_id, int local_dof)
{
    CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
    CHECK_BOUNDS(local_dof, 3, "Local DOF");
    
    return node_id * g_fem_dof_per_node + local_dof;
}

/* Apply boundary conditions */
fem_error_t assembly_apply_boundary_conditions(void)
{
    int node_id, dof, global_dof, i;

    printf("Applying boundary conditions...\n");
    int bc_count = 0;

    if (!g_global_force || !g_global_displ || !g_global_stiffness_values) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Global system arrays not initialized");
    }

    for (node_id = 0; node_id < g_num_nodes; node_id++) {
        for (dof = 0; dof < g_fem_dof_per_node; dof++) {
            if (g_node_bc_flags[node_id][dof] == 1) {
                global_dof = node_id * g_fem_dof_per_node + dof;

                if (global_dof < g_total_dof) {
                    double prescribed_base_value =
                        g_node_bc_values ? g_node_bc_values[node_id][dof] : g_node_displ[node_id][dof];
                    double prescribed_value = prescribed_base_value * g_fem_static_current_load_scale;
                    double original_diag = assembly_matrix_get_value(global_dof, global_dof);

                    for (i = 0; i < g_total_dof; i++) {
                        if (i == global_dof) {
                            continue;
                        }
                        if (!assembly_matrix_contains_entry(i, global_dof)) {
                            continue;
                        }
                        double kij = assembly_matrix_get_value(i, global_dof);
                        g_global_force[i] -= kij * prescribed_value;
                        fem_error_t err = assembly_matrix_set_value(i, global_dof, 0.0);
                        CHECK_ERROR(err);
                    }

                    fem_error_t err = assembly_matrix_set_value(global_dof, global_dof, 1.0);
                    CHECK_ERROR(err);

                    g_global_force[global_dof] = prescribed_value;

                    printf("  BC: Node %d DOF %d (global %d): diag %.3e -> 1.000, prescribed=%.3f\n",
                           node_id + 1, dof, global_dof, original_diag, prescribed_value);
                    bc_count++;
                }
            }
        }
    }

    printf("  Applied %d boundary conditions\n", bc_count);

    /* Debug: Print relevant part of global stiffness matrix */
    printf("  Global stiffness matrix sample (rows 0-5, cols 0-5):\n");
    for (int i = 0; i < 6 && i < g_total_dof; i++) {
        printf("    ");
        for (int j = 0; j < 6 && j < g_total_dof; j++) {
            printf("%8.1e ", assembly_matrix_get_value(i, j));
        }
        printf("\n");
    }

    printf("  Boundary conditions applied successfully\n");
    return FEM_SUCCESS;
}

/* Check matrix properties */
fem_error_t assembly_check_matrix_properties(void)
{
    double min_diagonal = 1.0e30;
    double max_diagonal = -1.0e30;
    int zero_diagonal_count = 0;

    printf("Checking global stiffness matrix properties...\n");

    if (!g_global_stiffness_values) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Global stiffness matrix not initialized");
    }

    for (int i = 0; i < g_total_dof; i++) {
        double diag_val = assembly_matrix_get_value(i, i);

        if (fabs(diag_val) < TOLERANCE) {
            zero_diagonal_count++;
        }

        if (diag_val < min_diagonal) min_diagonal = diag_val;
        if (diag_val > max_diagonal) max_diagonal = diag_val;
    }

    printf("  Diagonal terms: min = %e, max = %e\n", min_diagonal, max_diagonal);
    printf("  Zero diagonal terms: %d\n", zero_diagonal_count);

    if (zero_diagonal_count > 0) {
        return error_set(FEM_ERROR_SINGULAR_MATRIX, 
                        "Global stiffness matrix has %d zero diagonal terms", 
                        zero_diagonal_count);
    }

    if (min_diagonal <= 0.0) {
        return error_set(FEM_ERROR_SINGULAR_MATRIX, 
                        "Global stiffness matrix has non-positive diagonal terms");
    }

    printf("  Matrix properties check passed\n");
    return FEM_SUCCESS;
}

/* Assembly routine */
fem_error_t assembly_parallel_stiffness_matrix(void)
{
    fem_error_t err;

    err = assembly_prepare_global_system();
    CHECK_ERROR(err);

    err = assembly_clear_global_arrays();
    CHECK_ERROR(err);

#ifdef _OPENMP
    printf("Assembling global stiffness matrix (serial fallback, OpenMP build pending)...\n");
#else
    printf("Assembling global stiffness matrix...\n");
#endif
    printf("  Elements: %d\n", g_num_elements);

    for (int debug_idx = 0; debug_idx < g_num_elements && debug_idx < 5; ++debug_idx) {
        printf("    element %d type %d\n", debug_idx, g_element_type[debug_idx]);
    }
    fflush(stdout);

    for (int element_id = 0; element_id < g_num_elements; element_id++) {
        fem_error_t local_err = FEM_SUCCESS;

        fprintf(stderr, "    [debug] element %d raw type %d\n", element_id, g_element_type[element_id]);
        fflush(stderr);

        switch (g_element_type[element_id]) {
            case ELEMENT_T6: {
                double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
                for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                    for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                        ke[i][j] = 0.0;
                    }
                }
                if (g_fem_dof_per_node == 3) {
                    local_err = t6_element_shell_stiffness_matrix(element_id, ke);
                } else {
                    double ke_mem[T6_TOTAL_DOF][T6_TOTAL_DOF];
                    local_err = t6_element_stiffness_matrix(element_id, ke_mem);
                    if (local_err == FEM_SUCCESS) {
                        for (int i = 0; i < T6_TOTAL_DOF; ++i) {
                            for (int j = 0; j < T6_TOTAL_DOF; ++j) {
                                ke[i][j] = ke_mem[i][j];
                            }
                        }
                    }
                }
                if (local_err == FEM_SUCCESS) {
                    local_err = assembly_add_element_stiffness(element_id, ke);
                }
                break;
            }
            case ELEMENT_T3: {
                double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
                for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                    for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                        ke[i][j] = 0.0;
                    }
                }
                if (g_fem_dof_per_node == 3) {
                    local_err = t3_element_shell_stiffness(element_id, ke);
                } else {
                    double ke_mem[T3_TOTAL_DOF][T3_TOTAL_DOF];
                    local_err = t3_element_stiffness(element_id, ke_mem);
                    if (local_err == FEM_SUCCESS) {
                        for (int i = 0; i < T3_TOTAL_DOF; ++i) {
                            for (int j = 0; j < T3_TOTAL_DOF; ++j) {
                                ke[i][j] = ke_mem[i][j];
                            }
                        }
                    }
                }
                if (local_err == FEM_SUCCESS) {
                    local_err = assembly_add_element_stiffness_t3(element_id, ke);
                }
                break;
            }
            case ELEMENT_Q4: {
                double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF];
                double ke_mem[Q4_TOTAL_DOF][Q4_TOTAL_DOF];
                for (int i = 0; i < MAX_ELEMENT_DOF; ++i) {
                    for (int j = 0; j < MAX_ELEMENT_DOF; ++j) {
                        ke[i][j] = 0.0;
                    }
                }
                local_err = q4_element_stiffness(element_id, ke_mem);
                if (local_err == FEM_SUCCESS) {
                    for (int i = 0; i < Q4_TOTAL_DOF; ++i) {
                        for (int j = 0; j < Q4_TOTAL_DOF; ++j) {
                            ke[i][j] = ke_mem[i][j];
                        }
                    }
                }
                if (local_err == FEM_SUCCESS) {
                    local_err = assembly_add_element_stiffness_q4(element_id, ke);
                }
                break;
            }
            default:
                local_err = FEM_ERROR_INVALID_ELEMENT_TYPE;
                break;
        }

        if (local_err != FEM_SUCCESS) {
            printf("  Error assembling element %d into global matrix: %d\n", element_id, local_err);
            return local_err;
        }
    }

    printf("  Assembly completed\n");
    return FEM_SUCCESS;
}



/* Add T3 element stiffness matrix to global stiffness matrix */
fem_error_t assembly_add_element_stiffness_t3(int element_id,
                                             double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF])
{
    int dof_map[MAX_ELEMENT_DOF];
    int dof_count = assembly_element_total_dof(g_element_type[element_id]);
    fem_error_t err = FEM_SUCCESS;

    err = assembly_fill_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    for (int i = 0; i < dof_count; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < dof_count; j++) {
            int global_j = dof_map[j];
            if (global_j < 0 || global_j >= g_total_dof) {
                continue;
            }
            err = assembly_matrix_add_value(global_i, global_j, ke[i][j]);
            CHECK_ERROR(err);
        }
    }

    return FEM_SUCCESS;
}

/* Add Q4 element stiffness matrix to global stiffness matrix */
fem_error_t assembly_add_element_stiffness_q4(int element_id,
                                             double ke[MAX_ELEMENT_DOF][MAX_ELEMENT_DOF])
{
    int dof_map[MAX_ELEMENT_DOF];
    int dof_count = assembly_element_total_dof(g_element_type[element_id]);
    fem_error_t err = FEM_SUCCESS;

    err = assembly_fill_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    for (int i = 0; i < dof_count; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < dof_count; j++) {
            int global_j = dof_map[j];
            if (global_j < 0 || global_j >= g_total_dof) {
                continue;
            }
            err = assembly_matrix_add_value(global_i, global_j, ke[i][j]);
            CHECK_ERROR(err);
        }
    }

    return FEM_SUCCESS;
}

/* --- Distributed load helpers ------------------------------------------------ */

static void assembly_accumulate_force(int dof_count, const int *dof_map, const double *fe)
{
    for (int i = 0; i < dof_count; i++) {
        int global = dof_map[i];
        if (global >= 0 && global < g_total_dof) {
            g_global_force[global] += fe[i];
        }
    }
}

static fem_error_t assembly_apply_body_force(void)
{
    fem_error_t err = FEM_SUCCESS;

    for (int element_id = 0; element_id < g_num_elements; element_id++) {
        switch (g_element_type[element_id]) {
            case ELEMENT_T6:
                err = assembly_apply_body_force_t6(element_id);
                break;
            case ELEMENT_T3:
                err = assembly_apply_body_force_t3(element_id);
                break;
            case ELEMENT_Q4:
                err = assembly_apply_body_force_q4(element_id);
                break;
            default:
                /* Skip unsupported elements for body force */
                err = FEM_SUCCESS;
                break;
        }
        CHECK_ERROR(err);
    }

    return FEM_SUCCESS;
}

static double assembly_get_element_thickness(int element_id)
{
    int material_index = g_element_material[element_id];
    if (material_index < 0 || material_index >= g_num_materials) {
        material_index = 0;
    }
    double thickness = g_material_props[material_index][2];
    if (thickness <= 0.0) {
        thickness = 1.0;
    }
    return thickness;
}

static fem_error_t assembly_compute_element_area(int element_id, double *area_out)
{
    CHECK_NULL(area_out, "element area output");

    switch (g_element_type[element_id]) {
        case ELEMENT_T6:
            return assembly_compute_element_area_t6(element_id, area_out);
        case ELEMENT_T3:
            return assembly_compute_element_area_t3(element_id, area_out);
        case ELEMENT_Q4:
            return assembly_compute_element_area_q4(element_id, area_out);
        default:
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                             "lumped mass first cut: unsupported element type %d in area computation",
                             g_element_type[element_id]);
    }
}

static fem_error_t assembly_compute_element_area_t6(int element_id, double *area_out)
{
    double J[2][2], det_J;
    double area = 0.0;
    fem_error_t err;

    for (int gp = 0; gp < T6_GAUSS_POINTS; gp++) {
        double xi = g_t6_gauss_points[gp][0];
        double eta = g_t6_gauss_points[gp][1];
        double weight = g_t6_gauss_weights[gp];

        err = t6_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        area += weight * det_J;
    }

    *area_out = area;
    return FEM_SUCCESS;
}

static fem_error_t assembly_compute_element_area_t3(int element_id, double *area_out)
{
    double J[2][2], det_J;
    double area = 0.0;
    fem_error_t err;

    for (int gp = 0; gp < T3_GAUSS_POINTS; gp++) {
        double xi = t3_body_force_points[gp][0];
        double eta = t3_body_force_points[gp][1];
        double weight = t3_body_force_weights[gp];

        err = t3_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        area += weight * det_J;
    }

    *area_out = area;
    return FEM_SUCCESS;
}

static fem_error_t assembly_compute_element_area_q4(int element_id, double *area_out)
{
    double J[2][2], det_J;
    double area = 0.0;
    fem_error_t err;

    for (int gp = 0; gp < Q4_GAUSS_POINTS; gp++) {
        double xi = g_q4_gauss_points[gp][0];
        double eta = g_q4_gauss_points[gp][1];
        double weight = g_q4_gauss_weights[gp];

        err = q4_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        area += weight * det_J;
    }

    *area_out = area;
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_body_force_t6(int element_id)
{
    double fe[MAX_ELEMENT_DOF] = {0.0};
    double N[T6_NODES_PER_ELEMENT];
    double J[2][2], det_J;
    fem_error_t err;
    double thickness = assembly_get_element_thickness(element_id);

    for (int gp = 0; gp < T6_GAUSS_POINTS; gp++) {
        double xi = g_t6_gauss_points[gp][0];
        double eta = g_t6_gauss_points[gp][1];
        double weight = g_t6_gauss_weights[gp];

        err = t6_shape_functions(xi, eta, N);
        CHECK_ERROR(err);

        err = t6_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        double scale = weight * det_J * thickness;
        for (int i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            int base = i * assembly_element_dof_per_node(ELEMENT_T6);
            fe[base]     += N[i] * g_body_force[0] * scale * g_fem_static_current_load_scale;
            fe[base + 1] += N[i] * g_body_force[1] * scale * g_fem_static_current_load_scale;
        }
    }

    int dof_map[MAX_ELEMENT_DOF];
    err = assembly_get_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    assembly_accumulate_force(assembly_element_total_dof(ELEMENT_T6), dof_map, fe);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_body_force_t3(int element_id)
{
    double fe[MAX_ELEMENT_DOF] = {0.0};
    double N[T3_NODES_PER_ELEMENT];
    double J[2][2], det_J;
    fem_error_t err;
    double thickness = assembly_get_element_thickness(element_id);

    for (int gp = 0; gp < T3_GAUSS_POINTS; gp++) {
        double xi = t3_body_force_points[gp][0];
        double eta = t3_body_force_points[gp][1];
        double weight = t3_body_force_weights[gp];

        err = t3_shape_functions(xi, eta, N);
        CHECK_ERROR(err);

        err = t3_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        double scale = weight * det_J * thickness;
        for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
            int base = i * assembly_element_dof_per_node(ELEMENT_T3);
            fe[base]     += N[i] * g_body_force[0] * scale * g_fem_static_current_load_scale;
            fe[base + 1] += N[i] * g_body_force[1] * scale * g_fem_static_current_load_scale;
        }
    }

    int dof_map[MAX_ELEMENT_DOF];
    err = assembly_fill_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    assembly_accumulate_force(assembly_element_total_dof(ELEMENT_T3), dof_map, fe);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_body_force_q4(int element_id)
{
    double fe[MAX_ELEMENT_DOF] = {0.0};
    double N[Q4_NODES_PER_ELEMENT];
    double J[2][2], det_J;
    fem_error_t err;
    double thickness = assembly_get_element_thickness(element_id);

    for (int gp = 0; gp < Q4_GAUSS_POINTS; gp++) {
        double xi = g_q4_gauss_points[gp][0];
        double eta = g_q4_gauss_points[gp][1];
        double weight = g_q4_gauss_weights[gp];

        err = q4_shape_functions(xi, eta, N);
        CHECK_ERROR(err);

        err = q4_jacobian_matrix(element_id, xi, eta, J, &det_J);
        CHECK_ERROR(err);

        double scale = weight * det_J * thickness;
        for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
            int base = i * assembly_element_dof_per_node(ELEMENT_Q4);
            fe[base]     += N[i] * g_body_force[0] * scale * g_fem_static_current_load_scale;
            fe[base + 1] += N[i] * g_body_force[1] * scale * g_fem_static_current_load_scale;
        }
    }

    int dof_map[MAX_ELEMENT_DOF];
    err = assembly_fill_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    assembly_accumulate_force(assembly_element_total_dof(ELEMENT_Q4), dof_map, fe);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_traction_loads(void)
{
    for (int i = 0; i < g_num_tractions; i++) {
        fem_error_t err = assembly_apply_traction_surface(i);
        CHECK_ERROR(err);
    }
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_traction_surface(int surface_index)
{
    int node_indices[MAX_SURFACE_NODES];
    double coords[MAX_SURFACE_NODES][2];
    double fe_local[MAX_SURFACE_NODES * MAX_DOF_PER_NODE] = {0.0};

    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        node_indices[i] = g_traction_surfaces[surface_index][i];
        if (node_indices[i] < 0 || node_indices[i] >= g_num_nodes) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "Invalid node index %d in traction surface %d",
                             node_indices[i], surface_index + 1);
        }
        coords[i][0] = g_node_coords[node_indices[i]][0];
        coords[i][1] = g_node_coords[node_indices[i]][1];
    }

    const double tx = g_traction_values[surface_index][0] * g_fem_static_current_load_scale;
    const double ty = g_traction_values[surface_index][1] * g_fem_static_current_load_scale;

    for (int gp = 0; gp < 3; gp++) {
        double s = line_gauss_points[gp];
        double weight = line_gauss_weights[gp];

        double N[3];
        double dNds[3];

        N[0] = 0.5 * s * (s - 1.0);
        N[1] = 1.0 - s * s;
        N[2] = 0.5 * s * (s + 1.0);

        dNds[0] = s - 0.5;
        dNds[1] = -2.0 * s;
        dNds[2] = s + 0.5;

        double dx_ds = 0.0;
        double dy_ds = 0.0;
        for (int i = 0; i < MAX_SURFACE_NODES; i++) {
            dx_ds += dNds[i] * coords[i][0];
            dy_ds += dNds[i] * coords[i][1];
        }

        double jacobian = sqrt(dx_ds * dx_ds + dy_ds * dy_ds);
        double scaled_weight = weight * jacobian;

        for (int i = 0; i < MAX_SURFACE_NODES; i++) {
            int base = i * g_fem_dof_per_node;
            fe_local[base]     += N[i] * tx * scaled_weight;
            fe_local[base + 1] += N[i] * ty * scaled_weight;
        }
    }

    int dof_map[MAX_SURFACE_NODES * MAX_DOF_PER_NODE];
    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        for (int j = 0; j < g_fem_dof_per_node; ++j) {
            dof_map[i * g_fem_dof_per_node + j] = node_indices[i] * g_fem_dof_per_node + j;
        }
    }

    assembly_accumulate_force(MAX_SURFACE_NODES * g_fem_dof_per_node, dof_map, fe_local);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_pressure_loads(void)
{
    for (int i = 0; i < g_num_pressure_surfaces; i++) {
        fem_error_t err = assembly_apply_pressure_surface(i);
        CHECK_ERROR(err);
    }
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_pressure_surface(int surface_index)
{
    int node_count = g_pressure_surface_node_counts[surface_index];

    if (node_count == 3) {
        return assembly_apply_pressure_surface_t3(surface_index);
    }
    if (node_count == T6_NODES_PER_ELEMENT) {
        return assembly_apply_pressure_surface_t6(surface_index);
    }
    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Pressure surface %d has unsupported node_count=%d",
                     surface_index + 1,
                     node_count);
}

static fem_error_t assembly_apply_pressure_surface_t3(int surface_index)
{
    int node_indices[MAX_SURFACE_NODES];
    double coords[MAX_SURFACE_NODES][2];
    double fe_local[MAX_SURFACE_NODES * MAX_DOF_PER_NODE] = {0.0};

    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        node_indices[i] = g_pressure_surface_nodes[surface_index][i];
        if (node_indices[i] < 0 || node_indices[i] >= g_num_nodes) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "Invalid node index %d in pressure surface %d",
                             node_indices[i], surface_index + 1);
        }
        coords[i][0] = g_node_coords[node_indices[i]][0];
        coords[i][1] = g_node_coords[node_indices[i]][1];
    }

    for (int gp = 0; gp < 3; gp++) {
        double s = line_gauss_points[gp];
        double weight = line_gauss_weights[gp];

        double N[3];
        double dNds[3];

        N[0] = 0.5 * s * (s - 1.0);
        N[1] = 1.0 - s * s;
        N[2] = 0.5 * s * (s + 1.0);

        dNds[0] = s - 0.5;
        dNds[1] = -2.0 * s;
        dNds[2] = s + 0.5;

        double dx_ds = 0.0;
        double dy_ds = 0.0;
        for (int i = 0; i < MAX_SURFACE_NODES; i++) {
            dx_ds += dNds[i] * coords[i][0];
            dy_ds += dNds[i] * coords[i][1];
        }

        double jacobian = sqrt(dx_ds * dx_ds + dy_ds * dy_ds);
        if (jacobian < TOLERANCE) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Degenerate pressure surface %d (jacobian too small)",
                             surface_index + 1);
        }

        double nx = dy_ds / jacobian;
        double ny = -dx_ds / jacobian;
        double pressure = g_pressure_surface_values[surface_index] * g_fem_static_current_load_scale;
        double px = -pressure * nx;
        double py = -pressure * ny;

        double scaled_weight = weight * jacobian;

        for (int i = 0; i < MAX_SURFACE_NODES; i++) {
            int base = i * g_fem_dof_per_node;
            fe_local[base]     += N[i] * px * scaled_weight;
            fe_local[base + 1] += N[i] * py * scaled_weight;
        }
    }

    int dof_map[MAX_SURFACE_NODES * MAX_DOF_PER_NODE];
    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        for (int j = 0; j < g_fem_dof_per_node; ++j) {
            dof_map[i * g_fem_dof_per_node + j] = node_indices[i] * g_fem_dof_per_node + j;
        }
    }

    assembly_accumulate_force(MAX_SURFACE_NODES * g_fem_dof_per_node, dof_map, fe_local);
    return FEM_SUCCESS;
}

static fem_error_t assembly_accumulate_quadratic_edge_pressure(
    const double edge_coords[3][2],
    double pressure,
    const int target_local_indices[3],
    double *fe_local)
{
    for (int gp = 0; gp < 3; gp++) {
        double s = line_gauss_points[gp];
        double weight = line_gauss_weights[gp];
        double N[3];
        double dNds[3];
        double dx_ds = 0.0;
        double dy_ds = 0.0;
        double jacobian;
        double nx;
        double ny;
        double px;
        double py;
        double scaled_weight;

        N[0] = 0.5 * s * (s - 1.0);
        N[1] = 1.0 - s * s;
        N[2] = 0.5 * s * (s + 1.0);

        dNds[0] = s - 0.5;
        dNds[1] = -2.0 * s;
        dNds[2] = s + 0.5;

        for (int i = 0; i < 3; i++) {
            dx_ds += dNds[i] * edge_coords[i][0];
            dy_ds += dNds[i] * edge_coords[i][1];
        }

        jacobian = sqrt(dx_ds * dx_ds + dy_ds * dy_ds);
        if (jacobian < TOLERANCE) {
            return FEM_ERROR_INVALID_INPUT;
        }

        nx = dy_ds / jacobian;
        ny = -dx_ds / jacobian;
        px = -pressure * nx;
        py = -pressure * ny;
        scaled_weight = weight * jacobian;

        for (int i = 0; i < 3; i++) {
            int base = target_local_indices[i] * g_fem_dof_per_node;
            fe_local[base] += N[i] * px * scaled_weight;
            fe_local[base + 1] += N[i] * py * scaled_weight;
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_pressure_surface_t6(int surface_index)
{
    static const int edge_local_indices[3][3] = {
        {0, 3, 1},
        {1, 4, 2},
        {2, 5, 0}
    };
    int node_indices[T6_NODES_PER_ELEMENT];
    double coords[T6_NODES_PER_ELEMENT][2];
    double fe_local[T6_NODES_PER_ELEMENT * MAX_DOF_PER_NODE] = {0.0};
    double pressure = g_pressure_surface_values[surface_index] * g_fem_static_current_load_scale;

    for (int i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        node_indices[i] = g_pressure_surface_nodes[surface_index][i];
        if (node_indices[i] < 0 || node_indices[i] >= g_num_nodes) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "Invalid node index %d in pressure surface %d",
                             node_indices[i], surface_index + 1);
        }
        coords[i][0] = g_node_coords[node_indices[i]][0];
        coords[i][1] = g_node_coords[node_indices[i]][1];
    }

    for (int edge = 0; edge < 3; ++edge) {
        double edge_coords[3][2];
        fem_error_t err;

        for (int i = 0; i < 3; ++i) {
            int local_index = edge_local_indices[edge][i];
            edge_coords[i][0] = coords[local_index][0];
            edge_coords[i][1] = coords[local_index][1];
        }

        err = assembly_accumulate_quadratic_edge_pressure(
            edge_coords,
            pressure,
            edge_local_indices[edge],
            fe_local);
        if (err != FEM_SUCCESS) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Degenerate T6 pressure surface %d edge %d",
                             surface_index + 1,
                             edge + 1);
        }
    }

    {
        int dof_map[T6_NODES_PER_ELEMENT * MAX_DOF_PER_NODE];

        for (int i = 0; i < T6_NODES_PER_ELEMENT; i++) {
            for (int j = 0; j < g_fem_dof_per_node; ++j) {
                dof_map[i * g_fem_dof_per_node + j] = node_indices[i] * g_fem_dof_per_node + j;
            }
        }

        assembly_accumulate_force(T6_NODES_PER_ELEMENT * g_fem_dof_per_node, dof_map, fe_local);
    }
    return FEM_SUCCESS;
}
