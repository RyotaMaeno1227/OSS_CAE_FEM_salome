/* FEM4C - Global matrix assembly implementation
 * Assembly of global stiffness matrix and force vector
 */

#include "assembly.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include "../elements/t6/t6_element.h"
#include "../elements/t6/t6_stiffness.h"
#include "../elements/t3/t3_element.h"
#include "../elements/q4/q4_element.h"
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
static fem_error_t assembly_prepare_global_system(void);
static fem_error_t assembly_build_stiffness_profile(void);
static void assembly_zero_stiffness_matrix(void);
static int assembly_matrix_contains_entry(int row, int col);
static double assembly_matrix_get_value(int row, int col);
static fem_error_t assembly_matrix_set_value(int row, int col, double value);
static fem_error_t assembly_matrix_add_value(int row, int col, double value);
static fem_error_t assembly_collect_element_dofs(int element_id, int *dof_map, int *dof_count);

static fem_error_t assembly_prepare_global_system(void)
{
    fem_error_t err;
    int expected_dof = g_total_dof > 0 ? g_total_dof : g_num_nodes * 2;

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

    int dof_map[T6_TOTAL_DOF];
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
    switch (g_element_type[element_id]) {
        case ELEMENT_T6:
            *dof_count = T6_TOTAL_DOF;
            return assembly_get_element_dof_map(element_id, dof_map);
        case ELEMENT_T3:
            *dof_count = T3_TOTAL_DOF;
            for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
                int node_index = g_element_nodes[element_id][i];
                CHECK_BOUNDS(node_index, g_num_nodes, "Node ID");
                dof_map[2 * i]     = node_index * 2;
                dof_map[2 * i + 1] = node_index * 2 + 1;
            }
            return FEM_SUCCESS;
        case ELEMENT_Q4:
            *dof_count = Q4_TOTAL_DOF;
            for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
                int node_index = g_element_nodes[element_id][i];
                CHECK_BOUNDS(node_index, g_num_nodes, "Node ID");
                dof_map[2 * i]     = node_index * 2;
                dof_map[2 * i + 1] = node_index * 2 + 1;
            }
            return FEM_SUCCESS;
        default:
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                             "Unsupported element type %d in skyline profile build",
                             g_element_type[element_id]);
    }
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

/* Assemble global stiffness matrix */
fem_error_t assembly_global_stiffness_matrix(void)
{
    double ke[T6_TOTAL_DOF][T6_TOTAL_DOF];
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
            err = t6_element_stiffness_matrix(element_id, ke);
            CHECK_ERROR(err);

            err = assembly_add_element_stiffness(element_id, ke);
            CHECK_ERROR(err);
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            double ke_t3[T3_TOTAL_DOF][T3_TOTAL_DOF];
            err = t3_element_stiffness(element_id, ke_t3);
            CHECK_ERROR(err);

            err = assembly_add_element_stiffness_t3(element_id, ke_t3);
            CHECK_ERROR(err);
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            double ke_q4[Q4_TOTAL_DOF][Q4_TOTAL_DOF];
            err = q4_element_stiffness(element_id, ke_q4);
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
        for (dof = 0; dof < 2; dof++) { /* 2D problem */
            int global_dof = node_id * 2 + dof;
            if (global_dof < g_total_dof && fabs(g_node_force[node_id][dof]) > 0.0) {
                g_global_force[global_dof] += g_node_force[node_id][dof];
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
                                          double ke[T6_TOTAL_DOF][T6_TOTAL_DOF])
{
    int dof_map[T6_TOTAL_DOF];
    fem_error_t err;

    err = assembly_get_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    static int assembly_debug = 0;
    if (!assembly_debug) {
        printf("  DOF mapping for element %d: ", element_id);
        for (int i = 0; i < T6_TOTAL_DOF; i++) {
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

    for (int i = 0; i < T6_TOTAL_DOF; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < T6_TOTAL_DOF; j++) {
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
fem_error_t assembly_get_element_dof_map(int element_id, int dof_map[T6_TOTAL_DOF])
{
    int i, node_id;
    
    CHECK_BOUNDS(element_id, g_num_elements, "Element ID");
    
    /* Map element DOFs to global DOFs */
    for (i = 0; i < T6_NODES_PER_ELEMENT; i++) {
        node_id = g_element_nodes[element_id][i];
        CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
        
        dof_map[2*i]     = node_id * 2;     /* u displacement */
        dof_map[2*i + 1] = node_id * 2 + 1; /* v displacement */
    }
    
    return FEM_SUCCESS;
}

/* Get global DOF index */
fem_error_t assembly_get_global_dof_index(int node_id, int local_dof)
{
    CHECK_BOUNDS(node_id, g_num_nodes, "Node ID");
    CHECK_BOUNDS(local_dof, 3, "Local DOF");
    
    return node_id * 2 + local_dof; /* 2D problem */
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
        for (dof = 0; dof < 2; dof++) { /* 2D problem */
            if (g_node_bc_flags[node_id][dof] == 1) {
                global_dof = node_id * 2 + dof;

                if (global_dof < g_total_dof) {
                    double prescribed_value = g_node_displ[node_id][dof];
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
                double ke[T6_TOTAL_DOF][T6_TOTAL_DOF];
                local_err = t6_element_stiffness_matrix(element_id, ke);
                if (local_err == FEM_SUCCESS) {
                    local_err = assembly_add_element_stiffness(element_id, ke);
                }
                break;
            }
            case ELEMENT_T3: {
                double ke[T3_TOTAL_DOF][T3_TOTAL_DOF];
                local_err = t3_element_stiffness(element_id, ke);
                if (local_err == FEM_SUCCESS) {
                    local_err = assembly_add_element_stiffness_t3(element_id, ke);
                }
                break;
            }
            case ELEMENT_Q4: {
                double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF];
                local_err = q4_element_stiffness(element_id, ke);
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
                                             double ke[T3_TOTAL_DOF][T3_TOTAL_DOF])
{
    int dof_map[T3_TOTAL_DOF];
    fem_error_t err = FEM_SUCCESS;

    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        for (int j = 0; j < T3_DOF_PER_NODE; j++) {
            dof_map[i * T3_DOF_PER_NODE + j] = node_id * 2 + j;
        }
    }

    for (int i = 0; i < T3_TOTAL_DOF; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < T3_TOTAL_DOF; j++) {
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
                                             double ke[Q4_TOTAL_DOF][Q4_TOTAL_DOF])
{
    int dof_map[Q4_TOTAL_DOF];
    fem_error_t err = FEM_SUCCESS;

    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        int node_id = g_element_nodes[element_id][i];
        for (int j = 0; j < Q4_DOF_PER_NODE; j++) {
            dof_map[i * Q4_DOF_PER_NODE + j] = node_id * 2 + j;
        }
    }

    for (int i = 0; i < Q4_TOTAL_DOF; i++) {
        int global_i = dof_map[i];
        if (global_i < 0 || global_i >= g_total_dof) {
            continue;
        }
        for (int j = i; j < Q4_TOTAL_DOF; j++) {
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

static fem_error_t assembly_apply_body_force_t6(int element_id)
{
    double fe[T6_TOTAL_DOF] = {0.0};
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
            fe[2 * i]     += N[i] * g_body_force[0] * scale;
            fe[2 * i + 1] += N[i] * g_body_force[1] * scale;
        }
    }

    int dof_map[T6_TOTAL_DOF];
    err = assembly_get_element_dof_map(element_id, dof_map);
    CHECK_ERROR(err);

    assembly_accumulate_force(T6_TOTAL_DOF, dof_map, fe);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_body_force_t3(int element_id)
{
    double fe[T3_TOTAL_DOF] = {0.0};
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
            fe[2 * i]     += N[i] * g_body_force[0] * scale;
            fe[2 * i + 1] += N[i] * g_body_force[1] * scale;
        }
    }

    int dof_map[T3_TOTAL_DOF];
    for (int i = 0; i < T3_NODES_PER_ELEMENT; i++) {
        int node_index = g_element_nodes[element_id][i];
        dof_map[2 * i]     = node_index * 2;
        dof_map[2 * i + 1] = node_index * 2 + 1;
    }

    assembly_accumulate_force(T3_TOTAL_DOF, dof_map, fe);
    return FEM_SUCCESS;
}

static fem_error_t assembly_apply_body_force_q4(int element_id)
{
    double fe[Q4_TOTAL_DOF] = {0.0};
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
            fe[2 * i]     += N[i] * g_body_force[0] * scale;
            fe[2 * i + 1] += N[i] * g_body_force[1] * scale;
        }
    }

    int dof_map[Q4_TOTAL_DOF];
    for (int i = 0; i < Q4_NODES_PER_ELEMENT; i++) {
        int node_index = g_element_nodes[element_id][i];
        dof_map[2 * i]     = node_index * 2;
        dof_map[2 * i + 1] = node_index * 2 + 1;
    }

    assembly_accumulate_force(Q4_TOTAL_DOF, dof_map, fe);
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
    double fe_local[MAX_SURFACE_NODES * 2] = {0.0};

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

    const double tx = g_traction_values[surface_index][0];
    const double ty = g_traction_values[surface_index][1];

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
            fe_local[2 * i]     += N[i] * tx * scaled_weight;
            fe_local[2 * i + 1] += N[i] * ty * scaled_weight;
        }
    }

    int dof_map[MAX_SURFACE_NODES * 2];
    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        dof_map[2 * i]     = node_indices[i] * 2;
        dof_map[2 * i + 1] = node_indices[i] * 2 + 1;
    }

    assembly_accumulate_force(MAX_SURFACE_NODES * 2, dof_map, fe_local);
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
    int node_indices[MAX_SURFACE_NODES];
    double coords[MAX_SURFACE_NODES][2];
    double fe_local[MAX_SURFACE_NODES * 2] = {0.0};

    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        node_indices[i] = g_pressure_surfaces[surface_index][i];
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
        double pressure = g_pressure_value;
        double px = -pressure * nx;
        double py = -pressure * ny;

        double scaled_weight = weight * jacobian;

        for (int i = 0; i < MAX_SURFACE_NODES; i++) {
            fe_local[2 * i]     += N[i] * px * scaled_weight;
            fe_local[2 * i + 1] += N[i] * py * scaled_weight;
        }
    }

    int dof_map[MAX_SURFACE_NODES * 2];
    for (int i = 0; i < MAX_SURFACE_NODES; i++) {
        dof_map[2 * i]     = node_indices[i] * 2;
        dof_map[2 * i + 1] = node_indices[i] * 2 + 1;
    }

    assembly_accumulate_force(MAX_SURFACE_NODES * 2, dof_map, fe_local);
    return FEM_SUCCESS;
}
