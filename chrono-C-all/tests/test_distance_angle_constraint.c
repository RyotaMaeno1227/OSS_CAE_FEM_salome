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
    chrono_body2d_set_mass(body, 1.0, 0.25);
    chrono_body2d_set_circle_shape(body, 0.18);
    body->position[0] = x;
    body->position[1] = y;
    body->angle = angle;
    body->linear_velocity[0] = 0.6;
    body->linear_velocity[1] = -0.4;
    body->angular_velocity = 0.5;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    init_anchor(&anchor);
    init_body(&body, 0.5, 0.3, 0.35);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};

    double dx = body.position[0] - anchor.position[0];
    double dy = body.position[1] - anchor.position[1];
    double initial_distance = sqrt(dx * dx + dy * dy);

    ChronoDistanceAngleConstraint2D_C constraint;
    chrono_distance_angle_constraint2d_init(&constraint,
                                            &anchor,
                                            &body,
                                            local_anchor,
                                            local_anchor,
                                            initial_distance,
                                            0.35,
                                            axis_local);
    chrono_distance_angle_constraint2d_set_baumgarte(&constraint, 0.4, 0.3);
    chrono_distance_angle_constraint2d_set_slop(&constraint, 5e-4);
    chrono_distance_angle_constraint2d_set_max_correction(&constraint, 0.08, 0.15);
    chrono_distance_angle_constraint2d_set_softness_linear(&constraint, 0.01);
    chrono_distance_angle_constraint2d_set_softness_angle(&constraint, 0.2);
    chrono_distance_angle_constraint2d_set_distance_spring(&constraint, 25.0, 3.5);
    chrono_distance_angle_constraint2d_set_angle_spring(&constraint, 12.0, 2.0);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 22;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.004;
    const int total_steps = 3200;

    for (int step = 0; step < total_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        body.linear_velocity[0] *= 0.996;
        body.linear_velocity[1] *= 0.996;
        body.angular_velocity *= 0.996;
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
    }

    double final_distance = compute_distance(&anchor, &body, &constraint);
    double final_angle = body.angle - anchor.angle;

    if (!isfinite(final_distance) || !isfinite(final_angle)) {
        fprintf(stderr, "Distance-angle constraint failed: non-finite state.\n");
        return 1;
    }

    if (fabs(final_distance - constraint.rest_distance) > 0.01) {
        fprintf(stderr,
                "Distance-angle constraint failed: distance drifted (%.6f vs %.6f).\n",
                final_distance,
                constraint.rest_distance);
        return 1;
    }

    if (fabs(final_angle - constraint.rest_angle) > 0.02) {
        fprintf(stderr,
                "Distance-angle constraint failed: angle drifted (%.6f vs %.6f).\n",
                final_angle,
                constraint.rest_angle);
        return 1;
    }

    if (fabs(body.linear_velocity[1]) > 0.12 || fabs(body.angular_velocity) > 0.25) {
        fprintf(stderr,
                "Distance-angle constraint failed: residual motion too high (vy=%.6f, w=%.6f).\n",
                body.linear_velocity[1],
                body.angular_velocity);
        return 1;
    }

    printf("Distance-angle constraint test passed.\n");
    return 0;
}
