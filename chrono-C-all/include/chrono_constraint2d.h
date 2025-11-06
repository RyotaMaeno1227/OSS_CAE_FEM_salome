#ifndef CHRONO_CONSTRAINT2D_H
#define CHRONO_CONSTRAINT2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "chrono_body2d.h"
#include "chrono_logging.h"

typedef void (*ChronoConstraint2DPrepareFunc)(void *constraint, double dt);
typedef void (*ChronoConstraint2DStepFunc)(void *constraint);

typedef struct ChronoConstraint2DOps_C {
    ChronoConstraint2DPrepareFunc prepare;
    ChronoConstraint2DStepFunc apply_warm_start;
    ChronoConstraint2DStepFunc solve_velocity;
    ChronoConstraint2DStepFunc solve_position;
} ChronoConstraint2DOps_C;

typedef struct ChronoConstraint2DBase_C {
    const ChronoConstraint2DOps_C *ops;
    ChronoBody2D_C *body_a;
    ChronoBody2D_C *body_b;
    double accumulated_impulse;
    double effective_mass;
} ChronoConstraint2DBase_C;

#define CHRONO_COUPLED_MAX_EQ 4
#define CHRONO_COUPLED_DIAG_RANK_DEFICIENT 0x1u
#define CHRONO_COUPLED_DIAG_CONDITION_WARNING 0x2u

#define CHRONO_PRISMATIC_MOTOR_VELOCITY 0
#define CHRONO_PRISMATIC_MOTOR_POSITION 1
#define CHRONO_REVOLUTE_MOTOR_VELOCITY 0
#define CHRONO_REVOLUTE_MOTOR_POSITION 1
#define CHRONO_PLANAR_AXIS_X 0
#define CHRONO_PLANAR_AXIS_Y 1
#define CHRONO_PLANAR_AXIS_COUNT 2
#define CHRONO_PLANAR_MOTOR_VELOCITY 0
#define CHRONO_PLANAR_MOTOR_POSITION 1

typedef struct ChronoDistanceConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double rest_length;
    double baumgarte_beta;
    double softness_linear;
    double softness_angular;
    double slop;
    double max_correction;
    double normal[2];
    double ra[2];
    double rb[2];
    double bias;
    double spring_stiffness;
    double spring_damping;
    double spring_deflection;
    double cached_dt;
    double last_spring_force;
    double last_impulse;
    double accumulated_penetration;
} ChronoDistanceConstraint2D_C;

typedef struct ChronoRevoluteConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double ra[2];
    double rb[2];
    double effective_mass[2][2];
    double position_mass[2][2];
    double accumulated_impulse[2];
    double bias[2];
    double softness;
    double baumgarte_beta;
    double slop;
    double max_correction;
    int motor_enable;
    int motor_mode;
    double motor_speed;
    double motor_max_torque;
    double motor_position_target;
    double motor_position_gain;
    double motor_position_damping;
    double motor_mass;
    double motor_accumulated_impulse;
    double last_motor_torque;
    double cached_dt;
} ChronoRevoluteConstraint2D_C;

typedef struct ChronoGearConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double ratio;
    double phase;
    double softness;
    double baumgarte_beta;
    double bias;
    double motor_mass;
    double accumulated_impulse;
} ChronoGearConstraint2D_C;

typedef struct ChronoPrismaticConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double local_axis_a[2];
    double axis_world[2];
    double normal_world[2];
    double ra[2];
    double rb[2];
    double bias;
    double softness;
    double baumgarte_beta;
    double slop;
    double max_correction;
    double limit_lower;
    double limit_upper;
    int enable_limit;
    double limit_spring_stiffness;
    double limit_spring_damping;
    double motor_speed;
    double motor_max_force;
    int enable_motor;
    int motor_mode;
    double motor_position_target;
    double motor_position_gain;
    double motor_position_damping;
    double accumulated_motor_impulse;
    double limit_bias;
    double limit_accumulated_impulse;
    int limit_state;
    double motor_mass;
    double translation;
    double cached_dt;
    double last_motor_force;
    double last_limit_force;
    double last_limit_spring_force;
} ChronoPrismaticConstraint2D_C;

typedef struct ChronoSpringConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double rest_length;
    double stiffness;
    double damping;
    double ra[2];
    double rb[2];
    double direction[2];
    double current_length;
    double cached_dt;
    int velocity_applied;
} ChronoSpringConstraint2D_C;

typedef struct ChronoPlanarConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double local_axis_a[2];
    double axis_world[CHRONO_PLANAR_AXIS_COUNT][2];
    double ra[2];
    double rb[2];
    double translation[CHRONO_PLANAR_AXIS_COUNT];
    double mass[CHRONO_PLANAR_AXIS_COUNT];
    double bias[CHRONO_PLANAR_AXIS_COUNT];
    double softness;
    double baumgarte_beta;
    double slop;
    double max_correction;
    double limit_lower[CHRONO_PLANAR_AXIS_COUNT];
    double limit_upper[CHRONO_PLANAR_AXIS_COUNT];
    int enable_limit[CHRONO_PLANAR_AXIS_COUNT];
    double limit_spring_stiffness[CHRONO_PLANAR_AXIS_COUNT];
    double limit_spring_damping[CHRONO_PLANAR_AXIS_COUNT];
    double limit_bias[CHRONO_PLANAR_AXIS_COUNT];
    double limit_accumulated_impulse[CHRONO_PLANAR_AXIS_COUNT];
    int limit_state[CHRONO_PLANAR_AXIS_COUNT];
    double limit_deflection[CHRONO_PLANAR_AXIS_COUNT];
    double motor_speed[CHRONO_PLANAR_AXIS_COUNT];
    double motor_max_force[CHRONO_PLANAR_AXIS_COUNT];
    int enable_motor[CHRONO_PLANAR_AXIS_COUNT];
    int motor_mode[CHRONO_PLANAR_AXIS_COUNT];
    double motor_position_target[CHRONO_PLANAR_AXIS_COUNT];
    double motor_position_gain[CHRONO_PLANAR_AXIS_COUNT];
    double motor_position_damping[CHRONO_PLANAR_AXIS_COUNT];
    double motor_accumulated_impulse[CHRONO_PLANAR_AXIS_COUNT];
    double last_motor_force[CHRONO_PLANAR_AXIS_COUNT];
    double last_limit_force[CHRONO_PLANAR_AXIS_COUNT];
    double last_limit_spring_force[CHRONO_PLANAR_AXIS_COUNT];
    double cached_dt;
    double orientation_target;
    double orientation_mass;
    double orientation_bias;
    double orientation_accumulated_impulse;
} ChronoPlanarConstraint2D_C;

typedef struct ChronoDistanceAngleConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double rest_distance;
    double rest_angle;
    double axis_local[2];
    double normal[2];
    double ra[2];
    double rb[2];
    double bias_distance;
    double bias_angle;
    double baumgarte_distance;
    double baumgarte_angle;
    double softness_linear;
    double softness_angle;
    double slop;
    double max_correction_distance;
    double max_correction_angle;
    double mass_distance;
    double mass_angle;
    double spring_distance_stiffness;
    double spring_distance_damping;
    double spring_angle_stiffness;
    double spring_angle_damping;
    double spring_distance_deflection;
    double spring_angle_deflection;
    double cached_dt;
    double last_distance_impulse;
    double last_angle_impulse;
    double last_distance_force;
    double last_angle_force;
    double accumulated_distance_impulse;
    double accumulated_angle_impulse;
} ChronoDistanceAngleConstraint2D_C;

struct ChronoCoupledConstraint2D_C;

typedef struct ChronoCoupledConstraint2DDiagnostics_C {
    unsigned int flags;
    int rank;
    double condition_number;
    double min_pivot;
    double max_pivot;
    double condition_number_spectral;
    double min_eigenvalue;
    double max_eigenvalue;
} ChronoCoupledConstraint2DDiagnostics_C;

typedef struct ChronoCoupledConditionWarningEvent_C {
    double condition_number;
    double threshold;
    int active_equations;
    int auto_recover_enabled;
    int recovery_applied;
    ChronoLogLevel_C level;
    ChronoLogCategory_C category;
} ChronoCoupledConditionWarningEvent_C;

typedef void (*ChronoCoupledConditionWarningCallback_C)(
    const struct ChronoCoupledConstraint2D_C *constraint,
    const ChronoCoupledConditionWarningEvent_C *event,
    void *user_data);

typedef struct ChronoCoupledConditionWarningPolicy_C {
    int enable_logging;
    double log_cooldown;
    int enable_auto_recover;
    int max_drop;
    ChronoLogLevel_C log_level;
    ChronoLogCategory_C log_category;
    ChronoCoupledConditionWarningCallback_C log_callback;
    void *log_user_data;
} ChronoCoupledConditionWarningPolicy_C;

typedef struct ChronoCoupledConstraint2DEquationDesc_C {
    double ratio_distance;
    double ratio_angle;
    double target_offset;
    double softness_distance;
    double softness_angle;
    double spring_distance_stiffness;
    double spring_distance_damping;
    double spring_angle_stiffness;
    double spring_angle_damping;
} ChronoCoupledConstraint2DEquationDesc_C;

typedef struct ChronoCoupledConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double axis_local[2];
    double normal[2];
    double ra[2];
    double rb[2];
    double rest_distance;
    double rest_angle;
    double ratio_distance;
    double ratio_angle;
    double target_offset;
    double softness_distance;
    double softness_angle;
    double baumgarte;
    double slop;
    double max_correction;
    double cached_dt;
    double effective_mass;
    double gamma;
    double bias;
    double accumulated_impulse;
    double last_impulse;
    double last_distance_impulse;
    double last_angle_impulse;
    double last_distance_force;
    double last_angle_force;
    double spring_distance_stiffness;
    double spring_distance_damping;
    double spring_angle_stiffness;
    double spring_angle_damping;
    double spring_distance_deflection;
    double spring_angle_deflection;
    int equation_count;
    int equation_active[CHRONO_COUPLED_MAX_EQ];
    double ratio_distance_eq[CHRONO_COUPLED_MAX_EQ];
    double ratio_angle_eq[CHRONO_COUPLED_MAX_EQ];
    double target_offset_eq[CHRONO_COUPLED_MAX_EQ];
    double softness_distance_eq[CHRONO_COUPLED_MAX_EQ];
    double softness_angle_eq[CHRONO_COUPLED_MAX_EQ];
    double spring_distance_stiffness_eq[CHRONO_COUPLED_MAX_EQ];
    double spring_distance_damping_eq[CHRONO_COUPLED_MAX_EQ];
    double spring_angle_stiffness_eq[CHRONO_COUPLED_MAX_EQ];
    double spring_angle_damping_eq[CHRONO_COUPLED_MAX_EQ];
    double gamma_eq[CHRONO_COUPLED_MAX_EQ];
    double bias_eq[CHRONO_COUPLED_MAX_EQ];
    double last_impulse_eq[CHRONO_COUPLED_MAX_EQ];
    double last_distance_impulse_eq[CHRONO_COUPLED_MAX_EQ];
    double last_angle_impulse_eq[CHRONO_COUPLED_MAX_EQ];
    double last_distance_force_eq[CHRONO_COUPLED_MAX_EQ];
    double last_angle_force_eq[CHRONO_COUPLED_MAX_EQ];
    double accumulated_impulse_eq[CHRONO_COUPLED_MAX_EQ];
    double inv_mass_matrix[CHRONO_COUPLED_MAX_EQ][CHRONO_COUPLED_MAX_EQ];
    double system_matrix[CHRONO_COUPLED_MAX_EQ][CHRONO_COUPLED_MAX_EQ];
    ChronoCoupledConstraint2DDiagnostics_C diagnostics;
    ChronoCoupledConditionWarningPolicy_C condition_policy;
    double condition_warning_log_timer;
} ChronoCoupledConstraint2D_C;

typedef struct ChronoConstraint2DBatchConfig_C {
    int velocity_iterations;
    int position_iterations;
    int enable_parallel;
} ChronoConstraint2DBatchConfig_C;

typedef struct ChronoConstraint2DBatchWorkspace_C {
    int *island_ids;
    size_t island_ids_capacity;
    size_t *island_sizes;
    size_t island_sizes_capacity;
    size_t *island_offsets;
    size_t island_offsets_capacity;
    size_t *ordered_indices;
    size_t ordered_indices_capacity;
    ChronoConstraint2DBase_C **constraint_buffer;
    size_t constraint_buffer_capacity;
} ChronoConstraint2DBatchWorkspace_C;

/*
 * ワークスペースAPIの使い方:
 *   ChronoConstraint2DBatchWorkspace_C workspace;
 *   chrono_constraint2d_workspace_init(&workspace);
 *   while (running) {
 *       chrono_constraint2d_workspace_reset(&workspace);
 *       chrono_constraint2d_batch_solve(..., &workspace);
 *   }
 *   chrono_constraint2d_workspace_free(&workspace);
 */

void chrono_constraint2d_workspace_init(ChronoConstraint2DBatchWorkspace_C *workspace);
void chrono_constraint2d_workspace_reset(ChronoConstraint2DBatchWorkspace_C *workspace);
void chrono_constraint2d_workspace_free(ChronoConstraint2DBatchWorkspace_C *workspace);

size_t chrono_constraint2d_build_islands(ChronoConstraint2DBase_C **constraints,
                                         size_t count,
                                         int *island_ids);

void chrono_constraint2d_prepare(ChronoConstraint2DBase_C *constraint, double dt);
void chrono_constraint2d_apply_warm_start(ChronoConstraint2DBase_C *constraint);
void chrono_constraint2d_solve_velocity(ChronoConstraint2DBase_C *constraint);
void chrono_constraint2d_solve_position(ChronoConstraint2DBase_C *constraint);
void chrono_constraint2d_batch_solve(ChronoConstraint2DBase_C **constraints,
                                     size_t count,
                                     double dt,
                                     const ChronoConstraint2DBatchConfig_C *config,
                                     ChronoConstraint2DBatchWorkspace_C *workspace);

void chrono_distance_constraint2d_init(ChronoDistanceConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2],
                                       double rest_length);

void chrono_distance_constraint2d_set_baumgarte(ChronoDistanceConstraint2D_C *constraint, double beta);
void chrono_distance_constraint2d_set_softness(ChronoDistanceConstraint2D_C *constraint, double softness);
void chrono_distance_constraint2d_set_softness_linear(ChronoDistanceConstraint2D_C *constraint, double softness);
void chrono_distance_constraint2d_set_softness_angular(ChronoDistanceConstraint2D_C *constraint, double softness);
void chrono_distance_constraint2d_set_slop(ChronoDistanceConstraint2D_C *constraint, double slop);
void chrono_distance_constraint2d_set_max_correction(ChronoDistanceConstraint2D_C *constraint, double max_correction);
void chrono_distance_constraint2d_set_spring(ChronoDistanceConstraint2D_C *constraint,
                                             double stiffness,
                                             double damping);

void chrono_distance_constraint2d_prepare(ChronoDistanceConstraint2D_C *constraint, double dt);
void chrono_distance_constraint2d_apply_warm_start(ChronoDistanceConstraint2D_C *constraint);
void chrono_distance_constraint2d_solve_velocity(ChronoDistanceConstraint2D_C *constraint);
void chrono_distance_constraint2d_solve_position(ChronoDistanceConstraint2D_C *constraint);

void chrono_revolute_constraint2d_init(ChronoRevoluteConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2]);
void chrono_revolute_constraint2d_set_baumgarte(ChronoRevoluteConstraint2D_C *constraint, double beta);
void chrono_revolute_constraint2d_set_softness(ChronoRevoluteConstraint2D_C *constraint, double softness);
void chrono_revolute_constraint2d_set_slop(ChronoRevoluteConstraint2D_C *constraint, double slop);
void chrono_revolute_constraint2d_set_max_correction(ChronoRevoluteConstraint2D_C *constraint, double max_correction);
void chrono_revolute_constraint2d_enable_motor(ChronoRevoluteConstraint2D_C *constraint,
                                              int enable,
                                              double speed,
                                              double max_torque);
void chrono_revolute_constraint2d_set_motor_position_target(ChronoRevoluteConstraint2D_C *constraint,
                                                            double target_angle,
                                                            double proportional_gain,
                                                            double damping_gain);
void chrono_revolute_constraint2d_prepare(ChronoRevoluteConstraint2D_C *constraint, double dt);
void chrono_revolute_constraint2d_apply_warm_start(ChronoRevoluteConstraint2D_C *constraint);
void chrono_revolute_constraint2d_solve_velocity(ChronoRevoluteConstraint2D_C *constraint);
void chrono_revolute_constraint2d_solve_position(ChronoRevoluteConstraint2D_C *constraint);

void chrono_prismatic_constraint2d_init(ChronoPrismaticConstraint2D_C *constraint,
                                        ChronoBody2D_C *body_a,
                                        ChronoBody2D_C *body_b,
                                        const double local_anchor_a[2],
                                        const double local_anchor_b[2],
                                        const double local_axis_a[2]);
void chrono_prismatic_constraint2d_set_axis(ChronoPrismaticConstraint2D_C *constraint,
                                            const double local_axis_a[2]);
void chrono_prismatic_constraint2d_set_baumgarte(ChronoPrismaticConstraint2D_C *constraint, double beta);
void chrono_prismatic_constraint2d_set_softness(ChronoPrismaticConstraint2D_C *constraint, double softness);
void chrono_prismatic_constraint2d_set_slop(ChronoPrismaticConstraint2D_C *constraint, double slop);
void chrono_prismatic_constraint2d_set_max_correction(ChronoPrismaticConstraint2D_C *constraint, double max_correction);
void chrono_prismatic_constraint2d_enable_limit(ChronoPrismaticConstraint2D_C *constraint,
                                                int enable,
                                                double lower,
                                                double upper);
void chrono_prismatic_constraint2d_enable_motor(ChronoPrismaticConstraint2D_C *constraint,
                                                int enable,
                                                double speed,
                                                double max_force);
void chrono_prismatic_constraint2d_set_limit_spring(ChronoPrismaticConstraint2D_C *constraint,
                                                    double stiffness,
                                                    double damping);
void chrono_prismatic_constraint2d_set_motor_position_target(ChronoPrismaticConstraint2D_C *constraint,
                                                             double target_position,
                                                             double proportional_gain,
                                                             double damping_gain);
void chrono_prismatic_constraint2d_prepare(ChronoPrismaticConstraint2D_C *constraint, double dt);
void chrono_prismatic_constraint2d_apply_warm_start(ChronoPrismaticConstraint2D_C *constraint);
void chrono_prismatic_constraint2d_solve_velocity(ChronoPrismaticConstraint2D_C *constraint);
void chrono_prismatic_constraint2d_solve_position(ChronoPrismaticConstraint2D_C *constraint);

void chrono_planar_constraint2d_init(ChronoPlanarConstraint2D_C *constraint,
                                     ChronoBody2D_C *body_a,
                                     ChronoBody2D_C *body_b,
                                     const double local_anchor_a[2],
                                     const double local_anchor_b[2],
                                     const double local_axis_a[2]);
void chrono_planar_constraint2d_set_axes(ChronoPlanarConstraint2D_C *constraint,
                                         const double local_axis_a[2]);
void chrono_planar_constraint2d_set_baumgarte(ChronoPlanarConstraint2D_C *constraint, double beta);
void chrono_planar_constraint2d_set_softness(ChronoPlanarConstraint2D_C *constraint, double softness);
void chrono_planar_constraint2d_set_slop(ChronoPlanarConstraint2D_C *constraint, double slop);
void chrono_planar_constraint2d_set_max_correction(ChronoPlanarConstraint2D_C *constraint, double max_correction);
void chrono_planar_constraint2d_enable_limit(ChronoPlanarConstraint2D_C *constraint,
                                             int axis,
                                             int enable,
                                             double lower,
                                             double upper);
void chrono_planar_constraint2d_set_limit_spring(ChronoPlanarConstraint2D_C *constraint,
                                                 int axis,
                                                 double stiffness,
                                                 double damping);
void chrono_planar_constraint2d_enable_motor(ChronoPlanarConstraint2D_C *constraint,
                                             int axis,
                                             int enable,
                                             double speed,
                                             double max_force);
void chrono_planar_constraint2d_set_motor_position_target(ChronoPlanarConstraint2D_C *constraint,
                                                          int axis,
                                                          double target,
                                                          double proportional_gain,
                                                          double damping_gain);
void chrono_planar_constraint2d_set_orientation_target(ChronoPlanarConstraint2D_C *constraint, double target_angle);
void chrono_planar_constraint2d_prepare(ChronoPlanarConstraint2D_C *constraint, double dt);
void chrono_planar_constraint2d_apply_warm_start(ChronoPlanarConstraint2D_C *constraint);
void chrono_planar_constraint2d_solve_velocity(ChronoPlanarConstraint2D_C *constraint);
void chrono_planar_constraint2d_solve_position(ChronoPlanarConstraint2D_C *constraint);

void chrono_distance_angle_constraint2d_init(ChronoDistanceAngleConstraint2D_C *constraint,
                                            ChronoBody2D_C *body_a,
                                            ChronoBody2D_C *body_b,
                                            const double local_anchor_a[2],
                                            const double local_anchor_b[2],
                                            double rest_distance,
                                            double rest_angle,
                                            const double axis_local[2]);
void chrono_distance_angle_constraint2d_set_rest_distance(ChronoDistanceAngleConstraint2D_C *constraint,
                                                          double rest_distance);
void chrono_distance_angle_constraint2d_set_rest_angle(ChronoDistanceAngleConstraint2D_C *constraint,
                                                       double rest_angle);
void chrono_distance_angle_constraint2d_set_baumgarte(ChronoDistanceAngleConstraint2D_C *constraint,
                                                      double beta_distance,
                                                      double beta_angle);
void chrono_distance_angle_constraint2d_set_slop(ChronoDistanceAngleConstraint2D_C *constraint, double slop);
void chrono_distance_angle_constraint2d_set_max_correction(ChronoDistanceAngleConstraint2D_C *constraint,
                                                           double max_distance,
                                                           double max_angle);
void chrono_distance_angle_constraint2d_set_softness_linear(ChronoDistanceAngleConstraint2D_C *constraint,
                                                            double softness);
void chrono_distance_angle_constraint2d_set_softness_angle(ChronoDistanceAngleConstraint2D_C *constraint,
                                                           double softness);
void chrono_distance_angle_constraint2d_set_distance_spring(ChronoDistanceAngleConstraint2D_C *constraint,
                                                            double stiffness,
                                                            double damping);
void chrono_distance_angle_constraint2d_set_angle_spring(ChronoDistanceAngleConstraint2D_C *constraint,
                                                         double stiffness,
                                                         double damping);
void chrono_distance_angle_constraint2d_prepare(ChronoDistanceAngleConstraint2D_C *constraint, double dt);
void chrono_distance_angle_constraint2d_apply_warm_start(ChronoDistanceAngleConstraint2D_C *constraint);
void chrono_distance_angle_constraint2d_solve_velocity(ChronoDistanceAngleConstraint2D_C *constraint);
void chrono_distance_angle_constraint2d_solve_position(ChronoDistanceAngleConstraint2D_C *constraint);

void chrono_coupled_constraint2d_init(ChronoCoupledConstraint2D_C *constraint,
                                      ChronoBody2D_C *body_a,
                                      ChronoBody2D_C *body_b,
                                      const double local_anchor_a[2],
                                      const double local_anchor_b[2],
                                      const double axis_local[2],
                                      double rest_distance,
                                      double rest_angle,
                                      double ratio_distance,
                                      double ratio_angle,
                                      double target_offset);
void chrono_coupled_constraint2d_set_rest_distance(ChronoCoupledConstraint2D_C *constraint, double rest_distance);
void chrono_coupled_constraint2d_set_rest_angle(ChronoCoupledConstraint2D_C *constraint, double rest_angle);
void chrono_coupled_constraint2d_set_ratios(ChronoCoupledConstraint2D_C *constraint,
                                            double ratio_distance,
                                            double ratio_angle);
void chrono_coupled_constraint2d_set_target_offset(ChronoCoupledConstraint2D_C *constraint, double offset);
void chrono_coupled_constraint2d_set_baumgarte(ChronoCoupledConstraint2D_C *constraint, double beta);
void chrono_coupled_constraint2d_set_softness(ChronoCoupledConstraint2D_C *constraint, double softness);
void chrono_coupled_constraint2d_set_softness_distance(ChronoCoupledConstraint2D_C *constraint, double softness);
void chrono_coupled_constraint2d_set_softness_angle(ChronoCoupledConstraint2D_C *constraint, double softness);
void chrono_coupled_constraint2d_set_slop(ChronoCoupledConstraint2D_C *constraint, double slop);
void chrono_coupled_constraint2d_set_max_correction(ChronoCoupledConstraint2D_C *constraint, double max_correction);
void chrono_coupled_constraint2d_set_distance_spring(ChronoCoupledConstraint2D_C *constraint,
                                                     double stiffness,
                                                     double damping);
void chrono_coupled_constraint2d_set_angle_spring(ChronoCoupledConstraint2D_C *constraint,
                                                  double stiffness,
                                                  double damping);
void chrono_coupled_constraint2d_clear_equations(ChronoCoupledConstraint2D_C *constraint);
int chrono_coupled_constraint2d_add_equation(ChronoCoupledConstraint2D_C *constraint,
                                             const ChronoCoupledConstraint2DEquationDesc_C *desc);
int chrono_coupled_constraint2d_set_equation(ChronoCoupledConstraint2D_C *constraint,
                                             int index,
                                             const ChronoCoupledConstraint2DEquationDesc_C *desc);
int chrono_coupled_constraint2d_get_equation_count(const ChronoCoupledConstraint2D_C *constraint);
const ChronoCoupledConstraint2DDiagnostics_C *
chrono_coupled_constraint2d_get_diagnostics(const ChronoCoupledConstraint2D_C *constraint);
void chrono_coupled_constraint2d_get_condition_warning_policy(
    const ChronoCoupledConstraint2D_C *constraint,
    ChronoCoupledConditionWarningPolicy_C *out_policy);
void chrono_coupled_constraint2d_set_condition_warning_callback(
    ChronoCoupledConstraint2D_C *constraint,
    ChronoCoupledConditionWarningCallback_C callback,
    void *user_data);
void chrono_coupled_constraint2d_set_condition_warning_log_level(
    ChronoCoupledConstraint2D_C *constraint,
    ChronoLogLevel_C level,
    ChronoLogCategory_C category);
void chrono_coupled_constraint2d_set_condition_warning_policy(
    ChronoCoupledConstraint2D_C *constraint,
    const ChronoCoupledConditionWarningPolicy_C *policy);
void chrono_coupled_constraint2d_prepare(ChronoCoupledConstraint2D_C *constraint, double dt);
void chrono_coupled_constraint2d_apply_warm_start(ChronoCoupledConstraint2D_C *constraint);
void chrono_coupled_constraint2d_solve_velocity(ChronoCoupledConstraint2D_C *constraint);
void chrono_coupled_constraint2d_solve_position(ChronoCoupledConstraint2D_C *constraint);

void chrono_gear_constraint2d_init(ChronoGearConstraint2D_C *constraint,
                                   ChronoBody2D_C *body_a,
                                   ChronoBody2D_C *body_b,
                                   double ratio,
                                   double phase);
void chrono_gear_constraint2d_set_ratio(ChronoGearConstraint2D_C *constraint, double ratio);
void chrono_gear_constraint2d_set_phase(ChronoGearConstraint2D_C *constraint, double phase);
void chrono_gear_constraint2d_set_baumgarte(ChronoGearConstraint2D_C *constraint, double beta);
void chrono_gear_constraint2d_set_softness(ChronoGearConstraint2D_C *constraint, double softness);
void chrono_gear_constraint2d_prepare(ChronoGearConstraint2D_C *constraint, double dt);
void chrono_gear_constraint2d_apply_warm_start(ChronoGearConstraint2D_C *constraint);
void chrono_gear_constraint2d_solve_velocity(ChronoGearConstraint2D_C *constraint);
void chrono_gear_constraint2d_solve_position(ChronoGearConstraint2D_C *constraint);

void chrono_spring_constraint2d_init(ChronoSpringConstraint2D_C *constraint,
                                     ChronoBody2D_C *body_a,
                                     ChronoBody2D_C *body_b,
                                     const double local_anchor_a[2],
                                     const double local_anchor_b[2],
                                     double rest_length,
                                     double stiffness,
                                     double damping);
void chrono_spring_constraint2d_set_rest_length(ChronoSpringConstraint2D_C *constraint, double rest_length);
void chrono_spring_constraint2d_set_stiffness(ChronoSpringConstraint2D_C *constraint, double stiffness);
void chrono_spring_constraint2d_set_damping(ChronoSpringConstraint2D_C *constraint, double damping);
void chrono_spring_constraint2d_prepare(ChronoSpringConstraint2D_C *constraint, double dt);
void chrono_spring_constraint2d_apply_warm_start(ChronoSpringConstraint2D_C *constraint);
void chrono_spring_constraint2d_solve_velocity(ChronoSpringConstraint2D_C *constraint);
void chrono_spring_constraint2d_solve_position(ChronoSpringConstraint2D_C *constraint);

#ifdef __cplusplus
}
#endif

#endif
