#ifndef FEM4C_ANALYSIS_RUNNER_H
#define FEM4C_ANALYSIS_RUNNER_H

#include "../common/types.h"
#include "../mbd/constraint2d.h"

typedef enum {
    ANALYSIS_MODE_FEM = 0,
    ANALYSIS_MODE_MBD,
    ANALYSIS_MODE_COUPLED
} analysis_mode_t;

typedef struct {
    const analysis_control_t *analysis;
    const node_t *nodes;
    const element_t *elements;
    const material_t *materials;
    int num_nodes;
    int num_elements;
    int num_materials;
} coupled_fem_state_view_t;

typedef struct {
    const mbd_body_state2d_t *body_states;
    const mbd_constraint2d_t *constraints;
    int num_bodies;
    int num_constraints;
} coupled_mbd_state_view_t;

typedef enum {
    COUPLED_INTEGRATOR_NEWMARK_BETA = 0,
    COUPLED_INTEGRATOR_HHT_ALPHA = 1
} coupled_integrator_t;

typedef struct {
    double dt;
    int num_steps;
    int max_coupling_iterations;
    double residual_tolerance;
    coupled_integrator_t integrator;
    /* Integrator parameters (CLI/env/default): */
    double newmark_beta;
    double newmark_gamma;
    double hht_alpha;
} coupled_time_control_t;

typedef struct {
    coupled_fem_state_view_t fem;
    coupled_mbd_state_view_t mbd;
    coupled_time_control_t time;
} coupled_io_contract_t;

const char *analysis_mode_to_string(analysis_mode_t mode);
fem_error_t analysis_mode_parse(const char *text, analysis_mode_t *mode);
analysis_mode_t analysis_mode_from_env(void);

fem_error_t analysis_run(analysis_mode_t mode,
                         const char *input_filename,
                         const char *output_filename);

#endif /* FEM4C_ANALYSIS_RUNNER_H */
