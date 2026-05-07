/* FEM4C - Static Analysis Implementation
 * Linear static structural analysis
 */

#include "static.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include "../io/input.h"
#include "../io/output.h"
#include "../fem/assembly/assembly.h"
#include "../numerics/cg/cg_solver.h"
#include "../fem/element/t6/t6_stiffness.h"
#include "../fem/element/t3/t3_element.h"
#include "../fem/element/q4/q4_element.h"
#include "../fem/element/elements.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    int node_indices[MAX_SURFACE_NODES];
    int node_ids[MAX_SURFACE_NODES];
    int node_count;
    int segment_id;
} fem_contact_surface_edge_t;

typedef struct {
    int load_step;
    int pair_id;
    int slave_part_id;
    int master_part_id;
    int slave_surface_id;
    int master_surface_id;
    int slave_node_id;
    int master_segment_id;
    int active_flag;
    double gap_m;
    double penetration_m;
    double fn_n;
    int friction_active_flag;
    double ut_rel_m;
    double ft_t_n;
    double ft_trial_n;
    double ft_cap_n;
    char stick_slip_state[16];
    double mu_cap_used;
    double k_t_pen_used;
    double u_t_reg_m_used;
    int adhesion_active_flag;
    double fn_adh_n;
    double closest_x;
    double closest_y;
    double contact_x;
    double contact_y;
    double normal_x;
    double normal_y;
    double tangent_x;
    double tangent_y;
    double k_pen_base;
    double gamma_n_used;
    double k_pen_used;
    double k_adh_n_used;
    double gap_adh_max_m_used;
    char feedback_status[32];
    char feedback_source[16];
} fem_contact_generic_trace_row_t;

typedef struct {
    int load_step;
    int pair_id;
    double gamma_n;
    double delta_g_eff_m;
    double fn_ref_n;
    double p_max_pa;
    int valid_flag;
    char status[64];
} fem_contact_generic_feedback_row_t;

static fem_contact_generic_trace_row_t *g_fem_contact_generic_trace_rows = NULL;
static int g_fem_contact_generic_trace_count = 0;
static int g_fem_contact_generic_trace_capacity = 0;

static int static_fem_contact_generic_enabled(void);
static int static_fem_local_feedback_enabled(void);
static void static_reset_fem_contact_generic_trace(void);
static fem_error_t static_reserve_fem_contact_generic_trace_rows(int required);
static fem_error_t static_append_fem_contact_generic_trace_row(
    const fem_contact_generic_trace_row_t *row);
static int static_parse_csv_int_tokens(const char *line, int *values, int max_values);
static void static_trim_text(char *text);
static int static_split_csv_tokens(char *line, char **tokens, int max_tokens);
static int static_parse_strict_int(const char *text, int *value_out);
static int static_parse_strict_double(const char *text, double *value_out);
static int static_feedback_status_is_good(const char *status_text);
static fem_error_t static_load_fem_contact_generic_feedback_rows(
    fem_contact_generic_feedback_row_t **rows_out,
    int *row_count_out);
static const fem_contact_generic_feedback_row_t *static_find_fem_contact_generic_feedback_row(
    const fem_contact_generic_feedback_row_t *rows,
    int row_count,
    int load_step,
    int pair_id);
static fem_error_t static_load_fem_contact_surface_edges(
    int surface_slot,
    fem_contact_surface_edge_t **edges_out,
    int *edge_count_out);
static fem_error_t static_collect_unique_surface_nodes(
    const fem_contact_surface_edge_t *edges,
    int edge_count,
    int **node_indices_out,
    int *node_count_out);
static int static_find_fem_contact_generic_surface_index(int surface_id);
static void static_get_node_xy_from_displ(int node_index,
                                          const double *displ,
                                          double xy[2]);
static fem_error_t static_build_fem_contact_generic_state(const double *displ,
                                                          int load_step,
                                                          int accumulate_forces,
                                                          int record_trace,
                                                          int reset_trace_before_recording,
                                                          int *active_rows_out,
                                                          const fem_contact_generic_feedback_row_t *feedback_rows,
                                                          int feedback_row_count);
static void static_reset_fem_load_scale_bridge(void);
static fem_error_t static_compute_newmark_average_acceleration_coeffs(double dt,
                                                                      double *a0_out,
                                                                      double *a2_out,
                                                                      double *a3_out);
static fem_error_t static_build_implicit_oneway_predictor(const double *u_n,
                                                          const double *v_n,
                                                          const double *a_n,
                                                          double dt,
                                                          double *u_trial,
                                                          int vector_size);
static fem_error_t static_build_implicit_oneway_history_vector(const double *u_n,
                                                               const double *v_n,
                                                               const double *a_n,
                                                               double a0,
                                                               double a2,
                                                               double a3,
                                                               double *history_vector,
                                                               int vector_size);
static fem_error_t static_apply_implicit_oneway_corrector(const double *u_np1,
                                                          const double *u_n,
                                                          const double *v_n,
                                                          const double *a_n,
                                                          double dt,
                                                          double a0,
                                                          double a2,
                                                          double a3,
                                                          double *v_np1,
                                                          double *a_np1,
                                                          int vector_size);
static double static_get_prescribed_displacement_for_dof(int dof_index);
static void static_apply_prescribed_displacements_to_state(double *u_vec,
                                                           double *v_vec,
                                                           double *a_vec,
                                                           int vector_size);
static fem_error_t static_solve_explicit_oneway_first_cut(void);
static fem_error_t static_expand_fem_load_scale_schedule(double *scale_by_step,
                                                         int num_time_steps);
static fem_error_t static_solve_implicit_oneway_first_cut(void);
static fem_error_t static_solve_fem_contact_generic_mvp(void);
static fem_error_t static_write_fem_contact_generic_trace_csv(
    const char *output_filename);
static fem_error_t static_write_fem_contact_generic_replay_use_csv(
    const char *output_filename);

/* Main static analysis function */
fem_error_t static_analysis(const char* input_filename, const char* output_filename)
{
    fem_error_t err;
    clock_t start_time, end_time;
    
    printf("FEM4C Static Analysis\n");
    printf("====================\n\n");
    
    start_time = clock();
    
    /* Initialize analysis */
    err = static_analysis_initialize();
    CHECK_ERROR(err);
    
    /* Preprocessing phase */
    err = static_analysis_preprocessing(input_filename);
    CHECK_ERROR(err);
    
    /* Solution phase */
    err = static_analysis_solve();
    CHECK_ERROR(err);
    
    /* Postprocessing phase */
    err = static_analysis_postprocessing(output_filename);
    CHECK_ERROR(err);
    
    /* Finalize analysis */
    err = static_analysis_finalize();
    CHECK_ERROR(err);
    
    end_time = clock();
    g_solver_info.elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\nStatic Analysis Complete\n");
    printf("========================\n");
    printf("Total elapsed time: %.3f seconds\n", g_solver_info.elapsed_time);
    
    return FEM_SUCCESS;
}

/* Initialize analysis */
fem_error_t static_analysis_initialize(void)
{
    fem_error_t err;
    
    printf("Phase 1: Initialization\n");
    printf("-----------------------\n");
    
    /* Initialize global variables */
    err = globals_initialize();
    CHECK_ERROR(err);
    
    /* Initialize element management system */
    err = elements_initialize();
    CHECK_ERROR(err);

    /* Legacy T6 initialization for compatibility */
    err = t6_initialize();
    CHECK_ERROR(err);
    
    printf("  System initialized successfully\n\n");
    return FEM_SUCCESS;
}

/* Preprocessing phase */
fem_error_t static_analysis_preprocessing(const char* input_filename)
{
    fem_error_t err;
    
    printf("Phase 2: Preprocessing\n");
    printf("----------------------\n");
    
    /* Read input data */
    printf("  Reading input file: %s\n", input_filename);
    err = input_read_data(input_filename);
    CHECK_ERROR(err);
    
    /* Validate input */
    err = static_validate_input();
    CHECK_ERROR(err);
    
    /* Print problem summary */
    printf("  Problem summary:\n");
    printf("    Title: %s\n", g_analysis.title);
    printf("    Nodes: %d\n", g_num_nodes);
    printf("    Elements: %d\n", g_num_elements);
    printf("    Materials: %d\n", g_num_materials);
    printf("    DOF: %d\n", g_total_dof);
    
    printf("  Preprocessing completed successfully\n\n");
    return FEM_SUCCESS;
}

/* Solution phase */
fem_error_t static_analysis_solve(void)
{
    fem_error_t err;

    printf("Phase 3: Solution\n");
    printf("-----------------\n");

    if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_IMPLICIT_ONEWAY_NEWMARK) {
        err = static_solve_implicit_oneway_first_cut();
        CHECK_ERROR(err);
    }

    else if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE) {
        err = static_solve_explicit_oneway_first_cut();
        CHECK_ERROR(err);
    }

    else if (static_fem_contact_generic_enabled()) {
        err = static_solve_fem_contact_generic_mvp();
        CHECK_ERROR(err);
    } else {
        /* Assemble system */
        err = static_assemble_system();
        CHECK_ERROR(err);

        /* Solve equations */
        err = static_solve_equations();
        CHECK_ERROR(err);
    }
    
    printf("  Solution phase completed successfully\n\n");
    return FEM_SUCCESS;
}

static void static_reset_fem_load_scale_bridge(void)
{
    g_fem_static_current_load_step = 0;
    g_fem_static_current_load_scale = 1.0;
}

static fem_error_t static_compute_newmark_average_acceleration_coeffs(double dt,
                                                                      double *a0_out,
                                                                      double *a2_out,
                                                                      double *a3_out)
{
    const double beta = 0.25;
    const double gamma = 0.5;

    (void)gamma;
    CHECK_NULL(a0_out, "Newmark a0 output");
    CHECK_NULL(a2_out, "Newmark a2 output");
    CHECK_NULL(a3_out, "Newmark a3 output");

    if (dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "implicit one-way first cut: FEM_TIME_STEP_DT must be positive");
    }

    *a0_out = 1.0 / (beta * dt * dt);
    *a2_out = 1.0 / (beta * dt);
    *a3_out = 1.0 / (2.0 * beta) - 1.0;
    return FEM_SUCCESS;
}

static fem_error_t static_build_implicit_oneway_predictor(const double *u_n,
                                                          const double *v_n,
                                                          const double *a_n,
                                                          double dt,
                                                          double *u_trial,
                                                          int vector_size)
{
    const double beta = 0.25;

    CHECK_NULL(u_n, "implicit one-way committed displacement");
    CHECK_NULL(v_n, "implicit one-way committed velocity");
    CHECK_NULL(a_n, "implicit one-way committed acceleration");
    CHECK_NULL(u_trial, "implicit one-way trial displacement");

    for (int i = 0; i < vector_size; ++i) {
        u_trial[i] = u_n[i] + dt * v_n[i] + dt * dt * (0.5 - beta) * a_n[i];
    }

    return FEM_SUCCESS;
}

static fem_error_t static_build_implicit_oneway_history_vector(const double *u_n,
                                                               const double *v_n,
                                                               const double *a_n,
                                                               double a0,
                                                               double a2,
                                                               double a3,
                                                               double *history_vector,
                                                               int vector_size)
{
    CHECK_NULL(u_n, "implicit one-way committed displacement");
    CHECK_NULL(v_n, "implicit one-way committed velocity");
    CHECK_NULL(a_n, "implicit one-way committed acceleration");
    CHECK_NULL(history_vector, "implicit one-way history vector");

    for (int i = 0; i < vector_size; ++i) {
        history_vector[i] = a0 * u_n[i] + a2 * v_n[i] + a3 * a_n[i];
    }

    return FEM_SUCCESS;
}

static fem_error_t static_apply_implicit_oneway_corrector(const double *u_np1,
                                                          const double *u_n,
                                                          const double *v_n,
                                                          const double *a_n,
                                                          double dt,
                                                          double a0,
                                                          double a2,
                                                          double a3,
                                                          double *v_np1,
                                                          double *a_np1,
                                                          int vector_size)
{
    const double gamma = 0.5;

    CHECK_NULL(u_np1, "implicit one-way converged displacement");
    CHECK_NULL(u_n, "implicit one-way committed displacement");
    CHECK_NULL(v_n, "implicit one-way committed velocity");
    CHECK_NULL(a_n, "implicit one-way committed acceleration");
    CHECK_NULL(v_np1, "implicit one-way updated velocity");
    CHECK_NULL(a_np1, "implicit one-way updated acceleration");

    for (int i = 0; i < vector_size; ++i) {
        a_np1[i] = a0 * (u_np1[i] - u_n[i]) - a2 * v_n[i] - a3 * a_n[i];
        v_np1[i] = v_n[i] + dt * ((1.0 - gamma) * a_n[i] + gamma * a_np1[i]);
    }

    return FEM_SUCCESS;
}

static double static_get_prescribed_displacement_for_dof(int dof_index)
{
    int node_index = 0;
    int local_dof = 0;

    if (dof_index < 0 || g_fem_dof_per_node <= 0) {
        return 0.0;
    }

    node_index = dof_index / g_fem_dof_per_node;
    local_dof = dof_index % g_fem_dof_per_node;
    if (node_index < 0 || node_index >= g_num_nodes || local_dof < 0 || local_dof >= 3) {
        return 0.0;
    }

    return g_node_bc_values[node_index][local_dof] * g_fem_static_current_load_scale;
}

static void static_apply_prescribed_displacements_to_state(double *u_vec,
                                                           double *v_vec,
                                                           double *a_vec,
                                                           int vector_size)
{
    for (int dof = 0; dof < vector_size; ++dof) {
        int node_index = 0;
        int local_dof = 0;
        double prescribed = 0.0;

        if (g_fem_dof_per_node <= 0) {
            break;
        }

        node_index = dof / g_fem_dof_per_node;
        local_dof = dof % g_fem_dof_per_node;
        if (node_index < 0 || node_index >= g_num_nodes || local_dof < 0 || local_dof >= 3) {
            continue;
        }
        if (!g_node_bc_flags[node_index][local_dof]) {
            continue;
        }

        prescribed = static_get_prescribed_displacement_for_dof(dof);
        if (u_vec) {
            u_vec[dof] = prescribed;
        }
        if (v_vec) {
            v_vec[dof] = 0.0;
        }
        if (a_vec) {
            a_vec[dof] = 0.0;
        }
    }
}

static fem_error_t static_solve_explicit_oneway_first_cut(void)
{
    fem_error_t err = FEM_SUCCESS;
    double *u_nm1 = NULL;
    double *u_n = NULL;
    double *u_np1 = NULL;
    double *v_n = NULL;
    double *a_n = NULL;
    double *mass_diag = NULL;
    double *internal_force = NULL;
    double *residual_force = NULL;
    double dt = g_analysis.time_step_dt;
    int num_time_steps = g_analysis.num_time_steps;

    printf("  FEM explicit one-way first cut driver\n");
    printf("    integrator: EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE\n");
    printf("    time_step_count: %d\n", num_time_steps);

    u_nm1 = calloc((size_t)g_total_dof, sizeof(double));
    u_n = calloc((size_t)g_total_dof, sizeof(double));
    u_np1 = calloc((size_t)g_total_dof, sizeof(double));
    v_n = calloc((size_t)g_total_dof, sizeof(double));
    a_n = calloc((size_t)g_total_dof, sizeof(double));
    mass_diag = calloc((size_t)g_total_dof, sizeof(double));
    internal_force = calloc((size_t)g_total_dof, sizeof(double));
    residual_force = calloc((size_t)g_total_dof, sizeof(double));
    if (!u_nm1 || !u_n || !u_np1 || !v_n || !a_n || !mass_diag || !internal_force || !residual_force) {
        free(u_nm1);
        free(u_n);
        free(u_np1);
        free(v_n);
        free(a_n);
        free(mass_diag);
        free(internal_force);
        free(residual_force);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "explicit one-way first cut: local state allocation failed");
    }

    err = assembly_build_lumped_mass_diagonal(mass_diag);
    CHECK_ERROR_CLEANUP(err,
                        free(u_nm1);
                        free(u_n);
                        free(u_np1);
                        free(v_n);
                        free(a_n);
                        free(mass_diag);
                        free(internal_force);
                        free(residual_force));

    g_solver_info.friction_active_row_count = 0;
    g_solver_info.friction_stick_row_count = 0;
    g_solver_info.friction_slip_row_count = 0;
    g_solver_info.friction_max_abs_ft_t_n = 0.0;
    g_solver_info.friction_max_abs_ut_rel_m = 0.0;

    g_fem_static_current_load_scale = 1.0;
    static_apply_prescribed_displacements_to_state(u_nm1, v_n, a_n, g_total_dof);
    static_apply_prescribed_displacements_to_state(u_n, v_n, a_n, g_total_dof);

    for (int step_index = 0; step_index < num_time_steps; ++step_index) {
        int active_rows = 0;

        g_fem_static_current_load_step = step_index;
        g_fem_static_current_load_scale = 1.0;

        err = assembly_global_stiffness_matrix();
        CHECK_ERROR_CLEANUP(err,
                            static_reset_fem_load_scale_bridge();
                            free(u_nm1);
                            free(u_n);
                            free(u_np1);
                            free(v_n);
                            free(a_n);
                            free(mass_diag);
                            free(internal_force);
                            free(residual_force));

        err = assembly_global_force_vector();
        CHECK_ERROR_CLEANUP(err,
                            static_reset_fem_load_scale_bridge();
                            free(u_nm1);
                            free(u_n);
                            free(u_np1);
                            free(v_n);
                            free(a_n);
                            free(mass_diag);
                            free(internal_force);
                            free(residual_force));

        if (static_fem_contact_generic_enabled()) {
            err = static_build_fem_contact_generic_state(u_n,
                                                         step_index,
                                                         1,
                                                         1,
                                                         step_index == 0,
                                                         &active_rows,
                                                         NULL,
                                                         0);
            CHECK_ERROR_CLEANUP(err,
                                static_reset_fem_load_scale_bridge();
                                free(u_nm1);
                                free(u_n);
                                free(u_np1);
                                free(v_n);
                                free(a_n);
                                free(mass_diag);
                                free(internal_force);
                                free(residual_force));
        }

        err = cg_matrix_vector_multiply(NULL, u_n, internal_force, g_total_dof);
        CHECK_ERROR_CLEANUP(err,
                            static_reset_fem_load_scale_bridge();
                            free(u_nm1);
                            free(u_n);
                            free(u_np1);
                            free(v_n);
                            free(a_n);
                            free(mass_diag);
                            free(internal_force);
                            free(residual_force));

        for (int dof = 0; dof < g_total_dof; ++dof) {
            int node_index = dof / g_fem_dof_per_node;
            int local_dof = dof % g_fem_dof_per_node;

            residual_force[dof] = g_global_force[dof] - internal_force[dof];
            if (node_index >= 0 &&
                node_index < g_num_nodes &&
                local_dof >= 0 &&
                local_dof < 3 &&
                g_node_bc_flags[node_index][local_dof]) {
                residual_force[dof] = 0.0;
                a_n[dof] = 0.0;
                u_np1[dof] = static_get_prescribed_displacement_for_dof(dof);
                continue;
            }

            if (mass_diag[dof] <= 0.0) {
                static_reset_fem_load_scale_bridge();
                free(u_nm1);
                free(u_n);
                free(u_np1);
                free(v_n);
                free(a_n);
                free(mass_diag);
                free(internal_force);
                free(residual_force);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "explicit one-way first cut: non-positive lumped mass at dof %d",
                                 dof + 1);
            }

            a_n[dof] = residual_force[dof] / mass_diag[dof];
            u_np1[dof] = 2.0 * u_n[dof] - u_nm1[dof] + dt * dt * a_n[dof];
        }

        for (int dof = 0; dof < g_total_dof; ++dof) {
            v_n[dof] = (u_np1[dof] - u_nm1[dof]) / (2.0 * dt);
        }

        static_apply_prescribed_displacements_to_state(u_np1, v_n, a_n, g_total_dof);

        if (g_global_displ) {
            memcpy(g_global_displ, u_np1, (size_t)g_total_dof * sizeof(double));
        }
        if (g_node_displ) {
            for (int node_index = 0; node_index < g_num_nodes; ++node_index) {
                int base = node_index * g_fem_dof_per_node;
                g_node_displ[node_index][0] = u_np1[base];
                g_node_displ[node_index][1] = u_np1[base + 1];
                g_node_displ[node_index][2] = 0.0;
            }
        }

        memcpy(u_nm1, u_n, (size_t)g_total_dof * sizeof(double));
        memcpy(u_n, u_np1, (size_t)g_total_dof * sizeof(double));

        g_solver_info.step_index = step_index;
        g_solver_info.step_dt = dt;
        g_solver_info.step_load_scale = 1.0;
        g_solver_info.step_outer_iterations = 0;
        g_solver_info.step_linear_iterations = 0;
        g_solver_info.step_retry_count = 0;
        g_solver_info.step_converged = 1;
        g_solver_info.step_converged_after_retry = 0;
        g_solver_info.step_outer_metric = 0.0;
    }

    static_reset_fem_load_scale_bridge();
    free(u_nm1);
    free(u_n);
    free(u_np1);
    free(v_n);
    free(a_n);
    free(mass_diag);
    free(internal_force);
    free(residual_force);
    return FEM_SUCCESS;
}

static fem_error_t static_expand_fem_load_scale_schedule(double *scale_by_step,
                                                         int num_time_steps)
{
    int row_index = 0;
    double current_scale = 1.0;

    CHECK_NULL(scale_by_step, "dense fem load scale schedule");

    if (num_time_steps <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "implicit one-way first cut: FEM_NUM_TIME_STEPS must be positive");
    }

    if (g_num_fem_load_scale_steps <= 0) {
        for (int step_index = 0; step_index < num_time_steps; ++step_index) {
            scale_by_step[step_index] = 1.0;
        }
        return FEM_SUCCESS;
    }

    if (g_fem_load_scale_step_ids[0] != 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOAD_SCALE_STEP requires step 0 when rows are present");
    }

    for (int step_index = 0; step_index < num_time_steps; ++step_index) {
        if (row_index < g_num_fem_load_scale_steps &&
            g_fem_load_scale_step_ids[row_index] == step_index) {
            current_scale = g_fem_load_scale_step_scales[row_index];
            ++row_index;
        }
        scale_by_step[step_index] = current_scale;
    }

    return FEM_SUCCESS;
}

static fem_error_t static_solve_implicit_oneway_first_cut(void)
{
    fem_error_t err = FEM_SUCCESS;
    fem_contact_generic_feedback_row_t *feedback_rows = NULL;
    int feedback_row_count = 0;
    double *scale_by_step = NULL;
    double *u_n = NULL;
    double *v_n = NULL;
    double *a_n = NULL;
    double *v_np1 = NULL;
    double *a_np1 = NULL;
    double *u_trial = NULL;
    double *u_prev_trial = NULL;
    double *history_vector = NULL;
    double base_dt = g_analysis.time_step_dt;
    double dt_used = 0.0;
    double a0 = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    int num_time_steps = g_analysis.num_time_steps;

    printf("  FEM implicit one-way first cut driver\n");
    printf("    integrator: IMPLICIT_ONEWAY_NEWMARK\n");
    printf("    time_step_count: %d\n", num_time_steps);

    scale_by_step = calloc((size_t)num_time_steps, sizeof(double));
    if (!scale_by_step) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "implicit one-way first cut: load schedule allocation failed");
    }

    u_n = calloc((size_t)g_total_dof, sizeof(double));
    v_n = calloc((size_t)g_total_dof, sizeof(double));
    a_n = calloc((size_t)g_total_dof, sizeof(double));
    v_np1 = calloc((size_t)g_total_dof, sizeof(double));
    a_np1 = calloc((size_t)g_total_dof, sizeof(double));
    u_trial = calloc((size_t)g_total_dof, sizeof(double));
    u_prev_trial = calloc((size_t)g_total_dof, sizeof(double));
    history_vector = calloc((size_t)g_total_dof, sizeof(double));
    if (!u_n || !v_n || !a_n || !v_np1 || !a_np1 || !u_trial || !u_prev_trial || !history_vector) {
        free(scale_by_step);
        free(u_n);
        free(v_n);
        free(a_n);
        free(v_np1);
        free(a_np1);
        free(u_trial);
        free(u_prev_trial);
        free(history_vector);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "implicit one-way first cut: local state allocation failed");
    }

    err = static_expand_fem_load_scale_schedule(scale_by_step, num_time_steps);
    CHECK_ERROR_CLEANUP(err,
                        free(scale_by_step);
                        free(u_n);
                        free(v_n);
                        free(a_n);
                        free(v_np1);
                        free(a_np1);
                        free(u_trial);
                        free(u_prev_trial);
                        free(history_vector));

    if (static_fem_contact_generic_enabled()) {
        err = static_load_fem_contact_generic_feedback_rows(&feedback_rows, &feedback_row_count);
        CHECK_ERROR_CLEANUP(err,
                            free(scale_by_step);
                            free(u_n);
                            free(v_n);
                            free(a_n);
                            free(v_np1);
                            free(a_np1);
                            free(u_trial);
                            free(u_prev_trial);
                            free(history_vector));
    }

    for (int step_index = 0; step_index < num_time_steps; ++step_index) {
        int step_converged = 0;
        int retry_count = 0;
        int converged_after_retry = 0;
        int last_outer_iterations = 0;
        int last_linear_iterations = 0;
        double last_outer_metric = 0.0;

        g_fem_static_current_load_step = step_index;
        g_fem_static_current_load_scale = scale_by_step[step_index];

        if (step_index == 0) {
            printf("    implicit one-way first cut driver reached: step=%d load_scale=%e\n",
                   step_index,
                   scale_by_step[step_index]);
        }

        for (retry_count = 0; retry_count <= 1; ++retry_count) {
            int outer_iter_limit = static_fem_contact_generic_enabled() ? g_analysis.outer_max_iterations : 1;

            dt_used = (retry_count == 0) ? base_dt : (base_dt * 0.5);
            if (retry_count > 0) {
                printf("    retrying implicit one-way step=%d with dt-halving: base_dt=%e retry_dt=%e\n",
                       step_index,
                       base_dt,
                       dt_used);
            }

            err = static_compute_newmark_average_acceleration_coeffs(dt_used, &a0, &a2, &a3);
            CHECK_ERROR_CLEANUP(err,
                                static_reset_fem_load_scale_bridge();
                                free(feedback_rows);
                                free(scale_by_step);
                                free(u_n);
                                free(v_n);
                                free(a_n);
                                free(v_np1);
                                free(a_np1);
                                free(u_trial);
                                free(u_prev_trial);
                                free(history_vector));

            err = static_build_implicit_oneway_predictor(u_n,
                                                         v_n,
                                                         a_n,
                                                         dt_used,
                                                         u_trial,
                                                         g_total_dof);
            CHECK_ERROR_CLEANUP(err,
                                static_reset_fem_load_scale_bridge();
                                free(feedback_rows);
                                free(scale_by_step);
                                free(u_n);
                                free(v_n);
                                free(a_n);
                                free(v_np1);
                                free(a_np1);
                                free(u_trial);
                                free(u_prev_trial);
                                free(history_vector));

            for (int outer_iter = 0; outer_iter < outer_iter_limit; ++outer_iter) {
                int active_rows = 0;
                double max_disp_delta = 0.0;

                memcpy(u_prev_trial, u_trial, (size_t)g_total_dof * sizeof(double));

                err = assembly_global_stiffness_matrix();
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = assembly_add_lumped_mass_scaled_to_stiffness(a0);
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = assembly_global_force_vector();
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = static_build_implicit_oneway_history_vector(u_n,
                                                                  v_n,
                                                                  a_n,
                                                                  a0,
                                                                  a2,
                                                                  a3,
                                                                  history_vector,
                                                                  g_total_dof);
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = assembly_add_lumped_mass_times_vector_to_force(history_vector, 1.0);
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                if (static_fem_contact_generic_enabled()) {
                    err = static_build_fem_contact_generic_state(u_trial,
                                                                 step_index,
                                                                 1,
                                                                 0,
                                                                 0,
                                                                 &active_rows,
                                                                 feedback_rows,
                                                                 feedback_row_count);
                    CHECK_ERROR_CLEANUP(err,
                                        static_reset_fem_load_scale_bridge();
                                        free(feedback_rows);
                                        free(scale_by_step);
                                        free(u_n);
                                        free(v_n);
                                        free(a_n);
                                        free(v_np1);
                                        free(a_np1);
                                        free(u_trial);
                                        free(u_prev_trial);
                                        free(history_vector));
                }

                if (g_global_displ) {
                    memcpy(g_global_displ, u_trial, (size_t)g_total_dof * sizeof(double));
                }
                if (g_node_displ) {
                    memcpy(g_node_displ, u_trial, (size_t)g_total_dof * sizeof(double));
                }

                err = assembly_apply_boundary_conditions();
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = assembly_check_matrix_properties();
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                err = cg_solve_system();
                CHECK_ERROR_CLEANUP(err,
                                    static_reset_fem_load_scale_bridge();
                                    free(feedback_rows);
                                    free(scale_by_step);
                                    free(u_n);
                                    free(v_n);
                                    free(a_n);
                                    free(v_np1);
                                    free(a_np1);
                                    free(u_trial);
                                    free(u_prev_trial);
                                    free(history_vector));

                last_linear_iterations = g_solver_info.iterations;

                if (g_global_displ) {
                    memcpy(u_trial, g_global_displ, (size_t)g_total_dof * sizeof(double));
                }

                for (int i = 0; i < g_total_dof; ++i) {
                    double delta = fabs(u_trial[i] - u_prev_trial[i]);
                    if (delta > max_disp_delta) {
                        max_disp_delta = delta;
                    }
                }

                last_outer_metric = max_disp_delta;
                last_outer_iterations = outer_iter + 1;

                if (!static_fem_contact_generic_enabled() || max_disp_delta <= g_analysis.outer_tolerance) {
                    step_converged = 1;
                    converged_after_retry = (retry_count > 0) ? 1 : 0;
                    break;
                }
            }

            if (step_converged) {
                break;
            }
        }

        if (!step_converged) {
            static_reset_fem_load_scale_bridge();
            free(feedback_rows);
            free(scale_by_step);
            free(u_n);
            free(v_n);
            free(a_n);
            free(v_np1);
            free(a_np1);
            free(u_trial);
            free(u_prev_trial);
            free(history_vector);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "implicit one-way first cut outer fixed-point did not converge after dt-halving retry");
        }

        err = static_apply_implicit_oneway_corrector(u_trial,
                                                     u_n,
                                                     v_n,
                                                     a_n,
                                                     dt_used,
                                                     a0,
                                                     a2,
                                                     a3,
                                                     v_np1,
                                                     a_np1,
                                                     g_total_dof);
        CHECK_ERROR_CLEANUP(err,
                            static_reset_fem_load_scale_bridge();
                            free(feedback_rows);
                            free(scale_by_step);
                            free(u_n);
                            free(v_n);
                            free(a_n);
                            free(v_np1);
                            free(a_np1);
                            free(u_trial);
                            free(u_prev_trial);
                            free(history_vector));

        memcpy(u_n, u_trial, (size_t)g_total_dof * sizeof(double));
        memcpy(v_n, v_np1, (size_t)g_total_dof * sizeof(double));
        memcpy(a_n, a_np1, (size_t)g_total_dof * sizeof(double));

        g_solver_info.step_index = step_index;
        g_solver_info.step_dt = dt_used;
        g_solver_info.step_load_scale = scale_by_step[step_index];
        g_solver_info.step_outer_iterations = last_outer_iterations;
        g_solver_info.step_linear_iterations = last_linear_iterations;
        g_solver_info.step_retry_count = retry_count;
        g_solver_info.step_converged = 1;
        g_solver_info.step_converged_after_retry = converged_after_retry;
        g_solver_info.step_outer_metric = last_outer_metric;

        if (static_fem_contact_generic_enabled()) {
            err = static_build_fem_contact_generic_state(u_trial,
                                                         step_index,
                                                         0,
                                                         1,
                                                         0,
                                                         NULL,
                                                         feedback_rows,
                                                         feedback_row_count);
            CHECK_ERROR_CLEANUP(err,
                                static_reset_fem_load_scale_bridge();
                                free(feedback_rows);
                                free(scale_by_step);
                                free(u_n);
                                free(v_n);
                                free(a_n);
                                free(v_np1);
                                free(a_np1);
                                free(u_trial);
                                free(u_prev_trial);
                                free(history_vector));
        }
    }

    static_reset_fem_load_scale_bridge();
    free(feedback_rows);
    free(scale_by_step);
    free(u_n);
    free(v_n);
    free(a_n);
    free(v_np1);
    free(a_np1);
    free(u_trial);
    free(u_prev_trial);
    free(history_vector);

    err = static_check_equilibrium();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Equilibrium check failed\n");
    }

    return FEM_SUCCESS;
}

/* Postprocessing phase */
fem_error_t static_analysis_postprocessing(const char* output_filename)
{
    fem_error_t err;
    
    printf("Phase 4: Postprocessing\n");
    printf("-----------------------\n");
    
    /* Calculate element stresses */
    err = static_calculate_stresses();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Stress calculation failed, continuing...\n");
    }
    
    /* Write results */
    err = static_write_results(output_filename);
    CHECK_ERROR(err);
    
    /* Print solution summary */
    output_print_summary();
    
    printf("  Postprocessing completed successfully\n\n");
    return FEM_SUCCESS;
}

/* Finalize analysis */
fem_error_t static_analysis_finalize(void)
{
    fem_error_t err;
    
    err = globals_finalize();
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Assemble system matrices */
fem_error_t static_assemble_system(void)
{
    fem_error_t err;
    
    printf("  Assembling system matrices...\n");
    
    /* Assemble global stiffness matrix */
#ifdef _OPENMP
    err = assembly_parallel_stiffness_matrix();
#else
    err = assembly_global_stiffness_matrix();
#endif
    CHECK_ERROR(err);
    
    /* Assemble global force vector */
    err = assembly_global_force_vector();
    CHECK_ERROR(err);
    
    /* Apply boundary conditions */
    err = assembly_apply_boundary_conditions();
    CHECK_ERROR(err);
    
    /* Check matrix properties */
    err = assembly_check_matrix_properties();
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Solve system of equations */
fem_error_t static_solve_equations(void)
{
    fem_error_t err;
    
    printf("  Solving system of equations...\n");
    
    /* Solve using conjugate gradient method */
    err = cg_solve_system();
    CHECK_ERROR(err);
    
    /* Check equilibrium */
    err = static_check_equilibrium();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Equilibrium check failed\n");
    }
    
    return FEM_SUCCESS;
}

/* Calculate element stresses */
fem_error_t static_calculate_stresses(void)
{
    fem_error_t err;
    int element_id;
    double stress[T6_STRESS_COMPONENTS];
    
    printf("  Calculating element stresses...\n");
    
    /* Calculate stresses for all elements */
    for (element_id = 0; element_id < g_num_elements; element_id++) {
        if (g_element_type[element_id] == ELEMENT_T6) {
            err = t6_calculate_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for element %d\n", element_id + 1);
                continue;
            }

            /* Store stresses (placeholder - would need global stress storage) */
            /* For now, just validate the calculation worked */
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            err = t3_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for T3 element %d\n", element_id + 1);
                continue;
            }
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            err = q4_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for Q4 element %d\n", element_id + 1);
                continue;
            }
        }
    }
    
    return FEM_SUCCESS;
}

/* Write analysis results */
fem_error_t static_write_results(const char* output_filename)
{
    fem_error_t err;
    char vtk_filename[MAX_FILENAME_LEN];
    char csv_filename[MAX_FILENAME_LEN];
    
    printf("  Writing results to: %s\n", output_filename);

    /* Write CSV export (nodal displacements & element stresses) */
    strcpy(csv_filename, output_filename);
    char *dot_csv = strrchr(csv_filename, '.');
    if (dot_csv) {
        strcpy(dot_csv, ".csv");
    } else {
        strcat(csv_filename, ".csv");
    }
    err = output_export_csv(csv_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: CSV export failed (%s)\n", error_get_string(err));
    }
    
    /* Write standard results */
    err = output_write_results(output_filename);
    CHECK_ERROR(err);

    if (static_fem_contact_generic_enabled()) {
        err = static_write_fem_contact_generic_trace_csv(output_filename);
        CHECK_ERROR(err);
        err = static_write_fem_contact_generic_replay_use_csv(output_filename);
        CHECK_ERROR(err);
    }
    
    /* Create VTK filename */
    strcpy(vtk_filename, output_filename);
    char* dot = strrchr(vtk_filename, '.');
    if (dot) {
        strcpy(dot, ".vtk");
    } else {
        strcat(vtk_filename, ".vtk");
    }
    
    /* Write VTK results */
    printf("  Writing VTK results to: %s\n", vtk_filename);
    err = output_write_vtk_file(vtk_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: VTK output failed, continuing...\n");
    }

    /* Create F06 filename for Nastran-compatible output */
    char f06_filename[MAX_FILENAME_LEN];
    strcpy(f06_filename, output_filename);
    dot = strrchr(f06_filename, '.');
    if (dot) {
        strcpy(dot, ".f06");
    } else {
        strcat(f06_filename, ".f06");
    }

    /* Write F06 results */
    printf("  Writing Nastran F06 results to: %s\n", f06_filename);
    err = output_write_nastran_f06_file(f06_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: F06 output failed, continuing...\n");
    }

    return FEM_SUCCESS;
}

/* Validate input data */
fem_error_t static_validate_input(void)
{
    int element_id;
    fem_error_t err;
    
    printf("  Validating input data...\n");
    
    /* Check problem size */
    if (g_num_nodes <= 0 || g_num_elements <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Invalid problem size");
    }
    
    /* Validate all elements */
    for (element_id = 0; element_id < g_num_elements; element_id++) {
        if (g_element_type[element_id] == ELEMENT_T6) {
            err = t6_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] t6_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT, 
                               "Element validation failed for element %d", element_id + 1);
            }
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            err = t3_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] t3_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT,
                               "Element validation failed for element %d", element_id + 1);
            }
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            err = q4_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] q4_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT,
                               "Element validation failed for element %d", element_id + 1);
            }
        } else {
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                           "Unsupported element type %d in element %d",
                           g_element_type[element_id], element_id + 1);
        }
    }
    
    /* Check material properties */
    for (int i = 0; i < g_num_materials; i++) {
        if (g_material_props[i][0] <= 0.0) {
            return error_set(FEM_ERROR_INVALID_MATERIAL,
                           "Invalid Young's modulus for material %d", i + 1);
        }
        if (g_material_props[i][1] >= 0.5 || g_material_props[i][1] < -1.0) {
            return error_set(FEM_ERROR_INVALID_MATERIAL,
                           "Invalid Poisson's ratio for material %d", i + 1);
        }
    }

    if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_IMPLICIT_ONEWAY_NEWMARK) {
        if (g_fem_dof_per_node != 2) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem implicit one-way first cut: shell / 3 dof node is not supported yet; require 2 dof per node");
        }
    }

    if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE) {
        if (g_fem_dof_per_node != 2) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem explicit one-way first cut: shell / 3 dof node is not supported yet; require 2 dof per node");
        }
        if (g_analysis.spatial_dimension != 2) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem explicit one-way first cut: require 2D problem");
        }
    }

    if (g_num_fem_contact_generic_pairs > 0) {
        if (g_analysis.spatial_dimension != 2 || g_fem_dof_per_node != 2) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic macro penalty solver v1: require 2D with 2 dof per node");
        }
        if (g_fem_local_feedback_mode != FEM_LOCAL_FEEDBACK_MODE_NONE &&
            g_fem_local_feedback_mode != FEM_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic macro penalty solver v1: unsupported FEM_LOCAL_FEEDBACK_MODE=%d",
                             g_fem_local_feedback_mode);
        }
        for (int i = 0; i < g_num_fem_contact_generic_pairs; ++i) {
            if (g_fem_contact_generic_pair_mu[i] != 0.0) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "fem generic macro penalty solver v1: friction not implemented; require mu=0 (pair_id=%d)",
                                 g_fem_contact_generic_pair_ids[i]);
            }
            if (g_fem_contact_generic_pair_c_pen[i] != 0.0) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "fem generic macro penalty solver v1: damping not implemented; require c_pen=0 (pair_id=%d)",
                                 g_fem_contact_generic_pair_ids[i]);
            }
            if ((g_fem_contact_generic_pair_k_adh_n[i] > 0.0) !=
                (g_fem_contact_generic_pair_gap_adh_max_m[i] > 0.0)) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "fem generic macro penalty solver v1: adhesion requires both k_adh_n>0 and gap_adh_max_m>0 (pair_id=%d)",
                                 g_fem_contact_generic_pair_ids[i]);
            }
        }
    }

    printf("    Input validation passed\n");
    return FEM_SUCCESS;
}

static int static_fem_contact_generic_enabled(void)
{
    return g_num_fem_contact_generic_pairs > 0;
}

static int static_fem_local_feedback_enabled(void)
{
    return static_fem_contact_generic_enabled() &&
           g_fem_local_feedback_mode == FEM_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED &&
           g_fem_local_contact_file[0] != '\0';
}

static void static_reset_fem_contact_generic_trace(void)
{
    g_fem_contact_generic_trace_count = 0;
    g_solver_info.friction_active_row_count = 0;
    g_solver_info.friction_stick_row_count = 0;
    g_solver_info.friction_slip_row_count = 0;
    g_solver_info.friction_max_abs_ft_t_n = 0.0;
    g_solver_info.friction_max_abs_ut_rel_m = 0.0;
}

static fem_error_t static_reserve_fem_contact_generic_trace_rows(int required)
{
    fem_contact_generic_trace_row_t *tmp = NULL;
    int new_capacity = 0;

    if (required <= g_fem_contact_generic_trace_capacity) {
        return FEM_SUCCESS;
    }

    new_capacity = g_fem_contact_generic_trace_capacity > 0
        ? g_fem_contact_generic_trace_capacity
        : 32;
    while (new_capacity < required) {
        new_capacity *= 2;
    }

    tmp = realloc(g_fem_contact_generic_trace_rows,
                  (size_t)new_capacity * sizeof(*g_fem_contact_generic_trace_rows));
    CHECK_NULL(tmp, "FEM generic contact trace allocation failed");
    g_fem_contact_generic_trace_rows = tmp;
    g_fem_contact_generic_trace_capacity = new_capacity;
    return FEM_SUCCESS;
}

static fem_error_t static_append_fem_contact_generic_trace_row(
    const fem_contact_generic_trace_row_t *row)
{
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(row, "fem generic contact trace row");
    err = static_reserve_fem_contact_generic_trace_rows(g_fem_contact_generic_trace_count + 1);
    CHECK_ERROR(err);
    g_fem_contact_generic_trace_rows[g_fem_contact_generic_trace_count++] = *row;
    if (row->friction_active_flag) {
        g_solver_info.friction_active_row_count++;
        if (strcmp(row->stick_slip_state, "stick") == 0) {
            g_solver_info.friction_stick_row_count++;
        } else if (strcmp(row->stick_slip_state, "slip") == 0) {
            g_solver_info.friction_slip_row_count++;
        }
    }
    if (fabs(row->ft_t_n) > g_solver_info.friction_max_abs_ft_t_n) {
        g_solver_info.friction_max_abs_ft_t_n = fabs(row->ft_t_n);
    }
    if (fabs(row->ut_rel_m) > g_solver_info.friction_max_abs_ut_rel_m) {
        g_solver_info.friction_max_abs_ut_rel_m = fabs(row->ut_rel_m);
    }
    return FEM_SUCCESS;
}

static int static_parse_csv_int_tokens(const char *line, int *values, int max_values)
{
    char buffer[512];
    char *token = NULL;
    int count = 0;

    if (!line || !values || max_values <= 0) {
        return -1;
    }

    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    token = strtok(buffer, ", \t\r\n");
    while (token && count < max_values) {
        char *end_ptr = NULL;
        long value = 0;

        value = strtol(token, &end_ptr, 10);
        if (end_ptr == token || *end_ptr != '\0') {
            return -1;
        }
        values[count++] = (int)value;
        token = strtok(NULL, ", \t\r\n");
    }

    if (token != NULL) {
        return -1;
    }
    return count;
}

static void static_trim_text(char *text)
{
    size_t len = 0;
    char *start = text;

    if (!text) {
        return;
    }

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }
}

static int static_split_csv_tokens(char *line, char **tokens, int max_tokens)
{
    int count = 0;
    char *cursor = line;

    if (!line || !tokens || max_tokens <= 0) {
        return -1;
    }

    while (cursor && *cursor) {
        char *next = strchr(cursor, ',');
        if (count >= max_tokens) {
            return -1;
        }
        if (next) {
            *next = '\0';
        }
        static_trim_text(cursor);
        tokens[count++] = cursor;
        cursor = next ? (next + 1) : NULL;
    }

    return count;
}

static int static_parse_strict_int(const char *text, int *value_out)
{
    char *end_ptr = NULL;
    long value = 0;

    if (!text || !value_out) {
        return 0;
    }
    value = strtol(text, &end_ptr, 10);
    if (end_ptr == text || *end_ptr != '\0') {
        return 0;
    }
    *value_out = (int)value;
    return 1;
}

static int static_parse_strict_double(const char *text, double *value_out)
{
    char *end_ptr = NULL;
    double value = 0.0;

    if (!text || !value_out) {
        return 0;
    }
    value = strtod(text, &end_ptr);
    if (end_ptr == text || *end_ptr != '\0' || !isfinite(value)) {
        return 0;
    }
    *value_out = value;
    return 1;
}

static int static_feedback_status_is_good(const char *status_text)
{
    char lowered[64];
    size_t len = 0;

    if (!status_text) {
        return 0;
    }

    len = strlen(status_text);
    if (len >= sizeof(lowered)) {
        len = sizeof(lowered) - 1;
    }
    for (size_t i = 0; i < len; ++i) {
        lowered[i] = (char)tolower((unsigned char)status_text[i]);
    }
    lowered[len] = '\0';

    return strcmp(lowered, "ok") == 0 ||
           strcmp(lowered, "valid") == 0 ||
           strcmp(lowered, "1") == 0 ||
           strncmp(lowered, "ok_", 3) == 0 ||
           strncmp(lowered, "ok-", 3) == 0;
}

static fem_error_t static_load_fem_contact_generic_feedback_rows(
    fem_contact_generic_feedback_row_t **rows_out,
    int *row_count_out)
{
    FILE *fp = NULL;
    char line[1024];
    char *tokens[32];
    int token_count = 0;
    int load_step_col = -1;
    int pair_id_col = -1;
    int gamma_n_col = -1;
    int valid_flag_col = -1;
    int status_col = -1;
    int delta_g_eff_col = -1;
    int fn_ref_col = -1;
    int p_max_col = -1;
    fem_contact_generic_feedback_row_t *rows = NULL;
    int row_count = 0;
    int row_capacity = 0;

    CHECK_NULL(rows_out, "fem feedback rows out");
    CHECK_NULL(row_count_out, "fem feedback row count out");

    *rows_out = NULL;
    *row_count_out = 0;

    if (!static_fem_local_feedback_enabled()) {
        return FEM_SUCCESS;
    }

    fp = fopen(g_fem_local_contact_file, "r");
    CHECK_FILE(fp, g_fem_local_contact_file);

    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem generic replay feedback CSV is empty: %s",
                         g_fem_local_contact_file);
    }

    static_trim_text(line);
    token_count = static_split_csv_tokens(line, tokens, 32);
    if (token_count <= 0) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem generic replay feedback CSV has invalid header: %s",
                         g_fem_local_contact_file);
    }
    for (int i = 0; i < token_count; ++i) {
        if (strcmp(tokens[i], "load_step") == 0) {
            load_step_col = i;
        } else if (strcmp(tokens[i], "pair_id") == 0) {
            pair_id_col = i;
        } else if (strcmp(tokens[i], "gamma_n") == 0) {
            gamma_n_col = i;
        } else if (strcmp(tokens[i], "valid_flag") == 0) {
            valid_flag_col = i;
        } else if (strcmp(tokens[i], "status") == 0) {
            status_col = i;
        } else if (strcmp(tokens[i], "delta_g_eff_m") == 0) {
            delta_g_eff_col = i;
        } else if (strcmp(tokens[i], "fn_ref_n") == 0) {
            fn_ref_col = i;
        } else if (strcmp(tokens[i], "p_max_pa") == 0) {
            p_max_col = i;
        }
    }
    if (load_step_col < 0 || pair_id_col < 0 || gamma_n_col < 0 ||
        valid_flag_col < 0 || status_col < 0) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem generic replay feedback CSV missing required columns: %s",
                         g_fem_local_contact_file);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        fem_contact_generic_feedback_row_t row;
        int line_token_count = 0;

        if (strchr(line, '#')) {
            *strchr(line, '#') = '\0';
        }
        static_trim_text(line);
        if (line[0] == '\0') {
            continue;
        }

        line_token_count = static_split_csv_tokens(line, tokens, 32);
        if (line_token_count != token_count) {
            free(rows);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic replay feedback CSV row has wrong column count: %s",
                             g_fem_local_contact_file);
        }

        memset(&row, 0, sizeof(row));
        row.delta_g_eff_m = 0.0;
        row.fn_ref_n = 0.0;
        row.p_max_pa = 0.0;
        if (!static_parse_strict_int(tokens[load_step_col], &row.load_step) ||
            row.load_step < 0 ||
            !static_parse_strict_int(tokens[pair_id_col], &row.pair_id) ||
            row.pair_id < 0 ||
            !static_parse_strict_double(tokens[gamma_n_col], &row.gamma_n) ||
            row.gamma_n <= 0.0 ||
            !static_parse_strict_int(tokens[valid_flag_col], &row.valid_flag) ||
            (row.valid_flag != 0 && row.valid_flag != 1) ||
            tokens[status_col][0] == '\0') {
            free(rows);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic replay feedback CSV has invalid required values: %s",
                             g_fem_local_contact_file);
        }
        if (delta_g_eff_col >= 0 &&
            !static_parse_strict_double(tokens[delta_g_eff_col], &row.delta_g_eff_m)) {
            free(rows);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic replay feedback CSV has invalid delta_g_eff_m: %s",
                             g_fem_local_contact_file);
        }
        if (fn_ref_col >= 0 &&
            !static_parse_strict_double(tokens[fn_ref_col], &row.fn_ref_n)) {
            free(rows);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic replay feedback CSV has invalid fn_ref_n: %s",
                             g_fem_local_contact_file);
        }
        if (p_max_col >= 0 &&
            !static_parse_strict_double(tokens[p_max_col], &row.p_max_pa)) {
            free(rows);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "fem generic replay feedback CSV has invalid p_max_pa: %s",
                             g_fem_local_contact_file);
        }
        snprintf(row.status, sizeof(row.status), "%s", tokens[status_col]);

        for (int i = 0; i < row_count; ++i) {
            if (rows[i].load_step == row.load_step && rows[i].pair_id == row.pair_id) {
                free(rows);
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "fem generic replay feedback CSV has duplicate key load_step=%d pair_id=%d: %s",
                                 row.load_step,
                                 row.pair_id,
                                 g_fem_local_contact_file);
            }
        }

        if (row_count == row_capacity) {
            int new_capacity = row_capacity > 0 ? row_capacity * 2 : 8;
            fem_contact_generic_feedback_row_t *tmp =
                realloc(rows, (size_t)new_capacity * sizeof(*rows));
            if (!tmp) {
                free(rows);
                fclose(fp);
                return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                                 "Failed to allocate FEM generic replay feedback rows");
            }
            rows = tmp;
            row_capacity = new_capacity;
        }
        rows[row_count++] = row;
    }

    fclose(fp);
    *rows_out = rows;
    *row_count_out = row_count;
    return FEM_SUCCESS;
}

static const fem_contact_generic_feedback_row_t *static_find_fem_contact_generic_feedback_row(
    const fem_contact_generic_feedback_row_t *rows,
    int row_count,
    int load_step,
    int pair_id)
{
    if (!rows || row_count <= 0) {
        return NULL;
    }
    for (int i = 0; i < row_count; ++i) {
        if (rows[i].load_step == load_step && rows[i].pair_id == pair_id) {
            return &rows[i];
        }
    }
    return NULL;
}

static fem_error_t static_load_fem_contact_surface_edges(
    int surface_slot,
    fem_contact_surface_edge_t **edges_out,
    int *edge_count_out)
{
    FILE *fp = NULL;
    char line[512];
    fem_contact_surface_edge_t *edges = NULL;
    int edge_capacity = 0;
    int edge_count = 0;

    CHECK_NULL(edges_out, "surface edges out");
    CHECK_NULL(edge_count_out, "surface edge count out");

    if (surface_slot < 0 || surface_slot >= g_num_fem_contact_generic_surfaces) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM generic contact surface slot %d",
                         surface_slot);
    }

    fp = fopen(g_fem_contact_generic_surface_paths[surface_slot], "r");
    CHECK_FILE(fp, g_fem_contact_generic_surface_paths[surface_slot]);

    while (fgets(line, sizeof(line), fp) != NULL) {
        int values[MAX_SURFACE_NODES] = {0, 0, 0};
        int parsed = 0;

        if (strchr(line, '#')) {
            *strchr(line, '#') = '\0';
        }
        while (*line && isspace((unsigned char)*line)) {
            memmove(line, line + 1, strlen(line));
        }
        if (line[0] == '\0') {
            continue;
        }

        parsed = static_parse_csv_int_tokens(line, values, MAX_SURFACE_NODES);
        if (parsed != 2 && parsed != 3) {
            free(edges);
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM generic contact surface CSV row for surface_id=%d",
                             g_fem_contact_generic_surface_ids[surface_slot]);
        }

        if (edge_count == edge_capacity) {
            int new_capacity = edge_capacity > 0 ? edge_capacity * 2 : 8;
            fem_contact_surface_edge_t *tmp = realloc(edges,
                                                      (size_t)new_capacity * sizeof(*edges));
            if (!tmp) {
                free(edges);
                fclose(fp);
                return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                                 "Failed to allocate FEM generic contact surface edges");
            }
            edges = tmp;
            edge_capacity = new_capacity;
        }

        memset(&edges[edge_count], 0, sizeof(edges[edge_count]));
        edges[edge_count].node_count = parsed;
        edges[edge_count].segment_id = edge_count;
        for (int i = 0; i < parsed; ++i) {
            int node_id = values[i];
            if (node_id <= 0 ||
                node_id >= g_node_id_capacity ||
                g_node_id_to_index[node_id] < 0 ||
                g_node_id_to_index[node_id] >= g_num_nodes) {
                free(edges);
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "FEM generic contact surface_id %d references undefined node %d",
                                 g_fem_contact_generic_surface_ids[surface_slot],
                                 node_id);
            }
            edges[edge_count].node_ids[i] = node_id;
            edges[edge_count].node_indices[i] = g_node_id_to_index[node_id];
        }
        edge_count++;
    }

    fclose(fp);

    *edges_out = edges;
    *edge_count_out = edge_count;
    return FEM_SUCCESS;
}

static fem_error_t static_collect_unique_surface_nodes(
    const fem_contact_surface_edge_t *edges,
    int edge_count,
    int **node_indices_out,
    int *node_count_out)
{
    int *nodes = NULL;
    int node_count = 0;
    int node_capacity = 0;

    CHECK_NULL(edges, "surface edges");
    CHECK_NULL(node_indices_out, "unique node indices out");
    CHECK_NULL(node_count_out, "unique node count out");

    for (int edge_idx = 0; edge_idx < edge_count; ++edge_idx) {
        for (int local = 0; local < edges[edge_idx].node_count; ++local) {
            int node_index = edges[edge_idx].node_indices[local];
            int exists = 0;

            for (int i = 0; i < node_count; ++i) {
                if (nodes[i] == node_index) {
                    exists = 1;
                    break;
                }
            }
            if (exists) {
                continue;
            }

            if (node_count == node_capacity) {
                int new_capacity = node_capacity > 0 ? node_capacity * 2 : 8;
                int *tmp = realloc(nodes, (size_t)new_capacity * sizeof(*nodes));
                if (!tmp) {
                    free(nodes);
                    return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                                     "Failed to allocate slave node list");
                }
                nodes = tmp;
                node_capacity = new_capacity;
            }
            nodes[node_count++] = node_index;
        }
    }

    *node_indices_out = nodes;
    *node_count_out = node_count;
    return FEM_SUCCESS;
}

static int static_find_fem_contact_generic_surface_index(int surface_id)
{
    for (int i = 0; i < g_num_fem_contact_generic_surfaces; ++i) {
        if (g_fem_contact_generic_surface_ids[i] == surface_id) {
            return i;
        }
    }
    return -1;
}

static void static_get_node_xy_from_displ(int node_index,
                                          const double *displ,
                                          double xy[2])
{
    int base = node_index * g_fem_dof_per_node;

    xy[0] = g_node_coords[node_index][0] + (displ ? displ[base] : 0.0);
    xy[1] = g_node_coords[node_index][1] + (displ ? displ[base + 1] : 0.0);
}

static fem_error_t static_build_fem_contact_generic_state(const double *displ,
                                                          int load_step,
                                                          int accumulate_forces,
                                                          int record_trace,
                                                          int reset_trace_before_recording,
                                                          int *active_rows_out,
                                                          const fem_contact_generic_feedback_row_t *feedback_rows,
                                                          int feedback_row_count)
{
    int active_rows = 0;

    if (record_trace && reset_trace_before_recording) {
        static_reset_fem_contact_generic_trace();
    }

    for (int pair_slot = 0; pair_slot < g_num_fem_contact_generic_pairs; ++pair_slot) {
        fem_contact_surface_edge_t *slave_edges = NULL;
        fem_contact_surface_edge_t *master_edges = NULL;
        int *slave_nodes = NULL;
        int slave_edge_count = 0;
        int master_edge_count = 0;
        int slave_node_count = 0;
        int surface_i = g_fem_contact_generic_pair_surface_i[pair_slot];
        int surface_j = g_fem_contact_generic_pair_surface_j[pair_slot];
        int pair_id = g_fem_contact_generic_pair_ids[pair_slot];
        int slave_surface_slot = static_find_fem_contact_generic_surface_index(surface_i);
        int master_surface_slot = static_find_fem_contact_generic_surface_index(surface_j);
        const fem_contact_generic_feedback_row_t *feedback_row = NULL;
        double gamma_n_used = 1.0;
        double k_pen_base = g_fem_contact_generic_pair_k_pen[pair_slot];
        double k_pen_used = k_pen_base;
        double k_adh_n_used = g_fem_contact_generic_pair_k_adh_n[pair_slot];
        double gap_adh_max_m_used = g_fem_contact_generic_pair_gap_adh_max_m[pair_slot];
        const char *feedback_status = "none";
        const char *feedback_source = "NONE";
        fem_error_t err = FEM_SUCCESS;

        if (slave_surface_slot < 0 || master_surface_slot < 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM generic contact pair_id=%d references undefined surfaces",
                             pair_id);
        }

        feedback_row = static_find_fem_contact_generic_feedback_row(feedback_rows,
                                                                   feedback_row_count,
                                                                   load_step,
                                                                   pair_id);
        if (feedback_row) {
            if (feedback_row->valid_flag == 1 &&
                static_feedback_status_is_good(feedback_row->status) &&
                isfinite(feedback_row->gamma_n) &&
                feedback_row->gamma_n > 0.0) {
                gamma_n_used = feedback_row->gamma_n;
                k_pen_used = k_pen_base * gamma_n_used;
                feedback_status = feedback_row->status;
                feedback_source = "EXTERNAL";
            } else {
                feedback_status = "invalid_or_bad";
                feedback_source = "FALLBACK";
            }
        }

        err = static_load_fem_contact_surface_edges(slave_surface_slot,
                                                    &slave_edges,
                                                    &slave_edge_count);
        CHECK_ERROR(err);
        err = static_load_fem_contact_surface_edges(master_surface_slot,
                                                    &master_edges,
                                                    &master_edge_count);
        if (err != FEM_SUCCESS) {
            free(slave_edges);
            return err;
        }
        err = static_collect_unique_surface_nodes(slave_edges,
                                                  slave_edge_count,
                                                  &slave_nodes,
                                                  &slave_node_count);
        if (err != FEM_SUCCESS) {
            free(slave_edges);
            free(master_edges);
            return err;
        }

        for (int slave_idx = 0; slave_idx < slave_node_count; ++slave_idx) {
            double slave_xy[2];
            double best_gap = 1.0e300;
            double best_closest_x = 0.0;
            double best_closest_y = 0.0;
            double best_contact_x = 0.0;
            double best_contact_y = 0.0;
            double best_normal[2] = {0.0, 0.0};
            double best_tangent[2] = {0.0, 0.0};
            double best_lambda = 0.0;
            int best_segment_id = -1;
            int slave_node_index = slave_nodes[slave_idx];
            int slave_node_id = g_node_ids ? g_node_ids[slave_node_index] : (slave_node_index + 1);

            static_get_node_xy_from_displ(slave_node_index, displ, slave_xy);

            for (int edge_idx = 0; edge_idx < master_edge_count; ++edge_idx) {
                double p0[2];
                double p1[2];
                double seg[2];
                double seg_len = 0.0;
                double tangent[2];
                double normal[2];
                double ds[2];
                double s = 0.0;
                double lambda = 0.0;
                double closest[2];
                double gap = 0.0;

                if (master_edges[edge_idx].node_count != 2) {
                    free(slave_edges);
                    free(master_edges);
                    free(slave_nodes);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "fem generic macro penalty solver v1: quadratic master edges not implemented; require 2-node edges (pair_id=%d)",
                                     g_fem_contact_generic_pair_ids[pair_slot]);
                }

                static_get_node_xy_from_displ(master_edges[edge_idx].node_indices[0], displ, p0);
                static_get_node_xy_from_displ(master_edges[edge_idx].node_indices[1], displ, p1);
                seg[0] = p1[0] - p0[0];
                seg[1] = p1[1] - p0[1];
                seg_len = sqrt(seg[0] * seg[0] + seg[1] * seg[1]);
                if (seg_len <= 1.0e-14) {
                    const int master_segment_id = master_edges[edge_idx].segment_id;
                    free(slave_edges);
                    free(master_edges);
                    free(slave_nodes);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "fem generic macro penalty solver v1: degenerate master segment (pair_id=%d, master_segment_id=%d)",
                                     g_fem_contact_generic_pair_ids[pair_slot],
                                     master_segment_id);
                }

                tangent[0] = seg[0] / seg_len;
                tangent[1] = seg[1] / seg_len;
                normal[0] = -tangent[1];
                normal[1] = tangent[0];
                ds[0] = slave_xy[0] - p0[0];
                ds[1] = slave_xy[1] - p0[1];
                s = ds[0] * tangent[0] + ds[1] * tangent[1];
                lambda = s / seg_len;
                if (lambda < 0.0) {
                    lambda = 0.0;
                } else if (lambda > 1.0) {
                    lambda = 1.0;
                }
                closest[0] = p0[0] + lambda * seg[0];
                closest[1] = p0[1] + lambda * seg[1];
                gap = (slave_xy[0] - closest[0]) * normal[0] +
                      (slave_xy[1] - closest[1]) * normal[1];

                if (gap < best_gap) {
                    best_gap = gap;
                    best_closest_x = closest[0];
                    best_closest_y = closest[1];
                    best_contact_x = 0.5 * (slave_xy[0] + closest[0]);
                    best_contact_y = 0.5 * (slave_xy[1] + closest[1]);
                    best_normal[0] = normal[0];
                    best_normal[1] = normal[1];
                    best_tangent[0] = tangent[0];
                    best_tangent[1] = tangent[1];
                    best_lambda = lambda;
                    best_segment_id = master_edges[edge_idx].segment_id;
                }
            }

            if (best_segment_id >= 0) {
                fem_contact_generic_trace_row_t row;
                double penetration = best_gap < 0.0 ? -best_gap : 0.0;
                double gap_opening = best_gap > 0.0 ? best_gap : 0.0;
                double fn_contact = k_pen_used * penetration;
                double mu_cap_used = g_fem_contact_generic_pair_mu_cap[pair_slot];
                double k_t_pen_used = g_fem_contact_generic_pair_k_t_pen[pair_slot];
                double u_t_reg_m_used = g_fem_contact_generic_pair_u_t_reg_m[pair_slot];
                double fn_adh = 0.0;
                double fn_total_signed = 0.0;
                double ut_rel = 0.0;
                double ft_trial = 0.0;
                double ft_cap = 0.0;
                double ft_t = 0.0;
                int friction_active_flag = 0;
                const char *stick_slip_state = "none";

                if (k_adh_n_used > 0.0 &&
                    gap_adh_max_m_used > 0.0 &&
                    gap_opening > 0.0 &&
                    gap_opening <= gap_adh_max_m_used) {
                    fn_adh = k_adh_n_used * (gap_adh_max_m_used - gap_opening);
                }
                fn_total_signed = fn_contact - fn_adh;

                if (penetration > 0.0 &&
                    fn_contact > 0.0 &&
                    k_t_pen_used > 0.0 &&
                    u_t_reg_m_used > 0.0) {
                    const fem_contact_surface_edge_t *master_edge = &master_edges[best_segment_id];
                    int base_slave = slave_node_index * g_fem_dof_per_node;
                    int base_master0 = master_edge->node_indices[0] * g_fem_dof_per_node;
                    int base_master1 = master_edge->node_indices[1] * g_fem_dof_per_node;
                    double weight0 = 1.0 - best_lambda;
                    double weight1 = best_lambda;
                    double slave_disp_x = displ ? displ[base_slave] : 0.0;
                    double slave_disp_y = displ ? displ[base_slave + 1] : 0.0;
                    double master_disp_x =
                        weight0 * (displ ? displ[base_master0] : 0.0) +
                        weight1 * (displ ? displ[base_master1] : 0.0);
                    double master_disp_y =
                        weight0 * (displ ? displ[base_master0 + 1] : 0.0) +
                        weight1 * (displ ? displ[base_master1 + 1] : 0.0);
                    double rel_disp_x = slave_disp_x - master_disp_x;
                    double rel_disp_y = slave_disp_y - master_disp_y;

                    ut_rel = rel_disp_x * best_tangent[0] + rel_disp_y * best_tangent[1];
                    ft_trial = k_t_pen_used * ut_rel;
                    ft_cap = mu_cap_used * fn_contact;
                    friction_active_flag = 1;

                    if (fabs(ft_trial) <= ft_cap) {
                        ft_t = ft_trial;
                        stick_slip_state = "stick";
                    } else {
                        ft_t = ft_cap * ut_rel /
                               sqrt(ut_rel * ut_rel + u_t_reg_m_used * u_t_reg_m_used);
                        stick_slip_state = "slip";
                    }
                }

                memset(&row, 0, sizeof(row));
                row.load_step = load_step;
                row.pair_id = pair_id;
                row.slave_part_id = g_fem_contact_generic_surface_part_ids[slave_surface_slot];
                row.master_part_id = g_fem_contact_generic_surface_part_ids[master_surface_slot];
                row.slave_surface_id = surface_i;
                row.master_surface_id = surface_j;
                row.slave_node_id = slave_node_id;
                row.master_segment_id = best_segment_id;
                row.active_flag = penetration > 0.0 ? 1 : 0;
                row.gap_m = best_gap;
                row.penetration_m = penetration;
                row.fn_n = fn_contact;
                row.friction_active_flag = friction_active_flag;
                row.ut_rel_m = ut_rel;
                row.ft_t_n = ft_t;
                row.ft_trial_n = ft_trial;
                row.ft_cap_n = ft_cap;
                snprintf(row.stick_slip_state, sizeof(row.stick_slip_state), "%s", stick_slip_state);
                row.mu_cap_used = mu_cap_used;
                row.k_t_pen_used = k_t_pen_used;
                row.u_t_reg_m_used = u_t_reg_m_used;
                row.adhesion_active_flag = fn_adh > 0.0 ? 1 : 0;
                row.fn_adh_n = fn_adh;
                row.closest_x = best_closest_x;
                row.closest_y = best_closest_y;
                row.contact_x = best_contact_x;
                row.contact_y = best_contact_y;
                row.normal_x = best_normal[0];
                row.normal_y = best_normal[1];
                row.tangent_x = best_tangent[0];
                row.tangent_y = best_tangent[1];
                row.k_pen_base = k_pen_base;
                row.gamma_n_used = gamma_n_used;
                row.k_pen_used = k_pen_used;
                row.k_adh_n_used = k_adh_n_used;
                row.gap_adh_max_m_used = gap_adh_max_m_used;
                snprintf(row.feedback_status, sizeof(row.feedback_status), "%s", feedback_status);
                snprintf(row.feedback_source, sizeof(row.feedback_source), "%s", feedback_source);

                if (row.active_flag) {
                    active_rows++;
                }
                if ((row.active_flag || row.adhesion_active_flag) && accumulate_forces) {
                    int base_slave = slave_node_index * g_fem_dof_per_node;
                    const fem_contact_surface_edge_t *master_edge = &master_edges[best_segment_id];
                    int base_master0 = master_edge->node_indices[0] * g_fem_dof_per_node;
                    int base_master1 = master_edge->node_indices[1] * g_fem_dof_per_node;
                    double weight0 = 1.0 - best_lambda;
                    double weight1 = best_lambda;

                    g_global_force[base_slave] += fn_total_signed * best_normal[0];
                    g_global_force[base_slave + 1] += fn_total_signed * best_normal[1];
                    g_global_force[base_master0] -= weight0 * fn_total_signed * best_normal[0];
                    g_global_force[base_master0 + 1] -= weight0 * fn_total_signed * best_normal[1];
                    g_global_force[base_master1] -= weight1 * fn_total_signed * best_normal[0];
                    g_global_force[base_master1 + 1] -= weight1 * fn_total_signed * best_normal[1];

                    if (friction_active_flag) {
                        g_global_force[base_slave] -= ft_t * best_tangent[0];
                        g_global_force[base_slave + 1] -= ft_t * best_tangent[1];
                        g_global_force[base_master0] += weight0 * ft_t * best_tangent[0];
                        g_global_force[base_master0 + 1] += weight0 * ft_t * best_tangent[1];
                        g_global_force[base_master1] += weight1 * ft_t * best_tangent[0];
                        g_global_force[base_master1 + 1] += weight1 * ft_t * best_tangent[1];
                    }
                }

                if (record_trace) {
                    err = static_append_fem_contact_generic_trace_row(&row);
                    if (err != FEM_SUCCESS) {
                        free(slave_edges);
                        free(master_edges);
                        free(slave_nodes);
                        return err;
                    }
                }
            }
        }

        free(slave_edges);
        free(master_edges);
        free(slave_nodes);
    }

    if (active_rows_out) {
        *active_rows_out = active_rows;
    }
    return FEM_SUCCESS;
}

static fem_error_t static_solve_fem_contact_generic_mvp(void)
{
    fem_error_t err = FEM_SUCCESS;
    fem_contact_generic_feedback_row_t *feedback_rows = NULL;
    int feedback_row_count = 0;
    double *prev_displ = NULL;
    int max_outer_iter = 6;
    double disp_tol = 1.0e-10;
    int load_step_count = g_num_fem_static_load_steps > 0 ? g_num_fem_static_load_steps : 1;
    int iter = 0;

    printf("  FEM generic macro penalty solver v1\n");
    printf("    mode: slave-node to master-segment fixed-point MVP\n");

    err = static_load_fem_contact_generic_feedback_rows(&feedback_rows, &feedback_row_count);
    CHECK_ERROR(err);
    if (static_fem_local_feedback_enabled()) {
        printf("    replay feedback mode: LAGGED_REDUCED rows=%d\n", feedback_row_count);
    }
    if (g_num_fem_static_load_steps > 0) {
        printf("    native quasi-static load-step count: %d\n", g_num_fem_static_load_steps);
    }

    prev_displ = calloc((size_t)g_total_dof, sizeof(double));
    if (!prev_displ) {
        free(feedback_rows);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "FEM generic contact displacement history allocation failed");
    }

    static_reset_fem_contact_generic_trace();

    for (int step_slot = 0; step_slot < load_step_count; ++step_slot) {
        int load_step = g_num_fem_static_load_steps > 0 ? g_fem_static_load_step_ids[step_slot] : 0;
        double load_scale = g_num_fem_static_load_steps > 0 ? g_fem_static_load_step_scales[step_slot] : 1.0;
        int step_converged = 0;

        g_fem_static_current_load_step = load_step;
        g_fem_static_current_load_scale = load_scale;
        if (step_slot == 0) {
            memset(prev_displ, 0, (size_t)g_total_dof * sizeof(double));
            if (g_global_displ) {
                memset(g_global_displ, 0, (size_t)g_total_dof * sizeof(double));
            }
        } else {
            memcpy(prev_displ, g_global_displ, (size_t)g_total_dof * sizeof(double));
        }

        printf("    load_step %d: load_scale=%e\n", load_step, load_scale);

        for (iter = 0; iter < max_outer_iter; ++iter) {
            int active_rows = 0;
            double max_disp_delta = 0.0;

            err = assembly_global_stiffness_matrix();
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
            err = assembly_global_force_vector();
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
            err = static_build_fem_contact_generic_state(prev_displ,
                                                         load_step,
                                                         1,
                                                         0,
                                                         0,
                                                         &active_rows,
                                                         feedback_rows,
                                                         feedback_row_count);
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
            err = assembly_apply_boundary_conditions();
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
            err = assembly_check_matrix_properties();
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));

            printf("    load_step %d outer iter %d: active_rows=%d\n", load_step, iter, active_rows);

            err = cg_solve_system();
            CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));

            for (int i = 0; i < g_total_dof; ++i) {
                double delta = fabs(g_global_displ[i] - prev_displ[i]);
                if (delta > max_disp_delta) {
                    max_disp_delta = delta;
                }
            }
            printf("    load_step %d outer iter %d: max_disp_delta=%e\n",
                   load_step,
                   iter,
                   max_disp_delta);

            if (max_disp_delta <= disp_tol) {
                step_converged = 1;
                break;
            }

            memcpy(prev_displ, g_global_displ, (size_t)g_total_dof * sizeof(double));
        }

        if (!step_converged) {
            printf("    warning: FEM generic contact fixed-point load_step=%d reached max_outer_iter=%d without strict convergence\n",
                   load_step,
                   max_outer_iter);
        }

        err = static_build_fem_contact_generic_state(g_global_displ,
                                                     load_step,
                                                     0,
                                                     1,
                                                     0,
                                                     NULL,
                                                     feedback_rows,
                                                     feedback_row_count);
        CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
    }
    CHECK_ERROR_CLEANUP(err, free(feedback_rows); free(prev_displ));
    printf("    final FEM generic contact trace rows=%d\n", g_fem_contact_generic_trace_count);

    static_reset_fem_load_scale_bridge();

    err = static_check_equilibrium();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Equilibrium check failed\n");
    }

    free(feedback_rows);
    free(prev_displ);
    return FEM_SUCCESS;
}

static fem_error_t static_write_fem_contact_generic_trace_csv(
    const char *output_filename)
{
    FILE *fp = NULL;
    char trace_filename[MAX_FILENAME_LEN];

    CHECK_NULL(output_filename, "output filename for fem contact trace");

    if (!static_fem_contact_generic_enabled()) {
        return FEM_SUCCESS;
    }

    if (snprintf(trace_filename,
                 sizeof(trace_filename),
                 "%s.fem_contact_generic_trace.csv",
                 output_filename) >= (int)sizeof(trace_filename)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "FEM generic contact trace path too long for %s",
                         output_filename);
    }

    fp = fopen(trace_filename, "w");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot create FEM generic contact trace: %s",
                         trace_filename);
    }

    fprintf(fp,
            "pair_id,slave_node_id,master_segment_id,active_flag,gap_m,penetration_m,fn_n,contact_x,contact_y,normal_x,normal_y,tangent_x,tangent_y,source_mode,load_step,slave_part_id,master_part_id,slave_surface_id,master_surface_id,closest_x,closest_y,request_mode_hint,fn_macro_n,friction_active_flag,ut_rel_m,ft_t_n,ft_trial_n,ft_cap_n,stick_slip_state,mu_cap_used,k_t_pen_used,u_t_reg_m_used,adhesion_active_flag,fn_adh_n,k_adh_n_used,gap_adh_max_m_used\n");
    for (int i = 0; i < g_fem_contact_generic_trace_count; ++i) {
        const fem_contact_generic_trace_row_t *row = &g_fem_contact_generic_trace_rows[i];
        fprintf(fp,
                "%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%d,%d,%d,%d,%d,%.16e,%.16e,%s,%.16e,%d,%.16e,%.16e,%.16e,%.16e,%s,%.16e,%.16e,%.16e,%d,%.16e,%.16e,%.16e\n",
                row->pair_id,
                row->slave_node_id,
                row->master_segment_id,
                row->active_flag,
                row->gap_m,
                row->penetration_m,
                row->fn_n,
                row->contact_x,
                row->contact_y,
                row->normal_x,
                row->normal_y,
                row->tangent_x,
                row->tangent_y,
                "FEM_GENERIC_EDGE_SET_MVP",
                row->load_step,
                row->slave_part_id,
                row->master_part_id,
                row->slave_surface_id,
                row->master_surface_id,
                row->closest_x,
                row->closest_y,
                "normal_force",
                row->fn_n,
                row->friction_active_flag,
                row->ut_rel_m,
                row->ft_t_n,
                row->ft_trial_n,
                row->ft_cap_n,
                row->stick_slip_state,
                row->mu_cap_used,
                row->k_t_pen_used,
                row->u_t_reg_m_used,
                row->adhesion_active_flag,
                row->fn_adh_n,
                row->k_adh_n_used,
                row->gap_adh_max_m_used);
    }

    fclose(fp);
    printf("  Writing FEM generic contact trace to: %s\n", trace_filename);
    return FEM_SUCCESS;
}

static fem_error_t static_write_fem_contact_generic_replay_use_csv(
    const char *output_filename)
{
    FILE *fp = NULL;
    char replay_use_filename[MAX_FILENAME_LEN];

    CHECK_NULL(output_filename, "output filename for fem contact replay-use");

    if (!static_fem_contact_generic_enabled()) {
        return FEM_SUCCESS;
    }

    if (snprintf(replay_use_filename,
                 sizeof(replay_use_filename),
                 "%s.fem_contact_generic_replay_use.csv",
                 output_filename) >= (int)sizeof(replay_use_filename)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "FEM generic contact replay-use path too long for %s",
                         output_filename);
    }

    fp = fopen(replay_use_filename, "w");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot create FEM generic contact replay-use CSV: %s",
                         replay_use_filename);
    }

    fprintf(fp,
            "load_step,pair_id,gamma_n_used,k_pen_base,k_pen_used,feedback_status,feedback_source,active_flag,fn_n,penetration_m\n");
    for (int i = 0; i < g_fem_contact_generic_trace_count; ++i) {
        const fem_contact_generic_trace_row_t *row = &g_fem_contact_generic_trace_rows[i];
        fprintf(fp,
                "%d,%d,%.16e,%.16e,%.16e,%s,%s,%d,%.16e,%.16e\n",
                row->load_step,
                row->pair_id,
                row->gamma_n_used,
                row->k_pen_base,
                row->k_pen_used,
                row->feedback_status,
                row->feedback_source,
                row->active_flag,
                row->fn_n,
                row->penetration_m);
    }

    fclose(fp);
    printf("  Writing FEM generic contact replay-use to: %s\n", replay_use_filename);
    return FEM_SUCCESS;
}

/* Check equilibrium after solution */
fem_error_t static_check_equilibrium(void)
{
    double max_residual = 0.0;
    double residual;

    if (g_total_dof <= 0) {
        return FEM_SUCCESS;
    }

    double *ku = malloc((size_t)g_total_dof * sizeof(double));
    CHECK_NULL(ku, "Residual workspace allocation failed");

    fem_error_t err = cg_matrix_vector_multiply(NULL, g_global_displ, ku, g_total_dof);
    if (err != FEM_SUCCESS) {
        free(ku);
        return err;
    }

    for (int i = 0; i < g_total_dof; i++) {
        residual = ku[i] - g_global_force[i];
        double abs_res = fabs(residual);
        if (abs_res > max_residual) {
            max_residual = abs_res;
        }
    }

    free(ku);

    printf("    Maximum residual: %e\n", max_residual);

    if (max_residual > 1.0e-6) {
        return error_set(FEM_ERROR_CONVERGENCE_FAILED, 
                        "Large equilibrium residual: %e", max_residual);
    }

    return FEM_SUCCESS;
}
