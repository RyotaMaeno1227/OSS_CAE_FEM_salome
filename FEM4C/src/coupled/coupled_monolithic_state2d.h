#ifndef FEM4C_COUPLED_MONOLITHIC_STATE2D_H
#define FEM4C_COUPLED_MONOLITHIC_STATE2D_H

#include "coupled_run2d.h"

typedef struct {
    int num_rigid_unknowns;
    int num_flex_unknowns;
    int num_joint_multipliers;
    int num_interface_multipliers;
    int coupled_unknowns;
    int reduced_flex_body_count;
    int body_count;
    int interface_count;
} coupled_monolithic_layout2d_t;

typedef struct {
    int iteration_index;
    int coupled_unknowns;
    double residual_norm;
    double linear_solve_residual_inf;
} coupled_monolithic_iteration_log2d_t;

typedef struct {
    coupled_monolithic_layout2d_t layout;
    int active_flex_body_capacity;
    int *active_flex_body_ids;
    int iteration_limit;
    int unknown_capacity;
    double *target_unknowns;
    double *reduced_unknowns;
    double *reduced_residual;
    double *tangent;
    double *linear_rhs;
    double *delta_unknowns;
    coupled_monolithic_iteration_log2d_t last_iteration;
} coupled_monolithic_state2d_t;

void coupled_monolithic_state2d_zero(coupled_monolithic_state2d_t *state);
void coupled_monolithic_state2d_free(coupled_monolithic_state2d_t *state);
fem_error_t coupled_monolithic_state2d_reserve_active_flex_body_ids(
    coupled_monolithic_state2d_t *state,
    int required_capacity);
fem_error_t coupled_monolithic_state2d_reserve_unknown_storage(
    coupled_monolithic_state2d_t *state,
    int required_unknowns);
int coupled_monolithic_state2d_coupled_unknowns(
    const coupled_monolithic_state2d_t *state);
int coupled_monolithic_state2d_body_count(
    const coupled_monolithic_state2d_t *state);
int coupled_monolithic_state2d_interface_count(
    const coupled_monolithic_state2d_t *state);
const char *coupled_monolithic_state2d_solver_route_class(
    const coupled_monolithic_state2d_t *state);
const char *coupled_monolithic_state2d_sequence_name(
    const coupled_monolithic_state2d_t *state);

#endif /* FEM4C_COUPLED_MONOLITHIC_STATE2D_H */
