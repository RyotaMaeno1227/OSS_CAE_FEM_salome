#ifndef CHRONO_CONSTRAINT2D_H
#define CHRONO_CONSTRAINT2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "chrono_body2d.h"

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

typedef struct ChronoDistanceConstraint2D_C {
    ChronoConstraint2DBase_C base;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double rest_length;
    double baumgarte_beta;
    double softness;
    double slop;
    double max_correction;
    double normal[2];
    double ra[2];
    double rb[2];
    double bias;
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
} ChronoRevoluteConstraint2D_C;

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
    double motor_speed;
    double motor_max_force;
    int enable_motor;
    double accumulated_motor_impulse;
    double limit_bias;
    double limit_accumulated_impulse;
    int limit_state;
    double motor_mass;
    double translation;
    double cached_dt;
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
void chrono_distance_constraint2d_set_slop(ChronoDistanceConstraint2D_C *constraint, double slop);
void chrono_distance_constraint2d_set_max_correction(ChronoDistanceConstraint2D_C *constraint, double max_correction);

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
void chrono_prismatic_constraint2d_prepare(ChronoPrismaticConstraint2D_C *constraint, double dt);
void chrono_prismatic_constraint2d_apply_warm_start(ChronoPrismaticConstraint2D_C *constraint);
void chrono_prismatic_constraint2d_solve_velocity(ChronoPrismaticConstraint2D_C *constraint);
void chrono_prismatic_constraint2d_solve_position(ChronoPrismaticConstraint2D_C *constraint);

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
