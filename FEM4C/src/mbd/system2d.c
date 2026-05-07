#include "system2d.h"
#include "system2d_internal.h"
#include "../domain/mbd/assembler2d.h"
#include "../domain/mbd/forces2d.h"
#include "../domain/mbd/integrator_explicit2d.h"
#include "../domain/mbd/integrator_hht2d.h"
#include "../domain/mbd/integrator_newmark2d.h"
#include "../domain/mbd/kinematics2d.h"
#include "../numerics/dense/linear_solver_dense.h"
#include "output2d.h"
#include "../domain/mbd/projection2d.h"
#include "../domain/contact/contact_patch2d.h"
#include "../coupled/contact_patch_load2d.h"
#include "../analysis/static.h"
#include "../io/input.h"
#include "../common/error.h"
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MBD_DIAG_E_BODY_PARSE "E_BODY_PARSE"
#define MBD_DIAG_E_BODY_DYN_PARSE "E_BODY_DYN_PARSE"
#define MBD_DIAG_E_BODY_RANGE "E_BODY_RANGE"
#define MBD_DIAG_E_DUP_BODY "E_DUP_BODY"
#define MBD_DIAG_E_DISTANCE_PARSE "E_DISTANCE_PARSE"
#define MBD_DIAG_E_DISTANCE_RANGE "E_DISTANCE_RANGE"
#define MBD_DIAG_E_REVOLUTE_PARSE "E_REVOLUTE_PARSE"
#define MBD_DIAG_E_REVOLUTE_RANGE "E_REVOLUTE_RANGE"
#define MBD_DIAG_E_GRAVITY_PARSE "E_GRAVITY_PARSE"
#define MBD_DIAG_E_FORCE_PARSE "E_FORCE_PARSE"
#define MBD_DIAG_E_FORCE_RANGE "E_FORCE_RANGE"
#define MBD_DIAG_E_UNSUPPORTED_DIRECTIVE "E_UNSUPPORTED_DIRECTIVE"
#define MBD_DIAG_E_INCOMPLETE_INPUT "E_INCOMPLETE_INPUT"
#define MBD_DIAG_E_UNDEFINED_BODY_REF "E_UNDEFINED_BODY_REF"
#define MBD_DIAG_E_BODY_COUNT_RANGE "E_BODY_COUNT_RANGE"
#define MBD_DIAG_E_CONSTRAINT_BODY_RANGE "E_CONSTRAINT_BODY_RANGE"
#define MBD_HISTORY_OUTPUT_FILENAME_CAPACITY 1024
#define MBD_CONTACT_STIFFNESS_SMALL_EPS 1.0e-12
#define MBD_CONTACT_TANGENTIAL_SPEED_EPS 1.0e-12
#define MBD_LOCAL_FEEDBACK_GAMMA_MIN 0.0
#define MBD_LOCAL_FEEDBACK_GAMMA_MAX 1.0e6
#define MBD_LOCAL_FEEDBACK_MU_MIN 0.0
#define MBD_LOCAL_FEEDBACK_MU_MAX 1.0e2
#define MBD_SAME_TIME_REDUCED_MAX_ITERS 4
#define MBD_SAME_TIME_REDUCED_TOL_MU 1.0e-6
#define MBD_SAME_TIME_REDUCED_TOL_GAMMA 1.0e-6
#define MBD_SAME_TIME_REDUCED_TOL_FN 1.0e-6
#define MBD_SAME_TIME_REDUCED_RELAXATION 5.0e-1
#define MBD_MONOLITHIC_PATCH_MVP_SOURCE_MODE "MONOLITHIC_PATCH_MVP"
#define MBD_MONOLITHIC_PATCH_MVP_SOURCE_MARK "monolithic_patch_mvp"
#define MBD_MONOLITHIC_PATCH_MVP_ROUTE "circle_monolithic_local_patch_mvp"
#define MBD_MONOLITHIC_PATCH_MVP_PATCH_SIZE 2.0e-2
#define MBD_MONOLITHIC_PATCH_MVP_THICKNESS 1.0
#define MBD_MONOLITHIC_PATCH_MVP_RADIUS_SMALL 5.0e-2
#define MBD_MONOLITHIC_PATCH_MVP_RADIUS_LARGE 8.0e-2
#define MBD_MONOLITHIC_PATCH_MVP_TIME_STEP_SEC 1.0e-3
#define MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_SMALL_X 2.0e-2
#define MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_SMALL_Y 5.0e-3
#define MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_LARGE_X 3.2e-2
#define MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_LARGE_Y 8.0e-3
#define MBD_MONOLITHIC_PATCH_MVP_FIXTURE_SMALL "examples/contact_patch/patch_small_q4.dat"
#define MBD_MONOLITHIC_PATCH_MVP_FIXTURE_LARGE "examples/contact_patch/patch_large_q4.dat"
#define MBD_GENERIC_CONTACT_MVP_SOURCE_MODE "GENERIC_POLYLINE_MVP"
#define MBD_GENERIC_CONTACT_MVP_SOURCE_MARK "generic_polyline_mvp"
#define MBD_GENERIC_CONTACT_SEGMENT_EPS 1.0e-14
#define MBD_GENERIC_CONTACT_FRICTION_VREF_DEFAULT 1.0e-1
#define MBD_GENERIC_CONTACT_FRICTION_VSMOOTH_DEFAULT 1.0e-3
#define MBD_CONSTRAINT_RESIDUAL_TOL_DEFAULT 1.0e-1
#define MBD_PROJECTION_DENSE_RETRY_PIVOT_TOL 1.0e-16
#define MBD_MONOLITHIC_PROPER_MAX_ITER_DEFAULT 6
#define MBD_MONOLITHIC_PROPER_MAX_HALVING_DEFAULT 0
#define MBD_MONOLITHIC_PROPER_FEM_CALL_INTERVAL_DEFAULT 1
#define MBD_MONOLITHIC_PROPER_STRESS_RESIDUAL_TOL_DEFAULT 1.0e-2
#define MBD_MONOLITHIC_PROPER_DISPLACEMENT_RESIDUAL_TOL_DEFAULT 1.0e-2
#define MBD_MONOLITHIC_PROPER_CONTACT_PARAMETER_RESIDUAL_TOL_DEFAULT 2.0e-2
#define MBD_MONOLITHIC_PROPER_DELTA_MIN_DEFAULT 1.0e-6
#define MBD_MONOLITHIC_PROPER_MACRO_AREA_REF_DEFAULT 1.0e-2
#define MBD_MONOLITHIC_PROPER_RELAXATION_DEFAULT 5.5e-1
#define MBD_MONOLITHIC_PROPER_MAX_MU_MAPPING_POINTS 128

typedef struct {
    int id;
    int body_i;
    int body_j;
    int line_no;
} mbd_constraint_source_t;

typedef struct {
    int is_defined;
    int step;
    int pair_id;
    double gamma_n;
    double delta_g_eff;
    double fn_ref;
    double p_max;
    uint32_t valid_flag;
    char status[32];
} mbd_local_contact_source_row_t;

typedef struct {
    int is_defined;
    int step;
    int pair_id;
    double mu_eff;
    double h_min;
    uint32_t regime_flag;
    uint32_t valid_flag;
    char status[32];
} mbd_ehl_source_row_t;

typedef struct {
    int is_defined;
    int step;
    int iter;
    int pair_id;
    double gamma_n;
    double delta_g_eff;
    double fn_ref;
    double p_max;
    uint32_t valid_flag;
    char status[32];
} mbd_same_time_local_contact_source_row_t;

typedef struct {
    int is_defined;
    int step;
    int pair_id;
    int slave_vertex_id;
    int master_segment_id;
    double gamma_n;
    double delta_g_eff;
    double fn_ref;
    double p_max;
    uint32_t valid_flag;
    char status[32];
} mbd_generic_same_time_contact_source_row_t;

typedef struct {
    int is_defined;
    int step;
    int iter;
    int pair_id;
    double mu_eff;
    double h_min;
    uint32_t regime_flag;
    uint32_t valid_flag;
    char status[32];
} mbd_same_time_ehl_source_row_t;

typedef struct {
    int num_points;
    double local_points[MBD_GENERIC_CONTACT2D_MAX_SURFACE_POINTS][2];
} mbd_contact_surface_polyline_cache2d_t;

typedef struct {
    mbd_contact_feedback2d_t reduced_data;
    int status_ok;
    char fallback_reason[64];
    char status[32];
} mbd_same_time_reduced_lookup_result2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int slave_body_id;
    int master_body_id;
    int slave_vertex_index;
    int master_segment_index;
    int active;
    double fn;
    double penetration;
    double gamma_n_used;
    double k_pen_base;
    double k_pen_used;
    char feedback_status[32];
    char feedback_source[32];
} mbd_generic_contact_replay_use_row2d_t;

typedef struct {
    int body_capacity;
    int num_bodies;
    mbd_body2d_t *bodies;
    mbd_body_state2d_t *body_states;
    double (*current_generalized_force)[MBD_BODY2D_DOF];
    double (*previous_generalized_force)[MBD_BODY2D_DOF];
    int generalized_force_history_valid;
    int time_steps_executed;
    double time_dt;
} mbd_monolithic_proper_step_backup2d_t;

typedef struct {
    int active_flag;
    int pair_id;
    double penetration_m;
    double fn_total_n;
    double delta_eq_m;
    double pressure_surrogate_pa;
    double k_contact_eff;
    double mu_eff;
    char fem_output_path[1024];
    char fem_trace_path[1024];
    char artifact_dir[1024];
} mbd_monolithic_proper_fem_feedback2d_t;

typedef struct {
    double pressure_surrogate_pa;
    double mu_eff;
} mbd_monolithic_mu_mapping_point_t;

static mbd_generic_contact_replay_use_row2d_t
    mbd_generic_contact_replay_use_rows[MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS];
static int mbd_num_generic_contact_replay_use_rows = 0;

static int string_equals_ignore_case(const char *lhs, const char *rhs);
static double parse_env_double_or_default_with_status(const char *name,
                                                      double default_value,
                                                      double min_value,
                                                      double max_value,
                                                      const char **status_out);
static int parse_env_int_or_default_with_status(const char *name,
                                                int default_value,
                                                int min_value,
                                                int max_value,
                                                const char **status_out);
static fem_error_t mbd_system2d_apply_position_projection_if_enabled(mbd_system2d_t *system);
static fem_error_t mbd_system2d_dense_solve_with_projection_retry(const mbd_system2d_t *system,
                                                                  const double *matrix,
                                                                  const double *rhs,
                                                                  int n,
                                                                  double *solution);
static fem_error_t mbd_system2d_allocate_dense_workspace(int total_dof,
                                                         double **matrix,
                                                         double **rhs,
                                                         double **solution);
fem_error_t mbd_forces2d_apply_contact_loads(const mbd_system2d_t *system,
                                             int body_index,
                                             double generalized_force[MBD_BODY2D_DOF]);
const char *mbd_output2d_contact_trace_header_csv(void);
fem_error_t mbd_output2d_write_contact_trace_header(FILE *out);
fem_error_t mbd_output2d_write_contact_trace_snapshot(FILE *out,
                                                      int step,
                                                      double time,
                                                      const mbd_system2d_t *system);
const char *mbd_output2d_contact_feedback_header_csv(void);
fem_error_t mbd_output2d_write_contact_feedback_header(FILE *out);
fem_error_t mbd_output2d_write_contact_feedback_snapshot(FILE *out,
                                                         int step,
                                                         double time,
                                                         const mbd_system2d_t *system);
const char *mbd_output2d_contact_feedback_use_header_csv(void);
fem_error_t mbd_output2d_write_contact_feedback_use_header(FILE *out);
fem_error_t mbd_output2d_write_contact_feedback_use_snapshot(FILE *out,
                                                             int step,
                                                             double time,
                                                             const mbd_system2d_t *system);
const char *mbd_output2d_contact_reduced_data_header_csv(void);
fem_error_t mbd_output2d_write_contact_reduced_data_header(FILE *out);
fem_error_t mbd_output2d_write_contact_reduced_data_snapshot(FILE *out,
                                                             int step,
                                                             double time,
                                                             const mbd_system2d_t *system);
const char *mbd_output2d_same_time_reduced_iteration_header_csv(void);
fem_error_t mbd_output2d_write_same_time_reduced_iteration_header(FILE *out);
fem_error_t mbd_output2d_write_same_time_reduced_iteration_snapshot(
    FILE *out,
    const mbd_system2d_t *system);
const char *mbd_output2d_same_time_contact_request_header_csv(void);
fem_error_t mbd_output2d_write_same_time_contact_request_header(FILE *out);
fem_error_t mbd_output2d_write_same_time_contact_request_rows(
    FILE *out,
    const mbd_system2d_t *system);
static int source_marker_is_cli(const char *source_marker);
static fem_error_t mbd_integrator_parse(const char *text,
                                        mbd_integrator2d_t *integrator);
static mbd_integrator2d_t mbd_integrator_from_env(const char **status_out);
static fem_error_t mbd_time_control_from_env(mbd_time_control2d_t *time);
static int mbd_emit_step_trace(int requested_steps,
                               double dt,
                               mbd_integrator2d_t integrator);
static const char *skip_leading_spaces(const char *text);
static int line_starts_with_token(const char *line, const char *token);
static int line_starts_with_prefix(const char *line, const char *prefix);
static void line_to_excerpt(const char *line, char *out, size_t out_size);
static int parse_distance_line(const char *line, mbd_constraint2d_t *out);
static int parse_revolute_line(const char *line, mbd_constraint2d_t *out);
static fem_error_t mbd_system2d_try_load_case_from_input(const char *input_filename,
                                                         mbd_system2d_t *system);
static double mbd_constraint_residual_tol_from_env(const char **status_out);
static fem_error_t mbd_system2d_check_constraint_residual(const mbd_system2d_t *system,
                                                          double residual_tol,
                                                          double *residual_l2_out,
                                                          int *num_equations_out);
static int mbd_system2d_has_ground_body(const mbd_system2d_t *system);
static void mbd_system2d_lock_ground_body(mbd_body2d_t *body);
static fem_error_t mbd_system2d_enforce_ground_bodies(mbd_system2d_t *system);
static fem_error_t mbd_system2d_apply_ground_lock_to_dense_system(
    const mbd_system2d_t *system,
    double *matrix,
    double *rhs,
    int n);
static int mbd_dense_kkt2d_count_nonzero(const mbd_dense_kkt2d_t *kkt);
static double mbd_dense_kkt2d_rhs_l2(const mbd_dense_kkt2d_t *kkt);
static double mbd_dense_vector_l2(const double *values, int count);
static size_t mbd_system2d_body_bytes(int body_capacity);
static size_t mbd_system2d_body_force_bytes(int body_capacity);
static size_t mbd_system2d_constraint_bytes(int constraint_capacity);
static fem_error_t mbd_constraint_sources_reserve(
    mbd_constraint_source_t **sources,
    int *capacity,
    int required_capacity);
static fem_error_t mbd_dense_kkt2d_write_output(FILE *out,
                                                const mbd_dense_kkt2d_t *kkt);
static fem_error_t mbd_dense_solution_write_output(FILE *out,
                                                   const double *solution,
                                                   int count);
static fem_error_t mbd_system2d_compute_explicit_acceleration_field(
    const mbd_system2d_t *system,
    double (*acceleration)[MBD_BODY2D_DOF]);
static fem_error_t mbd_output2d_open_history_file(
    const char *output_filename,
    char history_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **history_out);
static fem_error_t mbd_output2d_write_history_snapshot(FILE *history_out,
                                                       int step,
                                                       double time,
                                                       const mbd_system2d_t *system);
static fem_error_t mbd_output2d_open_rigid_compare_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char rigid_compare_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **rigid_compare_out);
static fem_error_t mbd_output2d_open_contact_trace_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_trace_out);
static fem_error_t mbd_output2d_open_contact_feedback_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_feedback_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_feedback_out);
static fem_error_t mbd_output2d_open_contact_feedback_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_feedback_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_feedback_use_out);
static fem_error_t mbd_output2d_open_contact_reduced_data_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_reduced_data_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_reduced_data_out);
static fem_error_t mbd_output2d_open_same_time_reduced_iteration_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char same_time_reduced_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **same_time_reduced_iteration_out);
static fem_error_t mbd_output2d_open_generic_contact_trace_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_trace_out);
static fem_error_t mbd_output2d_open_generic_contact_replay_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_replay_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_replay_use_out);
static fem_error_t mbd_output2d_open_generic_contact_same_time_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_same_time_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_same_time_use_out);
static fem_error_t mbd_output2d_write_rigid_compare_snapshot(
    FILE *rigid_compare_out,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_contact_trace_step_snapshot(
    FILE *contact_trace_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_contact_feedback_step_snapshot(
    FILE *contact_feedback_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_contact_feedback_use_step_snapshot(
    FILE *contact_feedback_use_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_contact_reduced_data_step_snapshot(
    FILE *contact_reduced_data_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_same_time_reduced_iteration_step_snapshot(
    FILE *same_time_reduced_iteration_out,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_generic_contact_trace_step_snapshot(
    FILE *generic_contact_trace_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_generic_contact_replay_use_step_snapshot(
    FILE *generic_contact_replay_use_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
    FILE *generic_contact_same_time_use_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_same_time_contact_request_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char same_time_contact_request_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY]);
static fem_error_t mbd_output2d_open_monolithic_local_patch_rows_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char monolithic_local_patch_rows_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **monolithic_local_patch_rows_out);
static fem_error_t mbd_output2d_write_monolithic_local_patch_rows_step_snapshot(
    FILE *monolithic_local_patch_rows_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_write_monolithic_local_patch_summary_json(
    const char *summary_json_path,
    const char *rows_csv_path,
    const mbd_system2d_t *system);
static fem_error_t mbd_output2d_open_monolithic_proper_iteration_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char monolithic_proper_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **monolithic_proper_iteration_out);
static fem_error_t mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
    FILE *monolithic_proper_iteration_out,
    int step,
    double time,
    const mbd_system2d_t *system);
static void mbd_system2d_close_file_quiet(FILE **stream);
static fem_error_t mbd_system2d_close_file_checked(FILE **stream,
                                                   fem_error_t error_code,
                                                   const char *description,
                                                   const char *path);
static void mbd_output2d_close_optional_files(FILE **history_out,
                                              FILE **rigid_compare_out,
                                              FILE **contact_trace_out,
                                              FILE **contact_feedback_out,
                                              FILE **contact_feedback_use_out,
                                              FILE **contact_reduced_data_out,
                                              FILE **same_time_reduced_iteration_out);
static const mbd_contact_circle2d_t *mbd_system2d_find_contact_circle_const(
    const mbd_system2d_t *system,
    int body_id);
static const mbd_contact_halfspace2d_t *mbd_system2d_find_contact_halfspace_const(
    const mbd_system2d_t *system,
    int halfspace_id);
static const mbd_contact_surface_polyline2d_t *
mbd_system2d_find_contact_surface_polyline_const(const mbd_system2d_t *system,
                                                 int surface_id);
static fem_error_t mbd_system2d_load_contact_surface_polyline_csv(
    const char *csv_path,
    mbd_contact_surface_polyline_cache2d_t *surface_out);
static fem_error_t mbd_system2d_world_point_velocity(const mbd_body2d_t *body,
                                                     const double local_point[2],
                                                     double world_point[2],
                                                     double world_velocity[2]);
static fem_error_t mbd_system2d_append_generic_contact_trace_row(
    mbd_system2d_t *system,
    const mbd_contact_generic_trace2d_t *row);
static double mbd_system2d_generic_contact_mu_used(
    const mbd_contact_generic_pair2d_t *pair,
    double v_t);
static double mbd_system2d_generic_contact_slip_direction(
    const mbd_contact_generic_pair2d_t *pair,
    double v_t);
static fem_error_t mbd_system2d_append_generic_contact_replay_use_row(
    const mbd_generic_contact_replay_use_row2d_t *row);
static fem_error_t mbd_system2d_refresh_contact_forces_and_trace(mbd_system2d_t *system);
static fem_error_t mbd_system2d_refresh_contact_forces_and_trace_internal(
    mbd_system2d_t *system,
    int update_pair_stiffness);
static fem_error_t mbd_system2d_load_local_feedback_records(mbd_system2d_t *system);
static fem_error_t mbd_system2d_load_local_feedback_records_legacy_combined(
    mbd_system2d_t *system);
static fem_error_t mbd_system2d_load_local_feedback_records_contract_split(
    mbd_system2d_t *system);
static int mbd_system2d_find_or_create_local_feedback_record_index(
    mbd_system2d_t *system,
    int step,
    int pair_id);
static int mbd_system2d_parse_csv_fields(char *line,
                                         char **fields,
                                         int max_fields);
static int mbd_system2d_parse_uint32_field(const char *text, uint32_t *value_out);
static fem_error_t mbd_system2d_make_parent_dirs(const char *path);
static fem_error_t mbd_system2d_prepare_monolithic_local_patch_artifacts(
    mbd_system2d_t *system,
    const char *output_filename);
static fem_error_t mbd_system2d_run_monolithic_local_patch_circle_mvp(
    mbd_system2d_t *system,
    const mbd_contact_pair2d_t *pair,
    int pair_index,
    const mbd_contact_circle2d_t *circle_i,
    const mbd_contact_circle2d_t *circle_j,
    const double contact_point_world[2],
    const double normal_world[2],
    double gap,
    double penetration,
    double v_n,
    double v_t,
    mbd_contact_feedback2d_t *reduced_out,
    int *status_ok_out,
    const char **fallback_reason_out,
    const char **status_out);
static fem_error_t mbd_system2d_load_local_contact_source_rows(const char *path,
                                                               mbd_system2d_t *system);
static fem_error_t mbd_system2d_load_ehl_source_rows(const char *path,
                                                     mbd_system2d_t *system);
static fem_error_t mbd_system2d_load_monolithic_proper_internal_config_from_env(
    mbd_system2d_t *system);
static void mbd_system2d_reset_monolithic_proper_runtime(mbd_system2d_t *system);
static fem_error_t mbd_system2d_capture_monolithic_proper_step_backup(
    const mbd_system2d_t *system,
    mbd_monolithic_proper_step_backup2d_t *backup);
static fem_error_t mbd_system2d_restore_monolithic_proper_step_backup(
    mbd_system2d_t *system,
    const mbd_monolithic_proper_step_backup2d_t *backup);
static void mbd_monolithic_proper_step_backup_release(
    mbd_monolithic_proper_step_backup2d_t *backup);
static fem_error_t mbd_monolithic_proper_step_backup_reserve(
    mbd_monolithic_proper_step_backup2d_t *backup,
    int required_capacity);
static fem_error_t mbd_system2d_set_monolithic_proper_generic_contact_params(
    mbd_system2d_t *system,
    double k_contact_eff,
    double mu_eff);
static int mbd_system2d_find_monolithic_proper_representative_row(
    const mbd_system2d_t *system,
    int *pair_id_out,
    double *penetration_out);
static fem_error_t mbd_system2d_copy_file_bytes(const char *src_path,
                                                const char *dst_path);
static fem_error_t mbd_system2d_copy_template_support_csvs(
    const char *template_path,
    const char *target_dir);
static fem_error_t mbd_system2d_generate_monolithic_proper_fem_input(
    const mbd_system2d_t *system,
    double delta_eq_m,
    int step,
    int iter_index,
    int halving_retry_count,
    char generated_deck_path[1024],
    char fem_output_path[1024],
    char fem_trace_path[1024],
    char artifact_dir[1024]);
static fem_error_t mbd_system2d_load_mu_eff_mapping_points(
    const char *path,
    mbd_monolithic_mu_mapping_point_t *points,
    int max_points,
    int *point_count_out);
static double mbd_system2d_monolithic_relative_residual(double numerator,
                                                        double denominator,
                                                        double floor_value);
static double mbd_system2d_interpolate_mu_eff_mapping(
    const mbd_monolithic_mu_mapping_point_t *points,
    int point_count,
    double pressure_surrogate_pa);
static fem_error_t mbd_system2d_compute_monolithic_proper_feedback_from_fem(
    const mbd_system2d_t *system,
    const char *fem_output_path,
    const char *fem_trace_path,
    int representative_pair_id,
    double representative_penetration_m,
    mbd_monolithic_proper_fem_feedback2d_t *feedback_out);
static int mbd_system2d_monolithic_proper_step_is_sampled(
    const mbd_system2d_t *system,
    int step);
static fem_error_t mbd_output2d_write_monolithic_proper_iteration_detail_row(
    FILE *monolithic_proper_iteration_out,
    const mbd_system2d_t *system,
    int step,
    double time,
    int halving_retry_count,
    int feedback_available_flag,
    int contact_active_flag,
    int representative_pair_id,
    double representative_penetration_m,
    double fn_total_n,
    double delta_eq_m,
    double pressure_surrogate_pa,
    double k_iter_used_in_mbd,
    double mu_iter_used_in_mbd,
    const char *artifact_dir,
    const char *overall_status);
static fem_error_t mbd_system2d_do_monolithic_proper_explicit_step(
    mbd_system2d_t *system,
    int step,
    double time,
    FILE *monolithic_proper_iteration_out,
    int *iteration_row_count_out);
static const mbd_local_feedback_record2d_t *mbd_system2d_find_local_feedback_record(
    const mbd_system2d_t *system,
    int step,
    int pair_id);
static double mbd_local_feedback_clip(double value, double min_value, double max_value);
static int mbd_local_feedback_status_is_ok(const char *status);
static void mbd_system2d_clear_same_time_reduced_iterations(mbd_system2d_t *system);
static mbd_contact_feedback2d_t mbd_contact_feedback2d_make_basic(double mu_eff,
                                                                  double gamma_n);
static int mbd_system2d_same_time_reduced_has_split_sources(const mbd_system2d_t *system);
static int mbd_system2d_find_same_time_local_contact_source_row(
    const char *path,
    int step,
    int iter,
    int pair_id,
    mbd_same_time_local_contact_source_row_t *row_out);
static int mbd_system2d_find_same_time_ehl_source_row(
    const char *path,
    int step,
    int iter,
    int pair_id,
    mbd_same_time_ehl_source_row_t *row_out);
static int mbd_system2d_find_generic_same_time_contact_source_row(
    const char *path,
    int step,
    int pair_id,
    int slave_vertex_id,
    int master_segment_id,
    mbd_generic_same_time_contact_source_row_t *row_out);
static fem_error_t mbd_system2d_lookup_same_time_reduced_replay(
    const mbd_system2d_t *system,
    const mbd_contact_pair2d_t *pair,
    int step,
    int iter,
    mbd_same_time_reduced_lookup_result2d_t *result_out);
static double mbd_system2d_same_time_reduced_mu_target(const mbd_contact_pair2d_t *pair,
                                                       int active,
                                                       double v_t);
static double mbd_system2d_same_time_reduced_gamma_target(
    const mbd_contact_pair2d_t *pair,
    int active,
    double penetration,
    double fn);
static fem_error_t mbd_system2d_run_same_time_reduced_iteration(mbd_system2d_t *system);
static fem_error_t mbd_system2d_capture_body_summary_fields(
    const mbd_body2d_t *body,
    double q[MBD_BODY2D_DOF],
    double v[MBD_BODY2D_DOF],
    double force[MBD_BODY2D_DOF]);
static fem_error_t mbd_system2d_print_body_summary(int body_index,
                                                   const mbd_body2d_t *body);
static fem_error_t mbd_system2d_print_generalized_force_history_summary(
    const mbd_system2d_t *system);
static void mbd_system2d_reset_implicit_trace(mbd_system2d_t *system,
                                              const char *residual_mode,
                                              const char *scheme_mode,
                                              double residual_tolerance);
static fem_error_t mbd_system2d_write_body_output_row(FILE *out,
                                                      int body_index,
                                                      const mbd_body2d_t *body);
static const char *mbd_system2d_constraint_directives_csv(void);
static void mbd_system2d_ensure_trace_strings(mbd_system2d_t *system);
static int mbd_dense_kkt2d_count_nonzero(const mbd_dense_kkt2d_t *kkt)
{
    int count = 0;
    int row;
    int col;

    if (!kkt) {
        return 0;
    }

    for (row = 0; row < kkt->layout.total_dof; ++row) {
        for (col = 0; col < kkt->layout.total_dof; ++col) {
            if (fabs(kkt->matrix[row][col]) > 0.0) {
                ++count;
            }
        }
    }

    return count;
}

static double mbd_dense_kkt2d_rhs_l2(const mbd_dense_kkt2d_t *kkt)
{
    if (!kkt) {
        return 0.0;
    }

    return mbd_dense_vector_l2(kkt->rhs, kkt->layout.total_dof);
}

static double mbd_dense_vector_l2(const double *values, int count)
{
    double norm_sq = 0.0;
    int i;

    if (!values || count <= 0) {
        return 0.0;
    }

    for (i = 0; i < count; ++i) {
        norm_sq += values[i] * values[i];
    }

    return sqrt(norm_sq);
}

static size_t mbd_system2d_body_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(mbd_body2d_t);
}

static size_t mbd_system2d_body_force_bytes(int body_capacity)
{
    if (body_capacity <= 0) {
        return 0;
    }
    return (size_t) body_capacity * sizeof(double[MBD_BODY2D_DOF]);
}

static size_t mbd_system2d_constraint_bytes(int constraint_capacity)
{
    if (constraint_capacity <= 0) {
        return 0;
    }
    return (size_t) constraint_capacity * sizeof(mbd_constraint2d_t);
}

static fem_error_t mbd_constraint_sources_reserve(
    mbd_constraint_source_t **sources,
    int *capacity,
    int required_capacity)
{
    mbd_constraint_source_t *new_sources = NULL;

    CHECK_NULL(sources, "constraint source tracker");
    CHECK_NULL(capacity, "constraint source tracker capacity");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "constraint source tracker capacity %d must be positive",
                         required_capacity);
    }
    if (required_capacity <= *capacity) {
        return FEM_SUCCESS;
    }

    new_sources = (mbd_constraint_source_t *) calloc((size_t) required_capacity,
                                                     sizeof(*new_sources));
    if (!new_sources) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate constraint source tracker for %d constraints",
                         required_capacity);
    }
    if (*capacity > 0 && *sources) {
        memcpy(new_sources,
               *sources,
               (size_t) *capacity * sizeof(*new_sources));
    }

    free(*sources);
    *sources = new_sources;
    *capacity = required_capacity;
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_generic_contact_replay_use_step_snapshot(
    FILE *generic_contact_replay_use_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    int row_index = 0;

    if (!generic_contact_replay_use_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD generic contact replay-use system");

    for (row_index = 0; row_index < mbd_num_generic_contact_replay_use_rows; ++row_index) {
        const mbd_generic_contact_replay_use_row2d_t *row =
            &mbd_generic_contact_replay_use_rows[row_index];

        if (!row->is_defined) {
            continue;
        }
        if (!isfinite(row->fn) ||
            !isfinite(row->penetration) ||
            !isfinite(row->gamma_n_used) ||
            !isfinite(row->k_pen_base) ||
            !isfinite(row->k_pen_used) ||
            row->feedback_status[0] == '\0' ||
            row->feedback_source[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact replay-use row contains invalid values");
        }

        if (fprintf(generic_contact_replay_use_out,
                    "%d,%.16e,%d,%d,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%s\n",
                    step,
                    time,
                    row->pair_id,
                    row->slave_body_id,
                    row->master_body_id,
                    row->slave_vertex_index,
                    row->master_segment_index,
                    row->active,
                    row->fn,
                    row->penetration,
                    row->gamma_n_used,
                    row->k_pen_base,
                    row->k_pen_used,
                    row->feedback_status,
                    row->feedback_source) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write generic contact replay-use CSV row");
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
    FILE *generic_contact_same_time_use_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    int row_index = 0;

    if (!generic_contact_same_time_use_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD generic contact same-time-use system");

    for (row_index = 0; row_index < mbd_num_generic_contact_replay_use_rows; ++row_index) {
        const mbd_generic_contact_replay_use_row2d_t *row =
            &mbd_generic_contact_replay_use_rows[row_index];

        if (!row->is_defined) {
            continue;
        }
        if (!isfinite(row->fn) ||
            !isfinite(row->penetration) ||
            !isfinite(row->gamma_n_used) ||
            !isfinite(row->k_pen_base) ||
            !isfinite(row->k_pen_used) ||
            row->feedback_status[0] == '\0' ||
            row->feedback_source[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact same-time-use row contains invalid values");
        }

        if (fprintf(generic_contact_same_time_use_out,
                    "%d,%.16e,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%s\n",
                    step,
                    time,
                    row->pair_id,
                    row->slave_vertex_index,
                    row->master_segment_index,
                    row->active,
                    row->fn,
                    row->penetration,
                    row->gamma_n_used,
                    row->k_pen_base,
                    row->k_pen_used,
                    row->feedback_status,
                    row->feedback_source) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write generic contact same-time-use CSV row");
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_dense_kkt2d_write_output(FILE *out,
                                                const mbd_dense_kkt2d_t *kkt)
{
    int row;
    int col;
    int lambda_row;

    CHECK_NULL(out, "output file");
    CHECK_NULL(kkt, "mbd_dense_kkt2d");

    if (kkt->constraint_row_offsets) {
        for (row = 0; row < kkt->constraint_row_offset_capacity; ++row) {
            fprintf(out, "constraint_row_offset,%d,%d\n",
                    row, kkt->constraint_row_offsets[row]);
            if (row + 1 >= kkt->constraint_row_offset_capacity ||
                kkt->constraint_row_offsets[row] >= kkt->layout.lambda_dof) {
                break;
            }
        }
    }
    for (lambda_row = 0; lambda_row < kkt->layout.lambda_dof; ++lambda_row) {
        fprintf(out, "constraint_residual,%d,%.16e\n",
                lambda_row, kkt->constraint_residual[lambda_row]);
        fprintf(out, "constraint_phi_dot,%d,%.16e\n",
                lambda_row, kkt->constraint_phi_dot[lambda_row]);
        fprintf(out, "constraint_gamma_rhs,%d,%.16e\n",
                lambda_row, kkt->constraint_gamma_rhs[lambda_row]);
    }
    for (row = 0; row < kkt->layout.total_dof; ++row) {
        for (col = 0; col < kkt->layout.total_dof; ++col) {
            fprintf(out, "kkt_matrix,%d,%d,%.16e\n",
                    row, col, kkt->matrix[row][col]);
        }
    }
    for (row = 0; row < kkt->layout.total_dof; ++row) {
        fprintf(out, "kkt_rhs,%d,%.16e\n", row, kkt->rhs[row]);
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_dense_solution_write_output(FILE *out,
                                                   const double *solution,
                                                   int count)
{
    int row;

    CHECK_NULL(out, "output file");
    CHECK_NULL(solution, "dense solution");

    for (row = 0; row < count; ++row) {
        fprintf(out, "kkt_solution,%d,%.16e\n", row, solution[row]);
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_compute_explicit_acceleration_field(
    const mbd_system2d_t *system,
    double (*acceleration)[MBD_BODY2D_DOF])
{
    int body_index;

    CHECK_NULL(system, "mbd system");
    CHECK_NULL(acceleration, "explicit acceleration field");

    if (system->num_constraints == 0) {
        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            CHECK_ERROR(mbd_system2d_compute_explicit_acceleration(system,
                                                                   body_index,
                                                                   acceleration[body_index]));
        }
        return FEM_SUCCESS;
    }

    {
        mbd_dense_kkt2d_t dense_kkt;
        double *dense_matrix_compact = NULL;
        double *dense_rhs = NULL;
        double *dense_solution = NULL;
        int total_dof = 0;
        fem_error_t err = FEM_SUCCESS;

        mbd_dense_kkt2d_zero(&dense_kkt);
        err = mbd_dense_kkt2d_assemble(system, &dense_kkt);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        total_dof = dense_kkt.layout.total_dof;
        err = mbd_system2d_allocate_dense_workspace(total_dof,
                                                    &dense_matrix_compact,
                                                    &dense_rhs,
                                                    &dense_solution);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        err = mbd_dense_kkt2d_copy_compact(&dense_kkt, dense_matrix_compact);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        err = mbd_system2d_dense_solve_with_projection_retry(system,
                                                             dense_matrix_compact,
                                                             dense_kkt.rhs,
                                                             total_dof,
                                                             dense_solution);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }

        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            const int row = body_index * MBD_BODY2D_DOF;
            acceleration[body_index][0] = dense_solution[row + 0];
            acceleration[body_index][1] = dense_solution[row + 1];
            acceleration[body_index][2] = dense_solution[row + 2];
        }

cleanup:
        free(dense_solution);
        free(dense_rhs);
        free(dense_matrix_compact);
        mbd_dense_kkt2d_free(&dense_kkt);
        if (err != FEM_SUCCESS) {
            return err;
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_history_file(
    const char *output_filename,
    char history_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **history_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(history_output_filename, "MBD history output filename");
    CHECK_NULL(history_out, "MBD history output stream");

    written = snprintf(history_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.history.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "History output filename is too long for base path: %s",
                         output_filename);
    }

    *history_out = fopen(history_output_filename, "w");
    if (!*history_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD history output file: %s",
                         history_output_filename);
    }

    CHECK_ERROR_CLEANUP(mbd_output2d_write_header(*history_out),
                        mbd_system2d_close_file_quiet(history_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_history_snapshot(FILE *history_out,
                                                       int step,
                                                       double time,
                                                       const mbd_system2d_t *system)
{
    if (!history_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_system_snapshot(history_out, step, time, system);
}

static fem_error_t mbd_output2d_open_rigid_compare_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char rigid_compare_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **rigid_compare_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD rigid compare system");
    CHECK_NULL(rigid_compare_output_filename, "MBD rigid compare output filename");
    CHECK_NULL(rigid_compare_out, "MBD rigid compare output stream");

    rigid_compare_output_filename[0] = '\0';
    *rigid_compare_out = NULL;
    if (system->num_bodies < 2) {
        return FEM_SUCCESS;
    }

    written = snprintf(rigid_compare_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.rigid_compare.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Rigid compare output filename is too long for base path: %s",
                         output_filename);
    }

    *rigid_compare_out = fopen(rigid_compare_output_filename, "w");
    if (!*rigid_compare_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD rigid compare output file: %s",
                         rigid_compare_output_filename);
    }

    CHECK_ERROR_CLEANUP(mbd_output2d_write_rigid_compare_header(*rigid_compare_out),
                        mbd_system2d_close_file_quiet(rigid_compare_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_contact_trace_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_trace_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD contact trace system");
    CHECK_NULL(contact_trace_output_filename, "MBD contact trace output filename");
    CHECK_NULL(contact_trace_out, "MBD contact trace output stream");

    contact_trace_output_filename[0] = '\0';
    *contact_trace_out = NULL;
    if (system->num_contact_pairs <= 0) {
        return FEM_SUCCESS;
    }

    written = snprintf(contact_trace_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_trace.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Contact trace output filename is too long for base path: %s",
                         output_filename);
    }

    *contact_trace_out = fopen(contact_trace_output_filename, "w");
    if (!*contact_trace_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD contact trace output file: %s",
                         contact_trace_output_filename);
    }

    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_trace_header(*contact_trace_out),
                        mbd_system2d_close_file_quiet(contact_trace_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_contact_feedback_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_feedback_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_feedback_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD contact feedback system");
    CHECK_NULL(contact_feedback_output_filename, "MBD contact feedback output filename");
    CHECK_NULL(contact_feedback_out, "MBD contact feedback output stream");

    contact_feedback_output_filename[0] = '\0';
    *contact_feedback_out = NULL;
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT) {
        return FEM_SUCCESS;
    }

    written = snprintf(contact_feedback_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_feedback.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Contact feedback output filename is too long for base path: %s",
                         output_filename);
    }

    *contact_feedback_out = fopen(contact_feedback_output_filename, "w");
    if (!*contact_feedback_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD contact feedback output file: %s",
                         contact_feedback_output_filename);
    }

    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_header(*contact_feedback_out),
                        mbd_system2d_close_file_quiet(contact_feedback_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_contact_feedback_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_feedback_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_feedback_use_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD contact feedback-use system");
    CHECK_NULL(contact_feedback_use_output_filename, "MBD contact feedback-use output filename");
    CHECK_NULL(contact_feedback_use_out, "MBD contact feedback-use output stream");

    contact_feedback_use_output_filename[0] = '\0';
    *contact_feedback_use_out = NULL;
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT) {
        return FEM_SUCCESS;
    }

    written = snprintf(contact_feedback_use_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_feedback_use.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Contact feedback-use output filename is too long for base path: %s",
                         output_filename);
    }

    *contact_feedback_use_out = fopen(contact_feedback_use_output_filename, "w");
    if (!*contact_feedback_use_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD contact feedback-use output file: %s",
                         contact_feedback_use_output_filename);
    }

    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_use_header(*contact_feedback_use_out),
                        mbd_system2d_close_file_quiet(contact_feedback_use_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_contact_reduced_data_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char contact_reduced_data_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **contact_reduced_data_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD contact reduced-data system");
    CHECK_NULL(contact_reduced_data_output_filename,
               "MBD contact reduced-data output filename");
    CHECK_NULL(contact_reduced_data_out, "MBD contact reduced-data output stream");

    contact_reduced_data_output_filename[0] = '\0';
    *contact_reduced_data_out = NULL;
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT) {
        return FEM_SUCCESS;
    }

    written = snprintf(contact_reduced_data_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_reduced_data.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Contact reduced-data output filename is too long for base path: %s",
                         output_filename);
    }

    *contact_reduced_data_out = fopen(contact_reduced_data_output_filename, "w");
    if (!*contact_reduced_data_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD contact reduced-data output file: %s",
                         contact_reduced_data_output_filename);
    }

    CHECK_ERROR_CLEANUP(
        mbd_output2d_write_contact_reduced_data_header(*contact_reduced_data_out),
        mbd_system2d_close_file_quiet(contact_reduced_data_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_same_time_reduced_iteration_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char same_time_reduced_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **same_time_reduced_iteration_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD same-time reduced iteration system");
    CHECK_NULL(same_time_reduced_iteration_output_filename,
               "MBD same-time reduced iteration output filename");
    CHECK_NULL(same_time_reduced_iteration_out,
               "MBD same-time reduced iteration output stream");

    same_time_reduced_iteration_output_filename[0] = '\0';
    *same_time_reduced_iteration_out = NULL;
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT ||
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED) {
        return FEM_SUCCESS;
    }

    written = snprintf(same_time_reduced_iteration_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.same_time_reduced_iterations.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Same-time reduced iteration output filename is too long for base path: %s",
                         output_filename);
    }

    *same_time_reduced_iteration_out = fopen(same_time_reduced_iteration_output_filename, "w");
    if (!*same_time_reduced_iteration_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD same-time reduced iteration output file: %s",
                         same_time_reduced_iteration_output_filename);
    }

    CHECK_ERROR_CLEANUP(
        mbd_output2d_write_same_time_reduced_iteration_header(*same_time_reduced_iteration_out),
        mbd_system2d_close_file_quiet(same_time_reduced_iteration_out));
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_generic_contact_trace_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_trace_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD generic contact trace system");
    CHECK_NULL(generic_contact_trace_output_filename,
               "MBD generic contact trace output filename");
    CHECK_NULL(generic_contact_trace_out, "MBD generic contact trace output stream");

    generic_contact_trace_output_filename[0] = '\0';
    *generic_contact_trace_out = NULL;
    if (system->num_generic_contact_pairs <= 0) {
        return FEM_SUCCESS;
    }

    written = snprintf(generic_contact_trace_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_generic_trace.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Generic contact trace filename is too long for base path: %s",
                         output_filename);
    }

    *generic_contact_trace_out = fopen(generic_contact_trace_output_filename, "w");
    if (!*generic_contact_trace_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open generic contact trace output file: %s",
                         generic_contact_trace_output_filename);
    }

    if (fprintf(*generic_contact_trace_out,
                "step,time,pair_id,surface_i,surface_j,slave_body_id,master_body_id,slave_vertex_index,master_segment_index,active,gap,penetration,fn,k_used,contact_x,contact_y,slave_x,slave_y,n_x,n_y,source_mode,body_i,body_j,slave_vertex_id,master_segment_id,normal_x,normal_y,tangent_x,tangent_y,closest_x,closest_y,request_mode_hint,fn_macro_n,penetration_m,gap_m,v_t,mu_used,ft_tangent,ft_x,ft_y\n") <
        0) {
        mbd_system2d_close_file_quiet(generic_contact_trace_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write generic contact trace CSV header");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_monolithic_proper_iteration_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char monolithic_proper_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **monolithic_proper_iteration_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD monolithic proper system");
    CHECK_NULL(monolithic_proper_iteration_output_filename,
               "MBD monolithic proper iteration output filename");
    CHECK_NULL(monolithic_proper_iteration_out,
               "MBD monolithic proper iteration output stream");

    monolithic_proper_iteration_output_filename[0] = '\0';
    *monolithic_proper_iteration_out = NULL;
    if (system->monolithic_proper_mode == MBD_MONOLITHIC_PROPER_MODE_NONE) {
        return FEM_SUCCESS;
    }

    written = snprintf(monolithic_proper_iteration_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.monolithic_proper_iteration.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Monolithic proper iteration filename is too long for base path: %s",
                         output_filename);
    }

    *monolithic_proper_iteration_out = fopen(monolithic_proper_iteration_output_filename, "w");
    if (!*monolithic_proper_iteration_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD monolithic proper iteration output file: %s",
                         monolithic_proper_iteration_output_filename);
    }

    if (fprintf(*monolithic_proper_iteration_out,
                "step,time,context_step,iter_index,mode,k_contact_eff,mu_eff,stress_residual,displacement_residual,contact_parameter_residual,fem_residual,mbd_constraint_residual_l2,converged_flag,halving_retry_count,feedback_available_flag,contact_active_flag,representative_pair_id,representative_penetration_m,fn_total_n,delta_eq_m,pressure_surrogate_pa,k_iter_used_in_mbd,mu_iter_used_in_mbd,artifact_dir,overall_status\n") <
        0) {
        mbd_system2d_close_file_quiet(monolithic_proper_iteration_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write monolithic proper iteration CSV header");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_generic_contact_replay_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_replay_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_replay_use_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD generic contact replay-use system");
    CHECK_NULL(generic_contact_replay_use_output_filename,
               "MBD generic contact replay-use filename");
    CHECK_NULL(generic_contact_replay_use_out,
               "MBD generic contact replay-use output stream");

    generic_contact_replay_use_output_filename[0] = '\0';
    *generic_contact_replay_use_out = NULL;
    if (system->num_generic_contact_pairs <= 0) {
        return FEM_SUCCESS;
    }

    written = snprintf(generic_contact_replay_use_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_generic_replay_use.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Generic contact replay-use filename is too long for base path: %s",
                         output_filename);
    }

    *generic_contact_replay_use_out = fopen(generic_contact_replay_use_output_filename, "w");
    if (!*generic_contact_replay_use_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open generic contact replay-use output file: %s",
                         generic_contact_replay_use_output_filename);
    }

    if (fprintf(*generic_contact_replay_use_out,
                "step,time,pair_id,slave_body_id,master_body_id,slave_vertex_index,master_segment_index,active,fn,penetration,gamma_n_used,k_pen_base,k_pen_used,feedback_status,feedback_source\n") <
        0) {
        mbd_system2d_close_file_quiet(generic_contact_replay_use_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write generic contact replay-use CSV header");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_generic_contact_same_time_use_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char generic_contact_same_time_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **generic_contact_same_time_use_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD generic contact same-time-use system");
    CHECK_NULL(generic_contact_same_time_use_output_filename,
               "MBD generic contact same-time-use filename");
    CHECK_NULL(generic_contact_same_time_use_out,
               "MBD generic contact same-time-use output stream");

    generic_contact_same_time_use_output_filename[0] = '\0';
    *generic_contact_same_time_use_out = NULL;
    if (system->num_generic_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT ||
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED) {
        return FEM_SUCCESS;
    }

    written = snprintf(generic_contact_same_time_use_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_generic_same_time_use.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Generic contact same-time-use filename is too long for base path: %s",
                         output_filename);
    }

    *generic_contact_same_time_use_out = fopen(generic_contact_same_time_use_output_filename, "w");
    if (!*generic_contact_same_time_use_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open generic contact same-time-use output file: %s",
                         generic_contact_same_time_use_output_filename);
    }

    if (fprintf(*generic_contact_same_time_use_out,
                "step,time,pair_id,slave_vertex_id,master_segment_id,active,fn,penetration,gamma_n_used,k_pen_base,k_pen_used,feedback_status,feedback_source\n") <
        0) {
        mbd_system2d_close_file_quiet(generic_contact_same_time_use_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write generic contact same-time-use CSV header");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_open_monolithic_local_patch_rows_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char monolithic_local_patch_rows_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY],
    FILE **monolithic_local_patch_rows_out)
{
    int written = 0;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD monolithic local patch system");
    CHECK_NULL(monolithic_local_patch_rows_output_filename,
               "MBD monolithic local patch rows filename");
    CHECK_NULL(monolithic_local_patch_rows_out,
               "MBD monolithic local patch rows stream");

    monolithic_local_patch_rows_output_filename[0] = '\0';
    *monolithic_local_patch_rows_out = NULL;
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT ||
        system->local_contact_monolithic_mode !=
            MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE) {
        return FEM_SUCCESS;
    }

    written = snprintf(monolithic_local_patch_rows_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.contact_circle_monolithic_local_patch_rows.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Monolithic local patch rows filename is too long for base path: %s",
                         output_filename);
    }

    *monolithic_local_patch_rows_out = fopen(monolithic_local_patch_rows_output_filename, "w");
    if (!*monolithic_local_patch_rows_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open monolithic local patch rows output file: %s",
                         monolithic_local_patch_rows_output_filename);
    }

    if (fprintf(*monolithic_local_patch_rows_out,
                "step,time,pair_id,active,source_mode,fallback_reason,gap,penetration,gamma_n,delta_g_eff,fn_ref,p_max,valid_flag,status_ok,status,diagnostic_json\n") < 0) {
        mbd_system2d_close_file_quiet(monolithic_local_patch_rows_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write monolithic local patch rows CSV header");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_rigid_compare_snapshot(
    FILE *rigid_compare_out,
    double time,
    const mbd_system2d_t *system)
{
    if (!rigid_compare_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD rigid compare system");
    if (system->num_bodies < 2) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_rigid_compare_row(rigid_compare_out,
                                                time,
                                                system,
                                                system->bodies[0].id,
                                                system->bodies[1].id);
}

static fem_error_t mbd_output2d_write_contact_trace_step_snapshot(
    FILE *contact_trace_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    if (!contact_trace_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_contact_trace_snapshot(contact_trace_out,
                                                     step,
                                                     time,
                                                     system);
}

static fem_error_t mbd_output2d_write_contact_feedback_step_snapshot(
    FILE *contact_feedback_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    if (!contact_feedback_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_contact_feedback_snapshot(contact_feedback_out,
                                                        step,
                                                        time,
                                                        system);
}

static fem_error_t mbd_output2d_write_contact_feedback_use_step_snapshot(
    FILE *contact_feedback_use_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    if (!contact_feedback_use_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_contact_feedback_use_snapshot(contact_feedback_use_out,
                                                            step,
                                                            time,
                                                            system);
}

static fem_error_t mbd_output2d_write_contact_reduced_data_step_snapshot(
    FILE *contact_reduced_data_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    if (!contact_reduced_data_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_contact_reduced_data_snapshot(contact_reduced_data_out,
                                                            step,
                                                            time,
                                                            system);
}

static fem_error_t mbd_output2d_write_same_time_reduced_iteration_step_snapshot(
    FILE *same_time_reduced_iteration_out,
    const mbd_system2d_t *system)
{
    if (!same_time_reduced_iteration_out) {
        return FEM_SUCCESS;
    }

    return mbd_output2d_write_same_time_reduced_iteration_snapshot(
        same_time_reduced_iteration_out,
        system);
}

static fem_error_t mbd_output2d_write_same_time_contact_request_file(
    const char *output_filename,
    const mbd_system2d_t *system,
    char same_time_contact_request_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY])
{
    FILE *same_time_contact_request_out = NULL;
    int written = 0;
    fem_error_t close_err = FEM_SUCCESS;

    CHECK_NULL(output_filename, "MBD summary output filename");
    CHECK_NULL(system, "MBD same-time contact request system");
    CHECK_NULL(same_time_contact_request_output_filename,
               "MBD same-time contact request output filename");

    same_time_contact_request_output_filename[0] = '\0';
    if (system->num_contact_pairs <= 0 ||
        system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT ||
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED) {
        return FEM_SUCCESS;
    }

    written = snprintf(same_time_contact_request_output_filename,
                       MBD_HISTORY_OUTPUT_FILENAME_CAPACITY,
                       "%s.same_time_contact_requests.csv",
                       output_filename);
    if (written < 0 || written >= MBD_HISTORY_OUTPUT_FILENAME_CAPACITY) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Same-time contact request output filename is too long for base path: %s",
                         output_filename);
    }

    same_time_contact_request_out = fopen(same_time_contact_request_output_filename, "w");
    if (!same_time_contact_request_out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD same-time contact request output file: %s",
                         same_time_contact_request_output_filename);
    }

    CHECK_ERROR_CLEANUP(
        mbd_output2d_write_same_time_contact_request_header(same_time_contact_request_out),
        mbd_system2d_close_file_quiet(&same_time_contact_request_out));
    CHECK_ERROR_CLEANUP(
        mbd_output2d_write_same_time_contact_request_rows(same_time_contact_request_out, system),
        mbd_system2d_close_file_quiet(&same_time_contact_request_out));

    close_err = mbd_system2d_close_file_checked(&same_time_contact_request_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD same-time contact request output file",
                                                same_time_contact_request_output_filename);
    return close_err;
}

static fem_error_t mbd_output2d_write_generic_contact_trace_step_snapshot(
    FILE *generic_contact_trace_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    int row_index = 0;

    if (!generic_contact_trace_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD generic contact trace system");

    for (row_index = 0; row_index < system->num_current_generic_contact_trace_rows; ++row_index) {
        const mbd_contact_generic_trace2d_t *row =
            &system->current_generic_contact_trace_rows[row_index];

        if (!row->is_defined) {
            continue;
        }
        if (!isfinite(row->gap) ||
            !isfinite(row->penetration) ||
            !isfinite(row->fn) ||
            !isfinite(row->k_used) ||
            !isfinite(row->contact_point[0]) ||
            !isfinite(row->contact_point[1]) ||
            !isfinite(row->closest_point[0]) ||
            !isfinite(row->closest_point[1]) ||
            !isfinite(row->slave_point[0]) ||
            !isfinite(row->slave_point[1]) ||
            !isfinite(row->normal[0]) ||
            !isfinite(row->normal[1]) ||
            !isfinite(row->tangent[0]) ||
            !isfinite(row->tangent[1]) ||
            !isfinite(row->v_t) ||
            !isfinite(row->mu_used) ||
            !isfinite(row->ft_tangent) ||
            !isfinite(row->friction_force[0]) ||
            !isfinite(row->friction_force[1]) ||
            row->request_mode_hint[0] == '\0' ||
            row->source_mode[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact trace row contains invalid values");
        }

        if (fprintf(generic_contact_trace_out,
                    "%d,%.16e,%d,%d,%d,%d,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e\n",
                    step,
                    time,
                    row->pair_id,
                    row->surface_i,
                    row->surface_j,
                    row->slave_body_id,
                    row->master_body_id,
                    row->slave_vertex_index,
                    row->master_segment_index,
                    row->active,
                    row->gap,
                    row->penetration,
                    row->fn,
                    row->k_used,
                    row->contact_point[0],
                    row->contact_point[1],
                    row->slave_point[0],
                    row->slave_point[1],
                    row->normal[0],
                    row->normal[1],
                    row->source_mode,
                    row->slave_body_id,
                    row->master_body_id,
                    row->slave_vertex_index,
                    row->master_segment_index,
                    row->normal[0],
                    row->normal[1],
                    row->tangent[0],
                    row->tangent[1],
                    row->closest_point[0],
                    row->closest_point[1],
                    row->request_mode_hint,
                    row->fn,
                    row->penetration,
                    row->gap,
                    row->v_t,
                    row->mu_used,
                    row->ft_tangent,
                    row->friction_force[0],
                    row->friction_force[1]) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write generic contact trace CSV row");
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_monolithic_local_patch_rows_step_snapshot(
    FILE *monolithic_local_patch_rows_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    int pair_index;

    if (!monolithic_local_patch_rows_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD monolithic local patch system");

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_monolithic_local_patch_trace2d_t *row =
            &system->current_monolithic_local_patch_rows[pair_index];

        if (!row->is_defined) {
            continue;
        }
        if (!isfinite(row->gap) ||
            !isfinite(row->penetration) ||
            !isfinite(row->reduced_data.gamma_n) ||
            !isfinite(row->reduced_data.delta_g_eff) ||
            !isfinite(row->reduced_data.fn_ref) ||
            !isfinite(row->reduced_data.p_max) ||
            row->source_mode[0] == '\0' ||
            row->status[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "monolithic local patch row contains invalid values");
        }

        if (fprintf(monolithic_local_patch_rows_out,
                    "%d,%.16e,%d,%d,%s,%s,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%u,%d,%s,%s\n",
                    step,
                    time,
                    row->pair_id,
                    row->active,
                    row->source_mode,
                    row->fallback_reason,
                    row->gap,
                    row->penetration,
                    row->reduced_data.gamma_n,
                    row->reduced_data.delta_g_eff,
                    row->reduced_data.fn_ref,
                    row->reduced_data.p_max,
                    (unsigned int)row->reduced_data.valid_flag,
                    row->status_ok,
                    row->status,
                    row->diagnostic_json[0] != '\0' ? row->diagnostic_json : "none") < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write monolithic local patch rows CSV row");
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
    FILE *monolithic_proper_iteration_out,
    int step,
    double time,
    const mbd_system2d_t *system)
{
    double mbd_constraint_residual_l2 = 0.0;
    int num_equations = 0;
    const mbd_monolithic_proper_context2d_t *context = NULL;

    if (!monolithic_proper_iteration_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "MBD monolithic proper system");

    context = &system->monolithic_proper_context;
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &mbd_constraint_residual_l2,
                                                            &num_equations));
    (void)num_equations;

    if (fprintf(monolithic_proper_iteration_out,
                "%d,%.16e,%d,%d,%s,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%d,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%s\n",
                step,
                time,
                context->is_defined ? context->context_step : step,
                context->is_defined ? context->iter_index : 0,
                mbd_monolithic_proper_mode_to_string(system->monolithic_proper_mode),
                context->is_defined ? context->k_contact_eff : 0.0,
                context->is_defined ? context->mu_eff : 0.0,
                context->is_defined ? context->stress_residual : 0.0,
                context->is_defined ? context->displacement_residual : 0.0,
                context->is_defined ? context->contact_parameter_residual : 0.0,
                context->is_defined ? context->fem_residual : 0.0,
                mbd_constraint_residual_l2,
                context->is_defined ? context->converged_flag : 0,
                0,
                0,
                0,
                -1,
                0.0,
                0.0,
                0.0,
                0.0,
                0.0,
                0.0,
                "",
                "snapshot") < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write monolithic proper iteration CSV row");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_write_monolithic_local_patch_summary_json(
    const char *summary_json_path,
    const char *rows_csv_path,
    const mbd_system2d_t *system)
{
    FILE *out = NULL;

    CHECK_NULL(summary_json_path, "MBD monolithic local patch summary path");
    CHECK_NULL(rows_csv_path, "MBD monolithic local patch rows CSV path");
    CHECK_NULL(system, "MBD monolithic local patch system");

    out = fopen(summary_json_path, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open monolithic local patch summary JSON: %s",
                         summary_json_path);
    }

    if (fprintf(out,
                "{\n"
                "  \"route\": \"%s\",\n"
                "  \"source\": \"%s\",\n"
                "  \"supported_case\": \"circle_only\",\n"
                "  \"local_contact_monolithic_mode\": \"%s\",\n"
                "  \"rows_csv\": \"%s\",\n"
                "  \"artifact_root\": \"%s\",\n"
                "  \"fixture_small\": \"%s\",\n"
                "  \"fixture_large\": \"%s\",\n"
                "  \"patch_size\": %.16e,\n"
                "  \"thickness\": %.16e,\n"
                "  \"active_rows_total\": %d,\n"
                "  \"gamma_not_one_rows_total\": %d,\n"
                "  \"fn_positive_rows_total\": %d\n"
                "}\n",
                MBD_MONOLITHIC_PATCH_MVP_ROUTE,
                MBD_MONOLITHIC_PATCH_MVP_SOURCE_MARK,
                mbd_local_contact_monolithic_mode_to_string(
                    system->local_contact_monolithic_mode),
                rows_csv_path,
                system->local_contact_monolithic_artifact_root[0] != '\0'
                    ? system->local_contact_monolithic_artifact_root
                    : "none",
                MBD_MONOLITHIC_PATCH_MVP_FIXTURE_SMALL,
                MBD_MONOLITHIC_PATCH_MVP_FIXTURE_LARGE,
                MBD_MONOLITHIC_PATCH_MVP_PATCH_SIZE,
                MBD_MONOLITHIC_PATCH_MVP_THICKNESS,
                system->monolithic_local_patch_active_rows_total,
                system->monolithic_local_patch_gamma_not_one_rows_total,
                system->monolithic_local_patch_fn_positive_rows_total) < 0) {
        fclose(out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write monolithic local patch summary JSON: %s",
                         summary_json_path);
    }

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to close monolithic local patch summary JSON: %s",
                         summary_json_path);
    }
    return FEM_SUCCESS;
}

/* Clear the caller-owned FILE* before fclose so cleanup paths stay analyzer-safe. */
static void mbd_system2d_close_file_quiet(FILE **stream)
{
    FILE *handle = NULL;

    if (!stream || !*stream) {
        return;
    }

    handle = *stream;
    *stream = NULL;
    fclose(handle);
}

static fem_error_t mbd_system2d_close_file_checked(FILE **stream,
                                                   fem_error_t error_code,
                                                   const char *description,
                                                   const char *path)
{
    FILE *handle = NULL;

    if (!stream || !*stream) {
        return FEM_SUCCESS;
    }

    handle = *stream;
    *stream = NULL;
    if (fclose(handle) != 0) {
        return error_set(error_code,
                         "Cannot close %s: %s",
                         description,
                         path);
    }
    return FEM_SUCCESS;
}

static void mbd_output2d_close_optional_files(FILE **history_out,
                                              FILE **rigid_compare_out,
                                              FILE **contact_trace_out,
                                              FILE **contact_feedback_out,
                                              FILE **contact_feedback_use_out,
                                              FILE **contact_reduced_data_out,
                                              FILE **same_time_reduced_iteration_out)
{
    mbd_system2d_close_file_quiet(history_out);
    mbd_system2d_close_file_quiet(rigid_compare_out);
    mbd_system2d_close_file_quiet(contact_trace_out);
    mbd_system2d_close_file_quiet(contact_feedback_out);
    mbd_system2d_close_file_quiet(contact_feedback_use_out);
    mbd_system2d_close_file_quiet(contact_reduced_data_out);
    mbd_system2d_close_file_quiet(same_time_reduced_iteration_out);
}

static fem_error_t mbd_system2d_make_parent_dirs(const char *path)
{
    char buffer[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY];
    size_t i = 0;

    CHECK_NULL(path, "directory path");

    if (snprintf(buffer, sizeof(buffer), "%s", path) >= (int)sizeof(buffer)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "directory path is too long: %s",
                         path);
    }
    for (i = 1; buffer[i] != '\0'; ++i) {
        if (buffer[i] != '/') {
            continue;
        }
        buffer[i] = '\0';
        if (buffer[0] != '\0' && mkdir(buffer, 0777) != 0 && errno != EEXIST) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "cannot create directory: %s",
                             buffer);
        }
        buffer[i] = '/';
    }
    if (mkdir(buffer, 0777) != 0 && errno != EEXIST) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "cannot create directory: %s",
                         buffer);
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_prepare_monolithic_local_patch_artifacts(
    mbd_system2d_t *system,
    const char *output_filename)
{
    char path[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY];

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(output_filename, "summary output filename");

    system->local_contact_monolithic_artifact_root[0] = '\0';
    if (system->local_contact_monolithic_mode !=
        MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE) {
        return FEM_SUCCESS;
    }

    if (snprintf(system->local_contact_monolithic_artifact_root,
                 sizeof(system->local_contact_monolithic_artifact_root),
                 "%s.contact_circle_monolithic_local_patch",
                 output_filename) >=
        (int)sizeof(system->local_contact_monolithic_artifact_root)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic local patch artifact root is too long");
    }
    CHECK_ERROR(mbd_system2d_make_parent_dirs(
        system->local_contact_monolithic_artifact_root));
    if (snprintf(path, sizeof(path), "%s/patch_metadata",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(path)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic patch_metadata path is too long");
    }
    CHECK_ERROR(mbd_system2d_make_parent_dirs(path));
    if (snprintf(path, sizeof(path), "%s/patch_receiver",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(path)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic patch_receiver path is too long");
    }
    CHECK_ERROR(mbd_system2d_make_parent_dirs(path));
    if (snprintf(path, sizeof(path), "%s/diagnostics",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(path)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic diagnostics path is too long");
    }
    CHECK_ERROR(mbd_system2d_make_parent_dirs(path));
    return FEM_SUCCESS;
}

static double mbd_system2d_monolithic_local_patch_fn_macro_proxy(double gap,
                                                                 double penetration,
                                                                 double v_n,
                                                                 double v_t,
                                                                 int step,
                                                                 int iter_id)
{
    const double active_gap = fmax(0.0, -gap);
    const double active_pen = fmax(0.0, penetration);
    const double slip_scale = fabs(v_t);
    const double normal_scale = fmax(0.0, -v_n);

    return 1.0e2 +
           2.0e5 * active_pen +
           1.0e5 * active_gap +
           5.0e1 * normal_scale +
           1.0e1 * slip_scale +
           5.0 * (double)((step % 7) + iter_id);
}

static fem_error_t mbd_system2d_monolithic_local_patch_write_diag_json(
    const char *diag_path,
    const char *metadata_small_path,
    const char *metadata_large_path,
    const contact_patch_load2d_result_t *small_result,
    const contact_patch_load2d_result_t *large_result,
    int step,
    int pair_id,
    double time_value,
    double gap,
    double penetration,
    double fn_macro_proxy,
    const mbd_contact_feedback2d_t *reduced)
{
    FILE *out = NULL;

    CHECK_NULL(diag_path, "monolithic diag path");
    CHECK_NULL(metadata_small_path, "monolithic metadata small path");
    CHECK_NULL(metadata_large_path, "monolithic metadata large path");
    CHECK_NULL(small_result, "monolithic receiver small result");
    CHECK_NULL(large_result, "monolithic receiver large result");
    CHECK_NULL(reduced, "monolithic reduced feedback");

    out = fopen(diag_path, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "cannot open monolithic local patch diagnostic JSON: %s",
                         diag_path);
    }

    if (fprintf(out,
                "{\n"
                "  \"route\": \"%s\",\n"
                "  \"source\": \"%s\",\n"
                "  \"step\": %d,\n"
                "  \"pair_id\": %d,\n"
                "  \"time\": %.16e,\n"
                "  \"gap\": %.16e,\n"
                "  \"penetration\": %.16e,\n"
                "  \"fn_macro_proxy\": %.16e,\n"
                "  \"metadata_small_path\": \"%s\",\n"
                "  \"metadata_large_path\": \"%s\",\n"
                "  \"receiver_small_summary\": \"%s\",\n"
                "  \"receiver_large_summary\": \"%s\",\n"
                "  \"small_displacement_centroid_local\": [%.16e, %.16e],\n"
                "  \"large_displacement_centroid_local\": [%.16e, %.16e],\n"
                "  \"small_reaction_resultant_local\": [%.16e, %.16e, %.16e],\n"
                "  \"large_reaction_resultant_local\": [%.16e, %.16e, %.16e],\n"
                "  \"reduced\": {\n"
                "    \"gamma_n\": %.16e,\n"
                "    \"delta_g_eff\": %.16e,\n"
                "    \"fn_ref\": %.16e,\n"
                "    \"p_max\": %.16e,\n"
                "    \"valid_flag\": %u\n"
                "  }\n"
                "}\n",
                MBD_MONOLITHIC_PATCH_MVP_ROUTE,
                MBD_MONOLITHIC_PATCH_MVP_SOURCE_MARK,
                step,
                pair_id,
                time_value,
                gap,
                penetration,
                fn_macro_proxy,
                metadata_small_path,
                metadata_large_path,
                small_result->summary_path,
                large_result->summary_path,
                small_result->displacement_centroid_local[0],
                small_result->displacement_centroid_local[1],
                large_result->displacement_centroid_local[0],
                large_result->displacement_centroid_local[1],
                small_result->reaction_resultant_local[0],
                small_result->reaction_resultant_local[1],
                small_result->reaction_resultant_local[2],
                large_result->reaction_resultant_local[0],
                large_result->reaction_resultant_local[1],
                large_result->reaction_resultant_local[2],
                reduced->gamma_n,
                reduced->delta_g_eff,
                reduced->fn_ref,
                reduced->p_max,
                (unsigned int)reduced->valid_flag) < 0) {
        fclose(out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to write monolithic local patch diagnostic JSON: %s",
                         diag_path);
    }
    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to close monolithic local patch diagnostic JSON: %s",
                         diag_path);
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_run_monolithic_local_patch_circle_mvp(
    mbd_system2d_t *system,
    const mbd_contact_pair2d_t *pair,
    int pair_index,
    const mbd_contact_circle2d_t *circle_i,
    const mbd_contact_circle2d_t *circle_j,
    const double contact_point_world[2],
    const double normal_world[2],
    double gap,
    double penetration,
    double v_n,
    double v_t,
    mbd_contact_feedback2d_t *reduced_out,
    int *status_ok_out,
    const char **fallback_reason_out,
    const char **status_out)
{
    mbd_monolithic_local_patch_trace2d_t *row = NULL;
    contact_patch2d_t metadata_patches[2];
    contact_patch2d_t receiver_patches[2];
    contact_patch_load2d_result_t receiver_results[2];
    double time_value = 0.0;
    double fn_macro_proxy = 0.0;
    double disp_mean = 0.0;
    double reaction_mean = 0.0;
    char patch_metadata_dir[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY];
    char patch_receiver_dir[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY];
    char diagnostics_dir[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY];
    char diagnostic_json[MAX_FILENAME_LEN];
    int step = 0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(pair, "mbd contact pair");
    CHECK_NULL(circle_i, "mbd contact circle i");
    CHECK_NULL(circle_j, "mbd contact circle j");
    CHECK_NULL(contact_point_world, "monolithic contact point");
    CHECK_NULL(normal_world, "monolithic contact normal");
    CHECK_NULL(reduced_out, "monolithic reduced output");
    CHECK_NULL(status_ok_out, "monolithic status_ok output");
    CHECK_NULL(fallback_reason_out, "monolithic fallback output");
    CHECK_NULL(status_out, "monolithic status output");

    row = &system->current_monolithic_local_patch_rows[pair_index];
    memset(row, 0, sizeof(*row));
    step = system->current_step_index;
    time_value = system->time.dt * (double)step;

    row->is_defined = 1;
    row->pair_id = pair->pair_id;
    row->active = penetration > 0.0 ? 1 : 0;
    row->gap = gap;
    row->penetration = penetration;
    snprintf(row->source_mode, sizeof(row->source_mode), "%s",
             MBD_MONOLITHIC_PATCH_MVP_SOURCE_MARK);

    *reduced_out = mbd_contact_feedback2d_make_basic(pair->base_mu, 1.0);
    reduced_out->delta_g_eff = gap;
    reduced_out->fn_ref = 0.0;
    reduced_out->p_max = 0.0;
    reduced_out->valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_DELTA_G_EFF |
                               MBD_CONTACT_FEEDBACK2D_VALID_FN_REF |
                               MBD_CONTACT_FEEDBACK2D_VALID_P_MAX;

    if (pair->proxy_geometry != MBD_CONTACT_PROXY_CIRCLE_CIRCLE ||
        pair->body_i != 0 ||
        pair->body_j != 1 ||
        fabs(circle_i->radius - MBD_MONOLITHIC_PATCH_MVP_RADIUS_SMALL) > 1.0e-12 ||
        fabs(circle_j->radius - MBD_MONOLITHIC_PATCH_MVP_RADIUS_LARGE) > 1.0e-12 ||
        fabs(circle_i->thickness - MBD_MONOLITHIC_PATCH_MVP_THICKNESS) > 1.0e-12 ||
        fabs(circle_j->thickness - MBD_MONOLITHIC_PATCH_MVP_THICKNESS) > 1.0e-12) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_LOCAL_CONTACT_MONOLITHIC PATCH_MVP_CIRCLE is circle-only and requires body_i=0 body_j=1 radii=(%.16e,%.16e) thickness=%.16e",
                         MBD_MONOLITHIC_PATCH_MVP_RADIUS_SMALL,
                         MBD_MONOLITHIC_PATCH_MVP_RADIUS_LARGE,
                         MBD_MONOLITHIC_PATCH_MVP_THICKNESS);
    }

    if (!row->active) {
        row->reduced_data = *reduced_out;
        row->status_ok = 1;
        snprintf(row->fallback_reason, sizeof(row->fallback_reason), "%s", "inactive");
        snprintf(row->status, sizeof(row->status), "%s", "inactive");
        *status_ok_out = 1;
        *fallback_reason_out = "inactive";
        *status_out = "inactive";
        return FEM_SUCCESS;
    }

    if (system->local_contact_monolithic_artifact_root[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic local patch artifact root is not initialized");
    }

    if (snprintf(patch_metadata_dir, sizeof(patch_metadata_dir), "%s/patch_metadata",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(patch_metadata_dir) ||
        snprintf(patch_receiver_dir, sizeof(patch_receiver_dir), "%s/patch_receiver",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(patch_receiver_dir) ||
        snprintf(diagnostics_dir, sizeof(diagnostics_dir), "%s/diagnostics",
                 system->local_contact_monolithic_artifact_root) >= (int)sizeof(diagnostics_dir) ||
        snprintf(diagnostic_json, sizeof(diagnostic_json),
                 "%s/step%04d_pair%d_local_patch_mvp.json",
                 diagnostics_dir,
                 step,
                 pair->pair_id) >= (int)sizeof(diagnostic_json)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic local patch artifact path is too long");
    }

    contact_patch2d_zero(&metadata_patches[0]);
    contact_patch2d_zero(&metadata_patches[1]);
    contact_patch2d_zero(&receiver_patches[0]);
    contact_patch2d_zero(&receiver_patches[1]);
    contact_patch_load2d_result_zero(&receiver_results[0]);
    contact_patch_load2d_result_zero(&receiver_results[1]);

    fn_macro_proxy = mbd_system2d_monolithic_local_patch_fn_macro_proxy(
        gap,
        penetration,
        v_n,
        v_t,
        step,
        1);
    CHECK_ERROR(contact_patch2d_build(&metadata_patches[0],
                                      0,
                                      pair->pair_id,
                                      step,
                                      time_value,
                                      contact_point_world,
                                      normal_world,
                                      MBD_MONOLITHIC_PATCH_MVP_RADIUS_SMALL,
                                      gap,
                                      fn_macro_proxy,
                                      MBD_MONOLITHIC_PATCH_MVP_THICKNESS,
                                      MBD_MONOLITHIC_PATCH_MVP_PATCH_SIZE,
                                      patch_metadata_dir));
    CHECK_ERROR(contact_patch2d_build(&metadata_patches[1],
                                      1,
                                      pair->pair_id,
                                      step,
                                      time_value,
                                      contact_point_world,
                                      normal_world,
                                      MBD_MONOLITHIC_PATCH_MVP_RADIUS_LARGE,
                                      gap,
                                      fn_macro_proxy,
                                      MBD_MONOLITHIC_PATCH_MVP_THICKNESS,
                                      MBD_MONOLITHIC_PATCH_MVP_PATCH_SIZE,
                                      patch_metadata_dir));
    CHECK_ERROR(contact_patch2d_build_metadata_path(&metadata_patches[0],
                                                    patch_metadata_dir,
                                                    row->metadata_small_json));
    CHECK_ERROR(contact_patch2d_build_metadata_path(&metadata_patches[1],
                                                    patch_metadata_dir,
                                                    row->metadata_large_json));
    CHECK_ERROR(contact_patch2d_write_metadata_json(&metadata_patches[0],
                                                    row->metadata_small_json));
    CHECK_ERROR(contact_patch2d_write_metadata_json(&metadata_patches[1],
                                                    row->metadata_large_json));

    receiver_patches[0] = metadata_patches[0];
    receiver_patches[1] = metadata_patches[1];
    receiver_patches[0].contact_point_world[0] = MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_SMALL_X;
    receiver_patches[0].contact_point_world[1] = MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_SMALL_Y;
    receiver_patches[1].contact_point_world[0] = MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_LARGE_X;
    receiver_patches[1].contact_point_world[1] = MBD_MONOLITHIC_PATCH_MVP_LOCAL_CP_LARGE_Y;
    if (snprintf(receiver_patches[0].mesh_path, sizeof(receiver_patches[0].mesh_path),
                 "%s", MBD_MONOLITHIC_PATCH_MVP_FIXTURE_SMALL) >=
            (int)sizeof(receiver_patches[0].mesh_path) ||
        snprintf(receiver_patches[1].mesh_path, sizeof(receiver_patches[1].mesh_path),
                 "%s", MBD_MONOLITHIC_PATCH_MVP_FIXTURE_LARGE) >=
            (int)sizeof(receiver_patches[1].mesh_path) ||
        snprintf(receiver_patches[0].output_path, sizeof(receiver_patches[0].output_path),
                 "%s/patch_pair%d_body0_step%04d_receiver_summary.json",
                 patch_receiver_dir, pair->pair_id, step) >=
            (int)sizeof(receiver_patches[0].output_path) ||
        snprintf(receiver_patches[1].output_path, sizeof(receiver_patches[1].output_path),
                 "%s/patch_pair%d_body1_step%04d_receiver_summary.json",
                 patch_receiver_dir, pair->pair_id, step) >=
            (int)sizeof(receiver_patches[1].output_path)) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "monolithic receiver artifact path is too long");
    }
    CHECK_ERROR(contact_patch_load2d_run_fixture_static(&receiver_patches[0],
                                                        &receiver_results[0]));
    CHECK_ERROR(contact_patch_load2d_run_fixture_static(&receiver_patches[1],
                                                        &receiver_results[1]));
    snprintf(row->receiver_small_json, sizeof(row->receiver_small_json), "%s",
             receiver_results[0].summary_path);
    snprintf(row->receiver_large_json, sizeof(row->receiver_large_json), "%s",
             receiver_results[1].summary_path);

    disp_mean = 0.5 * (fabs(receiver_results[0].displacement_centroid_local[1]) +
                       fabs(receiver_results[1].displacement_centroid_local[1]));
    reaction_mean = 0.5 * (fabs(receiver_results[0].reaction_resultant_local[1]) +
                           fabs(receiver_results[1].reaction_resultant_local[1]));
    reduced_out->gamma_n = 1.0 + 1.0e6 * disp_mean + 1.0e3 * fmax(0.0, penetration);
    reduced_out->delta_g_eff = gap + disp_mean;
    reduced_out->fn_ref = reaction_mean;
    reduced_out->p_max = reaction_mean / fmax(MBD_MONOLITHIC_PATCH_MVP_PATCH_SIZE *
                                                  MBD_MONOLITHIC_PATCH_MVP_THICKNESS,
                                              1.0e-12);
    reduced_out->valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_DELTA_G_EFF |
                               MBD_CONTACT_FEEDBACK2D_VALID_FN_REF |
                               MBD_CONTACT_FEEDBACK2D_VALID_P_MAX;

    CHECK_ERROR(mbd_system2d_monolithic_local_patch_write_diag_json(
        diagnostic_json,
        row->metadata_small_json,
        row->metadata_large_json,
        &receiver_results[0],
        &receiver_results[1],
        step,
        pair->pair_id,
        time_value,
        gap,
        penetration,
        fn_macro_proxy,
        reduced_out));
    snprintf(row->diagnostic_json, sizeof(row->diagnostic_json), "%s", diagnostic_json);
    row->reduced_data = *reduced_out;
    row->status_ok = 1;
    snprintf(row->fallback_reason, sizeof(row->fallback_reason), "%s", "none");
    snprintf(row->status, sizeof(row->status), "%s", "ok");

    *status_ok_out = 1;
    *fallback_reason_out = "none";
    *status_out = "ok";
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_capture_body_summary_fields(
    const mbd_body2d_t *body,
    double q[MBD_BODY2D_DOF],
    double v[MBD_BODY2D_DOF],
    double force[MBD_BODY2D_DOF])
{
    double a[MBD_BODY2D_DOF];

    CHECK_NULL(body, "MBD body");
    CHECK_NULL(q, "MBD body coordinates");
    CHECK_NULL(v, "MBD body velocity");
    CHECK_NULL(force, "MBD body force");
    CHECK_ERROR(mbd_body2d_get_generalized_state(body, q, v, a));
    CHECK_ERROR(mbd_body2d_get_generalized_force(body, force));
    if (!isfinite(a[0]) || !isfinite(a[1]) || !isfinite(a[2])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output body acceleration must be finite");
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_print_body_summary(int body_index,
                                                   const mbd_body2d_t *body)
{
    double q[MBD_BODY2D_DOF];
    double v[MBD_BODY2D_DOF];
    double force[MBD_BODY2D_DOF];

    CHECK_ERROR(mbd_system2d_capture_body_summary_fields(body, q, v, force));
    printf("  body[%d]: id=%d mass=%.6e inertia=%.6e q=(%.6e, %.6e, %.6e) v=(%.6e, %.6e, %.6e) force=(%.6e, %.6e, %.6e) ground=%d\n",
           body_index,
           body->id,
           body->mass,
           body->inertia,
           q[0],
           q[1],
           q[2],
           v[0],
           v[1],
           v[2],
           force[0],
           force[1],
           force[2],
           body->is_ground);
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_print_generalized_force_history_summary(
    const mbd_system2d_t *system)
{
    int body_index;
    int dof;

    CHECK_NULL(system, "MBD system");

    printf("  generalized_force_history_valid: %d\n",
           system->generalized_force_history_valid ? 1 : 0);
    if (!system->generalized_force_history_valid) {
        return FEM_SUCCESS;
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
            if (!isfinite(system->current_generalized_force[body_index][dof]) ||
                !isfinite(system->previous_generalized_force[body_index][dof])) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD generalized force history summary must be finite");
            }
        }
        printf("  generalized_force_current[%d]: id=%d value=(%.6e, %.6e, %.6e)\n",
               body_index,
               system->bodies[body_index].id,
               system->current_generalized_force[body_index][0],
               system->current_generalized_force[body_index][1],
               system->current_generalized_force[body_index][2]);
        printf("  generalized_force_previous[%d]: id=%d value=(%.6e, %.6e, %.6e)\n",
               body_index,
               system->bodies[body_index].id,
               system->previous_generalized_force[body_index][0],
               system->previous_generalized_force[body_index][1],
               system->previous_generalized_force[body_index][2]);
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_write_body_output_row(FILE *out,
                                                      int body_index,
                                                      const mbd_body2d_t *body)
{
    double q[MBD_BODY2D_DOF];
    double v[MBD_BODY2D_DOF];
    double force[MBD_BODY2D_DOF];

    CHECK_NULL(out, "MBD output file");
    CHECK_ERROR(mbd_system2d_capture_body_summary_fields(body, q, v, force));
    if (fprintf(out,
                "body,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%d\n",
                body_index,
                body->id,
                body->mass,
                body->inertia,
                q[0],
                q[1],
                q[2],
                v[0],
                v[1],
                v[2],
                force[0],
                force[1],
                force[2],
                body->is_ground) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD output body row");
    }

    return FEM_SUCCESS;
}

static const char *mbd_system2d_constraint_directives_csv(void)
{
    return "MBD_DISTANCE|MBD_REVOLUTE";
}

static void mbd_system2d_reset_implicit_trace(mbd_system2d_t *system,
                                              const char *residual_mode,
                                              const char *scheme_mode,
                                              double residual_tolerance)
{
    if (!system) {
        return;
    }

    system->time.implicit_iterations_last = 0;
    system->implicit_residual_l2_last = 0.0;
    system->implicit_residual_tolerance_last = residual_tolerance;
    system->implicit_residual_num_equations_last = 0;
    system->implicit_converged = 0;
    system->implicit_residual_mode = residual_mode ? residual_mode : MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT;
    system->implicit_scheme_mode = scheme_mode ? scheme_mode : MBD_IMPLICIT_SCHEME_NOT_APPLICABLE;
    system->implicit_convergence_reason = MBD_IMPLICIT_REASON_NOT_RUN;
}

static const mbd_contact_circle2d_t *mbd_system2d_find_contact_circle_const(
    const mbd_system2d_t *system,
    int body_id)
{
    int i;

    if (!system) {
        return NULL;
    }

    for (i = 0; i < MBD_SYSTEM2D_MAX_BODIES; ++i) {
        if (system->contact_circles[i].is_defined &&
            system->contact_circles[i].body_id == body_id) {
            return &system->contact_circles[i];
        }
    }

    return NULL;
}

static const mbd_contact_halfspace2d_t *mbd_system2d_find_contact_halfspace_const(
    const mbd_system2d_t *system,
    int halfspace_id)
{
    int i = 0;

    if (!system) {
        return NULL;
    }

    for (i = 0; i < MBD_CONTACT2D_MAX_PAIRS; ++i) {
        if (system->contact_halfspaces[i].is_defined &&
            system->contact_halfspaces[i].halfspace_id == halfspace_id) {
            return &system->contact_halfspaces[i];
        }
    }

    return NULL;
}

static const mbd_contact_surface_polyline2d_t *
mbd_system2d_find_contact_surface_polyline_const(const mbd_system2d_t *system,
                                                 int surface_id)
{
    int i = 0;

    if (!system) {
        return NULL;
    }

    for (i = 0; i < system->num_contact_surface_polylines; ++i) {
        if (system->contact_surface_polylines[i].is_defined &&
            system->contact_surface_polylines[i].surface_id == surface_id) {
            return &system->contact_surface_polylines[i];
        }
    }

    return NULL;
}

static fem_error_t mbd_system2d_load_contact_surface_polyline_csv(
    const char *csv_path,
    mbd_contact_surface_polyline_cache2d_t *surface_out)
{
    FILE *fp = NULL;
    char line[512];
    int line_no = 0;

    CHECK_NULL(csv_path, "generic contact surface csv_path");
    CHECK_NULL(surface_out, "generic contact surface cache");

    memset(surface_out, 0, sizeof(*surface_out));
    fp = fopen(csv_path, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open generic contact surface CSV: %s",
                         csv_path);
    }

    while (fgets(line, sizeof(line), fp)) {
        char *cursor = line;
        char *comma = NULL;
        char *endptr_x = NULL;
        char *endptr_y = NULL;
        double x = 0.0;
        double y = 0.0;

        line_no += 1;
        while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0' || *cursor == '\n' || *cursor == '#') {
            continue;
        }
        if (line_no == 1 &&
            (strncmp(cursor, "x_m", 3) == 0 || strncmp(cursor, "x,", 2) == 0)) {
            continue;
        }

        comma = strchr(cursor, ',');
        if (!comma) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact surface CSV requires x_m,y_m rows: %s line %d",
                             csv_path,
                             line_no);
        }
        *comma = '\0';
        x = strtod(cursor, &endptr_x);
        y = strtod(comma + 1, &endptr_y);
        if (endptr_x == cursor || endptr_y == comma + 1 ||
            !isfinite(x) || !isfinite(y)) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact surface CSV row must contain finite x/y: %s line %d",
                             csv_path,
                             line_no);
        }
        if (surface_out->num_points >= MBD_GENERIC_CONTACT2D_MAX_SURFACE_POINTS) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic contact surface CSV exceeds supported point count %d: %s",
                             MBD_GENERIC_CONTACT2D_MAX_SURFACE_POINTS,
                             csv_path);
        }
        surface_out->local_points[surface_out->num_points][0] = x;
        surface_out->local_points[surface_out->num_points][1] = y;
        surface_out->num_points += 1;
    }

    fclose(fp);
    if (surface_out->num_points < 2) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact surface CSV requires at least 2 points: %s",
                         csv_path);
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_world_point_velocity(const mbd_body2d_t *body,
                                                     const double local_point[2],
                                                     double world_point[2],
                                                     double world_velocity[2])
{
    mbd_body_state2d_t state;
    double rotated[2];

    CHECK_NULL(body, "generic contact body");
    CHECK_NULL(local_point, "generic contact local point");
    CHECK_NULL(world_point, "generic contact world point");
    CHECK_NULL(world_velocity, "generic contact world velocity");

    CHECK_ERROR(mbd_body2d_to_state_view(body, &state));
    CHECK_ERROR(mbd_kinematics2d_local_point_to_world(&state, local_point, world_point));
    CHECK_ERROR(mbd_kinematics2d_rotate_local_vector(local_point, state.theta, rotated));
    world_velocity[0] = body->v[0] + (-body->v[2] * rotated[1]);
    world_velocity[1] = body->v[1] + (+body->v[2] * rotated[0]);
    if (!isfinite(world_velocity[0]) || !isfinite(world_velocity[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact world velocity must be finite");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_append_generic_contact_trace_row(
    mbd_system2d_t *system,
    const mbd_contact_generic_trace2d_t *row)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(row, "generic contact trace row");

    if (system->num_current_generic_contact_trace_rows >= MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact trace exceeds supported row count %d",
                         MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS);
    }
    system->current_generic_contact_trace_rows[system->num_current_generic_contact_trace_rows] = *row;
    system->num_current_generic_contact_trace_rows += 1;
    return FEM_SUCCESS;
}

static double mbd_system2d_generic_contact_mu_used(
    const mbd_contact_generic_pair2d_t *pair,
    double v_t)
{
    double speed = 0.0;
    double v_ref = 0.0;
    double weight = 0.0;

    if (!pair) {
        return 0.0;
    }

    speed = fabs(v_t);
    v_ref = fmax(pair->v_ref, MBD_CONTACT_TANGENTIAL_SPEED_EPS);
    weight = exp(-speed / v_ref);
    return fmax(0.0, pair->mu_dynamic + (pair->mu_static - pair->mu_dynamic) * weight);
}

static double mbd_system2d_generic_contact_slip_direction(
    const mbd_contact_generic_pair2d_t *pair,
    double v_t)
{
    double v_smooth = MBD_GENERIC_CONTACT_FRICTION_VSMOOTH_DEFAULT;

    if (pair) {
        v_smooth = fmax(pair->v_smooth, MBD_CONTACT_TANGENTIAL_SPEED_EPS);
    }
    return v_t / sqrt(v_t * v_t + v_smooth * v_smooth);
}

static fem_error_t mbd_system2d_append_generic_contact_replay_use_row(
    const mbd_generic_contact_replay_use_row2d_t *row)
{
    CHECK_NULL(row, "generic contact replay-use row");

    if (mbd_num_generic_contact_replay_use_rows >= MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact replay-use exceeds supported row count %d",
                         MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS);
    }
    mbd_generic_contact_replay_use_rows[mbd_num_generic_contact_replay_use_rows] = *row;
    mbd_num_generic_contact_replay_use_rows += 1;
    return FEM_SUCCESS;
}

static void mbd_system2d_clear_same_time_reduced_iterations(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }

    system->same_time_reduced_override_active = 0;
    memset(system->same_time_reduced_interface_data,
           0,
           sizeof(system->same_time_reduced_interface_data));
    memset(system->same_time_reduced_mu_used, 0, sizeof(system->same_time_reduced_mu_used));
    memset(system->same_time_reduced_gamma_n_used, 0, sizeof(system->same_time_reduced_gamma_n_used));
    memset(system->same_time_reduced_record_iter, 0, sizeof(system->same_time_reduced_record_iter));
    memset(system->same_time_reduced_status_ok, 0, sizeof(system->same_time_reduced_status_ok));
    memset(system->same_time_reduced_fallback_reason,
           0,
           sizeof(system->same_time_reduced_fallback_reason));
    memset(system->same_time_reduced_status, 0, sizeof(system->same_time_reduced_status));
    memset(system->same_time_reduced_vt, 0, sizeof(system->same_time_reduced_vt));
    memset(system->current_same_time_reduced_iterations,
           0,
           sizeof(system->current_same_time_reduced_iterations));
    system->num_current_same_time_reduced_iterations = 0;
}

static mbd_contact_feedback2d_t mbd_contact_feedback2d_make_basic(double mu_eff,
                                                                  double gamma_n)
{
    mbd_contact_feedback2d_t feedback;

    memset(&feedback, 0, sizeof(feedback));
    feedback.mu_eff = mu_eff;
    feedback.gamma_n = gamma_n;
    feedback.valid_flag = MBD_CONTACT_FEEDBACK2D_VALID_MU_EFF |
                          MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N;
    return feedback;
}

static int mbd_system2d_same_time_reduced_has_split_sources(const mbd_system2d_t *system)
{
    if (!system) {
        return 0;
    }

    return system->local_contact_filename[0] != '\0' ||
           system->ehl_filename[0] != '\0';
}

static int mbd_system2d_find_same_time_local_contact_source_row(
    const char *path,
    int step,
    int iter,
    int pair_id,
    mbd_same_time_local_contact_source_row_t *row_out)
{
    FILE *fp = NULL;
    char line[512];

    if (!path || path[0] == '\0' || !row_out || step < 0 || iter <= 0 || pair_id < 0) {
        return 0;
    }

    memset(row_out, 0, sizeof(*row_out));
    fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[9] = {NULL};
        int field_count = 0;
        mbd_same_time_local_contact_source_row_t row;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "step,", 5) == 0) {
            continue;
        }

        memset(&row, 0, sizeof(row));
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 9);
        if (field_count < 9) {
            continue;
        }
        if (sscanf(fields[0], "%d", &row.step) != 1 ||
            sscanf(fields[1], "%d", &row.iter) != 1 ||
            sscanf(fields[2], "%d", &row.pair_id) != 1 ||
            sscanf(fields[3], "%lf", &row.gamma_n) != 1 ||
            sscanf(fields[4], "%lf", &row.delta_g_eff) != 1 ||
            sscanf(fields[5], "%lf", &row.fn_ref) != 1 ||
            sscanf(fields[6], "%lf", &row.p_max) != 1 ||
            !mbd_system2d_parse_uint32_field(fields[7], &row.valid_flag)) {
            continue;
        }
        if (row.step != step || row.iter != iter || row.pair_id != pair_id) {
            continue;
        }

        row.is_defined = 1;
        snprintf(row.status, sizeof(row.status), "%s", fields[8]);
        *row_out = row;
        mbd_system2d_close_file_quiet(&fp);
        return 1;
    }

    mbd_system2d_close_file_quiet(&fp);
    return 0;
}

static int mbd_system2d_find_same_time_ehl_source_row(
    const char *path,
    int step,
    int iter,
    int pair_id,
    mbd_same_time_ehl_source_row_t *row_out)
{
    FILE *fp = NULL;
    char line[512];

    if (!path || path[0] == '\0' || !row_out || step < 0 || iter <= 0 || pair_id < 0) {
        return 0;
    }

    memset(row_out, 0, sizeof(*row_out));
    fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[8] = {NULL};
        int field_count = 0;
        mbd_same_time_ehl_source_row_t row;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "step,", 5) == 0) {
            continue;
        }

        memset(&row, 0, sizeof(row));
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 8);
        if (field_count < 8) {
            continue;
        }
        if (sscanf(fields[0], "%d", &row.step) != 1 ||
            sscanf(fields[1], "%d", &row.iter) != 1 ||
            sscanf(fields[2], "%d", &row.pair_id) != 1 ||
            sscanf(fields[3], "%lf", &row.mu_eff) != 1 ||
            sscanf(fields[4], "%lf", &row.h_min) != 1 ||
            !mbd_system2d_parse_uint32_field(fields[5], &row.regime_flag) ||
            !mbd_system2d_parse_uint32_field(fields[6], &row.valid_flag)) {
            continue;
        }
        if (row.step != step || row.iter != iter || row.pair_id != pair_id) {
            continue;
        }

        row.is_defined = 1;
        snprintf(row.status, sizeof(row.status), "%s", fields[7]);
        *row_out = row;
        mbd_system2d_close_file_quiet(&fp);
        return 1;
    }

    mbd_system2d_close_file_quiet(&fp);
    return 0;
}

static int mbd_system2d_find_generic_same_time_contact_source_row(
    const char *path,
    int step,
    int pair_id,
    int slave_vertex_id,
    int master_segment_id,
    mbd_generic_same_time_contact_source_row_t *row_out)
{
    FILE *fp = NULL;
    char line[512];

    if (!path || path[0] == '\0' || !row_out || step < 0 || pair_id < 0 ||
        slave_vertex_id < 0 || master_segment_id < 0) {
        return 0;
    }

    memset(row_out, 0, sizeof(*row_out));
    fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[12] = {NULL};
        int field_count = 0;
        mbd_generic_same_time_contact_source_row_t row;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "protocol_version,", 17) == 0) {
            continue;
        }

        memset(&row, 0, sizeof(row));
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 12);
        if (field_count < 12) {
            continue;
        }
        if (strcmp(fields[0], "mbd_macro_local_same_time_generic_contract_v1") != 0 ||
            strcmp(fields[1], "mbd_macro_local_same_time_generic_response_v1") != 0) {
            continue;
        }
        if (sscanf(fields[2], "%d", &row.step) != 1 ||
            sscanf(fields[3], "%d", &row.pair_id) != 1 ||
            sscanf(fields[4], "%d", &row.slave_vertex_id) != 1 ||
            sscanf(fields[5], "%d", &row.master_segment_id) != 1 ||
            sscanf(fields[6], "%lf", &row.gamma_n) != 1 ||
            sscanf(fields[7], "%lf", &row.delta_g_eff) != 1 ||
            sscanf(fields[8], "%lf", &row.fn_ref) != 1 ||
            sscanf(fields[9], "%lf", &row.p_max) != 1 ||
            !mbd_system2d_parse_uint32_field(fields[10], &row.valid_flag)) {
            continue;
        }
        if (row.step != step || row.pair_id != pair_id ||
            row.slave_vertex_id != slave_vertex_id ||
            row.master_segment_id != master_segment_id) {
            continue;
        }

        row.is_defined = 1;
        snprintf(row.status, sizeof(row.status), "%s", fields[11]);
        *row_out = row;
        mbd_system2d_close_file_quiet(&fp);
        return 1;
    }

    mbd_system2d_close_file_quiet(&fp);
    return 0;
}

static fem_error_t mbd_system2d_lookup_same_time_reduced_replay(
    const mbd_system2d_t *system,
    const mbd_contact_pair2d_t *pair,
    int step,
    int iter,
    mbd_same_time_reduced_lookup_result2d_t *result_out)
{
    mbd_same_time_local_contact_source_row_t local_row;
    mbd_same_time_ehl_source_row_t ehl_row;
    int local_found = 0;
    int ehl_found = 0;
    int local_ok = 0;
    int ehl_ok = 0;
    const char *fallback_reason = "none";
    const char *status = "ok";

    CHECK_NULL(system, "mbd same-time replay system");
    CHECK_NULL(pair, "mbd same-time replay pair");
    CHECK_NULL(result_out, "mbd same-time replay result");

    memset(result_out, 0, sizeof(*result_out));
    result_out->reduced_data = mbd_contact_feedback2d_make_basic(pair->base_mu, 1.0);

    local_found = mbd_system2d_find_same_time_local_contact_source_row(system->local_contact_filename,
                                                                       step,
                                                                       iter,
                                                                       pair->pair_id,
                                                                       &local_row);
    if (!local_found) {
        fallback_reason = system->local_contact_filename[0] == '\0'
                              ? "no_local_source"
                              : "no_local_row";
    } else if (local_row.valid_flag == 0u) {
        fallback_reason = "invalid_local_row";
    } else if (!mbd_local_feedback_status_is_ok(local_row.status)) {
        fallback_reason = "status_not_ok_local";
    } else if (!isfinite(local_row.gamma_n) ||
               !isfinite(local_row.delta_g_eff) ||
               !isfinite(local_row.fn_ref) ||
               !isfinite(local_row.p_max)) {
        fallback_reason = "non_finite_local";
    } else {
        local_ok = 1;
        result_out->reduced_data.gamma_n = local_row.gamma_n;
        result_out->reduced_data.delta_g_eff = local_row.delta_g_eff;
        result_out->reduced_data.fn_ref = local_row.fn_ref;
        result_out->reduced_data.p_max = local_row.p_max;
        result_out->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N |
                                               MBD_CONTACT_FEEDBACK2D_VALID_DELTA_G_EFF |
                                               MBD_CONTACT_FEEDBACK2D_VALID_FN_REF |
                                               MBD_CONTACT_FEEDBACK2D_VALID_P_MAX;
    }

    ehl_found = mbd_system2d_find_same_time_ehl_source_row(system->ehl_filename,
                                                           step,
                                                           iter,
                                                           pair->pair_id,
                                                           &ehl_row);
    if (!ehl_found) {
        if (strcmp(fallback_reason, "none") == 0) {
            fallback_reason = system->ehl_filename[0] == '\0'
                                  ? "no_ehl_source"
                                  : "no_ehl_row";
        }
    } else if (ehl_row.valid_flag == 0u) {
        if (strcmp(fallback_reason, "none") == 0) {
            fallback_reason = "invalid_ehl_row";
        }
    } else if (!mbd_local_feedback_status_is_ok(ehl_row.status)) {
        if (strcmp(fallback_reason, "none") == 0) {
            fallback_reason = "status_not_ok_ehl";
        }
    } else if (!isfinite(ehl_row.mu_eff) || !isfinite(ehl_row.h_min)) {
        if (strcmp(fallback_reason, "none") == 0) {
            fallback_reason = "non_finite_ehl";
        }
    } else {
        ehl_ok = 1;
        result_out->reduced_data.mu_eff = ehl_row.mu_eff;
        result_out->reduced_data.h_min = ehl_row.h_min;
        result_out->reduced_data.regime_flag = ehl_row.regime_flag;
        result_out->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_MU_EFF |
                                               MBD_CONTACT_FEEDBACK2D_VALID_H_MIN |
                                               MBD_CONTACT_FEEDBACK2D_VALID_REGIME_FLAG;
    }

    if (local_ok && ehl_ok) {
        result_out->status_ok = 1;
        snprintf(result_out->fallback_reason,
                 sizeof(result_out->fallback_reason),
                 "%s",
                 "none");
        snprintf(result_out->status, sizeof(result_out->status), "%s", "ok");
        return FEM_SUCCESS;
    }

    status = "fallback";
    result_out->status_ok = 0;
    snprintf(result_out->fallback_reason,
             sizeof(result_out->fallback_reason),
             "%s",
             fallback_reason);
    snprintf(result_out->status, sizeof(result_out->status), "%s", status);
    return FEM_SUCCESS;
}

static double mbd_system2d_same_time_reduced_mu_target(const mbd_contact_pair2d_t *pair,
                                                       int active,
                                                       double v_t)
{
    double target = 0.0;

    if (!pair) {
        return MBD_LOCAL_FEEDBACK_MU_MIN;
    }

    target = pair->base_mu;
    if (active) {
        target += 1.5e-1 * tanh(fabs(v_t) / 2.0e-1);
    }

    return mbd_local_feedback_clip(target,
                                   MBD_LOCAL_FEEDBACK_MU_MIN,
                                   MBD_LOCAL_FEEDBACK_MU_MAX);
}

static double mbd_system2d_same_time_reduced_gamma_target(
    const mbd_contact_pair2d_t *pair,
    int active,
    double penetration,
    double fn)
{
    double target = 1.0;
    double fn_scale = 1.0;

    if (!pair) {
        return 1.0;
    }

    if (active) {
        fn_scale = fmax(pair->base_k_n * 1.0e-3, 1.0);
        target += 1.5e-1 * tanh(fn / fn_scale);
        target += 1.0e-1 * tanh(penetration / 1.0e-3);
    }

    return mbd_local_feedback_clip(target,
                                   MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                                   MBD_LOCAL_FEEDBACK_GAMMA_MAX);
}

static fem_error_t mbd_system2d_run_same_time_reduced_iteration(mbd_system2d_t *system)
{
    double mu_guess[MBD_CONTACT2D_MAX_PAIRS];
    double gamma_guess[MBD_CONTACT2D_MAX_PAIRS];
    double mu_next[MBD_CONTACT2D_MAX_PAIRS];
    double gamma_next[MBD_CONTACT2D_MAX_PAIRS];
    mbd_contact_feedback2d_t reduced_next[MBD_CONTACT2D_MAX_PAIRS];
    mbd_same_time_reduced_lookup_result2d_t replay_lookup[MBD_CONTACT2D_MAX_PAIRS];
    double previous_fn[MBD_CONTACT2D_MAX_PAIRS];
    int has_previous_fn[MBD_CONTACT2D_MAX_PAIRS];
    const int same_time_replay_enabled = mbd_system2d_same_time_reduced_has_split_sources(system);
    int iter = 0;
    int pair_index = 0;

    CHECK_NULL(system, "mbd_system2d");

    mbd_system2d_clear_same_time_reduced_iterations(system);
    if (system->num_contact_pairs <= 0) {
        return mbd_system2d_refresh_contact_forces_and_trace(system);
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_contact_pair2d_t *pair = &system->contact_pairs[pair_index];

        mu_guess[pair_index] = pair->base_mu;
        gamma_guess[pair_index] = 1.0;
        mu_next[pair_index] = pair->base_mu;
        gamma_next[pair_index] = 1.0;
        reduced_next[pair_index] = mbd_contact_feedback2d_make_basic(pair->base_mu, 1.0);
        memset(&replay_lookup[pair_index], 0, sizeof(replay_lookup[pair_index]));
        replay_lookup[pair_index].status_ok = 1;
        system->same_time_reduced_status_ok[pair_index] = 1;
        snprintf(system->same_time_reduced_fallback_reason[pair_index],
                 sizeof(system->same_time_reduced_fallback_reason[pair_index]),
                 "%s",
                 "none");
        snprintf(system->same_time_reduced_status[pair_index],
                 sizeof(system->same_time_reduced_status[pair_index]),
                 "%s",
                 "same_time");
        snprintf(replay_lookup[pair_index].fallback_reason,
                 sizeof(replay_lookup[pair_index].fallback_reason),
                 "%s",
                 "none");
        snprintf(replay_lookup[pair_index].status,
                 sizeof(replay_lookup[pair_index].status),
                 "%s",
                 "same_time");
        previous_fn[pair_index] = 0.0;
        has_previous_fn[pair_index] = 0;
    }

    for (iter = 1; iter <= MBD_SAME_TIME_REDUCED_MAX_ITERS; ++iter) {
        int converged_all = 1;
        int final_iter = 0;
        const char *stop_reason = "continue";

        system->same_time_reduced_override_active = 1;
        for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
            system->same_time_reduced_mu_used[pair_index] = mu_guess[pair_index];
            system->same_time_reduced_gamma_n_used[pair_index] = gamma_guess[pair_index];
            system->same_time_reduced_interface_data[pair_index] =
                mbd_contact_feedback2d_make_basic(mu_guess[pair_index],
                                                  gamma_guess[pair_index]);
        }

        CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace_internal(system, 0));

        for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
            const mbd_contact_pair2d_t *pair = &system->contact_pairs[pair_index];
            const mbd_contact_trace2d_t *trace = &system->current_contact_trace[pair_index];
            const char *source_mode_before_lookup =
                same_time_replay_enabled ? "replay_lookup" : "built_in_relaxation";
            double mu_target = 0.0;
            double gamma_target = 1.0;
            double mu_delta = 0.0;
            double gamma_delta = 0.0;
            double fn_delta = 0.0;

            if (system->num_same_time_contact_request_rows >=
                (int)(sizeof(system->same_time_contact_requests) /
                      sizeof(system->same_time_contact_requests[0]))) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "same-time contact request trace exceeds supported storage");
            }
            {
                mbd_same_time_contact_request2d_t *request_row =
                    &system->same_time_contact_requests[system->num_same_time_contact_request_rows++];
                const double x_cp = 0.5 * (trace->cp1[0] + trace->cp2[0]);
                const double y_cp = 0.5 * (trace->cp1[1] + trace->cp2[1]);
                const double t_x = -trace->normal[1];
                const double t_y = trace->normal[0];

                memset(request_row, 0, sizeof(*request_row));
                request_row->is_defined = 1;
                request_row->step = system->current_step_index;
                request_row->iter = iter;
                request_row->pair_id = pair->pair_id;
                request_row->body_i = pair->body_i;
                request_row->body_j = pair->body_j;
                request_row->request_valid = trace->is_defined ? 1 : 0;
                request_row->contact_active = trace->active;
                request_row->x_cp = x_cp;
                request_row->y_cp = y_cp;
                request_row->n_x = trace->normal[0];
                request_row->n_y = trace->normal[1];
                request_row->t_x = t_x;
                request_row->t_y = t_y;
                request_row->gap = trace->gap;
                request_row->penetration = trace->penetration;
                request_row->v_n = trace->vn;
                request_row->v_t = system->same_time_reduced_vt[pair_index];
                snprintf(request_row->source_mode_before_lookup,
                         sizeof(request_row->source_mode_before_lookup),
                         "%s",
                         source_mode_before_lookup);
            }

            if (same_time_replay_enabled) {
                CHECK_ERROR(mbd_system2d_lookup_same_time_reduced_replay(system,
                                                                         pair,
                                                                         system->current_step_index,
                                                                         iter,
                                                                         &replay_lookup[pair_index]));
                reduced_next[pair_index] = replay_lookup[pair_index].reduced_data;
                mu_next[pair_index] = reduced_next[pair_index].mu_eff;
                gamma_next[pair_index] = reduced_next[pair_index].gamma_n;
            } else {
                mu_target = mbd_system2d_same_time_reduced_mu_target(
                    pair,
                    trace->active,
                    system->same_time_reduced_vt[pair_index]);
                gamma_target = mbd_system2d_same_time_reduced_gamma_target(pair,
                                                                           trace->active,
                                                                           trace->penetration,
                                                                           trace->fn);
                mu_next[pair_index] = mu_guess[pair_index] +
                                      MBD_SAME_TIME_REDUCED_RELAXATION *
                                          (mu_target - mu_guess[pair_index]);
                gamma_next[pair_index] = gamma_guess[pair_index] +
                                         MBD_SAME_TIME_REDUCED_RELAXATION *
                                             (gamma_target - gamma_guess[pair_index]);
                mu_next[pair_index] = mbd_local_feedback_clip(mu_next[pair_index],
                                                              MBD_LOCAL_FEEDBACK_MU_MIN,
                                                              MBD_LOCAL_FEEDBACK_MU_MAX);
                gamma_next[pair_index] = mbd_local_feedback_clip(gamma_next[pair_index],
                                                                 MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                                                                 MBD_LOCAL_FEEDBACK_GAMMA_MAX);
                reduced_next[pair_index] = mbd_contact_feedback2d_make_basic(mu_next[pair_index],
                                                                              gamma_next[pair_index]);
            }

            mu_delta = fabs(mu_next[pair_index] - mu_guess[pair_index]);
            gamma_delta = fabs(gamma_next[pair_index] - gamma_guess[pair_index]);
            fn_delta = has_previous_fn[pair_index]
                           ? fabs(trace->fn - previous_fn[pair_index])
                           : MBD_SAME_TIME_REDUCED_TOL_FN + 1.0;

            if (mu_delta > MBD_SAME_TIME_REDUCED_TOL_MU ||
                gamma_delta > MBD_SAME_TIME_REDUCED_TOL_GAMMA ||
                (has_previous_fn[pair_index] && fn_delta > MBD_SAME_TIME_REDUCED_TOL_FN)) {
                converged_all = 0;
            }
        }

        final_iter = converged_all || iter == MBD_SAME_TIME_REDUCED_MAX_ITERS;
        if (final_iter) {
            stop_reason = converged_all ? "converged" : "max_iter";
        }

        for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
            const mbd_contact_trace2d_t *trace = &system->current_contact_trace[pair_index];
            mbd_same_time_reduced_iteration2d_t *row = NULL;

            if (system->num_current_same_time_reduced_iterations >=
                (int)(sizeof(system->current_same_time_reduced_iterations) /
                      sizeof(system->current_same_time_reduced_iterations[0]))) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "same-time reduced iteration trace exceeds supported storage");
            }

            row = &system->current_same_time_reduced_iterations
                       [system->num_current_same_time_reduced_iterations++];
            memset(row, 0, sizeof(*row));
            row->is_defined = 1;
            row->step = system->current_step_index;
            row->iter = iter;
            row->pair_id = trace->pair_id;
            row->mu_guess = mu_guess[pair_index];
            row->mu_new = mu_next[pair_index];
            row->gamma_n_guess = gamma_guess[pair_index];
            row->gamma_n_new = gamma_next[pair_index];
            row->fn = trace->fn;
            row->gap = trace->gap;
            row->vt = system->same_time_reduced_vt[pair_index];
            snprintf(row->stop_reason, sizeof(row->stop_reason), "%s", stop_reason);

            previous_fn[pair_index] = trace->fn;
            has_previous_fn[pair_index] = 1;
        }

        for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
            mu_guess[pair_index] = mu_next[pair_index];
            gamma_guess[pair_index] = gamma_next[pair_index];
        }

        if (final_iter) {
            break;
        }
    }

    system->same_time_reduced_override_active = 1;
    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        system->same_time_reduced_mu_used[pair_index] = mu_guess[pair_index];
        system->same_time_reduced_gamma_n_used[pair_index] = gamma_guess[pair_index];
        system->same_time_reduced_record_iter[pair_index] = iter;
        system->same_time_reduced_interface_data[pair_index] = reduced_next[pair_index];
        system->same_time_reduced_status_ok[pair_index] = replay_lookup[pair_index].status_ok;
        snprintf(system->same_time_reduced_fallback_reason[pair_index],
                 sizeof(system->same_time_reduced_fallback_reason[pair_index]),
                 "%s",
                 replay_lookup[pair_index].fallback_reason);
        snprintf(system->same_time_reduced_status[pair_index],
                 sizeof(system->same_time_reduced_status[pair_index]),
                 "%s",
                 replay_lookup[pair_index].status);
    }

    CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace_internal(system, 0));
    if (same_time_replay_enabled) {
        for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
            if (!system->current_contact_feedback_use[pair_index].is_defined) {
                continue;
            }
            system->current_contact_feedback_use[pair_index].record_step =
                system->current_step_index;
            system->current_contact_feedback_use[pair_index].record_iter =
                system->same_time_reduced_record_iter[pair_index];
            system->current_contact_feedback_use[pair_index].status_ok =
                replay_lookup[pair_index].status_ok;
            snprintf(system->current_contact_feedback_use[pair_index].fallback_reason,
                     sizeof(system->current_contact_feedback_use[pair_index].fallback_reason),
                     "%s",
                     replay_lookup[pair_index].fallback_reason);
            snprintf(system->current_contact_feedback_use[pair_index].status,
                     sizeof(system->current_contact_feedback_use[pair_index].status),
                     "%s",
                     replay_lookup[pair_index].status);
        }
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_refresh_contact_forces_and_trace_internal(
    mbd_system2d_t *system,
    int update_pair_stiffness)
{
    int pair_index;

    CHECK_NULL(system, "mbd_system2d");

    mbd_system2d_ensure_trace_strings(system);
    CHECK_ERROR(mbd_system2d_clear_contact_forces(system));
    memset(system->current_contact_trace, 0, sizeof(system->current_contact_trace));
    memset(system->current_contact_feedback, 0, sizeof(system->current_contact_feedback));
    memset(system->current_contact_feedback_use, 0, sizeof(system->current_contact_feedback_use));
    memset(system->current_reduced_interface_data, 0, sizeof(system->current_reduced_interface_data));
    memset(system->current_monolithic_local_patch_rows,
           0,
           sizeof(system->current_monolithic_local_patch_rows));
    memset(system->current_generic_contact_trace_rows,
           0,
           sizeof(system->current_generic_contact_trace_rows));
    system->num_current_generic_contact_trace_rows = 0;
    memset(mbd_generic_contact_replay_use_rows,
           0,
           sizeof(mbd_generic_contact_replay_use_rows));
    mbd_num_generic_contact_replay_use_rows = 0;

    for (pair_index = 0; pair_index < system->num_generic_contact_pairs; ++pair_index) {
        const mbd_contact_generic_pair2d_t *pair = &system->generic_contact_pairs[pair_index];
        const mbd_contact_surface_polyline2d_t *surface_i = NULL;
        const mbd_contact_surface_polyline2d_t *surface_j = NULL;
        mbd_contact_surface_polyline_cache2d_t slave_surface;
        mbd_contact_surface_polyline_cache2d_t master_surface;
        int body_index_i = -1;
        int body_index_j = -1;
        const mbd_body2d_t *body_i = NULL;
        const mbd_body2d_t *body_j = NULL;
        int segment_index = 0;
        int vertex_index = 0;

        if (!pair->is_defined) {
            continue;
        }
        if (fabs(pair->c_n) > MBD_CONTACT_STIFFNESS_SMALL_EPS) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic macro penalty solver v1: damping not implemented; require c_pen=0 (pair_id=%d)",
                             pair->pair_id);
        }

        surface_i = mbd_system2d_find_contact_surface_polyline_const(system, pair->surface_i);
        surface_j = mbd_system2d_find_contact_surface_polyline_const(system, pair->surface_j);
        if (!surface_i || !surface_j) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic macro penalty solver v1: pair %d references undefined surface(s) %d/%d",
                             pair->pair_id,
                             pair->surface_i,
                             pair->surface_j);
        }
        if (surface_i->body_id == surface_j->body_id) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "generic macro penalty solver v1: pair %d requires distinct bodies (got body_id=%d)",
                             pair->pair_id,
                             surface_i->body_id);
        }
        CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, surface_i->body_id, &body_index_i));
        CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, surface_j->body_id, &body_index_j));
        body_i = &system->bodies[body_index_i];
        body_j = &system->bodies[body_index_j];
        CHECK_ERROR(mbd_system2d_load_contact_surface_polyline_csv(surface_i->csv_path,
                                                                   &slave_surface));
        CHECK_ERROR(mbd_system2d_load_contact_surface_polyline_csv(surface_j->csv_path,
                                                                   &master_surface));

        for (segment_index = 0; segment_index < master_surface.num_points - 1; ++segment_index) {
            const double *a_local = master_surface.local_points[segment_index];
            const double *b_local = master_surface.local_points[segment_index + 1];
            double a_world[2];
            double b_world[2];
            double a_velocity[2];
            double b_velocity[2];
            double segment[2];
            double length_sq = 0.0;

            CHECK_ERROR(mbd_system2d_world_point_velocity(body_j,
                                                          a_local,
                                                          a_world,
                                                          a_velocity));
            CHECK_ERROR(mbd_system2d_world_point_velocity(body_j,
                                                          b_local,
                                                          b_world,
                                                          b_velocity));
            segment[0] = b_world[0] - a_world[0];
            segment[1] = b_world[1] - a_world[1];
            length_sq = segment[0] * segment[0] + segment[1] * segment[1];
            if (!isfinite(length_sq) || length_sq <= MBD_GENERIC_CONTACT_SEGMENT_EPS) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "generic macro penalty solver v1: degenerate master segment (pair_id=%d surface_j=%d segment_index=%d)",
                                 pair->pair_id,
                                 pair->surface_j,
                                 segment_index);
            }
        }

        for (vertex_index = 0; vertex_index < slave_surface.num_points; ++vertex_index) {
            const double *vertex_local = slave_surface.local_points[vertex_index];
            double slave_world[2];
            double slave_velocity[2];
            const int generic_lagged_reduced_enabled =
                system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
                system->local_feedback_mode == MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED;
            const int generic_same_time_reduced_enabled =
                system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
                system->local_feedback_mode == MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED;
            const mbd_local_feedback_record2d_t *generic_feedback_record = NULL;
            mbd_generic_same_time_contact_source_row_t generic_same_time_row;
            double best_gap = 0.0;
            double best_penetration = 0.0;
            double best_fn = 0.0;
            double best_contact[2] = {0.0, 0.0};
            double best_closest[2] = {0.0, 0.0};
            double best_normal[2] = {0.0, 0.0};
            double best_tangent[2] = {0.0, 0.0};
            double best_master_velocity[2] = {0.0, 0.0};
            double gamma_n_used = 1.0;
            double k_pen_used = pair->base_k_n;
            double v_t = 0.0;
            double mu_used = 0.0;
            double ft = 0.0;
            double friction_force[2] = {0.0, 0.0};
            const char *feedback_source = "NONE";
            const char *feedback_status = "none";
            int best_segment_index = -1;

            CHECK_ERROR(mbd_system2d_world_point_velocity(body_i,
                                                          vertex_local,
                                                          slave_world,
                                                          slave_velocity));

            for (segment_index = 0; segment_index < master_surface.num_points - 1; ++segment_index) {
                const double *a_local = master_surface.local_points[segment_index];
                const double *b_local = master_surface.local_points[segment_index + 1];
                double a_world[2];
                double b_world[2];
                double a_velocity[2];
                double b_velocity[2];
                double segment[2];
                double length_sq = 0.0;
                double t_param = 0.0;
                double closest[2];
                double closest_velocity[2];
                double tangent[2];
                double normal[2];
                double offset[2];
                double gap = 0.0;
                double penetration = 0.0;

                CHECK_ERROR(mbd_system2d_world_point_velocity(body_j,
                                                              a_local,
                                                              a_world,
                                                              a_velocity));
                CHECK_ERROR(mbd_system2d_world_point_velocity(body_j,
                                                              b_local,
                                                              b_world,
                                                              b_velocity));
                segment[0] = b_world[0] - a_world[0];
                segment[1] = b_world[1] - a_world[1];
                length_sq = segment[0] * segment[0] + segment[1] * segment[1];
                t_param = ((slave_world[0] - a_world[0]) * segment[0] +
                           (slave_world[1] - a_world[1]) * segment[1]) /
                          length_sq;
                if (t_param < 0.0) {
                    t_param = 0.0;
                } else if (t_param > 1.0) {
                    t_param = 1.0;
                }
                closest[0] = a_world[0] + t_param * segment[0];
                closest[1] = a_world[1] + t_param * segment[1];
                closest_velocity[0] =
                    (1.0 - t_param) * a_velocity[0] + t_param * b_velocity[0];
                closest_velocity[1] =
                    (1.0 - t_param) * a_velocity[1] + t_param * b_velocity[1];
                tangent[0] = segment[0] / sqrt(length_sq);
                tangent[1] = segment[1] / sqrt(length_sq);
                normal[0] = -tangent[1];
                normal[1] = tangent[0];
                offset[0] = slave_world[0] - closest[0];
                offset[1] = slave_world[1] - closest[1];
                gap = offset[0] * normal[0] + offset[1] * normal[1];
                penetration = fmax(0.0, -gap);

                if (best_segment_index < 0 || gap < best_gap) {
                    best_gap = gap;
                    best_penetration = penetration;
                    best_closest[0] = closest[0];
                    best_closest[1] = closest[1];
                    best_contact[0] = 0.5 * (slave_world[0] + closest[0]);
                    best_contact[1] = 0.5 * (slave_world[1] + closest[1]);
                    best_normal[0] = normal[0];
                    best_normal[1] = normal[1];
                    best_tangent[0] = tangent[0];
                    best_tangent[1] = tangent[1];
                    best_master_velocity[0] = closest_velocity[0];
                    best_master_velocity[1] = closest_velocity[1];
                    best_segment_index = segment_index;
                }
            }

            if (best_segment_index < 0) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "generic macro penalty solver v1: no master segment found for pair_id=%d",
                                 pair->pair_id);
            }

            if (generic_lagged_reduced_enabled) {
                feedback_source = "FALLBACK";
                feedback_status = "fallback";
                if (system->current_step_index <= 0) {
                    feedback_status = "step0_fallback";
                } else if (system->local_feedback_filename[0] == '\0' &&
                           system->local_contact_filename[0] == '\0' &&
                           system->ehl_filename[0] == '\0') {
                    feedback_status = "no_file";
                } else {
                    generic_feedback_record =
                        mbd_system2d_find_local_feedback_record(system,
                                                                system->current_step_index - 1,
                                                                pair->pair_id);
                    if (!generic_feedback_record) {
                        feedback_status = "no_record";
                    } else if (!generic_feedback_record->status_ok) {
                        feedback_status =
                            generic_feedback_record->status[0] != '\0'
                                ? generic_feedback_record->status
                                : "status_not_ok";
                    } else if ((generic_feedback_record->reduced_data.valid_flag &
                                MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N) == 0u ||
                               !isfinite(generic_feedback_record->reduced_data.gamma_n) ||
                               generic_feedback_record->reduced_data.gamma_n <= 0.0) {
                        feedback_status = "invalid_gamma";
                    } else {
                        feedback_source = "EXTERNAL";
                        feedback_status =
                            generic_feedback_record->status[0] != '\0'
                                ? generic_feedback_record->status
                                : "ok";
                        gamma_n_used = mbd_local_feedback_clip(
                            generic_feedback_record->reduced_data.gamma_n,
                            MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                            MBD_LOCAL_FEEDBACK_GAMMA_MAX);
                        k_pen_used = gamma_n_used * pair->base_k_n;
                    }
                }
            } else if (generic_same_time_reduced_enabled) {
                feedback_source = "FALLBACK";
                feedback_status = "fallback";
                if (system->local_contact_filename[0] == '\0') {
                    feedback_status = "no_local_source";
                } else if (!mbd_system2d_find_generic_same_time_contact_source_row(
                               system->local_contact_filename,
                               system->current_step_index,
                               pair->pair_id,
                               vertex_index,
                               best_segment_index,
                               &generic_same_time_row)) {
                    feedback_status = "no_row";
                } else if (generic_same_time_row.valid_flag == 0u) {
                    feedback_status = "invalid_row";
                } else if (!mbd_local_feedback_status_is_ok(generic_same_time_row.status)) {
                    feedback_status =
                        generic_same_time_row.status[0] != '\0'
                            ? generic_same_time_row.status
                            : "status_not_ok";
                } else if (!isfinite(generic_same_time_row.gamma_n) ||
                           !isfinite(generic_same_time_row.delta_g_eff) ||
                           !isfinite(generic_same_time_row.fn_ref) ||
                           !isfinite(generic_same_time_row.p_max) ||
                           generic_same_time_row.gamma_n <= 0.0) {
                    feedback_status = "invalid_gamma";
                } else {
                    feedback_source = "EXTERNAL";
                    feedback_status =
                        generic_same_time_row.status[0] != '\0'
                            ? generic_same_time_row.status
                            : "ok";
                    gamma_n_used = mbd_local_feedback_clip(generic_same_time_row.gamma_n,
                                                           MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                                                           MBD_LOCAL_FEEDBACK_GAMMA_MAX);
                    k_pen_used = gamma_n_used * pair->base_k_n;
                }
            }

            best_fn = k_pen_used * best_penetration;
            v_t = ((best_master_velocity[0] - slave_velocity[0]) * best_tangent[0]) +
                  ((best_master_velocity[1] - slave_velocity[1]) * best_tangent[1]);
            if (best_penetration > 0.0 && best_fn > 0.0) {
                const double slip_direction =
                    mbd_system2d_generic_contact_slip_direction(pair, v_t);

                mu_used = mbd_system2d_generic_contact_mu_used(pair, v_t);
                ft = -mu_used * best_fn * slip_direction;
                friction_force[0] = -ft * best_tangent[0];
                friction_force[1] = -ft * best_tangent[1];
            }

            if (best_penetration > 0.0) {
                double force_slave[2] = {0.0, 0.0};
                double force_master[2] = {0.0, 0.0};
                double r_slave[2] = {0.0, 0.0};
                double r_master[2] = {0.0, 0.0};
                double best_wrench_slave[MBD_BODY2D_DOF] = {0.0, 0.0, 0.0};
                double best_wrench_master[MBD_BODY2D_DOF] = {0.0, 0.0, 0.0};

                force_slave[0] = -best_fn * best_normal[0];
                force_slave[1] = -best_fn * best_normal[1];
                force_master[0] = +best_fn * best_normal[0];
                force_master[1] = +best_fn * best_normal[1];
                force_slave[0] += friction_force[0];
                force_slave[1] += friction_force[1];
                force_master[0] -= friction_force[0];
                force_master[1] -= friction_force[1];
                r_slave[0] = slave_world[0] - body_i->q[0];
                r_slave[1] = slave_world[1] - body_i->q[1];
                r_master[0] = best_closest[0] - body_j->q[0];
                r_master[1] = best_closest[1] - body_j->q[1];
                best_wrench_slave[0] = force_slave[0];
                best_wrench_slave[1] = force_slave[1];
                best_wrench_slave[2] =
                    r_slave[0] * force_slave[1] - r_slave[1] * force_slave[0];
                best_wrench_master[0] = force_master[0];
                best_wrench_master[1] = force_master[1];
                best_wrench_master[2] =
                    r_master[0] * force_master[1] - r_master[1] * force_master[0];
                CHECK_ERROR(mbd_forces2d_add_generalized_force(system->contact_force[body_index_i],
                                                               best_wrench_slave));
                CHECK_ERROR(mbd_forces2d_add_generalized_force(system->contact_force[body_index_j],
                                                               best_wrench_master));
            }

            {
                mbd_contact_generic_trace2d_t row;
                mbd_generic_contact_replay_use_row2d_t replay_use_row;

                memset(&row, 0, sizeof(row));
                row.is_defined = 1;
                row.pair_id = pair->pair_id;
                row.surface_i = pair->surface_i;
                row.surface_j = pair->surface_j;
                row.slave_body_id = surface_i->body_id;
                row.master_body_id = surface_j->body_id;
                row.slave_vertex_index = vertex_index;
                row.master_segment_index = best_segment_index;
                row.active = best_penetration > 0.0 ? 1 : 0;
                row.gap = best_gap;
                row.penetration = best_penetration;
                row.fn = best_fn;
                row.k_used = k_pen_used;
                row.contact_point[0] = best_contact[0];
                row.contact_point[1] = best_contact[1];
                row.closest_point[0] = best_closest[0];
                row.closest_point[1] = best_closest[1];
                row.slave_point[0] = slave_world[0];
                row.slave_point[1] = slave_world[1];
                row.normal[0] = best_normal[0];
                row.normal[1] = best_normal[1];
                row.tangent[0] = best_tangent[0];
                row.tangent[1] = best_tangent[1];
                row.v_t = v_t;
                row.mu_used = mu_used;
                row.ft_tangent = ft;
                row.friction_force[0] = friction_force[0];
                row.friction_force[1] = friction_force[1];
                snprintf(row.request_mode_hint,
                         sizeof(row.request_mode_hint),
                         "%s",
                         "normal_force");
                snprintf(row.source_mode,
                         sizeof(row.source_mode),
                         "%s",
                         MBD_GENERIC_CONTACT_MVP_SOURCE_MODE);
                CHECK_ERROR(mbd_system2d_append_generic_contact_trace_row(system, &row));

                memset(&replay_use_row, 0, sizeof(replay_use_row));
                replay_use_row.is_defined = 1;
                replay_use_row.pair_id = pair->pair_id;
                replay_use_row.slave_body_id = surface_i->body_id;
                replay_use_row.master_body_id = surface_j->body_id;
                replay_use_row.slave_vertex_index = vertex_index;
                replay_use_row.master_segment_index = best_segment_index;
                replay_use_row.active = best_penetration > 0.0 ? 1 : 0;
                replay_use_row.fn = best_fn;
                replay_use_row.penetration = best_penetration;
                replay_use_row.gamma_n_used = gamma_n_used;
                replay_use_row.k_pen_base = pair->base_k_n;
                replay_use_row.k_pen_used = k_pen_used;
                snprintf(replay_use_row.feedback_status,
                         sizeof(replay_use_row.feedback_status),
                         "%s",
                         feedback_status);
                snprintf(replay_use_row.feedback_source,
                         sizeof(replay_use_row.feedback_source),
                         "%s",
                         feedback_source);
                CHECK_ERROR(mbd_system2d_append_generic_contact_replay_use_row(&replay_use_row));
            }
        }
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        mbd_contact_pair2d_t *pair = &system->contact_pairs[pair_index];
        const mbd_contact_circle2d_t *circle_i = NULL;
        const mbd_contact_circle2d_t *circle_j = NULL;
        const mbd_contact_halfspace2d_t *halfspace = NULL;
        int body_index_i = -1;
        int body_index_j = -1;
        const mbd_body2d_t *body_i = NULL;
        const mbd_body2d_t *body_j = NULL;
        double q_i[MBD_BODY2D_DOF];
        double q_j[MBD_BODY2D_DOF];
        double v_i[MBD_BODY2D_DOF];
        double v_j[MBD_BODY2D_DOF];
        double a_dummy[MBD_BODY2D_DOF];
        double d[2];
        double dist = 0.0;
        double normal[2] = {1.0, 0.0};
        double r1[2];
        double r2[2];
        double vp1[2];
        double vp2[2];
        double rel_v[2];
        double plane_normal[2] = {0.0, 0.0};
        double gap = 0.0;
        double penetration = 0.0;
        double v_n = 0.0;
        double v_close = 0.0;
        double tangent[2];
        double v_t = 0.0;
        double thickness = 0.0;
        double fn = 0.0;
        double contact_point_world[2] = {0.0, 0.0};
        double mu_used = 0.0;
        double gamma_n_used = 1.0;
        mbd_contact_feedback2d_t reduced_feedback_used;
        double ft = 0.0;
        double kn_base = 0.0;
        double kn_used = 0.0;
        double kn_out = 0.0;
        double f1_n[2];
        double f2_n[2];
        double f1_t[2];
        double f2_t[2];
        double f1[2];
        double f2[2];
        double wrench1[MBD_BODY2D_DOF];
        double wrench2[MBD_BODY2D_DOF];
        const int lagged_stiffness_enabled =
            system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
            system->contact_coupling_mode == MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS;
        const int lagged_reduced_enabled =
            system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
            system->local_feedback_mode == MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED;
        const int same_time_reduced_enabled =
            system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
            system->local_feedback_mode == MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED &&
            system->same_time_reduced_override_active;
        const int monolithic_local_patch_enabled =
            system->time.integrator == MBD_INTEGRATOR2D_EXPLICIT &&
            system->local_contact_monolithic_mode ==
                MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE;
        const mbd_local_feedback_record2d_t *local_feedback_record = NULL;
        const char *feedback_source_mode = "NONE";
        const char *fallback_reason = "mode_none";
        const char *feedback_status = "none";
        int feedback_status_ok = 0;
        int record_step = -1;
        int record_iter = -1;

        if (!pair->is_defined) {
            continue;
        }

        CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, pair->body_i, &body_index_i));
        body_i = &system->bodies[body_index_i];
        circle_i = mbd_system2d_find_contact_circle_const(system, pair->body_i);
        if (!circle_i) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "contact pair %d missing circle geometry for body id %d",
                             pair->pair_id,
                             pair->body_i);
        }

        CHECK_ERROR(mbd_body2d_get_generalized_state(body_i, q_i, v_i, a_dummy));

        if (pair->proxy_geometry == MBD_CONTACT_PROXY_CIRCLE_CIRCLE) {
            CHECK_ERROR(mbd_system2d_find_body_index_by_id(system, pair->body_j, &body_index_j));
            body_j = &system->bodies[body_index_j];
            circle_j = mbd_system2d_find_contact_circle_const(system, pair->body_j);
            if (!circle_j) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d missing circle geometry for body ids %d/%d",
                                 pair->pair_id,
                                 pair->body_i,
                                 pair->body_j);
            }
            if (!isfinite(circle_i->thickness) || circle_i->thickness <= 0.0 ||
                !isfinite(circle_j->thickness) || circle_j->thickness <= 0.0) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d requires positive finite thickness",
                                 pair->pair_id);
            }
            if (fabs(circle_i->thickness - circle_j->thickness) > 1.0e-12) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d requires matching circle thickness (body %d=%.16e body %d=%.16e)",
                                 pair->pair_id,
                                 pair->body_i,
                                 circle_i->thickness,
                                 pair->body_j,
                                 circle_j->thickness);
            }

            CHECK_ERROR(mbd_body2d_get_generalized_state(body_j, q_j, v_j, a_dummy));

            d[0] = q_j[0] - q_i[0];
            d[1] = q_j[1] - q_i[1];
            dist = sqrt(d[0] * d[0] + d[1] * d[1]);

            if (dist > MBD_CONTACT2D_COINCIDENCE_EPS) {
                normal[0] = d[0] / dist;
                normal[1] = d[1] / dist;
                pair->last_normal[0] = normal[0];
                pair->last_normal[1] = normal[1];
                pair->has_last_normal = 1;
            } else if (pair->has_last_normal) {
                const double last_norm = sqrt(pair->last_normal[0] * pair->last_normal[0] +
                                              pair->last_normal[1] * pair->last_normal[1]);
                if (last_norm > MBD_CONTACT2D_COINCIDENCE_EPS) {
                    normal[0] = pair->last_normal[0] / last_norm;
                    normal[1] = pair->last_normal[1] / last_norm;
                } else {
                    normal[0] = 1.0;
                    normal[1] = 0.0;
                }
            } else {
                printf("  Warning: contact pair %d center coincidence fallback uses normal=(1,0)\n",
                       pair->pair_id);
                pair->last_normal[0] = 1.0;
                pair->last_normal[1] = 0.0;
                pair->has_last_normal = 1;
                normal[0] = 1.0;
                normal[1] = 0.0;
            }
            gap = dist - (circle_i->radius + circle_j->radius);
            r1[0] = circle_i->radius * normal[0];
            r1[1] = circle_i->radius * normal[1];
            r2[0] = -circle_j->radius * normal[0];
            r2[1] = -circle_j->radius * normal[1];
            vp2[0] = v_j[0] + (-v_j[2] * r2[1]);
            vp2[1] = v_j[1] + (v_j[2] * r2[0]);
        } else {
            halfspace = mbd_system2d_find_contact_halfspace_const(system, pair->halfspace_id);
            if (!halfspace) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d missing halfspace geometry id %d",
                                 pair->pair_id,
                                 pair->halfspace_id);
            }
            if (!isfinite(circle_i->thickness) || circle_i->thickness <= 0.0 ||
                !isfinite(halfspace->thickness) || halfspace->thickness <= 0.0) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d requires positive finite thickness",
                                 pair->pair_id);
            }
            if (fabs(circle_i->thickness - halfspace->thickness) > 1.0e-12) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "contact pair %d requires matching circle/halfspace thickness",
                                 pair->pair_id);
            }

            plane_normal[0] = halfspace->normal[0];
            plane_normal[1] = halfspace->normal[1];
            dist = (q_i[0] - halfspace->point[0]) * plane_normal[0] +
                   (q_i[1] - halfspace->point[1]) * plane_normal[1];
            normal[0] = -plane_normal[0];
            normal[1] = -plane_normal[1];
            pair->last_normal[0] = normal[0];
            pair->last_normal[1] = normal[1];
            pair->has_last_normal = 1;
            gap = dist - circle_i->radius;
            r1[0] = circle_i->radius * normal[0];
            r1[1] = circle_i->radius * normal[1];
            q_j[0] = q_i[0] - dist * plane_normal[0];
            q_j[1] = q_i[1] - dist * plane_normal[1];
            q_j[2] = 0.0;
            r2[0] = 0.0;
            r2[1] = 0.0;
            v_j[0] = 0.0;
            v_j[1] = 0.0;
            v_j[2] = 0.0;
            vp2[0] = 0.0;
            vp2[1] = 0.0;
        }
        penetration = fmax(0.0, -gap);
        tangent[0] = -normal[1];
        tangent[1] = normal[0];
        vp1[0] = v_i[0] + (-v_i[2] * r1[1]);
        vp1[1] = v_i[1] + (v_i[2] * r1[0]);
        rel_v[0] = vp2[0] - vp1[0];
        rel_v[1] = vp2[1] - vp1[1];
        v_n = rel_v[0] * normal[0] + rel_v[1] * normal[1];
        v_t = rel_v[0] * tangent[0] + rel_v[1] * tangent[1];
        system->same_time_reduced_vt[pair_index] = v_t;
        v_close = fmax(0.0, -v_n);
        thickness = circle_i->thickness;
        mu_used = pair->base_mu;
        gamma_n_used = 1.0;
        reduced_feedback_used = mbd_contact_feedback2d_make_basic(mu_used, gamma_n_used);
        kn_base = pair->base_k_n;
        kn_used = pair->base_k_n;
        if (same_time_reduced_enabled) {
            feedback_source_mode = "SAME_TIME";
            fallback_reason =
                system->same_time_reduced_fallback_reason[pair_index][0] != '\0'
                    ? system->same_time_reduced_fallback_reason[pair_index]
                    : "none";
            feedback_status =
                system->same_time_reduced_status[pair_index][0] != '\0'
                    ? system->same_time_reduced_status[pair_index]
                    : "same_time";
            feedback_status_ok = system->same_time_reduced_status_ok[pair_index];
            record_step = system->current_step_index;
            record_iter = system->same_time_reduced_record_iter[pair_index];
            reduced_feedback_used = system->same_time_reduced_interface_data[pair_index];
            mu_used = mbd_local_feedback_clip(
                reduced_feedback_used.mu_eff,
                MBD_LOCAL_FEEDBACK_MU_MIN,
                MBD_LOCAL_FEEDBACK_MU_MAX);
            gamma_n_used = mbd_local_feedback_clip(
                reduced_feedback_used.gamma_n,
                MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                MBD_LOCAL_FEEDBACK_GAMMA_MAX);
            kn_used = gamma_n_used * pair->base_k_n;
        } else if (monolithic_local_patch_enabled) {
            contact_point_world[0] = 0.5 * ((q_i[0] + r1[0]) + (q_j[0] + r2[0]));
            contact_point_world[1] = 0.5 * ((q_i[1] + r1[1]) + (q_j[1] + r2[1]));
            feedback_source_mode = MBD_MONOLITHIC_PATCH_MVP_SOURCE_MODE;
            record_step = system->current_step_index;
            record_iter = 0;
            CHECK_ERROR(mbd_system2d_run_monolithic_local_patch_circle_mvp(
                system,
                pair,
                pair_index,
                circle_i,
                circle_j,
                contact_point_world,
                normal,
                gap,
                penetration,
                v_n,
                v_t,
                &reduced_feedback_used,
                &feedback_status_ok,
                &fallback_reason,
                &feedback_status));
            mu_used = pair->base_mu;
            gamma_n_used = mbd_local_feedback_clip(
                reduced_feedback_used.gamma_n,
                MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                MBD_LOCAL_FEEDBACK_GAMMA_MAX);
            kn_used = gamma_n_used * pair->base_k_n;
        } else if (lagged_reduced_enabled) {
            feedback_source_mode = "FALLBACK";
            fallback_reason = "step0_no_prev";
            feedback_status = "fallback";
            if (system->current_step_index <= 0) {
                fallback_reason = "step0_no_prev";
            } else if (system->local_feedback_filename[0] == '\0' &&
                       system->local_contact_filename[0] == '\0' &&
                       system->ehl_filename[0] == '\0') {
                fallback_reason = "no_file";
            } else if (system->local_feedback_filename[0] == '\0' &&
                       system->local_contact_filename[0] == '\0') {
                fallback_reason = "no_local_source";
            } else if (system->local_feedback_filename[0] == '\0' &&
                       system->ehl_filename[0] == '\0') {
                fallback_reason = "no_ehl_source";
            } else {
                local_feedback_record = mbd_system2d_find_local_feedback_record(system,
                                                                                system->current_step_index - 1,
                                                                                pair->pair_id);
                if (!local_feedback_record) {
                    fallback_reason = "no_record";
                } else if (!local_feedback_record->is_valid) {
                    record_step = local_feedback_record->step;
                    feedback_status_ok = local_feedback_record->status_ok;
                    feedback_status = local_feedback_record->status[0] != '\0'
                                          ? local_feedback_record->status
                                          : "invalid";
                    fallback_reason = "invalid_record";
                } else if (!local_feedback_record->status_ok) {
                    record_step = local_feedback_record->step;
                    feedback_status_ok = local_feedback_record->status_ok;
                    feedback_status = local_feedback_record->status[0] != '\0'
                                          ? local_feedback_record->status
                                          : "invalid";
                    fallback_reason = "status_not_ok";
                } else {
                    feedback_source_mode = "EXTERNAL";
                    fallback_reason = "none";
                    feedback_status_ok = local_feedback_record->status_ok;
                    feedback_status = local_feedback_record->status[0] != '\0'
                                          ? local_feedback_record->status
                                          : "ok";
                    record_step = local_feedback_record->step;
                    reduced_feedback_used = local_feedback_record->reduced_data;
                    mu_used = mbd_local_feedback_clip(reduced_feedback_used.mu_eff,
                                                      MBD_LOCAL_FEEDBACK_MU_MIN,
                                                      MBD_LOCAL_FEEDBACK_MU_MAX);
                    gamma_n_used = mbd_local_feedback_clip(reduced_feedback_used.gamma_n,
                                                           MBD_LOCAL_FEEDBACK_GAMMA_MIN,
                                                           MBD_LOCAL_FEEDBACK_GAMMA_MAX);
                    kn_used = gamma_n_used * pair->base_k_n;
                }
            }
        } else if (lagged_stiffness_enabled &&
                   pair->has_k_prev &&
                   pair->k_prev > MBD_CONTACT_STIFFNESS_SMALL_EPS) {
            kn_used = pair->k_prev;
        }
        fn = 0.0;
        if (penetration > 0.0) {
            fn = thickness * fmax(0.0, kn_used * penetration + pair->c_n * v_close);
        }
        ft = 0.0;
        if (penetration > 0.0 && fabs(v_t) > MBD_CONTACT_TANGENTIAL_SPEED_EPS) {
            ft = -((v_t > 0.0) ? 1.0 : -1.0) * mu_used * fn;
        }
        if (penetration > MBD_CONTACT_STIFFNESS_SMALL_EPS) {
            kn_out = fn / penetration;
        }

        f1_n[0] = -fn * normal[0];
        f1_n[1] = -fn * normal[1];
        f2_n[0] = +fn * normal[0];
        f2_n[1] = +fn * normal[1];
        f1_t[0] = -ft * tangent[0];
        f1_t[1] = -ft * tangent[1];
        f2_t[0] = +ft * tangent[0];
        f2_t[1] = +ft * tangent[1];
        f1[0] = f1_n[0] + f1_t[0];
        f1[1] = f1_n[1] + f1_t[1];
        f2[0] = f2_n[0] + f2_t[0];
        f2[1] = f2_n[1] + f2_t[1];
        wrench1[0] = f1[0];
        wrench1[1] = f1[1];
        wrench1[2] = r1[0] * f1[1] - r1[1] * f1[0];
        wrench2[0] = f2[0];
        wrench2[1] = f2[1];
        wrench2[2] = r2[0] * f2[1] - r2[1] * f2[0];

        CHECK_ERROR(mbd_forces2d_add_generalized_force(system->contact_force[body_index_i],
                                                       wrench1));
        if (pair->proxy_geometry == MBD_CONTACT_PROXY_CIRCLE_CIRCLE) {
            CHECK_ERROR(mbd_forces2d_add_generalized_force(system->contact_force[body_index_j],
                                                           wrench2));
        }

        system->current_contact_trace[pair_index].is_defined = 1;
        system->current_contact_trace[pair_index].pair_id = pair->pair_id;
        system->current_contact_trace[pair_index].active = penetration > 0.0 ? 1 : 0;
        system->current_contact_trace[pair_index].gap = gap;
        system->current_contact_trace[pair_index].penetration = penetration;
        system->current_contact_trace[pair_index].normal[0] = normal[0];
        system->current_contact_trace[pair_index].normal[1] = normal[1];
        system->current_contact_trace[pair_index].vn = v_n;
        system->current_contact_trace[pair_index].fn = fn;
        system->current_contact_trace[pair_index].cp1[0] = q_i[0] + r1[0];
        system->current_contact_trace[pair_index].cp1[1] = q_i[1] + r1[1];
        system->current_contact_trace[pair_index].cp2[0] = q_j[0] + r2[0];
        system->current_contact_trace[pair_index].cp2[1] = q_j[1] + r2[1];
        system->current_contact_trace[pair_index].f1[0] = f1[0];
        system->current_contact_trace[pair_index].f1[1] = f1[1];
        system->current_contact_trace[pair_index].m1_z = wrench1[2];
        system->current_contact_trace[pair_index].f2[0] = f2[0];
        system->current_contact_trace[pair_index].f2[1] = f2[1];
        system->current_contact_trace[pair_index].m2_z = wrench2[2];

        system->current_contact_feedback[pair_index].is_defined = 1;
        system->current_contact_feedback[pair_index].pair_id = pair->pair_id;
        system->current_contact_feedback[pair_index].active = penetration > 0.0 ? 1 : 0;
        system->current_contact_feedback[pair_index].gap = gap;
        system->current_contact_feedback[pair_index].penetration = penetration;
        system->current_contact_feedback[pair_index].fn = fn;
        system->current_contact_feedback[pair_index].kn_base = kn_base;
        system->current_contact_feedback[pair_index].kn_used = kn_used;
        system->current_contact_feedback[pair_index].kn_out = kn_out;

        system->current_contact_feedback_use[pair_index].is_defined = 1;
        system->current_contact_feedback_use[pair_index].pair_id = pair->pair_id;
        system->current_reduced_interface_data[pair_index] = reduced_feedback_used;
        system->current_contact_feedback_use[pair_index].reduced_data =
            system->current_reduced_interface_data[pair_index];
        system->current_contact_feedback_use[pair_index].mu_base = pair->base_mu;
        system->current_contact_feedback_use[pair_index].mu_used = mu_used;
        system->current_contact_feedback_use[pair_index].gamma_n_used = gamma_n_used;
        system->current_contact_feedback_use[pair_index].k_base = pair->base_k_n;
        system->current_contact_feedback_use[pair_index].k_used = kn_used;
        system->current_contact_feedback_use[pair_index].record_step = record_step;
        system->current_contact_feedback_use[pair_index].record_iter = record_iter;
        system->current_contact_feedback_use[pair_index].status_ok = feedback_status_ok;
        snprintf(system->current_contact_feedback_use[pair_index].source_mode,
                 sizeof(system->current_contact_feedback_use[pair_index].source_mode),
                 "%s",
                 feedback_source_mode);
        snprintf(system->current_contact_feedback_use[pair_index].fallback_reason,
                 sizeof(system->current_contact_feedback_use[pair_index].fallback_reason),
                 "%s",
                 fallback_reason);
        snprintf(system->current_contact_feedback_use[pair_index].status,
                 sizeof(system->current_contact_feedback_use[pair_index].status),
                 "%s",
                 feedback_status);

        if (monolithic_local_patch_enabled &&
            system->current_monolithic_local_patch_rows[pair_index].is_defined) {
            system->monolithic_local_patch_row_count_total += 1;
            if (system->current_monolithic_local_patch_rows[pair_index].active) {
                system->monolithic_local_patch_active_rows_total += 1;
            }
            if (fabs(system->current_monolithic_local_patch_rows[pair_index].reduced_data.gamma_n - 1.0) >
                1.0e-12) {
                system->monolithic_local_patch_gamma_not_one_rows_total += 1;
            }
            if (system->current_monolithic_local_patch_rows[pair_index].reduced_data.fn_ref > 0.0) {
                system->monolithic_local_patch_fn_positive_rows_total += 1;
            }
        }

        if (update_pair_stiffness && lagged_stiffness_enabled) {
            pair->k_prev = kn_out;
            pair->has_k_prev = kn_out > MBD_CONTACT_STIFFNESS_SMALL_EPS ? 1 : 0;
        }
    }

    return FEM_SUCCESS;
}

static void mbd_system2d_ensure_trace_strings(mbd_system2d_t *system)
{
    if (!system) {
        return;
    }

    if (!system->hht_force_history_mode) {
        system->hht_force_history_mode = MBD_HHT_FORCE_HISTORY_MODE_NOT_APPLICABLE;
    }
    if (!system->implicit_residual_mode) {
        system->implicit_residual_mode = MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT;
    }
    if (!system->implicit_scheme_mode) {
        system->implicit_scheme_mode = MBD_IMPLICIT_SCHEME_NOT_APPLICABLE;
    }
    if (!system->implicit_convergence_reason) {
        system->implicit_convergence_reason = MBD_IMPLICIT_REASON_NOT_RUN;
    }
    if (!system->position_projection_stop_reason) {
        system->position_projection_stop_reason = MBD_PROJECTION_STOP_DISABLED;
    }
}

static fem_error_t mbd_system2d_refresh_contact_forces_and_trace(mbd_system2d_t *system)
{
    return mbd_system2d_refresh_contact_forces_and_trace_internal(system, 0);
}

fem_error_t mbd_system2d_refresh_generalized_force_history(mbd_system2d_t *system)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(system));

    if (system->generalized_force_history_valid) {
        memcpy(system->previous_generalized_force,
               system->current_generalized_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
    } else {
        if (system->num_bodies > 0) {
            memset(system->previous_generalized_force,
                   0,
                   mbd_system2d_body_force_bytes(system->num_bodies));
        }
    }

    if (system->num_bodies > 0) {
        memset(system->current_generalized_force, 0, mbd_system2d_body_force_bytes(system->num_bodies));
    }
    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        CHECK_ERROR(mbd_forces2d_build_body_generalized_force(system,
                                                              body_index,
                                                              system->current_generalized_force[body_index]));
    }

    if (!system->generalized_force_history_valid) {
        memcpy(system->previous_generalized_force,
               system->current_generalized_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
        system->generalized_force_history_valid = 1;
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_setup_builtin_case(mbd_system2d_t *system)
{
    const double distance_anchor_i[2] = {0.0, 0.0};
    const double distance_anchor_j[2] = {0.0, 0.0};
    const double revolute_anchor_i[2] = {0.5, 0.0};
    const double revolute_anchor_j[2] = {-0.5, 0.0};
    const double q0[3] = {0.0, 0.0, 0.0};
    const double q1[3] = {1.2, 0.3, 0.1};
    mbd_body2d_t body;
    mbd_constraint2d_t constraint;

    CHECK_NULL(system, "mbd_system2d");

    mbd_system2d_zero(system);

    CHECK_ERROR(mbd_body2d_init_dyn(&body, 0,
                                    MBD_BODY2D_DEFAULT_MASS,
                                    MBD_BODY2D_DEFAULT_INERTIA,
                                    q0, NULL));
    CHECK_ERROR(mbd_system2d_add_body(system, 0, &body));

    CHECK_ERROR(mbd_body2d_init_dyn(&body, 1,
                                    MBD_BODY2D_DEFAULT_MASS,
                                    MBD_BODY2D_DEFAULT_INERTIA,
                                    q1, NULL));
    CHECK_ERROR(mbd_system2d_add_body(system, 1, &body));

    CHECK_ERROR(mbd_constraint_init_distance(&constraint, 1, 0, 1,
                                             distance_anchor_i, distance_anchor_j, 1.0));
    CHECK_ERROR(mbd_system2d_append_constraint(system, &constraint));

    CHECK_ERROR(mbd_constraint_init_revolute(&constraint, 2, 0, 1,
                                             revolute_anchor_i, revolute_anchor_j));
    CHECK_ERROR(mbd_system2d_append_constraint(system, &constraint));

    CHECK_ERROR(mbd_system2d_set_gravity(system, 0.0, -9.80665));
    system->from_input = 0;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_do_explicit_step(mbd_system2d_t *system)
{
    double (*acceleration)[MBD_BODY2D_DOF] = NULL;
    int body_index;
    const int same_time_reduced_enabled =
        system &&
        system->local_feedback_mode == MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "mbd system");
    err = mbd_system2d_enforce_ground_bodies(system);
    if (err != FEM_SUCCESS) {
        return err;
    }
    if (system->num_bodies > 0) {
        acceleration = (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                            sizeof(*acceleration));
        if (!acceleration) {
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Failed to allocate explicit acceleration field for %d bodies",
                             system->num_bodies);
        }
    }

    if (system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT) {
        free(acceleration);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Explicit step requested for non-explicit integrator");
    }
    system->hht_force_history_mode = MBD_HHT_FORCE_HISTORY_MODE_NOT_APPLICABLE;
    mbd_system2d_reset_implicit_trace(system,
                                      MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT,
                                      MBD_IMPLICIT_SCHEME_NOT_APPLICABLE,
                                      0.0);
    if (same_time_reduced_enabled) {
        err = mbd_system2d_run_same_time_reduced_iteration(system);
    } else {
        mbd_system2d_clear_same_time_reduced_iterations(system);
        err = mbd_system2d_refresh_contact_forces_and_trace(system);
    }
    if (err != FEM_SUCCESS) {
        free(acceleration);
        return err;
    }
    err = mbd_system2d_compute_explicit_acceleration_field(system, acceleration);
    if (err != FEM_SUCCESS) {
        free(acceleration);
        return err;
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        err = mbd_explicit2d_update_velocity(&system->bodies[body_index],
                                             system->time.dt,
                                             acceleration[body_index]);
        if (err != FEM_SUCCESS) {
            free(acceleration);
            return err;
        }
        err = mbd_explicit2d_update_position(&system->bodies[body_index],
                                             system->time.dt);
        if (err != FEM_SUCCESS) {
            free(acceleration);
            return err;
        }
    }

    system->time.steps_executed += 1;
    err = mbd_system2d_enforce_ground_bodies(system);
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_sync_body_states(system);
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_apply_position_projection_if_enabled(system);
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_refresh_contact_forces_and_trace_internal(system, 1);
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_clear_flexible_forces(system);
    }
    free(acceleration);
    return err;
}

fem_error_t mbd_system2d_compute_explicit_acceleration(
    const mbd_system2d_t *system,
    int body_index,
    double acceleration[MBD_BODY2D_DOF])
{
    double generalized_force[MBD_BODY2D_DOF] = {0.0, 0.0, 0.0};
    const mbd_body2d_t *body = NULL;

    CHECK_NULL(system, "mbd system");
    CHECK_NULL(acceleration, "explicit acceleration");

    if (body_index < 0 || body_index >= system->num_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "body_index %d outside supported range [0,%d)",
                         body_index,
                         system->num_bodies);
    }

    body = &system->bodies[body_index];
    if (body->is_ground) {
        acceleration[0] = 0.0;
        acceleration[1] = 0.0;
        acceleration[2] = 0.0;
        return FEM_SUCCESS;
    }

    CHECK_ERROR(mbd_forces2d_apply_user_loads(body, generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_gravity(system, body_index, generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_flexible_loads(system, body_index, generalized_force));
    CHECK_ERROR(mbd_forces2d_apply_contact_loads(system, body_index, generalized_force));

    if (body->mass <= 0.0 || body->inertia <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "explicit acceleration requires positive mass/inertia for body %d",
                         body->id);
    }

    acceleration[0] = generalized_force[0] / body->mass;
    acceleration[1] = generalized_force[1] / body->mass;
    acceleration[2] = generalized_force[2] / body->inertia;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_compute_layout(const mbd_system2d_t *system,
                                        mbd_kkt_layout_t *layout)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(layout, "mbd_kkt_layout");

    return mbd_kkt_compute_layout_from_constraints(system->num_bodies,
                                                   system->constraints,
                                                   system->num_constraints,
                                                   layout);
}

fem_error_t mbd_system2d_compute_constraint_residual_l2(const mbd_system2d_t *system,
                                                        double *residual_l2,
                                                        int *num_equations)
{
    double residual_norm_sq = 0.0;
    int total_equations = 0;
    int i;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(residual_l2, "residual_l2");
    CHECK_NULL(num_equations, "num_equations");

    CHECK_ERROR(mbd_kkt_count_constraint_equations(system->constraints,
                                                   system->num_constraints,
                                                   &total_equations));

    for (i = 0; i < system->num_constraints; ++i) {
        int r;
        const mbd_constraint2d_t *constraint = &system->constraints[i];
        mbd_body_state2d_t state_i_view;
        mbd_body_state2d_t state_j_view;
        mbd_constraint_eval2d_t eval;
        const mbd_body2d_t *body_i = NULL;
        const mbd_body2d_t *body_j = NULL;

        if (constraint->body_i < 0 || constraint->body_i >= system->num_bodies ||
            constraint->body_j < 0 || constraint->body_j >= system->num_bodies) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Constraint body index out of range (id=%d, body_i=%d, body_j=%d)",
                             MBD_DIAG_E_CONSTRAINT_BODY_RANGE,
                             constraint->id,
                             constraint->body_i,
                             constraint->body_j);
        }

        body_i = &system->bodies[constraint->body_i];
        body_j = &system->bodies[constraint->body_j];
        CHECK_ERROR(mbd_body2d_to_state_view(body_i, &state_i_view));
        CHECK_ERROR(mbd_body2d_to_state_view(body_j, &state_j_view));
        CHECK_ERROR(mbd_constraint_evaluate_accel_rhs(constraint,
                                                      &state_i_view,
                                                      &state_j_view,
                                                      body_i->v,
                                                      body_j->v,
                                                      MBD_CONSTRAINT2D_BAUMGARTE_ALPHA_DEFAULT,
                                                      MBD_CONSTRAINT2D_BAUMGARTE_BETA_DEFAULT,
                                                      &eval));

        for (r = 0; r < eval.num_equations; ++r) {
            residual_norm_sq += eval.residual[r] * eval.residual[r];
        }
    }

    *num_equations = total_equations;
    *residual_l2 = sqrt(residual_norm_sq);
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_compute_revolute_metrics(const mbd_system2d_t *system,
                                                  double *anchor_mismatch_max,
                                                  double *body_j_com_radius_max,
                                                  int *revolute_count)
{
    int constraint_index;
    double mismatch_max = 0.0;
    double radius_max = 0.0;
    int count = 0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(anchor_mismatch_max, "anchor mismatch max");
    CHECK_NULL(body_j_com_radius_max, "body_j_com radius max");
    CHECK_NULL(revolute_count, "revolute count");

    for (constraint_index = 0; constraint_index < system->num_constraints; ++constraint_index) {
        const mbd_constraint2d_t *constraint = &system->constraints[constraint_index];
        mbd_body_state2d_t state_i;
        mbd_body_state2d_t state_j;
        double anchor_i_world[2];
        double anchor_j_world[2];
        double mismatch = 0.0;
        double body_j_radius = 0.0;

        if (constraint->type != MBD_CONSTRAINT_REVOLUTE) {
            continue;
        }
        if (constraint->body_i < 0 || constraint->body_i >= system->num_bodies ||
            constraint->body_j < 0 || constraint->body_j >= system->num_bodies) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "revolute constraint body index out of range (id=%d, body_i=%d, body_j=%d)",
                             constraint->id,
                             constraint->body_i,
                             constraint->body_j);
        }

        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_i], &state_i));
        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_j], &state_j));
        CHECK_ERROR(mbd_kinematics2d_local_point_to_world(&state_i,
                                                          constraint->anchor_i,
                                                          anchor_i_world));
        CHECK_ERROR(mbd_kinematics2d_local_point_to_world(&state_j,
                                                          constraint->anchor_j,
                                                          anchor_j_world));

        mismatch = hypot(anchor_i_world[0] - anchor_j_world[0],
                         anchor_i_world[1] - anchor_j_world[1]);
        body_j_radius = hypot(system->bodies[constraint->body_j].q[0] - anchor_i_world[0],
                              system->bodies[constraint->body_j].q[1] - anchor_i_world[1]);
        if (mismatch > mismatch_max) {
            mismatch_max = mismatch;
        }
        if (body_j_radius > radius_max) {
            radius_max = body_j_radius;
        }
        ++count;
    }

    *anchor_mismatch_max = mismatch_max;
    *body_j_com_radius_max = radius_max;
    *revolute_count = count;
    return FEM_SUCCESS;
}

static double mbd_constraint_residual_tol_from_env(const char **status_out)
{
    return parse_env_double_or_default_with_status("FEM4C_MBD_CONSTRAINT_RESIDUAL_TOL",
                                                   MBD_CONSTRAINT_RESIDUAL_TOL_DEFAULT,
                                                   1.0e-16,
                                                   1.0e+6,
                                                   status_out);
}

static fem_error_t mbd_system2d_check_constraint_residual(const mbd_system2d_t *system,
                                                          double residual_tol,
                                                          double *residual_l2_out,
                                                          int *num_equations_out)
{
    double residual_l2 = 0.0;
    int num_equations = 0;

    CHECK_NULL(system, "mbd_system2d");

    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &residual_l2,
                                                            &num_equations));
    if (residual_l2_out) {
        *residual_l2_out = residual_l2;
    }
    if (num_equations_out) {
        *num_equations_out = num_equations;
    }
    if (num_equations > 0 && residual_l2 > residual_tol) {
        return error_set(FEM_ERROR_CONVERGENCE_FAILED,
                         "constraint residual %.16e exceeds tolerance %.16e (%d equations)",
                         residual_l2,
                         residual_tol,
                         num_equations);
    }

    return FEM_SUCCESS;
}

static int mbd_system2d_has_ground_body(const mbd_system2d_t *system)
{
    int body_index;

    if (!system) {
        return 0;
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        if (system->bodies[body_index].is_ground) {
            return 1;
        }
    }
    return 0;
}

static void mbd_system2d_lock_ground_body(mbd_body2d_t *body)
{
    if (!body || !body->is_ground) {
        return;
    }

    body->q[0] = body->reference_origin[0];
    body->q[1] = body->reference_origin[1];
    body->q[2] = body->reference_theta;
    memset(body->v, 0, sizeof(body->v));
    memset(body->a, 0, sizeof(body->a));
    memset(body->force, 0, sizeof(body->force));
}

static fem_error_t mbd_system2d_enforce_ground_bodies(mbd_system2d_t *system)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        if (!system->bodies[body_index].is_ground) {
            continue;
        }
        mbd_system2d_lock_ground_body(&system->bodies[body_index]);
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_apply_ground_lock_to_dense_system(
    const mbd_system2d_t *system,
    double *matrix,
    double *rhs,
    int n)
{
    int body_index;
    int row_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(matrix, "dense matrix");
    CHECK_NULL(rhs, "dense rhs");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        int dof;
        const int row0 = body_index * MBD_BODY2D_DOF;

        if (!system->bodies[body_index].is_ground) {
            continue;
        }
        if (row0 + MBD_BODY2D_DOF > n) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "ground body %d dense row exceeds solve size %d",
                             body_index,
                             n);
        }

        for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
            const int row = row0 + dof;
            int col_index;

            rhs[row] = 0.0;
            for (col_index = 0; col_index < n; ++col_index) {
                matrix[row * n + col_index] = 0.0;
                matrix[col_index * n + row] = 0.0;
            }
            matrix[row * n + row] = 1.0;
        }
    }

    for (row_index = system->num_bodies * MBD_BODY2D_DOF; row_index < n; ++row_index) {
        if (!isfinite(rhs[row_index])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "dense rhs row %d must be finite",
                             row_index);
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_allocate_dense_workspace(int total_dof,
                                                         double **matrix,
                                                         double **rhs,
                                                         double **solution)
{
    const size_t max_double_count = ((size_t) -1) / sizeof(double);
    size_t total_dof_size = 0;
    size_t matrix_entries = 0;

    CHECK_NULL(matrix, "dense matrix pointer");
    CHECK_NULL(rhs, "dense rhs pointer");
    CHECK_NULL(solution, "dense solution pointer");

    *matrix = NULL;
    *rhs = NULL;
    *solution = NULL;

    if (total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense total_dof %d must be positive",
                         total_dof);
    }

    total_dof_size = (size_t) total_dof;
    if (total_dof_size > max_double_count ||
        total_dof_size > max_double_count / total_dof_size) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense total_dof %d overflows matrix allocation",
                         total_dof);
    }
    matrix_entries = total_dof_size * total_dof_size;

    *matrix = (double *) calloc(matrix_entries, sizeof(**matrix));
    *rhs = (double *) calloc(total_dof_size, sizeof(**rhs));
    *solution = (double *) calloc(total_dof_size, sizeof(**solution));
    if (!*matrix || !*rhs || !*solution) {
        free(*solution);
        free(*rhs);
        free(*matrix);
        *solution = NULL;
        *rhs = NULL;
        *matrix = NULL;
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate dense workspace for total_dof=%d",
                         total_dof);
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_apply_position_projection_if_enabled(mbd_system2d_t *system)
{
    mbd_projection2d_options_t projection_options;
    mbd_projection2d_report_t projection_report;
    const char *source_status = MBD_SOURCE_DEFAULT;
    const char *max_iters_source_status = MBD_SOURCE_DEFAULT;
    const char *residual_tol_source_status = MBD_SOURCE_DEFAULT;

    CHECK_NULL(system, "mbd_system2d");

    mbd_projection2d_options_set_defaults(&projection_options);
    system->position_projection_enabled = parse_env_int_or_default_with_status("FEM4C_MBD_POSITION_PROJECTION",
                                                                               0,
                                                                               0,
                                                                               1,
                                                                               &source_status);
    if (!system->position_projection_enabled &&
        system->num_constraints > 0 &&
        mbd_system2d_has_ground_body(system)) {
        system->position_projection_enabled = 1;
        source_status = MBD_SOURCE_GROUND_AUTO;
    }
    system->position_projection_max_iterations =
        parse_env_int_or_default_with_status("FEM4C_MBD_POSITION_PROJECTION_MAX_ITERS",
                                             MBD_PROJECTION2D_DEFAULT_MAX_ITERS,
                                             1,
                                             64,
                                             &max_iters_source_status);
    system->position_projection_residual_tolerance =
        parse_env_double_or_default_with_status("FEM4C_MBD_POSITION_PROJECTION_RESIDUAL_TOL",
                                                MBD_PROJECTION2D_DEFAULT_RESIDUAL_TOL,
                                                1.0e-16,
                                                1.0,
                                                &residual_tol_source_status);
    system->position_projection_applied = 0;
    system->position_projection_target_reached = 0;
    system->position_projection_iterations_last = 0;
    system->position_projection_residual_l2_before = 0.0;
    system->position_projection_residual_l2_after = 0.0;
    system->position_projection_residual_reduction_ratio_last = 0.0;
    system->position_projection_velocity_residual_l2_before = 0.0;
    system->position_projection_velocity_residual_l2_after = 0.0;
    system->position_projection_velocity_reduction_ratio_last = 0.0;
    system->position_projection_correction_l2_last = 0.0;
    system->position_projection_source_status = source_status;
    system->position_projection_max_iterations_source_status = max_iters_source_status;
    system->position_projection_residual_tolerance_source_status = residual_tol_source_status;
    system->position_projection_stop_reason = MBD_PROJECTION_STOP_NOT_APPLIED;

    if (!system->position_projection_enabled) {
        system->position_projection_stop_reason = MBD_PROJECTION_STOP_DISABLED;
        return FEM_SUCCESS;
    }
    if (system->num_constraints <= 0) {
        system->position_projection_stop_reason = MBD_PROJECTION_STOP_NO_CONSTRAINTS;
        return FEM_SUCCESS;
    }

    projection_options.max_iterations = system->position_projection_max_iterations;
    projection_options.residual_tolerance = system->position_projection_residual_tolerance;
    CHECK_ERROR(mbd_projection2d_apply_with_options(system, &projection_options, &projection_report));
    system->position_projection_applied = projection_report.applied;
    system->position_projection_target_reached = projection_report.target_reached;
    system->position_projection_iterations_last = projection_report.iterations_applied;
    system->position_projection_residual_l2_before = projection_report.residual_l2_before;
    system->position_projection_residual_l2_after = projection_report.residual_l2_after;
    system->position_projection_residual_reduction_ratio_last = projection_report.residual_reduction_ratio;
    system->position_projection_velocity_residual_l2_before =
        projection_report.velocity_residual_l2_before;
    system->position_projection_velocity_residual_l2_after =
        projection_report.velocity_residual_l2_after;
    system->position_projection_velocity_reduction_ratio_last =
        projection_report.velocity_residual_reduction_ratio;
    system->position_projection_correction_l2_last = projection_report.correction_l2;
    system->position_projection_stop_reason = projection_report.stop_reason;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_solve_dense_kkt(mbd_system2d_t *system,
                                                double **dense_solution_out,
                                                int *dense_solution_count_out)
{
    mbd_dense_kkt2d_t dense_kkt;
    double *dense_matrix_compact = NULL;
    double *dense_rhs = NULL;
    double *dense_solution = NULL;
    int total_dof = 0;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(dense_solution_out, "dense KKT solution pointer");
    CHECK_NULL(dense_solution_count_out, "dense KKT solution count pointer");

    *dense_solution_out = NULL;
    *dense_solution_count_out = 0;

    mbd_dense_kkt2d_zero(&dense_kkt);
    err = mbd_dense_kkt2d_assemble(system, &dense_kkt);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    total_dof = dense_kkt.layout.total_dof;
    err = mbd_system2d_allocate_dense_workspace(total_dof,
                                                &dense_matrix_compact,
                                                &dense_rhs,
                                                &dense_solution);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = mbd_dense_kkt2d_copy_compact(&dense_kkt, dense_matrix_compact);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    memcpy(dense_rhs,
           dense_kkt.rhs,
           (size_t) total_dof * sizeof(*dense_rhs));
    err = mbd_system2d_apply_ground_lock_to_dense_system(system,
                                                         dense_matrix_compact,
                                                         dense_rhs,
                                                         total_dof);
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_dense_solve_with_projection_retry(system,
                                                             dense_matrix_compact,
                                                             dense_rhs,
                                                             total_dof,
                                                             dense_solution);
    }

cleanup:
    mbd_dense_kkt2d_free(&dense_kkt);
    free(dense_matrix_compact);
    free(dense_rhs);
    if (err != FEM_SUCCESS) {
        free(dense_solution);
        return err;
    }
    *dense_solution_out = dense_solution;
    *dense_solution_count_out = total_dof;
    return err;
}

static fem_error_t mbd_system2d_dense_solve_with_projection_retry(const mbd_system2d_t *system,
                                                                  const double *matrix,
                                                                  const double *rhs,
                                                                  int n,
                                                                  double *solution)
{
    fem_error_t err;
    const char *projection_env = getenv("FEM4C_MBD_POSITION_PROJECTION");
    const int projection_enabled =
        (system && system->position_projection_enabled) ||
        (projection_env && projection_env[0] != '\0' && atoi(projection_env) != 0);

    err = mbd_linear_solver_dense_solve(matrix,
                                        rhs,
                                        n,
                                        MBD_LINEAR_SOLVER_DENSE_DEFAULT_PIVOT_TOL,
                                        solution);
    if (err == FEM_SUCCESS) {
        return FEM_SUCCESS;
    }
    if (err != FEM_ERROR_SINGULAR_MATRIX || !projection_enabled) {
        return err;
    }

    error_clear();
    return mbd_linear_solver_dense_solve(matrix,
                                         rhs,
                                         n,
                                         MBD_PROJECTION_DENSE_RETRY_PIVOT_TOL,
                                         solution);
}

typedef fem_error_t (*mbd_system2d_iteration_prepare_fn)(mbd_system2d_t *system,
                                                         void *context);
typedef fem_error_t (*mbd_system2d_iteration_residual_fn)(mbd_system2d_t *system,
                                                          double *residual_l2,
                                                          int *num_equations,
                                                          void *context);

typedef struct {
    mbd_hht2d_params_t params;
    double previous_constraint_residual_l2;
} mbd_system2d_hht_iteration_context_t;

static fem_error_t mbd_system2d_prepare_hht_iteration(mbd_system2d_t *system,
                                                      void *context)
{
    mbd_system2d_hht_iteration_context_t *hht_context = (mbd_system2d_hht_iteration_context_t *) context;
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(hht_context, "hht iteration context");

    /* The HHT effective load already includes user/gravity/flexible contributions. */
    system->has_gravity = 0;
    system->gravity[0] = 0.0;
    system->gravity[1] = 0.0;
    if (system->num_bodies > 0) {
        memset(system->flexible_force, 0, mbd_system2d_body_force_bytes(system->num_bodies));
        memset(system->contact_force, 0, mbd_system2d_body_force_bytes(system->num_bodies));
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        double effective_force[MBD_BODY2D_DOF];

        CHECK_ERROR(mbd_forces2d_build_hht_effective_generalized_force(system,
                                                                       body_index,
                                                                       &hht_context->params,
                                                                       effective_force));
        CHECK_ERROR(mbd_body2d_set_generalized_force(&system->bodies[body_index],
                                                     effective_force));
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_compute_default_iteration_residual(mbd_system2d_t *system,
                                                                   double *residual_l2,
                                                                   int *num_equations,
                                                                   void *context)
{
    (void) context;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(residual_l2, "iteration residual");
    CHECK_NULL(num_equations, "iteration equation count");

    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            residual_l2,
                                                            num_equations));
    system->implicit_residual_l2_last = *residual_l2;
    system->implicit_residual_num_equations_last = *num_equations;
    system->implicit_residual_mode = MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_compute_hht_iteration_residual(mbd_system2d_t *system,
                                                               double *residual_l2,
                                                               int *num_equations,
                                                               void *context)
{
    mbd_system2d_hht_iteration_context_t *hht_context = (mbd_system2d_hht_iteration_context_t *) context;
    double current_constraint_residual_l2 = 0.0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(residual_l2, "iteration residual");
    CHECK_NULL(num_equations, "iteration equation count");
    CHECK_NULL(hht_context, "hht iteration context");

    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &current_constraint_residual_l2,
                                                            num_equations));
    CHECK_ERROR(mbd_hht2d_blend_scalar(current_constraint_residual_l2,
                                       hht_context->previous_constraint_residual_l2,
                                       &hht_context->params,
                                       residual_l2));
    system->implicit_residual_l2_last = *residual_l2;
    system->implicit_residual_num_equations_last = *num_equations;
    system->implicit_residual_mode = MBD_IMPLICIT_RESIDUAL_MODE_HHT_EFFECTIVE;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_apply_dense_solution_with_newmark(
    mbd_system2d_t *system,
    const mbd_body2d_t *base_bodies,
    const double *dense_solution,
    int dense_solution_count,
    const mbd_newmark2d_params_t *params)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(base_bodies, "newmark base bodies");
    CHECK_NULL(dense_solution, "dense KKT solution");
    CHECK_NULL(params, "newmark params");
    if (dense_solution_count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense solution count %d must be positive",
                         dense_solution_count);
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        const int row = body_index * MBD_BODY2D_DOF;
        double acceleration_next[MBD_BODY2D_DOF];
        mbd_body2d_t next_body;

        if (row + MBD_BODY2D_DOF > dense_solution_count) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "dense solution row %d exceeds available count %d",
                             row + MBD_BODY2D_DOF,
                             dense_solution_count);
        }

        acceleration_next[0] = dense_solution[row + 0];
        acceleration_next[1] = dense_solution[row + 1];
        acceleration_next[2] = dense_solution[row + 2];

        CHECK_ERROR(mbd_newmark2d_update_state(&base_bodies[body_index],
                                               acceleration_next,
                                               params,
                                               &next_body));
        system->bodies[body_index] = next_body;
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_execute_constrained_implicit_step(
    mbd_system2d_t *system,
    const mbd_body2d_t *base_bodies,
    const mbd_newmark2d_params_t *params,
    double residual_tol,
    const char *integrator_name,
    const char *implicit_scheme_mode,
    mbd_system2d_iteration_prepare_fn prepare_iteration,
    mbd_system2d_iteration_residual_fn compute_iteration_residual,
    void *iteration_context)
{
    double *dense_solution = NULL;
    int dense_solution_count = 0;
    double residual_l2 = 0.0;
    int num_equations = 0;
    int iteration;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(base_bodies, "newmark base bodies");
    CHECK_NULL(params, "newmark params");
    CHECK_NULL(integrator_name, "integrator name");
    CHECK_NULL(implicit_scheme_mode, "implicit scheme mode");
    if (!compute_iteration_residual) {
        compute_iteration_residual = mbd_system2d_compute_default_iteration_residual;
    }

    system->time.implicit_iterations_last = 0;
    system->implicit_residual_tolerance_last = residual_tol;
    system->implicit_residual_num_equations_last = 0;
    system->implicit_converged = 0;
    system->implicit_scheme_mode = implicit_scheme_mode;
    system->implicit_convergence_reason = MBD_IMPLICIT_REASON_NOT_RUN;
    for (iteration = 0; iteration < system->time.implicit_max_iterations; ++iteration) {
        if (prepare_iteration) {
            err = prepare_iteration(system, iteration_context);
            if (err != FEM_SUCCESS) {
                goto cleanup;
            }
        }
        free(dense_solution);
        dense_solution = NULL;
        dense_solution_count = 0;
        err = mbd_system2d_solve_dense_kkt(system,
                                           &dense_solution,
                                           &dense_solution_count);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        err = mbd_system2d_apply_dense_solution_with_newmark(system,
                                                             base_bodies,
                                                             dense_solution,
                                                             dense_solution_count,
                                                             params);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        free(dense_solution);
        dense_solution = NULL;
        dense_solution_count = 0;
        system->time.implicit_iterations_last = iteration + 1;
        err = compute_iteration_residual(system,
                                         &residual_l2,
                                         &num_equations,
                                         iteration_context);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        if (num_equations <= 0 || residual_l2 <= residual_tol) {
            system->implicit_converged = 1;
            system->implicit_convergence_reason =
                num_equations <= 0 ? MBD_IMPLICIT_REASON_NO_EQUATIONS
                                   : MBD_IMPLICIT_REASON_RESIDUAL_TOLERANCE;
            err = FEM_SUCCESS;
            goto cleanup;
        }
    }

    system->implicit_converged = 0;
    system->implicit_convergence_reason = MBD_IMPLICIT_REASON_ITERATION_CAP;
    err = error_set(FEM_ERROR_CONVERGENCE_FAILED,
                    "%s implicit step exceeded max iterations %d before convergence (residual_l2=%.16e tol=%.16e)",
                    integrator_name,
                    system->time.implicit_max_iterations,
                    residual_l2,
                    residual_tol);

cleanup:
    free(dense_solution);
    return err;
}

static fem_error_t mbd_system2d_build_newmark_predictor_bodies(
    const mbd_system2d_t *system,
    const mbd_newmark2d_params_t *params,
    mbd_body2d_t *predicted_bodies)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(predicted_bodies, "predicted bodies");
    CHECK_NULL(params, "newmark params");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        CHECK_ERROR(mbd_newmark2d_predict_state(&system->bodies[body_index],
                                                params,
                                                &predicted_bodies[body_index]));
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_prepare_newmark_lagged_contact_forces(
    mbd_system2d_t *system,
    const mbd_body2d_t *predicted_bodies)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(predicted_bodies, "newmark predicted bodies");

    if (system->num_bodies > 0) {
        memcpy(system->bodies, predicted_bodies, mbd_system2d_body_bytes(system->num_bodies));
    }
    CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(system));
    if (system->num_bodies > 0) {
        memset(system->current_generalized_force, 0, mbd_system2d_body_force_bytes(system->num_bodies));
    }

    /* Implicit contact uses predictor-evaluated macro contact as a lagged external load.
     * No contact Jacobian/tangent contribution is added to the KKT system. */
    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        CHECK_ERROR(mbd_forces2d_build_body_generalized_force(system,
                                                              body_index,
                                                              system->current_generalized_force[body_index]));
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_build_hht_predictor_bodies(
    const mbd_system2d_t *system,
    const mbd_hht2d_params_t *params,
    mbd_body2d_t *predicted_bodies)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(predicted_bodies, "predicted bodies");
    CHECK_NULL(params, "hht params");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        CHECK_ERROR(mbd_hht2d_predict_state(&system->bodies[body_index],
                                            params,
                                            &predicted_bodies[body_index]));
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_restore_hht_runtime_context(
    mbd_system2d_t *system,
    const double (*body_force_baseline)[MBD_BODY2D_DOF],
    const double (*flexible_force_baseline)[MBD_BODY2D_DOF],
    const double (*contact_force_baseline)[MBD_BODY2D_DOF],
    const double gravity_baseline[2],
    int has_gravity_baseline)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(body_force_baseline, "hht body force baseline");
    CHECK_NULL(flexible_force_baseline, "hht flexible force baseline");
    CHECK_NULL(contact_force_baseline, "hht contact force baseline");
    CHECK_NULL(gravity_baseline, "hht gravity baseline");

    system->gravity[0] = gravity_baseline[0];
    system->gravity[1] = gravity_baseline[1];
    system->has_gravity = has_gravity_baseline;
    if (system->num_bodies > 0) {
        memcpy(system->flexible_force,
               flexible_force_baseline,
               mbd_system2d_body_force_bytes(system->num_bodies));
        memcpy(system->contact_force,
               contact_force_baseline,
               mbd_system2d_body_force_bytes(system->num_bodies));
    }
    CHECK_ERROR(mbd_system2d_restore_body_forces(system, body_force_baseline));
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_execute_unconstrained_hht_step(
    mbd_system2d_t *system,
    const mbd_body2d_t *base_bodies,
    const mbd_hht2d_params_t *params)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(base_bodies, "hht base bodies");
    CHECK_NULL(params, "hht params");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        mbd_body2d_t next_body = base_bodies[body_index];

        CHECK_ERROR(mbd_hht2d_step_unconstrained(&base_bodies[body_index],
                                                 system->current_generalized_force[body_index],
                                                 system->previous_generalized_force[body_index],
                                                 params,
                                                 &next_body));
        system->bodies[body_index] = next_body;
    }

    system->implicit_converged = 1;
    system->implicit_convergence_reason = MBD_IMPLICIT_REASON_UNCONSTRAINED_DIRECT;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_execute_constrained_hht_step(
    mbd_system2d_t *system,
    const mbd_hht2d_params_t *params,
    const mbd_newmark2d_params_t *corrected_params,
    double constraint_residual_tol)
{
    mbd_body2d_t *base_bodies = NULL;
    mbd_body2d_t *predicted_bodies = NULL;
    double (*body_force_baseline)[MBD_BODY2D_DOF] = NULL;
    double (*flexible_force_baseline)[MBD_BODY2D_DOF] = NULL;
    double (*contact_force_baseline)[MBD_BODY2D_DOF] = NULL;
    double gravity_baseline[2];
    int has_gravity_baseline;
    int previous_num_equations;
    mbd_system2d_hht_iteration_context_t iteration_context;
    fem_error_t err;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(params, "hht params");
    CHECK_NULL(corrected_params, "hht corrected params");

    if (system->num_bodies > 0) {
        base_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*base_bodies));
        predicted_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*predicted_bodies));
        body_force_baseline =
            (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                sizeof(*body_force_baseline));
        flexible_force_baseline =
            (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                sizeof(*flexible_force_baseline));
        contact_force_baseline =
            (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                sizeof(*contact_force_baseline));
        if (!base_bodies || !predicted_bodies || !body_force_baseline ||
            !flexible_force_baseline || !contact_force_baseline) {
            free(base_bodies);
            free(predicted_bodies);
            free(body_force_baseline);
            free(flexible_force_baseline);
            free(contact_force_baseline);
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Failed to allocate constrained HHT scratch for %d bodies",
                             system->num_bodies);
        }
        memcpy(base_bodies, system->bodies, mbd_system2d_body_bytes(system->num_bodies));
    }
    CHECK_ERROR(mbd_system2d_build_hht_predictor_bodies(system,
                                                        params,
                                                        predicted_bodies));
    memset(&iteration_context, 0, sizeof(iteration_context));
    iteration_context.params = *params;
    previous_num_equations = 0;
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &iteration_context.previous_constraint_residual_l2,
                                                            &previous_num_equations));
    CHECK_ERROR(mbd_system2d_prepare_newmark_lagged_contact_forces(system,
                                                                   predicted_bodies));
    CHECK_ERROR(mbd_system2d_capture_body_forces(system, body_force_baseline));
    if (system->num_bodies > 0) {
        memcpy(flexible_force_baseline,
               system->flexible_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
        memcpy(contact_force_baseline,
               system->contact_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
    }
    gravity_baseline[0] = system->gravity[0];
    gravity_baseline[1] = system->gravity[1];
    has_gravity_baseline = system->has_gravity;

    err = mbd_system2d_execute_constrained_implicit_step(system,
                                                         base_bodies,
                                                         corrected_params,
                                                         constraint_residual_tol,
                                                         "hht",
                                                         MBD_IMPLICIT_SCHEME_HHT_MODIFIED_NEWTON,
                                                         mbd_system2d_prepare_hht_iteration,
                                                         mbd_system2d_compute_hht_iteration_residual,
                                                         &iteration_context);
    CHECK_ERROR(mbd_system2d_restore_hht_runtime_context(system,
                                                         body_force_baseline,
                                                         flexible_force_baseline,
                                                         contact_force_baseline,
                                                         gravity_baseline,
                                                         has_gravity_baseline));
    free(base_bodies);
    free(predicted_bodies);
    free(body_force_baseline);
    free(flexible_force_baseline);
    free(contact_force_baseline);
    CHECK_ERROR(err);
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_load(mbd_system2d_t *system,
                              const char *input_filename)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(input_filename, "MBD input filename");

    CHECK_ERROR(mbd_system2d_try_load_case_from_input(input_filename, system));
    CHECK_ERROR(input_read_coupled_directives(input_filename));
    if (!system->from_input) {
        CHECK_ERROR(mbd_system2d_setup_builtin_case(system));
    }
    CHECK_ERROR(mbd_system2d_enforce_ground_bodies(system));
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_do_newmark_step(mbd_system2d_t *system)
{
    mbd_newmark2d_params_t params;
    mbd_body2d_t *base_bodies = NULL;
    mbd_body2d_t *predicted_bodies = NULL;
    const double constraint_residual_tol = mbd_constraint_residual_tol_from_env(NULL);
    int body_index;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_ERROR(mbd_system2d_enforce_ground_bodies(system));
    if (system->num_bodies > 0) {
        base_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*base_bodies));
        predicted_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*predicted_bodies));
        if (!base_bodies || !predicted_bodies) {
            free(base_bodies);
            free(predicted_bodies);
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Failed to allocate Newmark scratch for %d bodies",
                             system->num_bodies);
        }
    }

    if (system->time.integrator != MBD_INTEGRATOR2D_NEWMARK_BETA) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Newmark step requested for non-Newmark integrator");
    }
    params.dt = system->time.dt;
    params.beta = system->time.newmark_beta;
    params.gamma = system->time.newmark_gamma;
    mbd_system2d_reset_implicit_trace(system,
                                      MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT,
                                      system->num_constraints == 0
                                          ? MBD_IMPLICIT_SCHEME_NEWMARK_FREE
                                          : MBD_IMPLICIT_SCHEME_NEWMARK_KKT,
                                      system->num_constraints == 0
                                          ? 0.0
                                          : constraint_residual_tol);
    system->hht_force_history_mode = MBD_HHT_FORCE_HISTORY_MODE_NOT_APPLICABLE;
    if (system->num_bodies > 0) {
        memcpy(base_bodies, system->bodies, mbd_system2d_body_bytes(system->num_bodies));
    }
    CHECK_ERROR(mbd_system2d_refresh_generalized_force_history(system));
    CHECK_ERROR(mbd_system2d_build_newmark_predictor_bodies(system,
                                                            &params,
                                                            predicted_bodies));
    CHECK_ERROR(mbd_system2d_prepare_newmark_lagged_contact_forces(system,
                                                                   predicted_bodies));

    if (system->num_constraints == 0) {
        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            double generalized_force[MBD_BODY2D_DOF];
            mbd_body2d_t next_body;

            CHECK_ERROR(mbd_system2d_get_current_generalized_force(system,
                                                                  body_index,
                                                                  generalized_force));

            CHECK_ERROR(mbd_newmark2d_step_unconstrained(&base_bodies[body_index],
                                                         generalized_force,
                                                         &params,
                                                         &next_body));
            system->bodies[body_index] = next_body;
        }
        system->implicit_converged = 1;
        system->implicit_convergence_reason = MBD_IMPLICIT_REASON_UNCONSTRAINED_DIRECT;
    } else {
        CHECK_ERROR(mbd_system2d_execute_constrained_implicit_step(system,
                                                                   base_bodies,
                                                                   &params,
                                                                   constraint_residual_tol,
                                                                   "newmark",
                                                                   MBD_IMPLICIT_SCHEME_NEWMARK_KKT,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL));
    }

    err = mbd_system2d_enforce_ground_bodies(system);
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_sync_body_states(system);
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_apply_position_projection_if_enabled(system);
    }
    if (system->num_constraints > 0) {
        if (err == FEM_SUCCESS) {
            err = mbd_system2d_check_constraint_residual(system,
                                                         constraint_residual_tol,
                                                         NULL,
                                                         NULL);
        }
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_clear_flexible_forces(system);
    }
    if (err == FEM_SUCCESS) {
        system->time.steps_executed += 1;
    }
    free(base_bodies);
    free(predicted_bodies);
    return err;
}

fem_error_t mbd_system2d_do_hht_step(mbd_system2d_t *system)
{
    mbd_hht2d_params_t params;
    mbd_newmark2d_params_t corrected_params;
    mbd_body2d_t *base_bodies = NULL;
    mbd_body2d_t *predicted_bodies = NULL;
    const double constraint_residual_tol = mbd_constraint_residual_tol_from_env(NULL);
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_ERROR(mbd_system2d_enforce_ground_bodies(system));
    if (system->num_bodies > 0) {
        base_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*base_bodies));
        predicted_bodies = (mbd_body2d_t *) calloc((size_t) system->num_bodies, sizeof(*predicted_bodies));
        if (!base_bodies || !predicted_bodies) {
            free(base_bodies);
            free(predicted_bodies);
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Failed to allocate HHT scratch for %d bodies",
                             system->num_bodies);
        }
    }

    if (system->time.integrator != MBD_INTEGRATOR2D_HHT_ALPHA) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "HHT step requested for non-HHT integrator");
    }

    CHECK_ERROR(mbd_hht2d_init_params(&params,
                                      system->time.dt,
                                      system->time.hht_alpha));
    CHECK_ERROR(mbd_hht2d_to_newmark_params(&params, &corrected_params));
    mbd_system2d_reset_implicit_trace(system,
                                      MBD_IMPLICIT_RESIDUAL_MODE_HHT_EFFECTIVE,
                                      system->num_constraints == 0
                                          ? MBD_IMPLICIT_SCHEME_HHT_FREE
                                          : MBD_IMPLICIT_SCHEME_HHT_MODIFIED_NEWTON,
                                      system->num_constraints == 0
                                          ? 0.0
                                          : constraint_residual_tol);
    system->hht_force_history_mode =
        MBD_HHT_FORCE_HISTORY_MODE_SYSTEM_OWNED_CURRENT_PREVIOUS;
    CHECK_ERROR(mbd_system2d_refresh_generalized_force_history(system));

    if (system->num_constraints == 0) {
        if (system->num_bodies > 0) {
            memcpy(base_bodies, system->bodies, mbd_system2d_body_bytes(system->num_bodies));
        }
        CHECK_ERROR(mbd_system2d_build_hht_predictor_bodies(system,
                                                            &params,
                                                            predicted_bodies));
        CHECK_ERROR(mbd_system2d_prepare_newmark_lagged_contact_forces(system,
                                                                       predicted_bodies));
        CHECK_ERROR(mbd_system2d_execute_unconstrained_hht_step(system,
                                                                base_bodies,
                                                                &params));
    } else {
        CHECK_ERROR(mbd_system2d_execute_constrained_hht_step(system,
                                                              &params,
                                                              &corrected_params,
                                                              constraint_residual_tol));
    }

    err = mbd_system2d_enforce_ground_bodies(system);
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_sync_body_states(system);
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_apply_position_projection_if_enabled(system);
    }
    if (system->num_constraints > 0) {
        if (err == FEM_SUCCESS) {
            err = mbd_system2d_check_constraint_residual(system,
                                                         constraint_residual_tol,
                                                         NULL,
                                                         NULL);
        }
    }
    if (err == FEM_SUCCESS) {
        err = mbd_system2d_clear_flexible_forces(system);
    }
    if (err == FEM_SUCCESS) {
        system->time.steps_executed += 1;
    }
    free(base_bodies);
    free(predicted_bodies);
    return err;
}

fem_error_t mbd_system2d_run(const char *input_filename,
                             const char *output_filename)
{
    mbd_system2d_t system;
    mbd_dense_kkt2d_t dense_kkt;
    mbd_kkt_layout_t layout;
    double *dense_matrix_compact = NULL;
    double *dense_solution = NULL;
    double *dense_rhs = NULL;
    double residual_l2 = 0.0;
    double kkt_rhs_l2 = 0.0;
    double kkt_solution_l2 = 0.0;
    double kkt_solve_residual_inf = 0.0;
    double constraint_residual_tol = MBD_CONSTRAINT_RESIDUAL_TOL_DEFAULT;
    const char *kkt_solve_source_status = MBD_SOURCE_DEFAULT;
    const char *constraint_residual_tol_source_status = MBD_SOURCE_DEFAULT;
    const char *step_execution_mode = "none";
    int kkt_solve_enabled = 0;
    int kkt_nonzero_count = 0;
    int num_equations = 0;
    int history_snapshot_count = 0;
    int rigid_compare_snapshot_count = 0;
    int contact_trace_snapshot_count = 0;
    int generic_contact_trace_snapshot_count = 0;
    int generic_contact_replay_use_snapshot_count = 0;
    int generic_contact_same_time_use_snapshot_count = 0;
    int contact_feedback_snapshot_count = 0;
    int contact_feedback_use_snapshot_count = 0;
    int contact_reduced_data_snapshot_count = 0;
    int same_time_reduced_iteration_snapshot_count = 0;
    int monolithic_local_patch_rows_snapshot_count = 0;
    int monolithic_proper_iteration_snapshot_count = 0;
    int i;
    FILE *out = NULL;
    FILE *history_out = NULL;
    FILE *rigid_compare_out = NULL;
    FILE *contact_trace_out = NULL;
    FILE *generic_contact_trace_out = NULL;
    FILE *generic_contact_replay_use_out = NULL;
    FILE *generic_contact_same_time_use_out = NULL;
    FILE *contact_feedback_out = NULL;
    FILE *contact_feedback_use_out = NULL;
    FILE *contact_reduced_data_out = NULL;
    FILE *same_time_reduced_iteration_out = NULL;
    FILE *monolithic_local_patch_rows_out = NULL;
    FILE *monolithic_proper_iteration_out = NULL;
    char history_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char rigid_compare_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char generic_contact_trace_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char generic_contact_replay_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char generic_contact_same_time_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char contact_feedback_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char contact_feedback_use_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char contact_reduced_data_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char same_time_reduced_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char same_time_contact_request_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char monolithic_local_patch_rows_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char monolithic_local_patch_summary_json_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};
    char monolithic_proper_iteration_output_filename[MBD_HISTORY_OUTPUT_FILENAME_CAPACITY] = {0};

    mbd_dense_kkt2d_zero(&dense_kkt);
    CHECK_NULL(input_filename, "MBD input filename");
    CHECK_NULL(output_filename, "MBD output filename");

    mbd_system2d_zero(&system);
    CHECK_ERROR(mbd_system2d_load(&system, input_filename));
    CHECK_ERROR(mbd_time_control_from_env(&system.time));
    CHECK_ERROR(mbd_system2d_load_monolithic_proper_internal_config_from_env(&system));
    CHECK_ERROR(mbd_system2d_load_local_feedback_records(&system));
    CHECK_ERROR(mbd_system2d_prepare_monolithic_local_patch_artifacts(&system, output_filename));
    constraint_residual_tol = mbd_constraint_residual_tol_from_env(&constraint_residual_tol_source_status);
    system.current_step_index = 0;
    CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(&system));
    CHECK_ERROR(mbd_output2d_open_history_file(output_filename,
                                               history_output_filename,
                                               &history_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_rigid_compare_file(output_filename,
                                                             &system,
                                                             rigid_compare_output_filename,
                                                             &rigid_compare_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_contact_trace_file(output_filename,
                                                             &system,
                                                             contact_trace_output_filename,
                                                             &contact_trace_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_generic_contact_trace_file(
                            output_filename,
                            &system,
                            generic_contact_trace_output_filename,
                            &generic_contact_trace_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_contact_feedback_file(output_filename,
                                                                &system,
                                                                contact_feedback_output_filename,
                                                                &contact_feedback_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_contact_feedback_use_file(output_filename,
                                                                    &system,
                                                                    contact_feedback_use_output_filename,
                                                                    &contact_feedback_use_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_contact_reduced_data_file(
                            output_filename,
                            &system,
                            contact_reduced_data_output_filename,
                            &contact_reduced_data_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_same_time_reduced_iteration_file(
                            output_filename,
                            &system,
                            same_time_reduced_iteration_output_filename,
                            &same_time_reduced_iteration_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_monolithic_local_patch_rows_file(
                            output_filename,
                            &system,
                            monolithic_local_patch_rows_output_filename,
                            &monolithic_local_patch_rows_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_generic_contact_replay_use_file(
                            output_filename,
                            &system,
                            generic_contact_replay_use_output_filename,
                            &generic_contact_replay_use_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR_CLEANUP(mbd_output2d_open_generic_contact_same_time_use_file(
                            output_filename,
                            &system,
                            generic_contact_same_time_use_output_filename,
                            &generic_contact_same_time_use_out),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR(mbd_output2d_open_monolithic_proper_iteration_file(
        output_filename,
        &system,
        monolithic_proper_iteration_output_filename,
        &monolithic_proper_iteration_out));
    if (system.local_contact_monolithic_mode ==
        MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE) {
        if (snprintf(monolithic_local_patch_summary_json_filename,
                     sizeof(monolithic_local_patch_summary_json_filename),
                     "%s.contact_circle_monolithic_local_patch_summary.json",
                     output_filename) >=
            (int)sizeof(monolithic_local_patch_summary_json_filename)) {
            mbd_output2d_close_optional_files(&history_out,
                                              &rigid_compare_out,
                                              &contact_trace_out,
                                              &contact_feedback_out,
                                              &contact_feedback_use_out,
                                              &contact_reduced_data_out,
                                              &same_time_reduced_iteration_out);
            mbd_system2d_close_file_quiet(&generic_contact_trace_out);
            mbd_system2d_close_file_quiet(&generic_contact_replay_use_out);
            mbd_system2d_close_file_quiet(&generic_contact_same_time_use_out);
            mbd_system2d_close_file_quiet(&monolithic_local_patch_rows_out);
            return error_set(FEM_ERROR_FILE_WRITE,
                             "monolithic local patch summary JSON filename is too long");
        }
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_history_snapshot(history_out,
                                                            0,
                                                            0.0,
                                                            &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    history_snapshot_count += 1;
    CHECK_ERROR_CLEANUP(mbd_output2d_write_rigid_compare_snapshot(rigid_compare_out,
                                                                  0.0,
                                                                  &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (rigid_compare_out) {
        rigid_compare_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_trace_step_snapshot(contact_trace_out,
                                                                       0,
                                                                       0.0,
                                                                       &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (contact_trace_out) {
        contact_trace_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_generic_contact_trace_step_snapshot(
                            generic_contact_trace_out,
                            0,
                            0.0,
                            &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (generic_contact_trace_out) {
        generic_contact_trace_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_step_snapshot(contact_feedback_out,
                                                                          0,
                                                                          0.0,
                                                                          &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (contact_feedback_out) {
        contact_feedback_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_use_step_snapshot(contact_feedback_use_out,
                                                                              0,
                                                                              0.0,
                                                                              &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (contact_feedback_use_out) {
        contact_feedback_use_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_reduced_data_step_snapshot(
                            contact_reduced_data_out,
                            0,
                            0.0,
                            &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (contact_reduced_data_out) {
        contact_reduced_data_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_monolithic_local_patch_rows_step_snapshot(
                            monolithic_local_patch_rows_out,
                            0,
                            0.0,
                            &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (monolithic_local_patch_rows_out) {
        monolithic_local_patch_rows_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_generic_contact_replay_use_step_snapshot(
                            generic_contact_replay_use_out,
                            0,
                            0.0,
                            &system),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    if (generic_contact_replay_use_out) {
        generic_contact_replay_use_snapshot_count += 1;
    }
    CHECK_ERROR_CLEANUP(
        mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
            generic_contact_same_time_use_out,
            0,
            0.0,
            &system),
        mbd_output2d_close_optional_files(&history_out,
                                          &rigid_compare_out,
                                          &contact_trace_out,
                                          &contact_feedback_out,
                                          &contact_feedback_use_out,
                                          &contact_reduced_data_out,
                                          &same_time_reduced_iteration_out));
    if (generic_contact_same_time_use_out) {
        generic_contact_same_time_use_snapshot_count += 1;
    }
    CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
        monolithic_proper_iteration_out,
        0,
        0.0,
        &system));
    if (monolithic_proper_iteration_out) {
        monolithic_proper_iteration_snapshot_count += 1;
    }
    if (system.time.integrator == MBD_INTEGRATOR2D_EXPLICIT) {
        int explicit_step = 0;
        if (system.time.steps_requested > 0) {
            step_execution_mode = system.num_constraints == 0 ? "explicit_free" : "explicit_kkt";
            if (system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                system.monolithic_proper_internal.enabled) {
                system.monolithic_proper_runtime.total_steps = system.time.steps_requested;
            }
            for (explicit_step = 0; explicit_step < system.time.steps_requested; ++explicit_step) {
                int sampled_monolithic_step = 0;
                system.current_step_index = explicit_step + 1;
                sampled_monolithic_step =
                    system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                    system.monolithic_proper_internal.enabled &&
                    mbd_system2d_monolithic_proper_step_is_sampled(&system,
                                                                   explicit_step + 1);
                if (system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                    system.monolithic_proper_internal.enabled &&
                    sampled_monolithic_step) {
                    CHECK_ERROR(mbd_system2d_do_monolithic_proper_explicit_step(
                        &system,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        monolithic_proper_iteration_out,
                        &monolithic_proper_iteration_snapshot_count));
                } else {
                    if (system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                        system.monolithic_proper_internal.enabled) {
                        mbd_system2d_reset_monolithic_proper_runtime(&system);
                        system.monolithic_proper_runtime.internal_loop_active = 1;
                        snprintf(system.monolithic_proper_runtime.overall_status,
                                 sizeof(system.monolithic_proper_runtime.overall_status),
                                 "%s",
                                 "not_sampled");
                    }
                    CHECK_ERROR(mbd_system2d_do_explicit_step(&system));
                }
                if (system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                    system.monolithic_proper_internal.enabled) {
                    system.monolithic_proper_runtime.accepted_steps += 1;
                    if (sampled_monolithic_step) {
                        system.monolithic_proper_runtime.sampled_step_count += 1;
                        system.monolithic_proper_runtime.monolithic_iteration_count_total +=
                            system.monolithic_proper_runtime.monolithic_iteration_count;
                        if (system.monolithic_proper_runtime.monolithic_iteration_count >
                            system.monolithic_proper_runtime.max_iteration_per_step) {
                            system.monolithic_proper_runtime.max_iteration_per_step =
                                system.monolithic_proper_runtime.monolithic_iteration_count;
                        }
                        system.monolithic_proper_runtime.halving_retry_count_total +=
                            system.monolithic_proper_runtime.halving_retry_count;
                        system.monolithic_proper_runtime.rejected_steps +=
                            system.monolithic_proper_runtime.halving_retry_count;
                        if (system.monolithic_proper_runtime.contact_active_flag) {
                            system.monolithic_proper_runtime.active_steps += 1;
                        }
                        if (system.monolithic_proper_runtime.converged_flag) {
                            system.monolithic_proper_runtime.converged_steps += 1;
                        }
                    }
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_history_snapshot(history_out,
                                                                        explicit_step + 1,
                                                                        system.time.dt * (explicit_step + 1),
                                                                        &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                history_snapshot_count += 1;
                CHECK_ERROR_CLEANUP(mbd_output2d_write_rigid_compare_snapshot(rigid_compare_out,
                                                                              system.time.dt * (explicit_step + 1),
                                                                              &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (rigid_compare_out) {
                    rigid_compare_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_trace_step_snapshot(contact_trace_out,
                                                                                   explicit_step + 1,
                                                                                   system.time.dt * (explicit_step + 1),
                                                                                   &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_trace_out) {
                    contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_trace_step_snapshot(
                        generic_contact_trace_out,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_trace_out) {
                    generic_contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_replay_use_step_snapshot(
                        generic_contact_replay_use_out,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_replay_use_out) {
                    generic_contact_replay_use_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
                        generic_contact_same_time_use_out,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_same_time_use_out) {
                    generic_contact_same_time_use_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_step_snapshot(contact_feedback_out,
                                                                                      explicit_step + 1,
                                                                                      system.time.dt * (explicit_step + 1),
                                                                                      &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_feedback_out) {
                    contact_feedback_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_feedback_use_step_snapshot(contact_feedback_use_out,
                                                                                          explicit_step + 1,
                                                                                          system.time.dt * (explicit_step + 1),
                                                                                          &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_feedback_use_out) {
                    contact_feedback_use_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_reduced_data_step_snapshot(
                                        contact_reduced_data_out,
                                        explicit_step + 1,
                                        system.time.dt * (explicit_step + 1),
                                        &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_reduced_data_out) {
                    contact_reduced_data_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_monolithic_local_patch_rows_step_snapshot(
                        monolithic_local_patch_rows_out,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (monolithic_local_patch_rows_out) {
                    monolithic_local_patch_rows_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_same_time_reduced_iteration_step_snapshot(
                        same_time_reduced_iteration_out,
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (same_time_reduced_iteration_out) {
                    same_time_reduced_iteration_snapshot_count +=
                        system.num_current_same_time_reduced_iterations;
                }
                if (!(system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                      system.monolithic_proper_internal.enabled)) {
                    CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
                        monolithic_proper_iteration_out,
                        explicit_step + 1,
                        system.time.dt * (explicit_step + 1),
                        &system));
                    if (monolithic_proper_iteration_out) {
                        monolithic_proper_iteration_snapshot_count += 1;
                    }
                }
            }
            if (system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
                system.monolithic_proper_internal.enabled) {
                const char *aggregate_status = "ok";

                if (system.monolithic_proper_runtime.failed_steps > 0) {
                    aggregate_status = "failed_nonconverged";
                } else if (system.monolithic_proper_runtime.sampled_step_count <= 0) {
                    aggregate_status = "no_sampled_steps";
                } else if (system.monolithic_proper_runtime.active_steps <
                           system.monolithic_proper_runtime.sampled_step_count) {
                    aggregate_status = "ok_with_skips";
                } else if (system.monolithic_proper_runtime.converged_steps <
                           system.monolithic_proper_runtime.active_steps) {
                    aggregate_status = "ok_partial";
                }
                snprintf(system.monolithic_proper_runtime.overall_status,
                         sizeof(system.monolithic_proper_runtime.overall_status),
                         "%s",
                         aggregate_status);
            }
        }
    } else if (system.time.integrator == MBD_INTEGRATOR2D_NEWMARK_BETA) {
        int newmark_step = 0;
        if (system.time.steps_requested > 0) {
            step_execution_mode = system.num_constraints == 0 ? "newmark_free" : "newmark_kkt";
            for (newmark_step = 0; newmark_step < system.time.steps_requested; ++newmark_step) {
                CHECK_ERROR(mbd_system2d_do_newmark_step(&system));
                CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(&system));
                CHECK_ERROR_CLEANUP(mbd_output2d_write_history_snapshot(history_out,
                                                                        newmark_step + 1,
                                                                        system.time.dt * (newmark_step + 1),
                                                                        &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                history_snapshot_count += 1;
                CHECK_ERROR_CLEANUP(mbd_output2d_write_rigid_compare_snapshot(rigid_compare_out,
                                                                              system.time.dt * (newmark_step + 1),
                                                                              &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (rigid_compare_out) {
                    rigid_compare_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_trace_step_snapshot(contact_trace_out,
                                                                                   newmark_step + 1,
                                                                                   system.time.dt * (newmark_step + 1),
                                                                                   &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_trace_out) {
                    contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_trace_step_snapshot(
                        generic_contact_trace_out,
                        newmark_step + 1,
                        system.time.dt * (newmark_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_trace_out) {
                    generic_contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_replay_use_step_snapshot(
                        generic_contact_replay_use_out,
                        newmark_step + 1,
                        system.time.dt * (newmark_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_replay_use_out) {
                    generic_contact_replay_use_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
                        generic_contact_same_time_use_out,
                        newmark_step + 1,
                        system.time.dt * (newmark_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_same_time_use_out) {
                    generic_contact_same_time_use_snapshot_count += 1;
                }
                CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
                    monolithic_proper_iteration_out,
                    newmark_step + 1,
                    system.time.dt * (newmark_step + 1),
                    &system));
                if (monolithic_proper_iteration_out) {
                    monolithic_proper_iteration_snapshot_count += 1;
                }
            }
        }
    } else if (system.time.integrator == MBD_INTEGRATOR2D_HHT_ALPHA) {
        int hht_step = 0;
        if (system.time.steps_requested > 0) {
            step_execution_mode = system.num_constraints == 0 ? "hht_free" : "hht_kkt";
            for (hht_step = 0; hht_step < system.time.steps_requested; ++hht_step) {
                CHECK_ERROR(mbd_system2d_do_hht_step(&system));
                CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(&system));
                CHECK_ERROR_CLEANUP(mbd_output2d_write_history_snapshot(history_out,
                                                                        hht_step + 1,
                                                                        system.time.dt * (hht_step + 1),
                                                                        &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                history_snapshot_count += 1;
                CHECK_ERROR_CLEANUP(mbd_output2d_write_rigid_compare_snapshot(rigid_compare_out,
                                                                              system.time.dt * (hht_step + 1),
                                                                              &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (rigid_compare_out) {
                    rigid_compare_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(mbd_output2d_write_contact_trace_step_snapshot(contact_trace_out,
                                                                                   hht_step + 1,
                                                                                   system.time.dt * (hht_step + 1),
                                                                                   &system),
                                    mbd_output2d_close_optional_files(&history_out,
                                                                      &rigid_compare_out,
                                                                      &contact_trace_out,
                                                                      &contact_feedback_out,
                                                                      &contact_feedback_use_out,
                                                                      &contact_reduced_data_out,
                                                                      &same_time_reduced_iteration_out));
                if (contact_trace_out) {
                    contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_trace_step_snapshot(
                        generic_contact_trace_out,
                        hht_step + 1,
                        system.time.dt * (hht_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_trace_out) {
                    generic_contact_trace_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_replay_use_step_snapshot(
                        generic_contact_replay_use_out,
                        hht_step + 1,
                        system.time.dt * (hht_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_replay_use_out) {
                    generic_contact_replay_use_snapshot_count += 1;
                }
                CHECK_ERROR_CLEANUP(
                    mbd_output2d_write_generic_contact_same_time_use_step_snapshot(
                        generic_contact_same_time_use_out,
                        hht_step + 1,
                        system.time.dt * (hht_step + 1),
                        &system),
                    mbd_output2d_close_optional_files(&history_out,
                                                      &rigid_compare_out,
                                                      &contact_trace_out,
                                                      &contact_feedback_out,
                                                      &contact_feedback_use_out,
                                                      &contact_reduced_data_out,
                                                      &same_time_reduced_iteration_out));
                if (generic_contact_same_time_use_out) {
                    generic_contact_same_time_use_snapshot_count += 1;
                }
                CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_step_snapshot(
                    monolithic_proper_iteration_out,
                    hht_step + 1,
                    system.time.dt * (hht_step + 1),
                    &system));
                if (monolithic_proper_iteration_out) {
                    monolithic_proper_iteration_snapshot_count += 1;
                }
            }
        }
    }
    CHECK_ERROR_CLEANUP(mbd_output2d_write_same_time_contact_request_file(
                            output_filename,
                            &system,
                            same_time_contact_request_output_filename),
                        mbd_output2d_close_optional_files(&history_out,
                                                          &rigid_compare_out,
                                                          &contact_trace_out,
                                                          &contact_feedback_out,
                                                          &contact_feedback_use_out,
                                                          &contact_reduced_data_out,
                                                          &same_time_reduced_iteration_out));
    CHECK_ERROR(mbd_system2d_sync_body_states(&system));
    CHECK_ERROR(mbd_system2d_compute_layout(&system, &layout));
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(&system,
                                                            &residual_l2,
                                                            &num_equations));
    CHECK_ERROR(mbd_dense_kkt2d_assemble(&system, &dense_kkt));
    layout = dense_kkt.layout;
    kkt_nonzero_count = mbd_dense_kkt2d_count_nonzero(&dense_kkt);
    kkt_rhs_l2 = mbd_dense_kkt2d_rhs_l2(&dense_kkt);
    kkt_solve_enabled = parse_env_int_or_default_with_status("FEM4C_MBD_SOLVE_DENSE_KKT",
                                                             0,
                                                             0,
                                                             1,
                                                             &kkt_solve_source_status);
    if (kkt_solve_enabled) {
        fem_error_t err = mbd_system2d_allocate_dense_workspace(dense_kkt.layout.total_dof,
                                                                &dense_matrix_compact,
                                                                &dense_rhs,
                                                                &dense_solution);
        if (err != FEM_SUCCESS) {
            return err;
        }
        err = mbd_dense_kkt2d_copy_compact(&dense_kkt, dense_matrix_compact);
        if (err != FEM_SUCCESS) {
            free(dense_solution);
            free(dense_rhs);
            free(dense_matrix_compact);
            mbd_dense_kkt2d_free(&dense_kkt);
            return err;
        }
        memcpy(dense_rhs,
               dense_kkt.rhs,
               (size_t) dense_kkt.layout.total_dof * sizeof(*dense_rhs));
        err = mbd_system2d_apply_ground_lock_to_dense_system(&system,
                                                             dense_matrix_compact,
                                                             dense_rhs,
                                                             dense_kkt.layout.total_dof);
        if (err != FEM_SUCCESS) {
            free(dense_solution);
            free(dense_rhs);
            free(dense_matrix_compact);
            mbd_dense_kkt2d_free(&dense_kkt);
            return err;
        }
        err = mbd_system2d_dense_solve_with_projection_retry(&system,
                                                             dense_matrix_compact,
                                                             dense_rhs,
                                                             dense_kkt.layout.total_dof,
                                                             dense_solution);
        if (err != FEM_SUCCESS) {
            free(dense_solution);
            free(dense_rhs);
            free(dense_matrix_compact);
            mbd_dense_kkt2d_free(&dense_kkt);
            return err;
        }
        kkt_solution_l2 = mbd_dense_vector_l2(dense_solution, dense_kkt.layout.total_dof);
        kkt_solve_residual_inf = mbd_linear_solver_dense_residual_inf(dense_matrix_compact,
                                                                      dense_rhs,
                                                                      dense_solution,
                                                                      dense_kkt.layout.total_dof);
    }

    if (!system.from_input) {
        printf("  mbd_source: builtin_fallback (input has no MBD_* entries)\n");
    } else {
        printf("  mbd_source: input_case (`MBD_BODY`/`MBD_BODY_DYN`/`MBD_GRAVITY`/`MBD_FORCE`/`MBD_BODY_CIRCLE`/`MBD_CONTACT_HALFSPACE`/`MBD_CONTACT_PAIR`/`MBD_CONTACT_PAIR_HALFSPACE`/`MBD_CONTACT_SURFACE_POLYLINE`/`MBD_CONTACT_PAIR_GENERIC`/`MBD_CONTACT_COUPLING_MODE`/`MBD_LOCAL_FEEDBACK_MODE`/`MBD_LOCAL_CONTACT_MONOLITHIC`/`MBD_LOCAL_FEEDBACK_FILE`/`MBD_LOCAL_CONTACT_FILE`/`MBD_EHL_FILE`/constraints)\n");
        printf("  mbd_caps: max_bodies=%d max_constraints=%d\n",
               MBD_SYSTEM2D_MAX_BODIES, system.constraint_capacity);
    }

    printf("MBD minimal case summary:\n");
    printf("  Bodies: %d\n", system.num_bodies);
    printf("  Constraints: %d\n", system.num_constraints);
    printf("  Constraint equations: %d\n", num_equations);
    printf("  constraint_equations: %d\n", num_equations);
    printf("  KKT layout: body_dof=%d lambda_dof=%d total_dof=%d\n",
           layout.body_dof, layout.lambda_dof, layout.total_dof);
    printf("  kkt_dense: nonzero=%d rhs_l2=%.6e\n",
           kkt_nonzero_count, kkt_rhs_l2);
    printf("  constraint_rhs: mode=baumgarte alpha=%.6e beta=%.6e\n",
           MBD_CONSTRAINT2D_BAUMGARTE_ALPHA_DEFAULT,
           MBD_CONSTRAINT2D_BAUMGARTE_BETA_DEFAULT);
    printf("  constraint_residual_tol: value=%.6e source=%s\n",
           constraint_residual_tol,
           constraint_residual_tol_source_status);
    printf("  kkt_solve: enabled=%d source=%s\n",
           kkt_solve_enabled, kkt_solve_source_status);
    if (kkt_solve_enabled) {
        printf("  kkt_solution: l2=%.6e residual_inf=%.6e\n",
               kkt_solution_l2, kkt_solve_residual_inf);
    }
    printf("  integrator: %s\n", mbd_integrator2d_to_string(system.time.integrator));
    printf("  integrator_source: %s\n", system.time.integrator_source_status);
    printf("  step_execution_mode: %s\n", step_execution_mode);
    printf("  implicit_params: beta=%.6e gamma=%.6e alpha=%.6e\n",
           system.time.newmark_beta,
           system.time.newmark_gamma,
           system.time.hht_alpha);
    printf("  implicit_param_sources: beta=%s gamma=%s alpha=%s\n",
           system.time.newmark_beta_source_status,
           system.time.newmark_gamma_source_status,
           system.time.hht_alpha_source_status);
    printf("  hht_force_history_mode: %s\n", system.hht_force_history_mode);
    CHECK_ERROR(mbd_system2d_print_generalized_force_history_summary(&system));
    printf("  implicit_result: iterations_last=%d max_iterations=%d max_iterations_source=%s converged=%d scheme=%s reason=%s residual_num_equations=%d residual_tolerance=%.6e\n",
           system.time.implicit_iterations_last,
           system.time.implicit_max_iterations,
           system.time.implicit_max_iterations_source_status,
           system.implicit_converged,
           system.implicit_scheme_mode,
           system.implicit_convergence_reason,
           system.implicit_residual_num_equations_last,
           system.implicit_residual_tolerance_last);
    printf("  implicit_result_residual: mode=%s l2=%.6e\n",
           system.implicit_residual_mode,
           system.implicit_residual_l2_last);
    printf("  position_projection: enabled=%d applied=%d target_reached=%d iterations=%d max_iters=%d residual_tol=%.6e source=%s max_source=%s tol_source=%s stop_reason=%s residual_before=%.6e residual_after=%.6e reduction_ratio=%.6e correction_l2=%.6e\n",
           system.position_projection_enabled,
           system.position_projection_applied,
           system.position_projection_target_reached,
           system.position_projection_iterations_last,
           system.position_projection_max_iterations,
           system.position_projection_residual_tolerance,
           system.position_projection_source_status,
           system.position_projection_max_iterations_source_status,
           system.position_projection_residual_tolerance_source_status,
           system.position_projection_stop_reason,
           system.position_projection_residual_l2_before,
           system.position_projection_residual_l2_after,
           system.position_projection_residual_reduction_ratio_last,
           system.position_projection_correction_l2_last);
    printf("  position_projection_velocity: residual_before=%.6e residual_after=%.6e reduction_ratio=%.6e\n",
           system.position_projection_velocity_residual_l2_before,
           system.position_projection_velocity_residual_l2_after,
           system.position_projection_velocity_reduction_ratio_last);
    printf("  trace_artifacts: history_snapshots=%d rigid_compare_enabled=%d rigid_compare_snapshots=%d\n",
           history_snapshot_count,
           rigid_compare_out ? 1 : 0,
           rigid_compare_snapshot_count);
    printf("  contact_trace: enabled=%d snapshot_count=%d pair_count=%d\n",
           contact_trace_out ? 1 : 0,
           contact_trace_snapshot_count,
           system.num_contact_pairs);
    printf("  generic_contact_trace: enabled=%d snapshot_count=%d pair_count=%d row_count_current=%d mode=%s\n",
           generic_contact_trace_out ? 1 : 0,
           generic_contact_trace_snapshot_count,
           system.num_generic_contact_pairs,
           system.num_current_generic_contact_trace_rows,
           MBD_GENERIC_CONTACT_MVP_SOURCE_MARK);
    printf("  generic_contact_replay_use: enabled=%d snapshot_count=%d local_feedback_mode=%s local_contact_file=%s legacy_file=%s\n",
           generic_contact_replay_use_out ? 1 : 0,
           generic_contact_replay_use_snapshot_count,
           mbd_local_feedback_mode_to_string(system.local_feedback_mode),
           system.local_contact_filename[0] != '\0' ? system.local_contact_filename : "none",
           system.local_feedback_filename[0] != '\0' ? system.local_feedback_filename : "none");
    printf("  generic_contact_same_time_use: enabled=%d snapshot_count=%d local_feedback_mode=%s local_contact_file=%s\n",
           generic_contact_same_time_use_out ? 1 : 0,
           generic_contact_same_time_use_snapshot_count,
           mbd_local_feedback_mode_to_string(system.local_feedback_mode),
           system.local_contact_filename[0] != '\0' ? system.local_contact_filename : "none");
    printf("  contact_feedback: enabled=%d snapshot_count=%d coupling_mode=%s\n",
           contact_feedback_out ? 1 : 0,
           contact_feedback_snapshot_count,
           mbd_contact_coupling_mode_to_string(system.contact_coupling_mode));
    printf("  contact_feedback_use: enabled=%d snapshot_count=%d local_feedback_mode=%s legacy_file=%s local_contact_file=%s ehl_file=%s records=%d\n",
           contact_feedback_use_out ? 1 : 0,
           contact_feedback_use_snapshot_count,
           mbd_local_feedback_mode_to_string(system.local_feedback_mode),
           system.local_feedback_filename[0] != '\0' ? system.local_feedback_filename : "none",
           system.local_contact_filename[0] != '\0' ? system.local_contact_filename : "none",
           system.ehl_filename[0] != '\0' ? system.ehl_filename : "none",
           system.num_local_feedback_records);
    printf("  contact_reduced_data: enabled=%d snapshot_count=%d\n",
           contact_reduced_data_out ? 1 : 0,
           contact_reduced_data_snapshot_count);
    printf("  same_time_reduced_iterations: enabled=%d row_count=%d\n",
           same_time_reduced_iteration_out ? 1 : 0,
           same_time_reduced_iteration_snapshot_count);
    printf("  same_time_contact_requests: enabled=%d row_count=%d\n",
           same_time_contact_request_output_filename[0] != '\0' ? 1 : 0,
           system.num_same_time_contact_request_rows);
    printf("  monolithic_local_patch: enabled=%d mode=%s row_snapshots=%d active_rows=%d gamma_var_rows=%d fn_positive_rows=%d artifact_root=%s\n",
           system.local_contact_monolithic_mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE ? 1 : 0,
           mbd_local_contact_monolithic_mode_to_string(
               system.local_contact_monolithic_mode),
           monolithic_local_patch_rows_snapshot_count,
           system.monolithic_local_patch_active_rows_total,
           system.monolithic_local_patch_gamma_not_one_rows_total,
           system.monolithic_local_patch_fn_positive_rows_total,
           system.local_contact_monolithic_artifact_root[0] != '\0'
               ? system.local_contact_monolithic_artifact_root
               : "none");
    printf("  monolithic_proper: enabled=%d mode=%s context_defined=%d context_step=%d iter=%d row_snapshots=%d internal_loop_active=%d total_steps=%d sampled_steps=%d active_steps=%d accepted_steps=%d rejected_steps=%d runtime_iter_count=%d iteration_count_total=%d max_iteration_per_step=%d halving_retry_count=%d halving_retry_count_total=%d converged_steps=%d failed_steps=%d k_contact_eff=%.6e mu_eff=%.6e stress_residual=%.6e displacement_residual=%.6e contact_parameter_residual=%.6e fem_residual=%.6e converged=%d overall_status=%s\n",
           system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE ? 1 : 0,
           mbd_monolithic_proper_mode_to_string(system.monolithic_proper_mode),
           system.monolithic_proper_context.is_defined,
           system.monolithic_proper_context.context_step,
           system.monolithic_proper_context.iter_index,
           monolithic_proper_iteration_snapshot_count,
           system.monolithic_proper_runtime.internal_loop_active,
           system.monolithic_proper_runtime.total_steps,
           system.monolithic_proper_runtime.sampled_step_count,
           system.monolithic_proper_runtime.active_steps,
           system.monolithic_proper_runtime.accepted_steps,
           system.monolithic_proper_runtime.rejected_steps,
           system.monolithic_proper_runtime.monolithic_iteration_count,
           system.monolithic_proper_runtime.monolithic_iteration_count_total,
           system.monolithic_proper_runtime.max_iteration_per_step,
           system.monolithic_proper_runtime.halving_retry_count,
           system.monolithic_proper_runtime.halving_retry_count_total,
           system.monolithic_proper_runtime.converged_steps,
           system.monolithic_proper_runtime.failed_steps,
           system.monolithic_proper_context.k_contact_eff,
           system.monolithic_proper_context.mu_eff,
           system.monolithic_proper_context.stress_residual,
           system.monolithic_proper_context.displacement_residual,
           system.monolithic_proper_context.contact_parameter_residual,
           system.monolithic_proper_context.fem_residual,
           system.monolithic_proper_context.converged_flag,
           system.monolithic_proper_runtime.overall_status);
    printf("  input_adapter_contract: primary=%s constraints=%s optional_geometry=%s\n",
           input_mbd_primary_directives_csv(),
           mbd_system2d_constraint_directives_csv(),
           input_mbd_optional_geometry_directives_csv());
    printf("  rigid_compare_contract: tip_geometry=%s root_reaction=%s energy=%s\n",
           mbd_output2d_rigid_compare_tip_geometry_contract(),
           mbd_output2d_rigid_compare_root_reaction_surface(),
           mbd_output2d_rigid_energy_surface());
    printf("  time_control: dt=%.6e steps=%d\n",
           system.time.dt, system.time.num_steps);
    printf("  time_fallback: dt=%s steps=%s\n",
           system.time.dt_source_status,
           system.time.steps_source_status);
    if (system.has_gravity) {
        printf("  gravity_input: gx=%.6e gy=%.6e\n",
               system.gravity[0], system.gravity[1]);
    }
    system.time.steps_executed = mbd_emit_step_trace(system.time.steps_requested,
                                                     system.time.dt,
                                                     system.time.integrator);
    printf("  steps_trace: requested=%d executed=%d\n",
           system.time.steps_requested, system.time.steps_executed);
    for (i = 0; i < system.num_bodies; ++i) {
        CHECK_ERROR(mbd_system2d_print_body_summary(i, &system.bodies[i]));
    }
    printf("  Constraint residual L2 norm: %.6e\n", residual_l2);
    printf("  residual_l2: %.6e\n", residual_l2);

    out = fopen(output_filename, "w");
    if (!out) {
        mbd_system2d_close_file_quiet(&history_out);
        mbd_system2d_close_file_quiet(&rigid_compare_out);
        mbd_system2d_close_file_quiet(&contact_trace_out);
        mbd_system2d_close_file_quiet(&generic_contact_trace_out);
        mbd_system2d_close_file_quiet(&contact_feedback_out);
        mbd_system2d_close_file_quiet(&contact_feedback_use_out);
        mbd_system2d_close_file_quiet(&contact_reduced_data_out);
        mbd_system2d_close_file_quiet(&same_time_reduced_iteration_out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open MBD output file: %s",
                         output_filename);
    }

    fprintf(out, "# FEM4C MBD minimal output\n");
    fprintf(out, "bodies,%d\n", system.num_bodies);
    fprintf(out, "constraints,%d\n", system.num_constraints);
    fprintf(out, "constraint_equations,%d\n", num_equations);
    fprintf(out, "body_dof,%d\n", layout.body_dof);
    fprintf(out, "lambda_dof,%d\n", layout.lambda_dof);
    fprintf(out, "total_dof,%d\n", layout.total_dof);
    fprintf(out, "kkt_nonzero_count,%d\n", kkt_nonzero_count);
    fprintf(out, "kkt_rhs_l2,%.16e\n", kkt_rhs_l2);
    fprintf(out, "constraint_rhs_mode,baumgarte\n");
    fprintf(out, "baumgarte_alpha,%.16e\n", MBD_CONSTRAINT2D_BAUMGARTE_ALPHA_DEFAULT);
    fprintf(out, "baumgarte_beta,%.16e\n", MBD_CONSTRAINT2D_BAUMGARTE_BETA_DEFAULT);
    fprintf(out, "constraint_residual_tol,%.16e\n", constraint_residual_tol);
    fprintf(out, "constraint_residual_tol_source_status,%s\n",
            constraint_residual_tol_source_status);
    fprintf(out, "kkt_solve_enabled,%d\n", kkt_solve_enabled);
    fprintf(out, "kkt_solve_source_status,%s\n", kkt_solve_source_status);
    fprintf(out, "kkt_solver_status,%s\n", kkt_solve_enabled ? "ok" : "disabled");
    fprintf(out, "kkt_solution_l2,%.16e\n", kkt_solution_l2);
    fprintf(out, "kkt_solve_residual_inf,%.16e\n", kkt_solve_residual_inf);
    fprintf(out, "integrator,%s\n", mbd_integrator2d_to_string(system.time.integrator));
    fprintf(out, "integrator_source_status,%s\n", system.time.integrator_source_status);
    fprintf(out, "step_execution_mode,%s\n", step_execution_mode);
    fprintf(out, "implicit_param_beta,%.16e\n", system.time.newmark_beta);
    fprintf(out, "implicit_param_gamma,%.16e\n", system.time.newmark_gamma);
    fprintf(out, "implicit_param_alpha,%.16e\n", system.time.hht_alpha);
    fprintf(out, "implicit_param_beta_source_status,%s\n", system.time.newmark_beta_source_status);
    fprintf(out, "implicit_param_gamma_source_status,%s\n", system.time.newmark_gamma_source_status);
    fprintf(out, "implicit_param_alpha_source_status,%s\n", system.time.hht_alpha_source_status);
    fprintf(out, "hht_force_history_mode,%s\n", system.hht_force_history_mode);
    fprintf(out, "implicit_result_max_iterations,%d\n", system.time.implicit_max_iterations);
    fprintf(out, "implicit_result_iterations_last,%d\n", system.time.implicit_iterations_last);
    fprintf(out, "implicit_result_converged,%d\n", system.implicit_converged);
    fprintf(out, "implicit_result_scheme,%s\n", system.implicit_scheme_mode);
    fprintf(out, "implicit_result_reason,%s\n", system.implicit_convergence_reason);
    fprintf(out, "implicit_result_residual_mode,%s\n", system.implicit_residual_mode);
    fprintf(out, "implicit_result_residual_l2_last,%.16e\n", system.implicit_residual_l2_last);
    fprintf(out, "implicit_result_residual_num_equations_last,%d\n",
            system.implicit_residual_num_equations_last);
    fprintf(out, "implicit_result_residual_tolerance_last,%.16e\n",
            system.implicit_residual_tolerance_last);
    fprintf(out, "implicit_result_max_iterations_source_status,%s\n",
            system.time.implicit_max_iterations_source_status);
    fprintf(out, "position_projection_enabled,%d\n", system.position_projection_enabled);
    fprintf(out, "position_projection_applied,%d\n", system.position_projection_applied);
    fprintf(out, "position_projection_target_reached,%d\n",
            system.position_projection_target_reached);
    fprintf(out, "position_projection_iterations_last,%d\n", system.position_projection_iterations_last);
    fprintf(out, "position_projection_max_iterations,%d\n", system.position_projection_max_iterations);
    fprintf(out, "position_projection_residual_tolerance,%.16e\n",
            system.position_projection_residual_tolerance);
    fprintf(out, "position_projection_source_status,%s\n", system.position_projection_source_status);
    fprintf(out, "position_projection_max_iterations_source_status,%s\n",
            system.position_projection_max_iterations_source_status);
    fprintf(out, "position_projection_residual_tolerance_source_status,%s\n",
            system.position_projection_residual_tolerance_source_status);
    fprintf(out, "position_projection_stop_reason,%s\n",
            system.position_projection_stop_reason);
    fprintf(out, "position_projection_residual_l2_before,%.16e\n",
            system.position_projection_residual_l2_before);
    fprintf(out, "position_projection_residual_l2_after,%.16e\n",
            system.position_projection_residual_l2_after);
    fprintf(out, "position_projection_residual_reduction_ratio_last,%.16e\n",
            system.position_projection_residual_reduction_ratio_last);
    fprintf(out, "position_projection_velocity_residual_l2_before,%.16e\n",
            system.position_projection_velocity_residual_l2_before);
    fprintf(out, "position_projection_velocity_residual_l2_after,%.16e\n",
            system.position_projection_velocity_residual_l2_after);
    fprintf(out, "position_projection_velocity_reduction_ratio_last,%.16e\n",
            system.position_projection_velocity_reduction_ratio_last);
    fprintf(out, "position_projection_correction_l2_last,%.16e\n",
            system.position_projection_correction_l2_last);
    fprintf(out, "history_snapshot_count,%d\n", history_snapshot_count);
    fprintf(out, "rigid_compare_enabled,%d\n", rigid_compare_out ? 1 : 0);
    fprintf(out, "rigid_compare_snapshot_count,%d\n", rigid_compare_snapshot_count);
    fprintf(out, "contact_trace_enabled,%d\n", contact_trace_out ? 1 : 0);
    fprintf(out, "contact_trace_snapshot_count,%d\n", contact_trace_snapshot_count);
    fprintf(out, "generic_contact_trace_enabled,%d\n", generic_contact_trace_out ? 1 : 0);
    fprintf(out, "generic_contact_trace_snapshot_count,%d\n",
            generic_contact_trace_snapshot_count);
    fprintf(out, "generic_contact_replay_use_enabled,%d\n",
            generic_contact_replay_use_out ? 1 : 0);
    fprintf(out, "generic_contact_replay_use_snapshot_count,%d\n",
            generic_contact_replay_use_snapshot_count);
    fprintf(out, "generic_contact_same_time_use_enabled,%d\n",
            generic_contact_same_time_use_out ? 1 : 0);
    fprintf(out, "generic_contact_same_time_use_snapshot_count,%d\n",
            generic_contact_same_time_use_snapshot_count);
    fprintf(out, "generic_contact_pair_count,%d\n", system.num_generic_contact_pairs);
    fprintf(out, "generic_contact_trace_row_count_current,%d\n",
            system.num_current_generic_contact_trace_rows);
    fprintf(out, "contact_feedback_enabled,%d\n", contact_feedback_out ? 1 : 0);
    fprintf(out, "contact_feedback_snapshot_count,%d\n", contact_feedback_snapshot_count);
    fprintf(out, "contact_feedback_use_enabled,%d\n", contact_feedback_use_out ? 1 : 0);
    fprintf(out, "contact_feedback_use_snapshot_count,%d\n", contact_feedback_use_snapshot_count);
    fprintf(out, "contact_reduced_data_enabled,%d\n", contact_reduced_data_out ? 1 : 0);
    fprintf(out, "contact_reduced_data_snapshot_count,%d\n",
            contact_reduced_data_snapshot_count);
    fprintf(out, "same_time_reduced_iteration_enabled,%d\n",
            same_time_reduced_iteration_out ? 1 : 0);
    fprintf(out, "same_time_reduced_iteration_row_count,%d\n",
            same_time_reduced_iteration_snapshot_count);
    fprintf(out, "same_time_contact_request_enabled,%d\n",
            same_time_contact_request_output_filename[0] != '\0' ? 1 : 0);
    fprintf(out, "same_time_contact_request_row_count,%d\n",
            system.num_same_time_contact_request_rows);
    fprintf(out, "monolithic_local_patch_enabled,%d\n",
            system.local_contact_monolithic_mode !=
                    MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE
                ? 1
                : 0);
    fprintf(out, "monolithic_local_patch_mode,%s\n",
            mbd_local_contact_monolithic_mode_to_string(
                system.local_contact_monolithic_mode));
    fprintf(out, "monolithic_local_patch_row_snapshot_count,%d\n",
            monolithic_local_patch_rows_snapshot_count);
    fprintf(out, "monolithic_local_patch_active_rows_total,%d\n",
            system.monolithic_local_patch_active_rows_total);
    fprintf(out, "monolithic_local_patch_gamma_not_one_rows_total,%d\n",
            system.monolithic_local_patch_gamma_not_one_rows_total);
    fprintf(out, "monolithic_local_patch_fn_positive_rows_total,%d\n",
            system.monolithic_local_patch_fn_positive_rows_total);
    fprintf(out, "monolithic_proper_enabled,%d\n",
            system.monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE ? 1 : 0);
    fprintf(out, "monolithic_proper_mode,%s\n",
            mbd_monolithic_proper_mode_to_string(system.monolithic_proper_mode));
    fprintf(out, "monolithic_proper_context_defined,%d\n",
            system.monolithic_proper_context.is_defined);
    fprintf(out, "monolithic_proper_context_step,%d\n",
            system.monolithic_proper_context.context_step);
    fprintf(out, "monolithic_proper_context_iter_index,%d\n",
            system.monolithic_proper_context.iter_index);
    fprintf(out, "monolithic_proper_context_k_contact_eff,%.16e\n",
            system.monolithic_proper_context.k_contact_eff);
    fprintf(out, "monolithic_proper_context_mu_eff,%.16e\n",
            system.monolithic_proper_context.mu_eff);
    fprintf(out, "monolithic_proper_context_stress_residual,%.16e\n",
            system.monolithic_proper_context.stress_residual);
    fprintf(out, "monolithic_proper_context_displacement_residual,%.16e\n",
            system.monolithic_proper_context.displacement_residual);
    fprintf(out, "monolithic_proper_context_contact_parameter_residual,%.16e\n",
            system.monolithic_proper_context.contact_parameter_residual);
    fprintf(out, "monolithic_proper_context_fem_residual,%.16e\n",
            system.monolithic_proper_context.fem_residual);
    fprintf(out, "monolithic_proper_context_converged,%d\n",
            system.monolithic_proper_context.converged_flag);
    fprintf(out, "monolithic_proper_internal_loop_active,%d\n",
            system.monolithic_proper_runtime.internal_loop_active);
    fprintf(out, "coupling_mode,%s\n", "monolithic");
    fprintf(out, "monolithic_proper_total_steps,%d\n",
            system.monolithic_proper_runtime.total_steps);
    fprintf(out, "monolithic_proper_sampled_step_count,%d\n",
            system.monolithic_proper_runtime.sampled_step_count);
    fprintf(out, "monolithic_proper_active_steps,%d\n",
            system.monolithic_proper_runtime.active_steps);
    fprintf(out, "monolithic_proper_accepted_steps,%d\n",
            system.monolithic_proper_runtime.accepted_steps);
    fprintf(out, "monolithic_proper_rejected_steps,%d\n",
            system.monolithic_proper_runtime.rejected_steps);
    fprintf(out, "monolithic_proper_iteration_count,%d\n",
            system.monolithic_proper_runtime.monolithic_iteration_count);
    fprintf(out, "monolithic_proper_iteration_count_total,%d\n",
            system.monolithic_proper_runtime.monolithic_iteration_count_total);
    fprintf(out, "monolithic_proper_max_iteration_per_step,%d\n",
            system.monolithic_proper_runtime.max_iteration_per_step);
    fprintf(out, "monolithic_proper_runtime_stress_residual,%.16e\n",
            system.monolithic_proper_runtime.stress_residual);
    fprintf(out, "monolithic_proper_runtime_displacement_residual,%.16e\n",
            system.monolithic_proper_runtime.displacement_residual);
    fprintf(out, "monolithic_proper_runtime_contact_parameter_residual,%.16e\n",
            system.monolithic_proper_runtime.contact_parameter_residual);
    fprintf(out, "monolithic_proper_runtime_fem_residual,%.16e\n",
            system.monolithic_proper_runtime.fem_residual);
    fprintf(out, "monolithic_proper_runtime_k_contact_eff,%.16e\n",
            system.monolithic_proper_runtime.k_contact_eff);
    fprintf(out, "monolithic_proper_runtime_mu_eff,%.16e\n",
            system.monolithic_proper_runtime.mu_eff);
    fprintf(out, "monolithic_proper_runtime_contact_active_flag,%d\n",
            system.monolithic_proper_runtime.contact_active_flag);
    fprintf(out, "monolithic_proper_runtime_feedback_available_flag,%d\n",
            system.monolithic_proper_runtime.feedback_available_flag);
    fprintf(out, "monolithic_proper_runtime_converged_flag,%d\n",
            system.monolithic_proper_runtime.converged_flag);
    fprintf(out, "monolithic_proper_runtime_halving_retry_count,%d\n",
            system.monolithic_proper_runtime.halving_retry_count);
    fprintf(out, "monolithic_proper_runtime_halving_retry_count_total,%d\n",
            system.monolithic_proper_runtime.halving_retry_count_total);
    fprintf(out, "monolithic_proper_runtime_converged_steps,%d\n",
            system.monolithic_proper_runtime.converged_steps);
    fprintf(out, "monolithic_proper_runtime_failed_steps,%d\n",
            system.monolithic_proper_runtime.failed_steps);
    fprintf(out, "monolithic_proper_runtime_overall_status,%s\n",
            system.monolithic_proper_runtime.overall_status);
    fprintf(out, "monolithic_proper_runtime_artifact_root,%s\n",
            system.monolithic_proper_runtime.artifact_root[0] != '\0'
                ? system.monolithic_proper_runtime.artifact_root
                : "none");
    fprintf(out, "monolithic_proper_iteration_snapshot_count,%d\n",
            monolithic_proper_iteration_snapshot_count);
    fprintf(out, "contact_pair_count,%d\n", system.num_contact_pairs);
    fprintf(out, "contact_coupling_mode,%s\n",
            mbd_contact_coupling_mode_to_string(system.contact_coupling_mode));
    fprintf(out, "local_feedback_mode,%s\n",
            mbd_local_feedback_mode_to_string(system.local_feedback_mode));
    fprintf(out, "local_feedback_file,%s\n",
            system.local_feedback_filename[0] != '\0'
                ? system.local_feedback_filename
                : "none");
    fprintf(out, "local_contact_file,%s\n",
            system.local_contact_filename[0] != '\0'
                ? system.local_contact_filename
                : "none");
    fprintf(out, "local_contact_monolithic_artifact_root,%s\n",
            system.local_contact_monolithic_artifact_root[0] != '\0'
                ? system.local_contact_monolithic_artifact_root
                : "none");
    fprintf(out, "ehl_file,%s\n",
            system.ehl_filename[0] != '\0'
                ? system.ehl_filename
                : "none");
    fprintf(out, "dt,%.16e\n", system.time.dt);
    fprintf(out, "steps,%d\n", system.time.num_steps);
    fprintf(out, "steps_requested,%d\n", system.time.steps_requested);
    fprintf(out, "steps_executed,%d\n", system.time.steps_executed);
    fprintf(out, "dt_source_status,%s\n", system.time.dt_source_status);
    fprintf(out, "steps_source_status,%s\n", system.time.steps_source_status);
    fprintf(out, "source,%s\n", system.from_input ? "input" : "builtin");
    fprintf(out, "history_csv,%s\n", history_output_filename);
    fprintf(out, "rigid_compare_csv,%s\n",
            rigid_compare_output_filename[0] != '\0'
                ? rigid_compare_output_filename
                : "disabled");
    fprintf(out, "contact_trace_csv,%s\n",
            contact_trace_output_filename[0] != '\0'
                ? contact_trace_output_filename
                : "disabled");
    fprintf(out, "generic_contact_trace_csv,%s\n",
            generic_contact_trace_output_filename[0] != '\0'
                ? generic_contact_trace_output_filename
                : "disabled");
    fprintf(out, "generic_contact_replay_use_csv,%s\n",
            generic_contact_replay_use_output_filename[0] != '\0'
                ? generic_contact_replay_use_output_filename
                : "disabled");
    fprintf(out, "generic_contact_same_time_use_csv,%s\n",
            generic_contact_same_time_use_output_filename[0] != '\0'
                ? generic_contact_same_time_use_output_filename
                : "disabled");
    fprintf(out, "contact_feedback_csv,%s\n",
            contact_feedback_output_filename[0] != '\0'
                ? contact_feedback_output_filename
                : "disabled");
    fprintf(out, "contact_feedback_use_csv,%s\n",
            contact_feedback_use_output_filename[0] != '\0'
                ? contact_feedback_use_output_filename
                : "disabled");
    fprintf(out, "contact_reduced_data_csv,%s\n",
            contact_reduced_data_output_filename[0] != '\0'
                ? contact_reduced_data_output_filename
                : "disabled");
    fprintf(out, "same_time_reduced_iteration_csv,%s\n",
            same_time_reduced_iteration_output_filename[0] != '\0'
                ? same_time_reduced_iteration_output_filename
                : "disabled");
    fprintf(out, "same_time_contact_request_csv,%s\n",
            same_time_contact_request_output_filename[0] != '\0'
                ? same_time_contact_request_output_filename
                : "disabled");
    fprintf(out, "monolithic_local_patch_rows_csv,%s\n",
            monolithic_local_patch_rows_output_filename[0] != '\0'
                ? monolithic_local_patch_rows_output_filename
                : "disabled");
    fprintf(out, "monolithic_local_patch_summary_json,%s\n",
            monolithic_local_patch_summary_json_filename[0] != '\0'
                ? monolithic_local_patch_summary_json_filename
                : "disabled");
    fprintf(out, "monolithic_proper_iteration_csv,%s\n",
            monolithic_proper_iteration_output_filename[0] != '\0'
                ? monolithic_proper_iteration_output_filename
                : "disabled");
    fprintf(out, "input_adapter_primary_directives,%s\n",
            input_mbd_primary_directives_csv());
    fprintf(out, "input_adapter_constraint_directives,%s\n",
            mbd_system2d_constraint_directives_csv());
    fprintf(out, "input_adapter_optional_geometry_directives,%s\n",
            input_mbd_optional_geometry_directives_csv());
    fprintf(out, "rigid_compare_tip_geometry_contract,%s\n",
            mbd_output2d_rigid_compare_tip_geometry_contract());
    fprintf(out, "rigid_compare_root_reaction_surface,%s\n",
            mbd_output2d_rigid_compare_root_reaction_surface());
    fprintf(out, "energy_surface,%s\n",
            mbd_output2d_rigid_energy_surface());
    fprintf(out, "artifact_route_class,product_adjacent\n");
    fprintf(out, "artifact_family,rigid_compare_v1\n");
    fprintf(out, "artifact_preferred_compare_source,rigid_compare_csv\n");
    CHECK_ERROR(mbd_output2d_write_generalized_force_history_rows(out, &system));
    fprintf(out, "gravity_enabled,%d\n", system.has_gravity);
    fprintf(out, "gravity_x,%.16e\n", system.gravity[0]);
    fprintf(out, "gravity_y,%.16e\n", system.gravity[1]);
    for (i = 0; i < system.num_bodies; ++i) {
        CHECK_ERROR(mbd_system2d_write_body_output_row(out, i, &system.bodies[i]));
    }
    CHECK_ERROR(mbd_dense_kkt2d_write_output(out, &dense_kkt));
    if (kkt_solve_enabled) {
        CHECK_ERROR(mbd_dense_solution_write_output(out,
                                                    dense_solution,
                                                    dense_kkt.layout.total_dof));
    }
    fprintf(out, "residual_l2,%.16e\n", residual_l2);

    {
        fem_error_t close_out_err = mbd_system2d_close_file_checked(&out,
                                                                    FEM_ERROR_FILE_WRITE,
                                                                    "MBD output file",
                                                                    output_filename);
        if (close_out_err != FEM_SUCCESS) {
            mbd_system2d_close_file_quiet(&history_out);
            mbd_system2d_close_file_quiet(&rigid_compare_out);
            mbd_system2d_close_file_quiet(&contact_trace_out);
            mbd_system2d_close_file_quiet(&generic_contact_trace_out);
            mbd_system2d_close_file_quiet(&generic_contact_replay_use_out);
            mbd_system2d_close_file_quiet(&generic_contact_same_time_use_out);
            mbd_system2d_close_file_quiet(&contact_feedback_out);
            mbd_system2d_close_file_quiet(&contact_feedback_use_out);
            mbd_system2d_close_file_quiet(&contact_reduced_data_out);
            mbd_system2d_close_file_quiet(&same_time_reduced_iteration_out);
            mbd_system2d_close_file_quiet(&monolithic_local_patch_rows_out);
            mbd_system2d_close_file_quiet(&monolithic_proper_iteration_out);
            free(dense_solution);
            free(dense_rhs);
            free(dense_matrix_compact);
            mbd_dense_kkt2d_free(&dense_kkt);
            return close_out_err;
        }
    }
    CHECK_ERROR(mbd_system2d_close_file_checked(&history_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD history output file",
                                                history_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&rigid_compare_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD rigid compare output file",
                                                rigid_compare_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&contact_trace_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD contact trace output file",
                                                contact_trace_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&generic_contact_trace_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD generic contact trace output file",
                                                generic_contact_trace_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&generic_contact_replay_use_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD generic contact replay-use output file",
                                                generic_contact_replay_use_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&generic_contact_same_time_use_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD generic contact same-time-use output file",
                                                generic_contact_same_time_use_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&contact_feedback_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD contact feedback output file",
                                                contact_feedback_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&contact_feedback_use_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD contact feedback-use output file",
                                                contact_feedback_use_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&contact_reduced_data_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD contact reduced-data output file",
                                                contact_reduced_data_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&same_time_reduced_iteration_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD same-time reduced iteration output file",
                                                same_time_reduced_iteration_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&monolithic_local_patch_rows_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD monolithic local patch rows output file",
                                                monolithic_local_patch_rows_output_filename));
    CHECK_ERROR(mbd_system2d_close_file_checked(&monolithic_proper_iteration_out,
                                                FEM_ERROR_FILE_WRITE,
                                                "MBD monolithic proper iteration output file",
                                                monolithic_proper_iteration_output_filename));
    if (monolithic_local_patch_summary_json_filename[0] != '\0') {
        CHECK_ERROR(mbd_output2d_write_monolithic_local_patch_summary_json(
            monolithic_local_patch_summary_json_filename,
            monolithic_local_patch_rows_output_filename,
            &system));
    }

    free(dense_solution);
    free(dense_rhs);
    free(dense_matrix_compact);
    mbd_dense_kkt2d_free(&dense_kkt);
    return FEM_SUCCESS;
}

static int string_equals_ignore_case(const char *lhs, const char *rhs)
{
    size_t i = 0;

    if (!lhs || !rhs) {
        return 0;
    }

    while (lhs[i] != '\0' && rhs[i] != '\0') {
        if (tolower((unsigned char)lhs[i]) != tolower((unsigned char)rhs[i])) {
            return 0;
        }
        ++i;
    }

    return lhs[i] == '\0' && rhs[i] == '\0';
}

static double parse_env_double_or_default_with_status(const char *name,
                                                      double default_value,
                                                      double min_value,
                                                      double max_value,
                                                      const char **status_out)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    double parsed = 0.0;

    if (!env_value || env_value[0] == '\0') {
        if (status_out) {
            *status_out = MBD_SOURCE_DEFAULT;
        }
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }

    errno = 0;
    parsed = strtod(env_value, &end_ptr);
    if (end_ptr == env_value || *end_ptr != '\0' || !isfinite(parsed) || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %.6e\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %.6e..%.6e), fallback to %.6e\n",
                name, env_value, min_value, max_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK;
        }
        return default_value;
    }
    if (status_out) {
        *status_out = MBD_SOURCE_ENV;
    }
    return parsed;
}

static int parse_env_int_or_default_with_status(const char *name,
                                                int default_value,
                                                int min_value,
                                                int max_value,
                                                const char **status_out)
{
    const char *env_value = getenv(name);
    char *end_ptr = NULL;
    long parsed = 0;

    if (!env_value || env_value[0] == '\0') {
        if (status_out) {
            *status_out = MBD_SOURCE_DEFAULT;
        }
        return default_value;
    }
    if (isspace((unsigned char)env_value[0])) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }

    errno = 0;
    parsed = strtol(env_value, &end_ptr, 10);
    if (end_ptr == env_value || *end_ptr != '\0' || errno == ERANGE) {
        fprintf(stderr,
                "Warning: invalid %s='%s', fallback to %d\n",
                name, env_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return default_value;
    }
    if (parsed < min_value || parsed > max_value) {
        fprintf(stderr,
                "Warning: out-of-range %s='%s' (allowed %d..%d), fallback to %d\n",
                name, env_value, min_value, max_value, default_value);
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK;
        }
        return default_value;
    }
    if (status_out) {
        *status_out = MBD_SOURCE_ENV;
    }
    return (int)parsed;
}

static int source_marker_is_cli(const char *source_marker)
{
    if (!source_marker || source_marker[0] == '\0') {
        return 0;
    }
    return string_equals_ignore_case(source_marker, MBD_SOURCE_CLI);
}

static fem_error_t mbd_integrator_parse(const char *text,
                                        mbd_integrator2d_t *integrator)
{
    CHECK_NULL(text, "mbd integrator");
    CHECK_NULL(integrator, "mbd integrator out");

    if (string_equals_ignore_case(text, "explicit")) {
        *integrator = MBD_INTEGRATOR2D_EXPLICIT;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "newmark_beta") ||
        string_equals_ignore_case(text, "newmark-beta") ||
        string_equals_ignore_case(text, "newmark")) {
        *integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
        return FEM_SUCCESS;
    }
    if (string_equals_ignore_case(text, "hht_alpha") ||
        string_equals_ignore_case(text, "hht-alpha") ||
        string_equals_ignore_case(text, "hht")) {
        *integrator = MBD_INTEGRATOR2D_HHT_ALPHA;
        return FEM_SUCCESS;
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Unknown MBD integrator '%s' (expected: explicit|newmark_beta|hht_alpha)",
                     text);
}

static mbd_integrator2d_t mbd_integrator_from_env(const char **status_out)
{
    mbd_integrator2d_t integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
    const char *env_integrator = getenv("FEM4C_MBD_INTEGRATOR");

    if (!env_integrator || env_integrator[0] == '\0') {
        if (status_out) {
            *status_out = MBD_SOURCE_DEFAULT;
        }
        return integrator;
    }

    if (mbd_integrator_parse(env_integrator, &integrator) != FEM_SUCCESS) {
        fprintf(stderr,
                "Warning: invalid FEM4C_MBD_INTEGRATOR='%s', fallback to 'newmark_beta'\n",
                env_integrator);
        integrator = MBD_INTEGRATOR2D_NEWMARK_BETA;
        if (status_out) {
            *status_out = MBD_SOURCE_ENV_INVALID_FALLBACK;
        }
        return integrator;
    }

    if (status_out) {
        *status_out = MBD_SOURCE_ENV;
    }
    return integrator;
}

static fem_error_t mbd_time_control_from_env(mbd_time_control2d_t *time)
{
    const char *newmark_beta_source_marker = NULL;
    const char *newmark_gamma_source_marker = NULL;
    const char *hht_alpha_source_marker = NULL;
    const char *implicit_max_iterations_source_marker = NULL;
    const char *dt_source_marker = NULL;
    const char *steps_source_marker = NULL;

    CHECK_NULL(time, "mbd time control");

    mbd_time_control2d_set_defaults(time);
    time->integrator = mbd_integrator_from_env(&time->integrator_source_status);
    time->newmark_beta = parse_env_double_or_default_with_status("FEM4C_MBD_NEWMARK_BETA",
                                                                 2.5e-1, 1.0e-12, 1.0,
                                                                 &time->newmark_beta_source_status);
    time->newmark_gamma = parse_env_double_or_default_with_status("FEM4C_MBD_NEWMARK_GAMMA",
                                                                  5.0e-1, 1.0e-12, 1.5,
                                                                  &time->newmark_gamma_source_status);
    time->hht_alpha = parse_env_double_or_default_with_status("FEM4C_MBD_HHT_ALPHA",
                                                              -5.0e-2, -1.0 / 3.0, 0.0,
                                                              &time->hht_alpha_source_status);
    time->dt = parse_env_double_or_default_with_status("FEM4C_MBD_DT",
                                                       1.0e-3, 1.0e-12, 1.0e3,
                                                       &time->dt_source_status);
    time->num_steps = parse_env_int_or_default_with_status("FEM4C_MBD_STEPS",
                                                           1, 0, 1000000,
                                                           &time->steps_source_status);
    time->implicit_max_iterations = parse_env_int_or_default_with_status("FEM4C_MBD_IMPLICIT_MAX_ITERS",
                                                                         1, 0, 1000,
                                                                         &time->implicit_max_iterations_source_status);
    time->steps_requested = time->num_steps;
    time->steps_executed = 0;
    time->implicit_iterations_last = 0;

    newmark_beta_source_marker = getenv("FEM4C_MBD_NEWMARK_BETA_SOURCE");
    newmark_gamma_source_marker = getenv("FEM4C_MBD_NEWMARK_GAMMA_SOURCE");
    hht_alpha_source_marker = getenv("FEM4C_MBD_HHT_ALPHA_SOURCE");
    implicit_max_iterations_source_marker = getenv("FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE");
    dt_source_marker = getenv("FEM4C_MBD_DT_SOURCE");
    steps_source_marker = getenv("FEM4C_MBD_STEPS_SOURCE");

    if (source_marker_is_cli(getenv("FEM4C_MBD_INTEGRATOR_SOURCE")) &&
        strcmp(time->integrator_source_status, MBD_SOURCE_ENV) == 0) {
        time->integrator_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(newmark_beta_source_marker) &&
        strcmp(time->newmark_beta_source_status, MBD_SOURCE_ENV) == 0) {
        time->newmark_beta_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(newmark_gamma_source_marker) &&
        strcmp(time->newmark_gamma_source_status, MBD_SOURCE_ENV) == 0) {
        time->newmark_gamma_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(hht_alpha_source_marker) &&
        strcmp(time->hht_alpha_source_status, MBD_SOURCE_ENV) == 0) {
        time->hht_alpha_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(implicit_max_iterations_source_marker) &&
        strcmp(time->implicit_max_iterations_source_status, MBD_SOURCE_ENV) == 0) {
        time->implicit_max_iterations_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(dt_source_marker) &&
        strcmp(time->dt_source_status, MBD_SOURCE_ENV) == 0) {
        time->dt_source_status = MBD_SOURCE_CLI;
    }
    if (source_marker_is_cli(steps_source_marker) &&
        strcmp(time->steps_source_status, MBD_SOURCE_ENV) == 0) {
        time->steps_source_status = MBD_SOURCE_CLI;
    }

    return FEM_SUCCESS;
}

static int mbd_emit_step_trace(int requested_steps,
                               double dt,
                               mbd_integrator2d_t integrator)
{
    int step = 0;
    int executed = 0;
    const int compact_threshold = 16;
    const char *integrator_name = mbd_integrator2d_to_string(integrator);

    if (requested_steps <= 0) {
        return 0;
    }

    for (step = 1; step <= requested_steps; ++step) {
        int should_print = 0;
        ++executed;

        if (requested_steps <= compact_threshold) {
            should_print = 1;
        } else if (step <= 3 || step > requested_steps - 3) {
            should_print = 1;
        }

        if (should_print) {
            const double step_time = dt * (double)step;
            printf("  mbd_step=%d/%d t=%.6e integrator=%s\n",
                   step, requested_steps, step_time, integrator_name);
        } else if (step == 4) {
            printf("  mbd_step=... (%d steps omitted for compact trace)\n",
                   requested_steps - 6);
        }
    }

    return executed;
}

static const char *skip_leading_spaces(const char *text)
{
    const char *p = text;

    while (p && *p != '\0' && isspace((unsigned char)*p)) {
        ++p;
    }
    return p;
}

static int line_starts_with_token(const char *line, const char *token)
{
    const char *p;
    size_t token_len;

    if (!line || !token) {
        return 0;
    }
    p = skip_leading_spaces(line);
    token_len = strlen(token);
    if (strncmp(p, token, token_len) != 0) {
        return 0;
    }
    return p[token_len] == '\0' || isspace((unsigned char)p[token_len]);
}

static int line_starts_with_prefix(const char *line, const char *prefix)
{
    const char *p;
    size_t prefix_len;

    if (!line || !prefix) {
        return 0;
    }
    p = skip_leading_spaces(line);
    prefix_len = strlen(prefix);
    return strncmp(p, prefix, prefix_len) == 0;
}

static void line_to_excerpt(const char *line, char *out, size_t out_size)
{
    size_t i = 0;

    if (!line || !out || out_size == 0) {
        return;
    }

    while (line[i] != '\0' && line[i] != '\n' && line[i] != '\r' && i + 1 < out_size) {
        out[i] = line[i];
        ++i;
    }
    out[i] = '\0';
}

static int parse_distance_line(const char *line, mbd_constraint2d_t *out)
{
    int id = 0;
    int body_i = 0;
    int body_j = 0;
    double anchor_i_x = 0.0;
    double anchor_i_y = 0.0;
    double anchor_j_x = 0.0;
    double anchor_j_y = 0.0;
    double distance = 0.0;
    int scanned;

    if (!line_starts_with_token(line, "MBD_DISTANCE")) {
        return 0;
    }
    scanned = sscanf(line, "MBD_DISTANCE %d %d %d %lf %lf %lf %lf %lf",
                     &id, &body_i, &body_j,
                     &anchor_i_x, &anchor_i_y,
                     &anchor_j_x, &anchor_j_y,
                     &distance);
    if (scanned != 8) {
        return -1;
    }
    if (!isfinite(anchor_i_x) || !isfinite(anchor_i_y) ||
        !isfinite(anchor_j_x) || !isfinite(anchor_j_y) ||
        !isfinite(distance) || distance <= 0.0) {
        return -1;
    }

    {
        const double anchor_i[2] = {anchor_i_x, anchor_i_y};
        const double anchor_j[2] = {anchor_j_x, anchor_j_y};
        if (mbd_constraint_init_distance(out, id, body_i, body_j,
                                         anchor_i, anchor_j, distance) != FEM_SUCCESS) {
            return -1;
        }
    }

    return 1;
}

static int parse_revolute_line(const char *line, mbd_constraint2d_t *out)
{
    int id = 0;
    int body_i = 0;
    int body_j = 0;
    double anchor_i_x = 0.0;
    double anchor_i_y = 0.0;
    double anchor_j_x = 0.0;
    double anchor_j_y = 0.0;
    int scanned;

    if (!line_starts_with_token(line, "MBD_REVOLUTE")) {
        return 0;
    }
    scanned = sscanf(line, "MBD_REVOLUTE %d %d %d %lf %lf %lf %lf",
                     &id, &body_i, &body_j,
                     &anchor_i_x, &anchor_i_y,
                     &anchor_j_x, &anchor_j_y);
    if (scanned != 7) {
        return -1;
    }
    if (!isfinite(anchor_i_x) || !isfinite(anchor_i_y) ||
        !isfinite(anchor_j_x) || !isfinite(anchor_j_y)) {
        return -1;
    }

    {
        const double anchor_i[2] = {anchor_i_x, anchor_i_y};
        const double anchor_j[2] = {anchor_j_x, anchor_j_y};
        if (mbd_constraint_init_revolute(out, id, body_i, body_j,
                                         anchor_i, anchor_j) != FEM_SUCCESS) {
            return -1;
        }
    }

    return 1;
}

static fem_error_t mbd_system2d_try_load_case_from_input(const char *input_filename,
                                                         mbd_system2d_t *system)
{
    fem_error_t err;
    FILE *fp = NULL;
    char line[512];
    char excerpt[160];
    mbd_constraint_source_t *constraint_sources = NULL;
    int constraint_source_capacity = 0;
    int loaded_constraints = 0;
    int saw_mbd_entry = 0;
    int first_mbd_line = 0;
    int max_seen_referenced_body = -1;
    int line_no = 0;
    int i;

    CHECK_NULL(system, "mbd_system2d");

    mbd_system2d_zero(system);
    err = input_read_mbd_body_directives(input_filename,
                                         system,
                                         &saw_mbd_entry,
                                         &first_mbd_line);
    CHECK_ERROR(err);
    if (!saw_mbd_entry) {
        return FEM_SUCCESS;
    }

    fp = fopen(input_filename, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND,
                         "Cannot open MBD input file: %s",
                         input_filename);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        mbd_constraint2d_t parsed_constraint;
        int parsed = 0;
        ++line_no;

        parsed = parse_distance_line(line, &parsed_constraint);
        if (parsed == -1) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            mbd_system2d_close_file_quiet(&fp);
            free(constraint_sources);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_DISTANCE at line %d: '%s'",
                             MBD_DIAG_E_DISTANCE_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            int body_index_i = -1;
            int body_index_j = -1;

            if (mbd_system2d_find_body_index_by_id(system,
                                                   parsed_constraint.body_i,
                                                   &body_index_i) != FEM_SUCCESS ||
                mbd_system2d_find_body_index_by_id(system,
                                                   parsed_constraint.body_j,
                                                   &body_index_j) != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_DISTANCE at line %d references undefined body id (%d,%d)",
                                 MBD_DIAG_E_UNDEFINED_BODY_REF,
                                 line_no,
                                 parsed_constraint.body_i,
                                 parsed_constraint.body_j);
            }
            parsed_constraint.body_i = body_index_i;
            parsed_constraint.body_j = body_index_j;
            if (mbd_system2d_append_constraint(system, &parsed_constraint) != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Failed to register MBD_DISTANCE id %d at line %d",
                                 MBD_DIAG_E_DISTANCE_PARSE,
                                 parsed_constraint.id,
                                 line_no);
            }
            err = mbd_constraint_sources_reserve(&constraint_sources,
                                                 &constraint_source_capacity,
                                                 loaded_constraints + 1);
            if (err != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return err;
            }
            constraint_sources[loaded_constraints].id = parsed_constraint.id;
            constraint_sources[loaded_constraints].body_i = parsed_constraint.body_i;
            constraint_sources[loaded_constraints].body_j = parsed_constraint.body_j;
            constraint_sources[loaded_constraints].line_no = line_no;
            if (parsed_constraint.body_i > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_i;
            }
            if (parsed_constraint.body_j > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_j;
            }
            ++loaded_constraints;
            continue;
        }

        parsed = parse_revolute_line(line, &parsed_constraint);
        if (parsed == -1) {
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            mbd_system2d_close_file_quiet(&fp);
            free(constraint_sources);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_REVOLUTE at line %d: '%s'",
                             MBD_DIAG_E_REVOLUTE_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            int body_index_i = -1;
            int body_index_j = -1;

            if (mbd_system2d_find_body_index_by_id(system,
                                                   parsed_constraint.body_i,
                                                   &body_index_i) != FEM_SUCCESS ||
                mbd_system2d_find_body_index_by_id(system,
                                                   parsed_constraint.body_j,
                                                   &body_index_j) != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_REVOLUTE at line %d references undefined body id (%d,%d)",
                                 MBD_DIAG_E_UNDEFINED_BODY_REF,
                                 line_no,
                                 parsed_constraint.body_i,
                                 parsed_constraint.body_j);
            }
            parsed_constraint.body_i = body_index_i;
            parsed_constraint.body_j = body_index_j;
            if (mbd_system2d_append_constraint(system, &parsed_constraint) != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Failed to register MBD_REVOLUTE id %d at line %d",
                                 MBD_DIAG_E_REVOLUTE_PARSE,
                                 parsed_constraint.id,
                                 line_no);
            }
            err = mbd_constraint_sources_reserve(&constraint_sources,
                                                 &constraint_source_capacity,
                                                 loaded_constraints + 1);
            if (err != FEM_SUCCESS) {
                mbd_system2d_close_file_quiet(&fp);
                free(constraint_sources);
                return err;
            }
            constraint_sources[loaded_constraints].id = parsed_constraint.id;
            constraint_sources[loaded_constraints].body_i = parsed_constraint.body_i;
            constraint_sources[loaded_constraints].body_j = parsed_constraint.body_j;
            constraint_sources[loaded_constraints].line_no = line_no;
            if (parsed_constraint.body_i > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_i;
            }
            if (parsed_constraint.body_j > max_seen_referenced_body) {
                max_seen_referenced_body = parsed_constraint.body_j;
            }
            ++loaded_constraints;
            continue;
        }

        if (line_starts_with_prefix(line, "MBD_")) {
            if (line_starts_with_token(line, "MBD_BODY") ||
                line_starts_with_token(line, "MBD_BODY_DYN") ||
                line_starts_with_token(line, "MBD_BODY_GROUND") ||
                line_starts_with_token(line, "MBD_GRAVITY") ||
                line_starts_with_token(line, "MBD_FORCE") ||
                line_starts_with_token(line, "MBD_BODY_CIRCLE") ||
                line_starts_with_token(line, "MBD_CONTACT_HALFSPACE") ||
                line_starts_with_token(line, "MBD_CONTACT_PAIR") ||
                line_starts_with_token(line, "MBD_CONTACT_PAIR_HALFSPACE") ||
                line_starts_with_token(line, "MBD_CONTACT_SURFACE_POLYLINE") ||
                line_starts_with_token(line, "MBD_CONTACT_PAIR_GENERIC") ||
                line_starts_with_token(line, "MBD_CONTACT_COUPLING_MODE") ||
                line_starts_with_token(line, "MBD_LOCAL_FEEDBACK_MODE") ||
                line_starts_with_token(line, "MBD_LOCAL_CONTACT_MONOLITHIC") ||
                line_starts_with_token(line, "MBD_MONOLITHIC_PROPER_MODE") ||
                line_starts_with_token(line, "MBD_MONOLITHIC_PROPER_CONTEXT") ||
                line_starts_with_token(line, "MBD_LOCAL_FEEDBACK_FILE") ||
                line_starts_with_token(line, "MBD_LOCAL_CONTACT_FILE") ||
                line_starts_with_token(line, "MBD_EHL_FILE")) {
                continue;
            }
            line_to_excerpt(line, excerpt, sizeof(excerpt));
            mbd_system2d_close_file_quiet(&fp);
            free(constraint_sources);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Unsupported MBD directive at line %d: '%s'",
                             MBD_DIAG_E_UNSUPPORTED_DIRECTIVE,
                             line_no,
                             excerpt);
        }
    }

    err = mbd_system2d_close_file_checked(&fp,
                                          FEM_ERROR_FILE_READ,
                                          "MBD input file after read",
                                          input_filename);
    if (err != FEM_SUCCESS) {
        free(constraint_sources);
        return err;
    }

    if (loaded_constraints < 1 && system->num_bodies < 1) {
        free(constraint_sources);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[%s] MBD input is incomplete: no bodies or constraints found (first MBD line: %d)",
                         MBD_DIAG_E_INCOMPLETE_INPUT,
                         first_mbd_line);
    }
    if (loaded_constraints > 0 && max_seen_referenced_body < 1) {
        free(constraint_sources);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[%s] MBD input is incomplete: constraints must reference at least body 0 and 1 (first MBD line: %d)",
                         MBD_DIAG_E_INCOMPLETE_INPUT,
                         first_mbd_line);
    }

    for (i = 0; i < loaded_constraints; ++i) {
        int body_i = constraint_sources[i].body_i;
        int body_j = constraint_sources[i].body_j;
        int constraint_line = constraint_sources[i].line_no;
        int constraint_id = constraint_sources[i].id;

        if (body_i >= 0 && !mbd_body2d_is_defined(&system->bodies[body_i])) {
            free(constraint_sources);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by constraint id %d at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             body_i,
                             constraint_id,
                             constraint_line);
        }
        if (body_j >= 0 && !mbd_body2d_is_defined(&system->bodies[body_j])) {
            free(constraint_sources);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by constraint id %d at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             body_j,
                             constraint_id,
                             constraint_line);
        }
    }

    if (loaded_constraints >= 3) {
        printf("  mbd_constraint_lines_processed: %d (third+ constraints accepted)\n",
               loaded_constraints);
    }

    if (max_seen_referenced_body + 1 > system->num_bodies) {
        system->num_bodies = max_seen_referenced_body + 1;
    }
    system->num_constraints = loaded_constraints;
    system->from_input = 1;
    err = mbd_system2d_sync_body_states(system);
    free(constraint_sources);
    CHECK_ERROR(err);
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_load_local_feedback_records(mbd_system2d_t *system)
{
    CHECK_NULL(system, "mbd_system2d");

    system->num_local_feedback_records = 0;
    memset(system->local_feedback_records, 0, sizeof(system->local_feedback_records));
    if (system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED) {
        return FEM_SUCCESS;
    }
    if (system->local_feedback_filename[0] != '\0') {
        return mbd_system2d_load_local_feedback_records_legacy_combined(system);
    }
    if (system->local_contact_filename[0] == '\0' &&
        system->ehl_filename[0] == '\0') {
        return FEM_SUCCESS;
    }
    return mbd_system2d_load_local_feedback_records_contract_split(system);
}

static fem_error_t mbd_system2d_load_local_feedback_records_legacy_combined(
    mbd_system2d_t *system)
{
    FILE *fp = NULL;
    char line[512];

    CHECK_NULL(system, "mbd_system2d");

    fp = fopen(system->local_feedback_filename, "r");
    if (!fp) {
        printf("  Warning: local feedback file could not be opened, fallback remains active: %s\n",
               system->local_feedback_filename);
        return FEM_SUCCESS;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *token = NULL;
        char *fields[5] = {NULL, NULL, NULL, NULL, NULL};
        int field_count = 0;
        int record_index = -1;
        int i = 0;
        mbd_local_feedback_record2d_t record;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "step,", 5) == 0) {
            continue;
        }

        memset(&record, 0, sizeof(record));
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        token = strtok(row_copy, ",\r\n");
        while (token && field_count < 5) {
            fields[field_count++] = token;
            token = strtok(NULL, ",\r\n");
        }
        if (field_count != 5) {
            continue;
        }
        if (sscanf(fields[0], "%d", &record.step) != 1 ||
            sscanf(fields[1], "%d", &record.pair_id) != 1 ||
            sscanf(fields[2], "%lf", &record.mu_eff) != 1 ||
            sscanf(fields[3], "%lf", &record.gamma_n) != 1) {
            continue;
        }
        if (record.step < 0 || record.pair_id < 0) {
            continue;
        }

        record.is_defined = 1;
        record.reduced_data = mbd_contact_feedback2d_make_basic(record.mu_eff,
                                                                record.gamma_n);
        record.status_ok = mbd_local_feedback_status_is_ok(fields[4]);
        snprintf(record.status, sizeof(record.status), "%s", fields[4]);
        record.is_valid = isfinite(record.reduced_data.mu_eff) &&
                          isfinite(record.reduced_data.gamma_n) &&
                          record.reduced_data.gamma_n >= 0.0;

        for (i = 0; i < system->num_local_feedback_records; ++i) {
            if (system->local_feedback_records[i].is_defined &&
                system->local_feedback_records[i].step == record.step &&
                system->local_feedback_records[i].pair_id == record.pair_id) {
                record_index = i;
                break;
            }
        }
        if (record_index < 0) {
            if (system->num_local_feedback_records >= MBD_LOCAL_FEEDBACK2D_MAX_RECORDS) {
                break;
            }
            record_index = system->num_local_feedback_records;
            system->num_local_feedback_records += 1;
        }
        system->local_feedback_records[record_index] = record;
    }

    CHECK_ERROR(mbd_system2d_close_file_checked(&fp,
                                                FEM_ERROR_FILE_READ,
                                                "local feedback file after read",
                                                system->local_feedback_filename));
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_load_local_feedback_records_contract_split(
    mbd_system2d_t *system)
{
    int i;

    CHECK_NULL(system, "mbd_system2d");

    if (system->local_contact_filename[0] != '\0') {
        CHECK_ERROR(mbd_system2d_load_local_contact_source_rows(system->local_contact_filename,
                                                                system));
    }
    if (system->ehl_filename[0] != '\0') {
        CHECK_ERROR(mbd_system2d_load_ehl_source_rows(system->ehl_filename, system));
    }

    for (i = 0; i < system->num_local_feedback_records; ++i) {
        mbd_local_feedback_record2d_t *record = &system->local_feedback_records[i];

        if (!record->is_defined) {
            continue;
        }
        record->mu_eff = record->reduced_data.mu_eff;
        record->gamma_n = record->reduced_data.gamma_n;
        record->is_valid =
            (record->reduced_data.valid_flag & MBD_CONTACT_FEEDBACK2D_VALID_MU_EFF) != 0u &&
            (record->reduced_data.valid_flag & MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N) != 0u &&
            record->status_ok &&
            isfinite(record->reduced_data.mu_eff) &&
            isfinite(record->reduced_data.gamma_n) &&
            record->reduced_data.gamma_n >= 0.0;
        if (record->status[0] == '\0') {
            snprintf(record->status,
                     sizeof(record->status),
                     "%s",
                     record->status_ok ? "ok" : "invalid");
        }
    }

    return FEM_SUCCESS;
}

static int mbd_system2d_find_or_create_local_feedback_record_index(
    mbd_system2d_t *system,
    int step,
    int pair_id)
{
    int i;

    if (!system || step < 0 || pair_id < 0) {
        return -1;
    }

    for (i = 0; i < system->num_local_feedback_records; ++i) {
        if (system->local_feedback_records[i].is_defined &&
            system->local_feedback_records[i].step == step &&
            system->local_feedback_records[i].pair_id == pair_id) {
            return i;
        }
    }

    if (system->num_local_feedback_records >= MBD_LOCAL_FEEDBACK2D_MAX_RECORDS) {
        return -1;
    }

    i = system->num_local_feedback_records;
    system->num_local_feedback_records += 1;
    memset(&system->local_feedback_records[i], 0, sizeof(system->local_feedback_records[i]));
    system->local_feedback_records[i].is_defined = 1;
    system->local_feedback_records[i].status_ok = 1;
    system->local_feedback_records[i].step = step;
    system->local_feedback_records[i].pair_id = pair_id;
    return i;
}

static int mbd_system2d_parse_csv_fields(char *line,
                                         char **fields,
                                         int max_fields)
{
    char *token = NULL;
    int count = 0;

    if (!line || !fields || max_fields <= 0) {
        return 0;
    }

    token = strtok(line, ",\r\n");
    while (token && count < max_fields) {
        fields[count++] = token;
        token = strtok(NULL, ",\r\n");
    }
    return count;
}

static int mbd_system2d_parse_uint32_field(const char *text, uint32_t *value_out)
{
    char *end = NULL;
    unsigned long value = 0ul;

    if (!text || !value_out) {
        return 0;
    }

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || (end && *end != '\0')) {
        return 0;
    }
    *value_out = (uint32_t)value;
    return 1;
}

static fem_error_t mbd_system2d_load_local_contact_source_rows(const char *path,
                                                               mbd_system2d_t *system)
{
    FILE *fp = NULL;
    char line[512];

    CHECK_NULL(path, "local contact source path");
    CHECK_NULL(system, "mbd_system2d");

    fp = fopen(path, "r");
    if (!fp) {
        printf("  Warning: local contact source could not be opened, fallback remains active: %s\n",
               path);
        return FEM_SUCCESS;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[8] = {NULL};
        int field_count = 0;
        int step = -1;
        int pair_id = -1;
        double gamma_n = 0.0;
        double delta_g_eff = 0.0;
        double fn_ref = 0.0;
        double p_max = 0.0;
        uint32_t valid_flag = 0u;
        int record_index = -1;
        int source_ok = 0;
        mbd_local_feedback_record2d_t *record = NULL;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "step,", 5) == 0) {
            continue;
        }

        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 8);
        if (field_count < 8) {
            continue;
        }
        if (sscanf(fields[0], "%d", &step) != 1 ||
            sscanf(fields[1], "%d", &pair_id) != 1 ||
            sscanf(fields[2], "%lf", &gamma_n) != 1 ||
            sscanf(fields[3], "%lf", &delta_g_eff) != 1 ||
            sscanf(fields[4], "%lf", &fn_ref) != 1 ||
            sscanf(fields[5], "%lf", &p_max) != 1 ||
            !mbd_system2d_parse_uint32_field(fields[6], &valid_flag)) {
            continue;
        }
        record_index = mbd_system2d_find_or_create_local_feedback_record_index(system,
                                                                               step,
                                                                               pair_id);
        if (record_index < 0) {
            break;
        }

        record = &system->local_feedback_records[record_index];
        record->reduced_data.gamma_n = gamma_n;
        record->reduced_data.delta_g_eff = delta_g_eff;
        record->reduced_data.fn_ref = fn_ref;
        record->reduced_data.p_max = p_max;
        if (isfinite(gamma_n)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N;
        }
        if (isfinite(delta_g_eff)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_DELTA_G_EFF;
        }
        if (isfinite(fn_ref)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_FN_REF;
        }
        if (isfinite(p_max)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_P_MAX;
        }
        source_ok = (valid_flag != 0u) && mbd_local_feedback_status_is_ok(fields[7]);
        record->status_ok = record->status_ok && source_ok;
        if (!source_ok) {
            snprintf(record->status, sizeof(record->status), "%s", fields[7]);
        } else if (record->status[0] == '\0') {
            snprintf(record->status, sizeof(record->status), "%s", "ok");
        }
    }

    CHECK_ERROR(mbd_system2d_close_file_checked(&fp,
                                                FEM_ERROR_FILE_READ,
                                                "local contact source file after read",
                                                path));
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_load_ehl_source_rows(const char *path,
                                                     mbd_system2d_t *system)
{
    FILE *fp = NULL;
    char line[512];

    CHECK_NULL(path, "ehl source path");
    CHECK_NULL(system, "mbd_system2d");

    fp = fopen(path, "r");
    if (!fp) {
        printf("  Warning: EHL source could not be opened, fallback remains active: %s\n",
               path);
        return FEM_SUCCESS;
    }

    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[7] = {NULL};
        int field_count = 0;
        int step = -1;
        int pair_id = -1;
        double mu_eff = 0.0;
        double h_min = 0.0;
        uint32_t regime_flag = 0u;
        uint32_t valid_flag = 0u;
        int record_index = -1;
        int source_ok = 0;
        mbd_local_feedback_record2d_t *record = NULL;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (strncmp(line, "step,", 5) == 0) {
            continue;
        }

        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 7);
        if (field_count < 7) {
            continue;
        }
        if (sscanf(fields[0], "%d", &step) != 1 ||
            sscanf(fields[1], "%d", &pair_id) != 1 ||
            sscanf(fields[2], "%lf", &mu_eff) != 1 ||
            sscanf(fields[3], "%lf", &h_min) != 1 ||
            !mbd_system2d_parse_uint32_field(fields[4], &regime_flag) ||
            !mbd_system2d_parse_uint32_field(fields[5], &valid_flag)) {
            continue;
        }
        record_index = mbd_system2d_find_or_create_local_feedback_record_index(system,
                                                                               step,
                                                                               pair_id);
        if (record_index < 0) {
            break;
        }

        record = &system->local_feedback_records[record_index];
        record->reduced_data.mu_eff = mu_eff;
        record->reduced_data.h_min = h_min;
        record->reduced_data.regime_flag = regime_flag;
        if (isfinite(mu_eff)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_MU_EFF;
        }
        if (isfinite(h_min)) {
            record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_H_MIN;
        }
        record->reduced_data.valid_flag |= MBD_CONTACT_FEEDBACK2D_VALID_REGIME_FLAG;
        source_ok = (valid_flag != 0u) && mbd_local_feedback_status_is_ok(fields[6]);
        record->status_ok = record->status_ok && source_ok;
        if (!source_ok) {
            snprintf(record->status, sizeof(record->status), "%s", fields[6]);
        } else if (record->status[0] == '\0') {
            snprintf(record->status, sizeof(record->status), "%s", "ok");
        }
    }

    CHECK_ERROR(mbd_system2d_close_file_checked(&fp,
                                                FEM_ERROR_FILE_READ,
                                                "EHL source file after read",
                                                path));
    return FEM_SUCCESS;
}

static void mbd_system2d_reset_monolithic_proper_runtime(mbd_system2d_t *system)
{
    mbd_monolithic_proper_runtime2d_t previous;
    char artifact_root[1024];

    if (!system) {
        return;
    }

    previous = system->monolithic_proper_runtime;
    snprintf(artifact_root,
             sizeof(artifact_root),
             "%s",
             system->monolithic_proper_runtime.artifact_root);
    memset(&system->monolithic_proper_runtime, 0, sizeof(system->monolithic_proper_runtime));
    system->monolithic_proper_runtime.total_steps = previous.total_steps;
    system->monolithic_proper_runtime.sampled_step_count = previous.sampled_step_count;
    system->monolithic_proper_runtime.active_steps = previous.active_steps;
    system->monolithic_proper_runtime.accepted_steps = previous.accepted_steps;
    system->monolithic_proper_runtime.rejected_steps = previous.rejected_steps;
    system->monolithic_proper_runtime.monolithic_iteration_count_total =
        previous.monolithic_iteration_count_total;
    system->monolithic_proper_runtime.max_iteration_per_step =
        previous.max_iteration_per_step;
    system->monolithic_proper_runtime.converged_steps = previous.converged_steps;
    system->monolithic_proper_runtime.failed_steps = previous.failed_steps;
    system->monolithic_proper_runtime.halving_retry_count_total =
        previous.halving_retry_count_total;
    snprintf(system->monolithic_proper_runtime.artifact_root,
             sizeof(system->monolithic_proper_runtime.artifact_root),
             "%s",
             artifact_root);
    snprintf(system->monolithic_proper_runtime.overall_status,
             sizeof(system->monolithic_proper_runtime.overall_status),
             "%s",
             "not_run");
}

static fem_error_t mbd_system2d_load_monolithic_proper_internal_config_from_env(
    mbd_system2d_t *system)
{
    const char *fem_template_path = NULL;
    const char *mu_mapping_table_path = NULL;
    const char *artifact_root = NULL;

    CHECK_NULL(system, "mbd_system2d");

    memset(&system->monolithic_proper_internal, 0, sizeof(system->monolithic_proper_internal));
    mbd_system2d_reset_monolithic_proper_runtime(system);

    if (system->monolithic_proper_mode == MBD_MONOLITHIC_PROPER_MODE_NONE) {
        return FEM_SUCCESS;
    }

    system->monolithic_proper_internal.enabled =
        parse_env_int_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_INTERNAL",
                                             0,
                                             0,
                                             1,
                                             NULL);
    system->monolithic_proper_internal.fem_call_interval =
        parse_env_int_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_FEM_CALL_INTERVAL",
                                             MBD_MONOLITHIC_PROPER_FEM_CALL_INTERVAL_DEFAULT,
                                             1,
                                             1000000,
                                             NULL);
    system->monolithic_proper_internal.max_iterations =
        parse_env_int_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_MAX_ITERATIONS",
                                             MBD_MONOLITHIC_PROPER_MAX_ITER_DEFAULT,
                                             1,
                                             128,
                                             NULL);
    system->monolithic_proper_internal.max_halving_retries =
        parse_env_int_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_MAX_HALVING_RETRIES",
                                             MBD_MONOLITHIC_PROPER_MAX_HALVING_DEFAULT,
                                             0,
                                             8,
                                             NULL);
    system->monolithic_proper_internal.stress_residual_tol =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_STRESS_RESIDUAL_TOL",
                                                MBD_MONOLITHIC_PROPER_STRESS_RESIDUAL_TOL_DEFAULT,
                                                0.0,
                                                1.0e6,
                                                NULL);
    system->monolithic_proper_internal.displacement_residual_tol =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_DISPLACEMENT_RESIDUAL_TOL",
                                                MBD_MONOLITHIC_PROPER_DISPLACEMENT_RESIDUAL_TOL_DEFAULT,
                                                0.0,
                                                1.0e6,
                                                NULL);
    system->monolithic_proper_internal.contact_parameter_residual_tol =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_CONTACT_PARAMETER_RESIDUAL_TOL",
                                                MBD_MONOLITHIC_PROPER_CONTACT_PARAMETER_RESIDUAL_TOL_DEFAULT,
                                                0.0,
                                                1.0e6,
                                                NULL);
    system->monolithic_proper_internal.delta_min =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_DELTA_MIN",
                                                MBD_MONOLITHIC_PROPER_DELTA_MIN_DEFAULT,
                                                1.0e-18,
                                                1.0,
                                                NULL);
    system->monolithic_proper_internal.macro_area_ref_m2 =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_MACRO_AREA_REF_M2",
                                                MBD_MONOLITHIC_PROPER_MACRO_AREA_REF_DEFAULT,
                                                1.0e-18,
                                                1.0e18,
                                                NULL);
    system->monolithic_proper_internal.k_relaxation =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_K_RELAXATION",
                                                MBD_MONOLITHIC_PROPER_RELAXATION_DEFAULT,
                                                1.0e-6,
                                                1.0,
                                                NULL);
    system->monolithic_proper_internal.mu_relaxation =
        parse_env_double_or_default_with_status("FEM4C_MBD_MONOLITHIC_PROPER_MU_RELAXATION",
                                                MBD_MONOLITHIC_PROPER_RELAXATION_DEFAULT,
                                                1.0e-6,
                                                1.0,
                                                NULL);

    fem_template_path = getenv("FEM4C_MBD_MONOLITHIC_PROPER_FEM_TEMPLATE");
    mu_mapping_table_path = getenv("FEM4C_MBD_MONOLITHIC_PROPER_MU_MAPPING_TABLE");
    artifact_root = getenv("FEM4C_MBD_MONOLITHIC_PROPER_ARTIFACT_ROOT");
    if (fem_template_path && fem_template_path[0] != '\0') {
        snprintf(system->monolithic_proper_internal.fem_template_path,
                 sizeof(system->monolithic_proper_internal.fem_template_path),
                 "%s",
                 fem_template_path);
    }
    if (mu_mapping_table_path && mu_mapping_table_path[0] != '\0') {
        snprintf(system->monolithic_proper_internal.mu_mapping_table_path,
                 sizeof(system->monolithic_proper_internal.mu_mapping_table_path),
                 "%s",
                 mu_mapping_table_path);
    }
    if (artifact_root && artifact_root[0] != '\0') {
        snprintf(system->monolithic_proper_runtime.artifact_root,
                 sizeof(system->monolithic_proper_runtime.artifact_root),
                 "%s",
                 artifact_root);
    }

    if (!system->monolithic_proper_internal.enabled) {
        snprintf(system->monolithic_proper_runtime.overall_status,
                 sizeof(system->monolithic_proper_runtime.overall_status),
                 "%s",
                 "runner_managed");
        return FEM_SUCCESS;
    }
    if (system->monolithic_proper_internal.fem_template_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM4C_MBD_MONOLITHIC_PROPER_FEM_TEMPLATE is required when internal monolithic proper is enabled");
    }
    if (system->monolithic_proper_internal.mu_mapping_table_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM4C_MBD_MONOLITHIC_PROPER_MU_MAPPING_TABLE is required when internal monolithic proper is enabled");
    }
    if (system->monolithic_proper_runtime.artifact_root[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM4C_MBD_MONOLITHIC_PROPER_ARTIFACT_ROOT is required when internal monolithic proper is enabled");
    }
    system->monolithic_proper_runtime.internal_loop_active = 1;
    snprintf(system->monolithic_proper_runtime.overall_status,
             sizeof(system->monolithic_proper_runtime.overall_status),
             "%s",
             "configured");
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_capture_monolithic_proper_step_backup(
    const mbd_system2d_t *system,
    mbd_monolithic_proper_step_backup2d_t *backup)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(backup, "monolithic proper step backup");

    CHECK_ERROR(mbd_monolithic_proper_step_backup_reserve(backup, system->num_bodies));
    backup->num_bodies = system->num_bodies;
    if (system->num_bodies > 0) {
        memcpy(backup->bodies, system->bodies, mbd_system2d_body_bytes(system->num_bodies));
        memcpy(backup->body_states,
               system->body_states,
               (size_t) system->num_bodies * sizeof(*backup->body_states));
        memcpy(backup->current_generalized_force,
               system->current_generalized_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
        memcpy(backup->previous_generalized_force,
               system->previous_generalized_force,
               mbd_system2d_body_force_bytes(system->num_bodies));
    }
    backup->generalized_force_history_valid = system->generalized_force_history_valid;
    backup->time_steps_executed = system->time.steps_executed;
    backup->time_dt = system->time.dt;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_restore_monolithic_proper_step_backup(
    mbd_system2d_t *system,
    const mbd_monolithic_proper_step_backup2d_t *backup)
{
    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(backup, "monolithic proper step backup");

    CHECK_ERROR(mbd_system2d_reserve_body_storage(system, backup->num_bodies));
    system->num_bodies = backup->num_bodies;
    if (backup->num_bodies > 0) {
        memcpy(system->bodies, backup->bodies, mbd_system2d_body_bytes(backup->num_bodies));
        memcpy(system->body_states,
               backup->body_states,
               (size_t) backup->num_bodies * sizeof(*system->body_states));
        memcpy(system->current_generalized_force,
               backup->current_generalized_force,
               mbd_system2d_body_force_bytes(backup->num_bodies));
        memcpy(system->previous_generalized_force,
               backup->previous_generalized_force,
               mbd_system2d_body_force_bytes(backup->num_bodies));
    }
    system->generalized_force_history_valid = backup->generalized_force_history_valid;
    system->time.steps_executed = backup->time_steps_executed;
    system->time.dt = backup->time_dt;
    CHECK_ERROR(mbd_system2d_enforce_ground_bodies(system));
    CHECK_ERROR(mbd_system2d_sync_body_states(system));
    CHECK_ERROR(mbd_system2d_refresh_contact_forces_and_trace(system));
    return FEM_SUCCESS;
}

static void mbd_monolithic_proper_step_backup_release(
    mbd_monolithic_proper_step_backup2d_t *backup)
{
    if (!backup) {
        return;
    }

    free(backup->bodies);
    free(backup->body_states);
    free(backup->current_generalized_force);
    free(backup->previous_generalized_force);
    memset(backup, 0, sizeof(*backup));
}

static fem_error_t mbd_monolithic_proper_step_backup_reserve(
    mbd_monolithic_proper_step_backup2d_t *backup,
    int required_capacity)
{
    mbd_body2d_t *new_bodies = NULL;
    mbd_body_state2d_t *new_body_states = NULL;
    double (*new_current_generalized_force)[MBD_BODY2D_DOF] = NULL;
    double (*new_previous_generalized_force)[MBD_BODY2D_DOF] = NULL;
    int body_index;

    CHECK_NULL(backup, "monolithic proper step backup");

    if (required_capacity < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "monolithic proper backup body capacity %d must be non-negative",
                         required_capacity);
    }
    if (required_capacity <= backup->body_capacity) {
        return FEM_SUCCESS;
    }
    if (required_capacity == 0) {
        return FEM_SUCCESS;
    }

    new_bodies = (mbd_body2d_t *) calloc((size_t) required_capacity, sizeof(*new_bodies));
    new_body_states = (mbd_body_state2d_t *) calloc((size_t) required_capacity,
                                                    sizeof(*new_body_states));
    new_current_generalized_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_current_generalized_force));
    new_previous_generalized_force =
        (double (*)[MBD_BODY2D_DOF]) calloc((size_t) required_capacity,
                                            sizeof(*new_previous_generalized_force));
    if (!new_bodies || !new_body_states || !new_current_generalized_force ||
        !new_previous_generalized_force) {
        free(new_bodies);
        free(new_body_states);
        free(new_current_generalized_force);
        free(new_previous_generalized_force);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate monolithic proper backup for capacity %d",
                         required_capacity);
    }

    if (backup->body_capacity > 0) {
        memcpy(new_bodies, backup->bodies, mbd_system2d_body_bytes(backup->body_capacity));
        memcpy(new_body_states,
               backup->body_states,
               (size_t) backup->body_capacity * sizeof(*new_body_states));
        memcpy(new_current_generalized_force,
               backup->current_generalized_force,
               mbd_system2d_body_force_bytes(backup->body_capacity));
        memcpy(new_previous_generalized_force,
               backup->previous_generalized_force,
               mbd_system2d_body_force_bytes(backup->body_capacity));
    }
    for (body_index = backup->body_capacity; body_index < required_capacity; ++body_index) {
        mbd_body2d_zero(&new_bodies[body_index]);
    }

    free(backup->bodies);
    free(backup->body_states);
    free(backup->current_generalized_force);
    free(backup->previous_generalized_force);
    backup->bodies = new_bodies;
    backup->body_states = new_body_states;
    backup->current_generalized_force = new_current_generalized_force;
    backup->previous_generalized_force = new_previous_generalized_force;
    backup->body_capacity = required_capacity;
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_set_monolithic_proper_generic_contact_params(
    mbd_system2d_t *system,
    double k_contact_eff,
    double mu_eff)
{
    int pair_index = 0;

    CHECK_NULL(system, "mbd_system2d");
    if (!isfinite(k_contact_eff) || k_contact_eff <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid monolithic proper k_contact_eff %.16e",
                         k_contact_eff);
    }
    if (!isfinite(mu_eff) || mu_eff < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid monolithic proper mu_eff %.16e",
                         mu_eff);
    }

    for (pair_index = 0; pair_index < system->num_generic_contact_pairs; ++pair_index) {
        mbd_contact_generic_pair2d_t *pair = &system->generic_contact_pairs[pair_index];
        if (!pair->is_defined) {
            continue;
        }
        pair->base_k_n = k_contact_eff;
        pair->mu_static = mu_eff;
        pair->mu_dynamic = mu_eff;
    }
    return FEM_SUCCESS;
}

static int mbd_system2d_find_monolithic_proper_representative_row(
    const mbd_system2d_t *system,
    int *pair_id_out,
    double *penetration_out)
{
    int row_index = 0;
    int best_row = -1;
    double best_penetration = 0.0;

    if (pair_id_out) {
        *pair_id_out = -1;
    }
    if (penetration_out) {
        *penetration_out = 0.0;
    }
    if (!system) {
        return 0;
    }

    for (row_index = 0; row_index < system->num_current_generic_contact_trace_rows; ++row_index) {
        const mbd_contact_generic_trace2d_t *row = &system->current_generic_contact_trace_rows[row_index];
        if (!row->active) {
            continue;
        }
        if (best_row < 0 || row->penetration > best_penetration) {
            best_row = row_index;
            best_penetration = row->penetration;
        }
    }

    if (best_row < 0) {
        return 0;
    }
    if (pair_id_out) {
        *pair_id_out = system->current_generic_contact_trace_rows[best_row].pair_id;
    }
    if (penetration_out) {
        *penetration_out = system->current_generic_contact_trace_rows[best_row].penetration;
    }
    return 1;
}

static fem_error_t mbd_system2d_copy_file_bytes(const char *src_path,
                                                const char *dst_path)
{
    FILE *src = NULL;
    FILE *dst = NULL;
    char buffer[4096];
    char dst_parent[1024];
    char *slash = NULL;
    size_t nread = 0;

    CHECK_NULL(src_path, "source path");
    CHECK_NULL(dst_path, "destination path");
    snprintf(dst_parent, sizeof(dst_parent), "%s", dst_path);
    slash = strrchr(dst_parent, '/');
    if (slash) {
        *slash = '\0';
        if (dst_parent[0] != '\0') {
            CHECK_ERROR(mbd_system2d_make_parent_dirs(dst_parent));
        }
    }

    src = fopen(src_path, "rb");
    if (!src) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open source file for copy: %s",
                         src_path);
    }
    dst = fopen(dst_path, "wb");
    if (!dst) {
        fclose(src);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open destination file for copy: %s",
                         dst_path);
    }

    while ((nread = fread(buffer, 1u, sizeof(buffer), src)) > 0u) {
        if (fwrite(buffer, 1u, nread, dst) != nread) {
            fclose(src);
            fclose(dst);
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed while copying %s to %s",
                             src_path,
                             dst_path);
        }
    }
    if (ferror(src)) {
        fclose(src);
        fclose(dst);
        return error_set(FEM_ERROR_FILE_READ,
                         "Failed while reading %s",
                         src_path);
    }
    fclose(src);
    fclose(dst);
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_copy_template_support_csvs(
    const char *template_path,
    const char *target_dir)
{
    char template_dir[1024];
    char src_path[1024];
    char dst_path[1024];
    char *slash = NULL;
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    CHECK_NULL(template_path, "monolithic proper FEM template path");
    CHECK_NULL(target_dir, "monolithic proper target dir");

    snprintf(template_dir, sizeof(template_dir), "%s", template_path);
    slash = strrchr(template_dir, '/');
    if (slash) {
        *slash = '\0';
    } else {
        snprintf(template_dir, sizeof(template_dir), "%s", ".");
    }

    dir = opendir(template_dir);
    if (!dir) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open FEM template directory: %s",
                         template_dir);
    }
    while ((entry = readdir(dir)) != NULL) {
        size_t name_len = strlen(entry->d_name);
        if (name_len < 4u) {
            continue;
        }
        if (strcmp(entry->d_name + name_len - 4u, ".csv") != 0) {
            continue;
        }
        if (snprintf(src_path, sizeof(src_path), "%s/%s", template_dir, entry->d_name) >=
            (int)sizeof(src_path) ||
            snprintf(dst_path, sizeof(dst_path), "%s/%s", target_dir, entry->d_name) >=
                (int)sizeof(dst_path)) {
            closedir(dir);
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Monolithic proper CSV support path too long");
        }
        CHECK_ERROR(mbd_system2d_copy_file_bytes(src_path, dst_path));
    }
    closedir(dir);
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_generate_monolithic_proper_fem_input(
    const mbd_system2d_t *system,
    double delta_eq_m,
    int step,
    int iter_index,
    int halving_retry_count,
    char generated_deck_path[1024],
    char fem_output_path[1024],
    char fem_trace_path[1024],
    char artifact_dir[1024])
{
    FILE *template_fp = NULL;
    FILE *generated_fp = NULL;
    char line[4096];
    const char *template_path = NULL;
    const char *placeholder = "__PRESCRIBED_UY_M__";
    char replacement[128];

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(generated_deck_path, "generated FEM deck path");
    CHECK_NULL(fem_output_path, "FEM output path");
    CHECK_NULL(fem_trace_path, "FEM trace path");
    CHECK_NULL(artifact_dir, "artifact dir path");

    if (delta_eq_m < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Monolithic proper delta_eq_m must be >= 0");
    }

    template_path = system->monolithic_proper_internal.fem_template_path;
    if (snprintf(artifact_dir,
                 1024,
                 "%s/step%06d/iter%02d_retry%02d",
                 system->monolithic_proper_runtime.artifact_root,
                 step,
                 iter_index,
                 halving_retry_count) >=
        1024) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Monolithic proper artifact dir path too long");
    }
    CHECK_ERROR(mbd_system2d_make_parent_dirs(artifact_dir));
    if (mkdir(artifact_dir, 0777) != 0 && errno != EEXIST) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot create monolithic proper artifact dir: %s",
                         artifact_dir);
    }
    CHECK_ERROR(mbd_system2d_copy_template_support_csvs(template_path, artifact_dir));

    if (snprintf(generated_deck_path, 1024, "%s/generated_fem_step.dat", artifact_dir) >= 1024 ||
        snprintf(fem_output_path, 1024, "%s/fem_main_output.out", artifact_dir) >= 1024 ||
        snprintf(fem_trace_path,
                 1024,
                 "%s/fem_main_output.out.fem_contact_generic_trace.csv",
                 artifact_dir) >= 1024) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Monolithic proper FEM artifact path too long");
    }

    template_fp = fopen(template_path, "r");
    if (!template_fp) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open monolithic proper FEM template: %s",
                         template_path);
    }
    generated_fp = fopen(generated_deck_path, "w");
    if (!generated_fp) {
        fclose(template_fp);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open generated monolithic proper FEM deck: %s",
                         generated_deck_path);
    }

    snprintf(replacement, sizeof(replacement), "%.16e", -delta_eq_m);
    while (fgets(line, sizeof(line), template_fp)) {
        char *cursor = line;
        char *match = NULL;
        while ((match = strstr(cursor, placeholder)) != NULL) {
            *match = '\0';
            if (fputs(cursor, generated_fp) == EOF ||
                fputs(replacement, generated_fp) == EOF) {
                fclose(template_fp);
                fclose(generated_fp);
                return error_set(FEM_ERROR_FILE_WRITE,
                                 "Failed to write generated monolithic proper FEM deck: %s",
                                 generated_deck_path);
            }
            cursor = match + strlen(placeholder);
        }
        if (fputs(cursor, generated_fp) == EOF) {
            fclose(template_fp);
            fclose(generated_fp);
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write generated monolithic proper FEM deck: %s",
                             generated_deck_path);
        }
    }
    fclose(template_fp);
    fclose(generated_fp);

    (void)step;
    return FEM_SUCCESS;
}

static int mbd_system2d_compare_mu_mapping_points(const void *lhs,
                                                  const void *rhs)
{
    const mbd_monolithic_mu_mapping_point_t *lhs_point =
        (const mbd_monolithic_mu_mapping_point_t *)lhs;
    const mbd_monolithic_mu_mapping_point_t *rhs_point =
        (const mbd_monolithic_mu_mapping_point_t *)rhs;

    if (lhs_point->pressure_surrogate_pa < rhs_point->pressure_surrogate_pa) {
        return -1;
    }
    if (lhs_point->pressure_surrogate_pa > rhs_point->pressure_surrogate_pa) {
        return 1;
    }
    return 0;
}

static fem_error_t mbd_system2d_load_mu_eff_mapping_points(
    const char *path,
    mbd_monolithic_mu_mapping_point_t *points,
    int max_points,
    int *point_count_out)
{
    FILE *fp = NULL;
    char line[512];
    int point_count = 0;
    int header_seen = 0;
    int row_number = 0;

    CHECK_NULL(path, "mu mapping path");
    CHECK_NULL(points, "mu mapping points");
    CHECK_NULL(point_count_out, "mu mapping point count");

    fp = fopen(path, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open monolithic proper mu mapping table: %s",
                         path);
    }
    while (fgets(line, sizeof(line), fp)) {
        char row_copy[512];
        char *fields[8] = {NULL};
        int field_count = 0;
        double pressure = 0.0;
        double mu_eff = 0.0;

        row_number += 1;
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        if (!header_seen) {
            header_seen = 1;
            if (strstr(line, "pressure_surrogate_pa") != NULL) {
                continue;
            }
        }
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 8);
        if (field_count < 2) {
            continue;
        }
        if (sscanf(fields[0], "%lf", &pressure) != 1 ||
            sscanf(fields[1], "%lf", &mu_eff) != 1) {
            continue;
        }
        if (pressure < 0.0 || mu_eff < 0.0) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Monolithic proper mu mapping row %d must have nonnegative pressure_surrogate_pa and mu_eff: %s",
                             row_number,
                             path);
        }
        if (point_count >= max_points) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Monolithic proper mu mapping table exceeds max point count %d",
                             max_points);
        }
        points[point_count].pressure_surrogate_pa = pressure;
        points[point_count].mu_eff = mu_eff;
        point_count += 1;
    }
    fclose(fp);
    if (point_count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Monolithic proper mu mapping table is empty: %s",
                         path);
    }
    if (point_count > 1) {
        qsort(points,
              (size_t)point_count,
              sizeof(points[0]),
              mbd_system2d_compare_mu_mapping_points);
    }
    *point_count_out = point_count;
    return FEM_SUCCESS;
}

static double mbd_system2d_monolithic_relative_residual(double numerator,
                                                        double denominator,
                                                        double floor_value)
{
    return fabs(numerator) / fmax(fabs(denominator), floor_value);
}

static double mbd_system2d_interpolate_mu_eff_mapping(
    const mbd_monolithic_mu_mapping_point_t *points,
    int point_count,
    double pressure_surrogate_pa)
{
    int point_index = 0;

    if (!points || point_count <= 0) {
        return 0.0;
    }
    if (pressure_surrogate_pa <= points[0].pressure_surrogate_pa) {
        return points[0].mu_eff;
    }
    if (pressure_surrogate_pa >= points[point_count - 1].pressure_surrogate_pa) {
        return points[point_count - 1].mu_eff;
    }
    for (point_index = 1; point_index < point_count; ++point_index) {
        double p0 = points[point_index - 1].pressure_surrogate_pa;
        double p1 = points[point_index].pressure_surrogate_pa;
        double mu0 = points[point_index - 1].mu_eff;
        double mu1 = points[point_index].mu_eff;
        if (p0 <= pressure_surrogate_pa && pressure_surrogate_pa <= p1) {
            double t = 0.0;
            if (fabs(p1 - p0) <= 1.0e-18) {
                return mu1;
            }
            t = (pressure_surrogate_pa - p0) / (p1 - p0);
            return mu0 + t * (mu1 - mu0);
        }
    }
    return points[point_count - 1].mu_eff;
}

static fem_error_t mbd_system2d_compute_monolithic_proper_feedback_from_fem(
    const mbd_system2d_t *system,
    const char *fem_output_path,
    const char *fem_trace_path,
    int representative_pair_id,
    double representative_penetration_m,
    mbd_monolithic_proper_fem_feedback2d_t *feedback_out)
{
    FILE *fp = NULL;
    char line[2048];
    int active_idx = -1;
    int pair_idx = -1;
    int penetration_idx = -1;
    int fn_macro_idx = -1;
    int point_count = 0;
    mbd_monolithic_mu_mapping_point_t
        points[MBD_MONOLITHIC_PROPER_MAX_MU_MAPPING_POINTS];

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(fem_output_path, "monolithic proper FEM output path");
    CHECK_NULL(fem_trace_path, "monolithic proper FEM trace path");
    CHECK_NULL(feedback_out, "monolithic proper feedback output");

    memset(feedback_out, 0, sizeof(*feedback_out));
    snprintf(feedback_out->fem_output_path, sizeof(feedback_out->fem_output_path), "%s", fem_output_path);
    snprintf(feedback_out->fem_trace_path, sizeof(feedback_out->fem_trace_path), "%s", fem_trace_path);
    feedback_out->pair_id = representative_pair_id;
    feedback_out->delta_eq_m = representative_penetration_m;

    CHECK_ERROR(mbd_system2d_load_mu_eff_mapping_points(
        system->monolithic_proper_internal.mu_mapping_table_path,
        points,
        MBD_MONOLITHIC_PROPER_MAX_MU_MAPPING_POINTS,
        &point_count));

    fp = fopen(fem_trace_path, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open monolithic proper FEM trace: %s",
                         fem_trace_path);
    }
    while (fgets(line, sizeof(line), fp)) {
        char row_copy[2048];
        char *fields[32] = {NULL};
        int field_count = 0;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
            continue;
        }
        snprintf(row_copy, sizeof(row_copy), "%s", line);
        field_count = mbd_system2d_parse_csv_fields(row_copy, fields, 32);
        if (field_count <= 0) {
            continue;
        }
        if (strcmp(fields[0], "pair_id") == 0) {
            int field_index = 0;
            for (field_index = 0; field_index < field_count; ++field_index) {
                if (strcmp(fields[field_index], "active_flag") == 0) {
                    active_idx = field_index;
                } else if (strcmp(fields[field_index], "pair_id") == 0) {
                    pair_idx = field_index;
                } else if (strcmp(fields[field_index], "penetration_m") == 0) {
                    penetration_idx = field_index;
                } else if (strcmp(fields[field_index], "fn_macro_n") == 0) {
                    fn_macro_idx = field_index;
                }
            }
            continue;
        }
        if (active_idx < 0 || pair_idx < 0 || penetration_idx < 0 || fn_macro_idx < 0 ||
            field_count <= fn_macro_idx) {
            continue;
        }
        if (atoi(fields[active_idx]) == 1) {
            double penetration_m = atof(fields[penetration_idx]);
            double fn_macro_n = atof(fields[fn_macro_idx]);
            feedback_out->active_flag = 1;
            feedback_out->pair_id = atoi(fields[pair_idx]);
            feedback_out->fn_total_n += fn_macro_n;
            if (penetration_m > feedback_out->delta_eq_m) {
                feedback_out->delta_eq_m = penetration_m;
            }
        }
    }
    fclose(fp);

    if (!feedback_out->active_flag) {
        return FEM_SUCCESS;
    }
    feedback_out->pressure_surrogate_pa =
        feedback_out->fn_total_n /
        fmax(system->monolithic_proper_internal.macro_area_ref_m2,
             system->monolithic_proper_internal.delta_min);
    feedback_out->k_contact_eff =
        feedback_out->fn_total_n /
        fmax(feedback_out->delta_eq_m, system->monolithic_proper_internal.delta_min);
    feedback_out->mu_eff = mbd_system2d_interpolate_mu_eff_mapping(
        points,
        point_count,
        feedback_out->pressure_surrogate_pa);
    return FEM_SUCCESS;
}

static int mbd_system2d_monolithic_proper_step_is_sampled(
    const mbd_system2d_t *system,
    int step)
{
    int interval = 1;

    if (!system) {
        return 0;
    }
    interval = system->monolithic_proper_internal.fem_call_interval > 0
                   ? system->monolithic_proper_internal.fem_call_interval
                   : 1;
    if (step <= 0) {
        return 0;
    }
    return ((step - 1) % interval) == 0 ? 1 : 0;
}

static fem_error_t mbd_output2d_write_monolithic_proper_iteration_detail_row(
    FILE *monolithic_proper_iteration_out,
    const mbd_system2d_t *system,
    int step,
    double time,
    int halving_retry_count,
    int feedback_available_flag,
    int contact_active_flag,
    int representative_pair_id,
    double representative_penetration_m,
    double fn_total_n,
    double delta_eq_m,
    double pressure_surrogate_pa,
    double k_iter_used_in_mbd,
    double mu_iter_used_in_mbd,
    const char *artifact_dir,
    const char *overall_status)
{
    double mbd_constraint_residual_l2 = 0.0;
    int num_equations = 0;
    const mbd_monolithic_proper_context2d_t *context = NULL;

    if (!monolithic_proper_iteration_out) {
        return FEM_SUCCESS;
    }
    CHECK_NULL(system, "mbd monolithic proper system");
    context = &system->monolithic_proper_context;
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &mbd_constraint_residual_l2,
                                                            &num_equations));
    (void)num_equations;
    if (fprintf(monolithic_proper_iteration_out,
                "%d,%.16e,%d,%d,%s,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%d,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%s\n",
                step,
                time,
                context->is_defined ? context->context_step : step,
                context->is_defined ? context->iter_index : 0,
                mbd_monolithic_proper_mode_to_string(system->monolithic_proper_mode),
                context->is_defined ? context->k_contact_eff : 0.0,
                context->is_defined ? context->mu_eff : 0.0,
                context->is_defined ? context->stress_residual : 0.0,
                context->is_defined ? context->displacement_residual : 0.0,
                context->is_defined ? context->contact_parameter_residual : 0.0,
                context->is_defined ? context->fem_residual : 0.0,
                mbd_constraint_residual_l2,
                context->is_defined ? context->converged_flag : 0,
                halving_retry_count,
                feedback_available_flag,
                contact_active_flag,
                representative_pair_id,
                representative_penetration_m,
                fn_total_n,
                delta_eq_m,
                pressure_surrogate_pa,
                k_iter_used_in_mbd,
                mu_iter_used_in_mbd,
                artifact_dir ? artifact_dir : "",
                overall_status ? overall_status : "") < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write monolithic proper iteration detail row");
    }
    return FEM_SUCCESS;
}

static fem_error_t mbd_system2d_do_monolithic_proper_explicit_step(
    mbd_system2d_t *system,
    int step,
    double time,
    FILE *monolithic_proper_iteration_out,
    int *iteration_row_count_out)
{
    mbd_monolithic_proper_step_backup2d_t backup;
    mbd_monolithic_proper_fem_feedback2d_t previous_feedback;
    int previous_feedback_available = 0;
    double k_iter = 0.0;
    double mu_iter = 0.0;
    double base_dt = 0.0;
    int halving_retry_count = 0;
    fem_error_t err = FEM_SUCCESS;

    memset(&backup, 0, sizeof(backup));

    CHECK_NULL(system, "mbd_system2d");
    if (system->time.integrator != MBD_INTEGRATOR2D_EXPLICIT) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Internal monolithic proper loop currently supports explicit MBD only");
    }

    mbd_system2d_reset_monolithic_proper_runtime(system);
    system->monolithic_proper_runtime.internal_loop_active = 1;
    base_dt = system->time.dt;
    CHECK_ERROR(mbd_system2d_capture_monolithic_proper_step_backup(system, &backup));

    if (system->num_generic_contact_pairs <= 0) {
        err = error_set(FEM_ERROR_INVALID_INPUT,
                        "Internal monolithic proper loop requires at least one generic contact pair");
        goto cleanup;
    }
    k_iter = system->monolithic_proper_context.is_defined &&
                     system->monolithic_proper_context.k_contact_eff > 0.0
                 ? system->monolithic_proper_context.k_contact_eff
                 : system->generic_contact_pairs[0].base_k_n;
    mu_iter = system->monolithic_proper_context.is_defined &&
                      system->monolithic_proper_context.mu_eff >= 0.0
                  ? system->monolithic_proper_context.mu_eff
                  : system->generic_contact_pairs[0].mu_dynamic;
    memset(&previous_feedback, 0, sizeof(previous_feedback));

    for (halving_retry_count = 0;
         halving_retry_count <= system->monolithic_proper_internal.max_halving_retries;
         ++halving_retry_count) {
        int iter_index = 0;
        int substep_count = 1 << halving_retry_count;
        previous_feedback_available = 0;
        memset(&previous_feedback, 0, sizeof(previous_feedback));

        for (iter_index = 0; iter_index < system->monolithic_proper_internal.max_iterations;
             ++iter_index) {
            int substep = 0;
            int representative_pair_id = -1;
            double representative_penetration_m = 0.0;
            int contact_active_flag = 0;
            int feedback_available_flag = 0;
            double stress_residual = 0.0;
            double displacement_residual = 0.0;
            double contact_parameter_residual = 0.0;
            double fem_residual = 0.0;
            double current_k_used = k_iter;
            double current_mu_used = mu_iter;
            const char *status_text = "active_update";
            char generated_deck_path[1024] = {0};
            char fem_output_path[1024] = {0};
            char fem_trace_path[1024] = {0};
            char artifact_dir[1024] = {0};
            mbd_monolithic_proper_fem_feedback2d_t feedback;

            memset(&feedback, 0, sizeof(feedback));
            CHECK_ERROR(mbd_system2d_restore_monolithic_proper_step_backup(system, &backup));
            system->time.dt = base_dt / (double)substep_count;
            CHECK_ERROR(mbd_system2d_set_monolithic_proper_generic_contact_params(system,
                                                                                  current_k_used,
                                                                                  current_mu_used));
            for (substep = 0; substep < substep_count; ++substep) {
                CHECK_ERROR(mbd_system2d_do_explicit_step(system));
            }

            contact_active_flag = mbd_system2d_find_monolithic_proper_representative_row(
                system,
                &representative_pair_id,
                &representative_penetration_m);
            system->monolithic_proper_context.is_defined = 1;
            system->monolithic_proper_context.context_step = step;
            system->monolithic_proper_context.iter_index = iter_index;

            if (!contact_active_flag) {
                system->monolithic_proper_context.k_contact_eff = current_k_used;
                system->monolithic_proper_context.mu_eff = current_mu_used;
                system->monolithic_proper_context.stress_residual = 0.0;
                system->monolithic_proper_context.displacement_residual = 0.0;
                system->monolithic_proper_context.contact_parameter_residual = 0.0;
                system->monolithic_proper_context.fem_residual = 0.0;
                system->monolithic_proper_context.converged_flag = 1;
                system->monolithic_proper_runtime.monolithic_iteration_count = iter_index + 1;
                system->monolithic_proper_runtime.halving_retry_count = halving_retry_count;
                system->monolithic_proper_runtime.contact_active_flag = 0;
                system->monolithic_proper_runtime.feedback_available_flag = 0;
                system->monolithic_proper_runtime.converged_flag = 1;
                snprintf(system->monolithic_proper_runtime.overall_status,
                         sizeof(system->monolithic_proper_runtime.overall_status),
                         "%s",
                         "no_active_contact");
                CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_detail_row(
                    monolithic_proper_iteration_out,
                    system,
                    step,
                    time,
                    halving_retry_count,
                    0,
                    0,
                    representative_pair_id,
                    representative_penetration_m,
                    0.0,
                    0.0,
                    0.0,
                    current_k_used,
                    current_mu_used,
                    "",
                    "no_active_contact"));
                if (iteration_row_count_out) {
                    *iteration_row_count_out += 1;
                }
                system->time.dt = base_dt;
                err = FEM_SUCCESS;
                goto cleanup;
            }

            CHECK_ERROR(mbd_system2d_generate_monolithic_proper_fem_input(system,
                                                                          representative_penetration_m,
                                                                          step,
                                                                          iter_index,
                                                                          halving_retry_count,
                                                                          generated_deck_path,
                                                                          fem_output_path,
                                                                          fem_trace_path,
                                                                          artifact_dir));
            CHECK_ERROR(static_analysis(generated_deck_path, fem_output_path));
            CHECK_ERROR(mbd_system2d_compute_monolithic_proper_feedback_from_fem(
                system,
                fem_output_path,
                fem_trace_path,
                representative_pair_id,
                representative_penetration_m,
                &feedback));

            feedback_available_flag = feedback.active_flag;
            if (feedback_available_flag) {
                if (previous_feedback_available) {
                    stress_residual = mbd_system2d_monolithic_relative_residual(
                        feedback.fn_total_n - previous_feedback.fn_total_n,
                        feedback.fn_total_n,
                        1.0);
                    displacement_residual = mbd_system2d_monolithic_relative_residual(
                        feedback.delta_eq_m - previous_feedback.delta_eq_m,
                        feedback.delta_eq_m,
                        system->monolithic_proper_internal.delta_min);
                }
                contact_parameter_residual = fmax(
                    mbd_system2d_monolithic_relative_residual(
                        feedback.k_contact_eff - current_k_used,
                        feedback.k_contact_eff,
                        system->monolithic_proper_internal.delta_min),
                    mbd_system2d_monolithic_relative_residual(
                        feedback.mu_eff - current_mu_used,
                        feedback.mu_eff,
                        1.0e-6));
                fem_residual = fmax(stress_residual, displacement_residual);
            } else {
                status_text = "fem_trace_no_active_contact";
            }

            system->monolithic_proper_context.k_contact_eff =
                feedback_available_flag ? feedback.k_contact_eff : current_k_used;
            system->monolithic_proper_context.mu_eff =
                feedback_available_flag ? feedback.mu_eff : current_mu_used;
            system->monolithic_proper_context.stress_residual = stress_residual;
            system->monolithic_proper_context.displacement_residual = displacement_residual;
            system->monolithic_proper_context.contact_parameter_residual =
                contact_parameter_residual;
            system->monolithic_proper_context.fem_residual = fem_residual;
            system->monolithic_proper_context.converged_flag =
                previous_feedback_available &&
                feedback_available_flag &&
                stress_residual <= system->monolithic_proper_internal.stress_residual_tol &&
                displacement_residual <=
                    system->monolithic_proper_internal.displacement_residual_tol &&
                contact_parameter_residual <=
                    system->monolithic_proper_internal.contact_parameter_residual_tol;

            system->monolithic_proper_runtime.monolithic_iteration_count = iter_index + 1;
            system->monolithic_proper_runtime.halving_retry_count = halving_retry_count;
            system->monolithic_proper_runtime.feedback_available_flag = feedback_available_flag;
            system->monolithic_proper_runtime.contact_active_flag = 1;
            system->monolithic_proper_runtime.converged_flag =
                system->monolithic_proper_context.converged_flag;
            system->monolithic_proper_runtime.stress_residual = stress_residual;
            system->monolithic_proper_runtime.displacement_residual = displacement_residual;
            system->monolithic_proper_runtime.contact_parameter_residual =
                contact_parameter_residual;
            system->monolithic_proper_runtime.fem_residual = fem_residual;
            system->monolithic_proper_runtime.k_contact_eff =
                system->monolithic_proper_context.k_contact_eff;
            system->monolithic_proper_runtime.mu_eff =
                system->monolithic_proper_context.mu_eff;
            system->monolithic_proper_runtime.fn_total_n = feedback.fn_total_n;
            system->monolithic_proper_runtime.delta_eq_m = feedback.delta_eq_m;
            system->monolithic_proper_runtime.pressure_surrogate_pa =
                feedback.pressure_surrogate_pa;
            system->monolithic_proper_runtime.representative_pair_id =
                representative_pair_id;
            system->monolithic_proper_runtime.representative_penetration_m =
                representative_penetration_m;

            if (system->monolithic_proper_context.converged_flag) {
                status_text = "converged";
            }
            snprintf(system->monolithic_proper_runtime.overall_status,
                     sizeof(system->monolithic_proper_runtime.overall_status),
                     "%s",
                     status_text);
            CHECK_ERROR(mbd_output2d_write_monolithic_proper_iteration_detail_row(
                monolithic_proper_iteration_out,
                system,
                step,
                time,
                halving_retry_count,
                feedback_available_flag,
                1,
                representative_pair_id,
                representative_penetration_m,
                feedback.fn_total_n,
                feedback.delta_eq_m,
                feedback.pressure_surrogate_pa,
                current_k_used,
                current_mu_used,
                artifact_dir,
                status_text));
            if (iteration_row_count_out) {
                *iteration_row_count_out += 1;
            }

            if (system->monolithic_proper_context.converged_flag) {
                system->time.dt = base_dt;
                err = FEM_SUCCESS;
                goto cleanup;
            }
            if (!feedback_available_flag) {
                break;
            }
            previous_feedback = feedback;
            previous_feedback_available = 1;
            k_iter = (1.0 - system->monolithic_proper_internal.k_relaxation) * current_k_used +
                     system->monolithic_proper_internal.k_relaxation * feedback.k_contact_eff;
            mu_iter = (1.0 - system->monolithic_proper_internal.mu_relaxation) * current_mu_used +
                      system->monolithic_proper_internal.mu_relaxation * feedback.mu_eff;
        }
    }

    system->time.dt = base_dt;
    system->monolithic_proper_runtime.failed_steps += 1;
    system->monolithic_proper_runtime.rejected_steps += halving_retry_count;
    snprintf(system->monolithic_proper_runtime.overall_status,
             sizeof(system->monolithic_proper_runtime.overall_status),
             "%s",
             "failed_nonconverged");
    err = error_set(FEM_ERROR_INVALID_INPUT,
                    "Monolithic proper internal loop did not converge at step %d",
                    step);

cleanup:
    mbd_monolithic_proper_step_backup_release(&backup);
    return err;
}

static const mbd_local_feedback_record2d_t *mbd_system2d_find_local_feedback_record(
    const mbd_system2d_t *system,
    int step,
    int pair_id)
{
    int i;

    if (!system) {
        return NULL;
    }

    for (i = 0; i < system->num_local_feedback_records; ++i) {
        const mbd_local_feedback_record2d_t *record = &system->local_feedback_records[i];

        if (record->is_defined &&
            record->step == step &&
            record->pair_id == pair_id) {
            return record;
        }
    }

    return NULL;
}

static int mbd_local_feedback_status_is_ok(const char *status)
{
    if (!status || status[0] == '\0') {
        return 0;
    }
    return string_equals_ignore_case(status, "ok") ||
           string_equals_ignore_case(status, "valid") ||
           strcmp(status, "1") == 0;
}

static double mbd_local_feedback_clip(double value, double min_value, double max_value)
{
    if (!isfinite(value)) {
        return min_value;
    }
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}
