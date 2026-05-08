#ifndef FEM4C_MBD_SYSTEM2D_H
#define FEM4C_MBD_SYSTEM2D_H

#include "kernel/body2d.h"
#include "../contact/kernel/contact2d.h"
#include "kernel/kkt2d.h"

#define MBD_SYSTEM2D_MAX_BODIES 8
#define MBD_SYSTEM2D_MAX_CONSTRAINTS 8
#define MBD_GENERIC_CONTACT2D_MAX_SURFACES 16
#define MBD_GENERIC_CONTACT2D_MAX_PAIRS 8
#define MBD_GENERIC_CONTACT2D_MAX_SURFACE_POINTS 128
#define MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS \
    (MBD_GENERIC_CONTACT2D_MAX_PAIRS * MBD_GENERIC_CONTACT2D_MAX_SURFACE_POINTS)

typedef enum {
    MBD_INTEGRATOR2D_EXPLICIT = 0,
    MBD_INTEGRATOR2D_NEWMARK_BETA = 1,
    MBD_INTEGRATOR2D_HHT_ALPHA = 2
} mbd_integrator2d_t;

typedef enum {
    MBD_CONTACT_COUPLING_MODE_ONE_WAY = 0,
    MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS = 1
} mbd_contact_coupling_mode_t;

typedef enum {
    MBD_LOCAL_FEEDBACK_MODE_NONE = 0,
    MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED = 1,
    MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED = 2
} mbd_local_feedback_mode_t;

typedef enum {
    MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE = 0,
    MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE = 1
} mbd_local_contact_monolithic_mode_t;

typedef enum {
    MBD_MONOLITHIC_PROPER_MODE_NONE = 0,
    MBD_MONOLITHIC_PROPER_MODE_BLOCK_NEWTON_V1 = 1
} mbd_monolithic_proper_mode_t;

typedef struct {
    int is_defined;
    int context_step;
    int iter_index;
    double k_contact_eff;
    double mu_eff;
    double stress_residual;
    double displacement_residual;
    double contact_parameter_residual;
    double fem_residual;
    int converged_flag;
} mbd_monolithic_proper_context2d_t;

typedef struct {
    int enabled;
    char fem_template_path[1024];
    char mu_mapping_table_path[1024];
    int fem_call_interval;
    int max_iterations;
    int max_halving_retries;
    double stress_residual_tol;
    double displacement_residual_tol;
    double contact_parameter_residual_tol;
    double delta_min;
    double macro_area_ref_m2;
    double k_relaxation;
    double mu_relaxation;
} mbd_monolithic_proper_internal_config2d_t;

typedef struct {
    int internal_loop_active;
    int total_steps;
    int sampled_step_count;
    int active_steps;
    int accepted_steps;
    int rejected_steps;
    int monolithic_iteration_count_total;
    int max_iteration_per_step;
    int converged_steps;
    int failed_steps;
    int monolithic_iteration_count;
    int halving_retry_count;
    int halving_retry_count_total;
    int feedback_available_flag;
    int contact_active_flag;
    int converged_flag;
    double stress_residual;
    double displacement_residual;
    double contact_parameter_residual;
    double fem_residual;
    double k_contact_eff;
    double mu_eff;
    double fn_total_n;
    double delta_eq_m;
    double pressure_surrogate_pa;
    int representative_pair_id;
    double representative_penetration_m;
    char overall_status[32];
    char artifact_root[1024];
} mbd_monolithic_proper_runtime2d_t;

typedef struct {
    int is_defined;
    int step;
    int iter;
    int pair_id;
    double mu_guess;
    double mu_new;
    double gamma_n_guess;
    double gamma_n_new;
    double fn;
    double gap;
    double vt;
    char stop_reason[32];
} mbd_same_time_reduced_iteration2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int active;
    double gap;
    double penetration;
    mbd_contact_feedback2d_t reduced_data;
    int status_ok;
    char source_mode[32];
    char fallback_reason[64];
    char status[32];
    char diagnostic_json[MAX_FILENAME_LEN];
    char metadata_small_json[MAX_FILENAME_LEN];
    char metadata_large_json[MAX_FILENAME_LEN];
    char receiver_small_json[MAX_FILENAME_LEN];
    char receiver_large_json[MAX_FILENAME_LEN];
} mbd_monolithic_local_patch_trace2d_t;

typedef struct {
    double dt;
    int num_steps;
    int steps_requested;
    int steps_executed;
    mbd_integrator2d_t integrator;
    const char *integrator_source_status;
    double newmark_beta;
    double newmark_gamma;
    double hht_alpha;
    int implicit_max_iterations;
    int implicit_iterations_last;
    const char *dt_source_status;
    const char *steps_source_status;
    const char *newmark_beta_source_status;
    const char *newmark_gamma_source_status;
    const char *hht_alpha_source_status;
    const char *implicit_max_iterations_source_status;
} mbd_time_control2d_t;

typedef struct {
    int is_defined;
    int surface_id;
    int body_id;
    char csv_path[1024];
} mbd_contact_surface_polyline2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int surface_i;
    int surface_j;
    double base_k_n;
    double c_n;
    double base_mu;
    double mu_static;
    double mu_dynamic;
    double v_ref;
    double v_smooth;
} mbd_contact_generic_pair2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int surface_i;
    int surface_j;
    int slave_body_id;
    int master_body_id;
    int slave_vertex_index;
    int master_segment_index;
    int active;
    double gap;
    double penetration;
    double fn;
    double k_used;
    double contact_point[2];
    double closest_point[2];
    double slave_point[2];
    double normal[2];
    double tangent[2];
    double v_t;
    double mu_used;
    double ft_tangent;
    double friction_force[2];
    char request_mode_hint[32];
    char source_mode[32];
} mbd_contact_generic_trace2d_t;

typedef struct {
    mbd_body2d_t *bodies;
    mbd_body_state2d_t *body_states;
    int num_bodies;
    int body_capacity;
    mbd_constraint2d_t *constraints;
    int num_constraints;
    int constraint_capacity;
    double gravity[2];
    int has_gravity;
    double (*flexible_force)[MBD_BODY2D_DOF];
    double (*contact_force)[MBD_BODY2D_DOF];
    mbd_contact_circle2d_t contact_circles[MBD_SYSTEM2D_MAX_BODIES];
    mbd_contact_halfspace2d_t contact_halfspaces[MBD_CONTACT2D_MAX_PAIRS];
    int num_contact_circles;
    int num_contact_halfspaces;
    mbd_contact_surface_polyline2d_t
        contact_surface_polylines[MBD_GENERIC_CONTACT2D_MAX_SURFACES];
    int num_contact_surface_polylines;
    mbd_contact_pair2d_t contact_pairs[MBD_CONTACT2D_MAX_PAIRS];
    int num_contact_pairs;
    mbd_contact_generic_pair2d_t
        generic_contact_pairs[MBD_GENERIC_CONTACT2D_MAX_PAIRS];
    int num_generic_contact_pairs;
    mbd_contact_generic_trace2d_t
        current_generic_contact_trace_rows[MBD_GENERIC_CONTACT2D_MAX_TRACE_ROWS];
    int num_current_generic_contact_trace_rows;
    mbd_contact_trace2d_t current_contact_trace[MBD_CONTACT2D_MAX_PAIRS];
    mbd_contact_feedback_trace2d_t current_contact_feedback[MBD_CONTACT2D_MAX_PAIRS];
    mbd_contact_feedback_use_trace2d_t current_contact_feedback_use[MBD_CONTACT2D_MAX_PAIRS];
    mbd_contact_coupling_mode_t contact_coupling_mode;
    mbd_local_feedback_mode_t local_feedback_mode;
    mbd_local_contact_monolithic_mode_t local_contact_monolithic_mode;
    mbd_monolithic_proper_mode_t monolithic_proper_mode;
    mbd_monolithic_proper_context2d_t monolithic_proper_context;
    mbd_monolithic_proper_internal_config2d_t monolithic_proper_internal;
    mbd_monolithic_proper_runtime2d_t monolithic_proper_runtime;
    char local_feedback_filename[1024];
    char local_contact_filename[1024];
    char ehl_filename[1024];
    char local_contact_monolithic_artifact_root[1024];
    mbd_local_feedback_record2d_t local_feedback_records[MBD_LOCAL_FEEDBACK2D_MAX_RECORDS];
    int num_local_feedback_records;
    mbd_contact_feedback2d_t current_reduced_interface_data[MBD_CONTACT2D_MAX_PAIRS];
    mbd_monolithic_local_patch_trace2d_t
        current_monolithic_local_patch_rows[MBD_CONTACT2D_MAX_PAIRS];
    int monolithic_local_patch_row_count_total;
    int monolithic_local_patch_active_rows_total;
    int monolithic_local_patch_gamma_not_one_rows_total;
    int monolithic_local_patch_fn_positive_rows_total;
    int current_step_index;
    int same_time_reduced_override_active;
    mbd_contact_feedback2d_t same_time_reduced_interface_data[MBD_CONTACT2D_MAX_PAIRS];
    double same_time_reduced_mu_used[MBD_CONTACT2D_MAX_PAIRS];
    double same_time_reduced_gamma_n_used[MBD_CONTACT2D_MAX_PAIRS];
    int same_time_reduced_record_iter[MBD_CONTACT2D_MAX_PAIRS];
    int same_time_reduced_status_ok[MBD_CONTACT2D_MAX_PAIRS];
    char same_time_reduced_fallback_reason[MBD_CONTACT2D_MAX_PAIRS][64];
    char same_time_reduced_status[MBD_CONTACT2D_MAX_PAIRS][32];
    double same_time_reduced_vt[MBD_CONTACT2D_MAX_PAIRS];
    mbd_same_time_reduced_iteration2d_t
        current_same_time_reduced_iterations[MBD_CONTACT2D_MAX_PAIRS * 8];
    int num_current_same_time_reduced_iterations;
    mbd_same_time_contact_request2d_t
        same_time_contact_requests[MBD_SAME_TIME_CONTACT_REQUEST2D_MAX_ROWS];
    int num_same_time_contact_request_rows;
    double (*current_generalized_force)[MBD_BODY2D_DOF];
    double (*previous_generalized_force)[MBD_BODY2D_DOF];
    int generalized_force_history_valid;
    const char *hht_force_history_mode;
    double implicit_residual_l2_last;
    double implicit_residual_tolerance_last;
    int implicit_residual_num_equations_last;
    int implicit_converged;
    const char *implicit_residual_mode;
    const char *implicit_scheme_mode;
    const char *implicit_convergence_reason;
    int position_projection_enabled;
    int position_projection_applied;
    int position_projection_target_reached;
    int position_projection_iterations_last;
    int position_projection_max_iterations;
    double position_projection_residual_l2_before;
    double position_projection_residual_l2_after;
    double position_projection_residual_reduction_ratio_last;
    double position_projection_velocity_residual_l2_before;
    double position_projection_velocity_residual_l2_after;
    double position_projection_velocity_reduction_ratio_last;
    double position_projection_residual_tolerance;
    double position_projection_correction_l2_last;
    const char *position_projection_source_status;
    const char *position_projection_max_iterations_source_status;
    const char *position_projection_residual_tolerance_source_status;
    const char *position_projection_stop_reason;
    int from_input;
    mbd_time_control2d_t time;
} mbd_system2d_t;

typedef struct {
    int body_capacity;
    int num_bodies;
    mbd_body2d_t *bodies;
    mbd_body_state2d_t *body_states;
    double (*flexible_force)[MBD_BODY2D_DOF];
    double (*contact_force)[MBD_BODY2D_DOF];
    double (*current_generalized_force)[MBD_BODY2D_DOF];
    double (*previous_generalized_force)[MBD_BODY2D_DOF];
    double contact_pair_last_normal[MBD_CONTACT2D_MAX_PAIRS][2];
    int contact_pair_has_last_normal[MBD_CONTACT2D_MAX_PAIRS];
    int generalized_force_history_valid;
    int steps_executed;
} mbd_system2d_snapshot_t;

const char *mbd_integrator2d_to_string(mbd_integrator2d_t integrator);
const char *mbd_contact_coupling_mode_to_string(mbd_contact_coupling_mode_t mode);
const char *mbd_local_feedback_mode_to_string(mbd_local_feedback_mode_t mode);
const char *mbd_local_contact_monolithic_mode_to_string(
    mbd_local_contact_monolithic_mode_t mode);
const char *mbd_monolithic_proper_mode_to_string(
    mbd_monolithic_proper_mode_t mode);

void mbd_time_control2d_set_defaults(mbd_time_control2d_t *time);
void mbd_system2d_zero(mbd_system2d_t *system);
fem_error_t mbd_system2d_init(mbd_system2d_t *system);
void mbd_system2d_free(mbd_system2d_t *system);
fem_error_t mbd_system2d_reserve_body_storage(mbd_system2d_t *system,
                                              int required_capacity);
void mbd_system2d_release_body_storage(mbd_system2d_t *system);
fem_error_t mbd_system2d_reserve_constraint_storage(mbd_system2d_t *system,
                                                    int required_capacity);
void mbd_system2d_release_constraint_storage(mbd_system2d_t *system);
fem_error_t mbd_system2d_clone(mbd_system2d_t *dst,
                               const mbd_system2d_t *src);
fem_error_t mbd_system2d_snapshot_reserve_body_storage(mbd_system2d_snapshot_t *snapshot,
                                                       int required_capacity);
void mbd_system2d_snapshot_release_body_storage(mbd_system2d_snapshot_t *snapshot);
void mbd_system2d_snapshot_free(mbd_system2d_snapshot_t *snapshot);
fem_error_t mbd_system2d_snapshot_capture(mbd_system2d_snapshot_t *snapshot,
                                          const mbd_system2d_t *src);
fem_error_t mbd_system2d_snapshot_restore(mbd_system2d_t *dst,
                                          const mbd_system2d_snapshot_t *snapshot);

fem_error_t mbd_system2d_find_body_index_by_id(const mbd_system2d_t *system,
                                               int body_id,
                                               int *body_index);
fem_error_t mbd_system2d_get_body_const(const mbd_system2d_t *system,
                                        int body_id,
                                        const mbd_body2d_t **body);
fem_error_t mbd_system2d_get_body_mut(mbd_system2d_t *system,
                                      int body_id,
                                      mbd_body2d_t **body);
fem_error_t mbd_system2d_set_gravity(mbd_system2d_t *system,
                                     double gravity_x,
                                     double gravity_y);
fem_error_t mbd_system2d_set_contact_coupling_mode(
    mbd_system2d_t *system,
    mbd_contact_coupling_mode_t mode);
fem_error_t mbd_system2d_set_local_feedback_mode(
    mbd_system2d_t *system,
    mbd_local_feedback_mode_t mode);
fem_error_t mbd_system2d_set_local_contact_monolithic_mode(
    mbd_system2d_t *system,
    mbd_local_contact_monolithic_mode_t mode);
fem_error_t mbd_system2d_set_monolithic_proper_mode(
    mbd_system2d_t *system,
    mbd_monolithic_proper_mode_t mode);
fem_error_t mbd_system2d_set_monolithic_proper_context(
    mbd_system2d_t *system,
    const mbd_monolithic_proper_context2d_t *context);
fem_error_t mbd_system2d_set_local_feedback_file(
    mbd_system2d_t *system,
    const char *path);
fem_error_t mbd_system2d_set_local_contact_file(
    mbd_system2d_t *system,
    const char *path);
fem_error_t mbd_system2d_set_ehl_file(
    mbd_system2d_t *system,
    const char *path);
fem_error_t mbd_system2d_clear_flexible_forces(mbd_system2d_t *system);
fem_error_t mbd_system2d_clear_contact_forces(mbd_system2d_t *system);
fem_error_t mbd_system2d_register_body_circle(mbd_system2d_t *system,
                                              int body_id,
                                              double radius,
                                              double thickness);
fem_error_t mbd_system2d_register_contact_halfspace(mbd_system2d_t *system,
                                                    int halfspace_id,
                                                    const double point[2],
                                                    const double normal[2],
                                                    double thickness);
fem_error_t mbd_system2d_register_contact_surface_polyline(
    mbd_system2d_t *system,
    int surface_id,
    int body_id,
    const char *csv_path);
fem_error_t mbd_system2d_append_contact_pair(mbd_system2d_t *system,
                                             int pair_id,
                                             int body_i,
                                             int body_j,
                                             double k_n,
                                             double c_n,
                                             double mu_base);
fem_error_t mbd_system2d_append_contact_halfspace_pair(mbd_system2d_t *system,
                                                       int pair_id,
                                                       int body_circle,
                                                       int halfspace_id,
                                                       double k_n,
                                                       double c_n,
                                                       double mu_base);
fem_error_t mbd_system2d_append_generic_contact_pair(
    mbd_system2d_t *system,
    int pair_id,
    int surface_i,
    int surface_j,
    double k_n,
    double c_n,
    double mu_base,
    double mu_static,
    double mu_dynamic,
    double v_ref,
    double v_smooth);
fem_error_t mbd_system2d_add_flexible_generalized_force(
    mbd_system2d_t *system,
    int body_id,
    const double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_refresh_generalized_force_history(mbd_system2d_t *system);
fem_error_t mbd_system2d_get_current_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_get_previous_generalized_force(
    const mbd_system2d_t *system,
    int body_index,
    double generalized_force[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_capture_body_forces(
    const mbd_system2d_t *system,
    double (*body_force)[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_restore_body_forces(
    mbd_system2d_t *system,
    const double (*body_force)[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_set_time_control(mbd_system2d_t *system,
                                          const mbd_time_control2d_t *time);
fem_error_t mbd_system2d_add_body(mbd_system2d_t *system,
                                  int body_index,
                                  const mbd_body2d_t *body);
fem_error_t mbd_system2d_add_body_state(mbd_system2d_t *system,
                                        int body_index,
                                        const mbd_body_state2d_t *state);
fem_error_t mbd_system2d_sync_body_states(mbd_system2d_t *system);
fem_error_t mbd_system2d_append_constraint(mbd_system2d_t *system,
                                           const mbd_constraint2d_t *constraint);
fem_error_t mbd_system2d_setup_builtin_case(mbd_system2d_t *system);
fem_error_t mbd_system2d_do_explicit_step(mbd_system2d_t *system);
fem_error_t mbd_system2d_do_newmark_step(mbd_system2d_t *system);
fem_error_t mbd_system2d_do_hht_step(mbd_system2d_t *system);
fem_error_t mbd_system2d_compute_explicit_acceleration(
    const mbd_system2d_t *system,
    int body_index,
    double acceleration[MBD_BODY2D_DOF]);
fem_error_t mbd_system2d_compute_layout(const mbd_system2d_t *system,
                                        mbd_kkt_layout_t *layout);
fem_error_t mbd_system2d_compute_constraint_residual_l2(const mbd_system2d_t *system,
                                                        double *residual_l2,
                                                        int *num_equations);
fem_error_t mbd_system2d_compute_revolute_metrics(const mbd_system2d_t *system,
                                                  double *anchor_mismatch_max,
                                                  double *body_j_com_radius_max,
                                                  int *revolute_count);
fem_error_t mbd_system2d_load(mbd_system2d_t *system,
                              const char *input_filename);
fem_error_t mbd_system2d_run(const char *input_filename,
                             const char *output_filename);

#endif /* FEM4C_MBD_SYSTEM2D_H */
