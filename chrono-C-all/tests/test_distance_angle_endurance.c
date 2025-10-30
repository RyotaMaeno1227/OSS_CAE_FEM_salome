#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static double compute_distance(const ChronoBody2D_C *anchor,
                               const ChronoBody2D_C *body,
                               const ChronoDistanceAngleConstraint2D_C *constraint) {
    double world_a[2];
    double world_b[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, world_a);
    chrono_body2d_local_to_world(body, constraint->local_anchor_b, world_b);
    double dx = world_b[0] - world_a[0];
    double dy = world_b[1] - world_a[1];
    return sqrt(dx * dx + dy * dy);
}

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = 0.0;
    anchor->position[1] = 0.0;
}

static void init_body(ChronoBody2D_C *body, double x, double y, double angle) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.24);
    chrono_body2d_set_circle_shape(body, 0.17);
    body->position[0] = x;
    body->position[1] = y;
    body->angle = angle;
    body->linear_velocity[0] = 0.6;
    body->linear_velocity[1] = -0.5;
    body->angular_velocity = 0.7;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    init_anchor(&anchor);
    init_body(&body, 0.6, 0.35, 0.45);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};
    double initial_distance = compute_distance(&anchor, &body, (ChronoDistanceAngleConstraint2D_C *)&body);
    double initial_angle = body.angle - anchor.angle;

    ChronoDistanceAngleConstraint2D_C constraint;
    chrono_distance_angle_constraint2d_init(&constraint,
                                            &anchor,
                                            &body,
                                            local_anchor,
                                            local_anchor,
                                            initial_distance,
                                            initial_angle,
                                            axis_local);
    chrono_distance_angle_constraint2d_set_baumgarte(&constraint, 0.3, 0.28);
    chrono_distance_angle_constraint2d_set_slop(&constraint, 5e-4);
    chrono_distance_angle_constraint2d_set_max_correction(&constraint, 0.08, 0.18);
    chrono_distance_angle_constraint2d_set_softness_linear(&constraint, 0.015);
    chrono_distance_angle_constraint2d_set_softness_angle(&constraint, 0.22);
    chrono_distance_angle_constraint2d_set_distance_spring(&constraint, 28.0, 3.2);
    chrono_distance_angle_constraint2d_set_angle_spring(&constraint, 14.0, 2.4);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.0045;
    const int total_steps = 4800;
    const int switch_step = 1800;
    const int switch_step_2 = 3200;

    double max_distance_error = 0.0;
    double max_angle_error = 0.0;
    double max_distance_force = 0.0;
    double max_angle_force = 0.0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == switch_step) {
            chrono_distance_angle_constraint2d_set_rest_distance(&constraint, initial_distance - 0.05);
            chrono_distance_angle_constraint2d_set_rest_angle(&constraint, initial_angle + 0.10);
        } else if (step == switch_step_2) {
            chrono_distance_angle_constraint2d_set_rest_distance(&constraint, initial_distance - 0.02);
            chrono_distance_angle_constraint2d_set_rest_angle(&constraint, initial_angle - 0.06);
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);

        body.linear_velocity[0] *= 0.996;
        body.linear_velocity[1] *= 0.996;
        body.angular_velocity *= 0.996;

        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);

        double dist = compute_distance(&anchor, &body, &constraint);
        double angle = body.angle - anchor.angle;
        double dist_err = fabs(dist - constraint.rest_distance);
        double ang_err = fabs(angle - constraint.rest_angle);
        if (dist_err > max_distance_error) {
            max_distance_error = dist_err;
        }
        if (ang_err > max_angle_error) {
            max_angle_error = ang_err;
        }
        if (fabs(constraint.last_distance_force) > max_distance_force) {
            max_distance_force = fabs(constraint.last_distance_force);
        }
        if (fabs(constraint.last_angle_force) > max_angle_force) {
            max_angle_force = fabs(constraint.last_angle_force);
        }
    }

    double final_distance = compute_distance(&anchor, &body, &constraint);
    double final_angle = body.angle - anchor.angle;

    if (!isfinite(final_distance) || !isfinite(final_angle)) {
        fprintf(stderr, "Distance-angle endurance failed: non-finite final state.\n");
        return 1;
    }

    if (fabs(final_distance - constraint.rest_distance) > 0.012 ||
        fabs(final_angle - constraint.rest_angle) > 0.02) {
        fprintf(stderr,
                "Distance-angle endurance failed: final mismatch (dist=%.6f, angle=%.6f).\n",
                final_distance - constraint.rest_distance,
                final_angle - constraint.rest_angle);
        return 1;
    }

    if (max_distance_error > 0.2 || max_angle_error > 0.25) {
        fprintf(stderr,
                "Distance-angle endurance failed: transient error too large (dist=%.6f, angle=%.6f).\n",
                max_distance_error,
                max_angle_error);
        return 1;
    }

    if (max_distance_force > 85.0 || max_angle_force > 45.0) {
        fprintf(stderr,
                "Distance-angle endurance failed: force spikes (dist=%.6f, angle=%.6f).\n",
                max_distance_force,
                max_angle_force);
        return 1;
    }

    printf("Distance-angle endurance test passed.\n");
    return 0;
}
