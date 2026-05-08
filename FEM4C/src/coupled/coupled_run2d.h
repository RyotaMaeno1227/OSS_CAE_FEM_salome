#ifndef FEM4C_COUPLED_RUN2D_H
#define FEM4C_COUPLED_RUN2D_H

#include "../common/types.h"
#include "../mbd/system/system2d.h"
#include "case2d.h"
#include "fem_model_copy.h"

typedef enum {
    COUPLED_INTEGRATOR_EXPLICIT = 0,
    COUPLED_INTEGRATOR_NEWMARK_BETA = 1,
    COUPLED_INTEGRATOR_HHT_ALPHA = 2
} coupled_integrator_t;

typedef enum {
    COUPLED_SCHEME_ONEWAY_SNAPSHOT = 0,
    COUPLED_SCHEME_STAGGERED_EXPLICIT = 1,
    COUPLED_SCHEME_FIXED_POINT_STRONG = 2,
    COUPLED_SCHEME_MONOLITHIC_STRONG_V1 = 3,
    COUPLED_SCHEME_DELAYED_COSIM_V1_5 = 4
} coupled_scheme_t;

typedef struct {
    double dt;
    int num_steps;
    int max_coupling_iterations;
    double residual_tolerance;
    coupled_integrator_t integrator;
    coupled_scheme_t scheme;
    int scheme_is_legacy_default;
    double newmark_beta;
    double newmark_gamma;
    double hht_alpha;
    double marker_relaxation;
} coupled_time_control_t;

typedef struct {
    int num_nodes;
    int num_elements;
    int num_materials;
    const analysis_control_t *analysis;
} coupled_master_fem_view_t;

#define COUPLED_STEP_HISTORY2D_REASON_MAX_LEN 64

typedef struct {
    int step_index;
    double time;
    double constraint_residual_l2;
    double coupling_residual_l2;
    int flex_solves;
    int fixed_point_iterations;
    int coupling_converged;
    char coupling_reason[COUPLED_STEP_HISTORY2D_REASON_MAX_LEN];
    int exchange_lag_steps;
    int sample_hold_active;
    int delayed_snapshot_step;
    int flex_body_count;
    int flex_body_storage_capacity;
    int *flex_body_ids;
    int *flex_body_full_reassembly_count;
    int *flex_body_static_solve_count;
    double (*flex_body_reaction_root)[3];
    double (*flex_body_reaction_tip)[3];
    double (*flex_body_root_force)[3];
    double (*flex_body_tip_force)[3];
    double (*flex_body_total_force)[3];
    int snapshot_record_count;
    int snapshot_record_capacity;
    int *snapshot_body_ids;
    int *snapshot_iteration_indices;
    char (*snapshot_paths)[MAX_FILENAME_LEN];
} coupled_step_history2d_t;

typedef struct {
    coupled_master_fem_view_t master_fem;
    mbd_system2d_t mbd_system;
    coupled_case2d_t case_data;
    fem_model_t *flex_models;
    int flex_model_capacity;
    int flex_model_count;
    coupled_time_control_t time;
} coupled_run2d_t;

#define COUPLED_RUN2D_ARTIFACT_METADATA_COLUMNS_CSV \
    "mbd_integrator,coupling_scheme,feedback_to_mbd," \
    "artifact_route_class,artifact_family,artifact_preferred_compare_source," \
    "artifact_aux_exports"

#define COUPLED_RUN2D_COMPARE_SCHEMA_YEAR1_2LINK_V1 \
    "year1_2link_v1"
#define COUPLED_RUN2D_COMPARE_STEP_COLUMNS_YEAR1_2LINK_V1_CSV \
    "step_index,time,constraint_residual_l2,coupling_residual_l2," \
    "flex_solves,compare_iteration_count,coupling_converged," \
    "exchange_lag_steps,sample_hold_active,delayed_snapshot_step"

const char *coupled_integrator_to_string(coupled_integrator_t integrator);
fem_error_t coupled_integrator_parse(const char *text,
                                     coupled_integrator_t *integrator);
coupled_integrator_t coupled_integrator_from_env(void);
const char *coupled_scheme_to_string(coupled_scheme_t scheme);
fem_error_t coupled_scheme_parse(const char *text,
                                 coupled_scheme_t *scheme);
fem_error_t coupled_time_control_from_env(coupled_time_control_t *time);
const char *coupled_run2d_artifact_metadata_columns_csv(void);
const char *coupled_run2d_interface_centers_header_csv(void);
const char *coupled_run2d_reaction_map_header_csv(void);
const char *coupled_run2d_observation_points_header_csv(void);

void coupled_run2d_zero(coupled_run2d_t *run);
void coupled_run2d_free(coupled_run2d_t *run);
void coupled_run2d_free_dynamic_buffers(coupled_run2d_t *run);
fem_error_t coupled_run2d_reserve_flex_model_storage(coupled_run2d_t *run,
                                                     int required_capacity);
void coupled_step_history2d_free_dynamic_buffers(coupled_step_history2d_t *history);
fem_error_t coupled_step_history2d_reserve_flex_body_storage(
    coupled_step_history2d_t *history,
    int required_capacity);
fem_error_t coupled_step_history2d_reserve_snapshot_storage(
    coupled_step_history2d_t *history,
    int required_capacity);

fem_error_t coupled_run2d(const char *input_filename,
                          const char *output_filename);

#endif /* FEM4C_COUPLED_RUN2D_H */
