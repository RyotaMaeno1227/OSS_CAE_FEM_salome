#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static double compute_distance_raw(const ChronoBody2D_C *anchor,
                                   const ChronoBody2D_C *body) {
    double world_a[2] = {anchor->position[0], anchor->position[1]};
    double world_b[2] = {body->position[0], body->position[1]};
    double dx = world_b[0] - world_a[0];
    double dy = world_b[1] - world_a[1];
    return sqrt(dx * dx + dy * dy);
}

static double compute_distance_constraint(const ChronoBody2D_C *anchor,
                                          const ChronoBody2D_C *body,
                                          const ChronoCoupledConstraint2D_C *constraint) {
    double world_a[2];
    double world_b[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, world_a);
    chrono_body2d_local_to_world(body, constraint->local_anchor_b, world_b);
    double dx = world_b[0] - world_a[0];
    double dy = world_b[1] - world_a[1];
    return sqrt(dx * dx + dy * dy);
}

static void init_anchor(ChronoBody2D_C *anchor, double x, double y) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = x;
    anchor->position[1] = y;
}

static void init_body(ChronoBody2D_C *body, double x, double y, double angle) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.22);
    chrono_body2d_set_circle_shape(body, 0.18);
    body->position[0] = x;
    body->position[1] = y;
    body->angle = angle;
    body->linear_velocity[0] = 0.45;
    body->linear_velocity[1] = -0.35;
    body->angular_velocity = 0.5;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    init_anchor(&anchor, -0.2, 0.1);
    init_body(&body, 0.45, 0.35, 0.4);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};

    double initial_distance = compute_distance_raw(&anchor, &body);
    double initial_angle = body.angle - anchor.angle;

    ChronoCoupledConstraint2D_C constraint;
    chrono_coupled_constraint2d_init(&constraint,
                                     &anchor,
                                     &body,
                                     local_anchor,
                                     local_anchor,
                                     axis_local,
                                     initial_distance,
                                     initial_angle,
                                     1.0,
                                     0.5,
                                     0.0);
    chrono_coupled_constraint2d_set_baumgarte(&constraint, 0.4);
    chrono_coupled_constraint2d_set_softness(&constraint, 0.02);
    chrono_coupled_constraint2d_set_slop(&constraint, 5e-4);
    chrono_coupled_constraint2d_set_max_correction(&constraint, 0.08);

    // provide new targets to converge to
    chrono_coupled_constraint2d_set_rest_distance(&constraint, initial_distance - 0.1);
    chrono_coupled_constraint2d_set_rest_angle(&constraint, initial_angle + 0.15);
    chrono_coupled_constraint2d_set_target_offset(&constraint, 0.0);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 22;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.0045;
    const int total_steps = 3400;

    for (int step = 0; step < total_steps; ++step) {
        if (step == 1600) {
            chrono_coupled_constraint2d_set_rest_distance(&constraint, initial_distance - 0.05);
            chrono_coupled_constraint2d_set_rest_angle(&constraint, initial_angle - 0.1);
            chrono_coupled_constraint2d_set_target_offset(&constraint, 0.02);
        }
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        body.linear_velocity[0] *= 0.997;
        body.linear_velocity[1] *= 0.997;
        body.angular_velocity *= 0.997;
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
    }

    double final_distance = compute_distance_constraint(&anchor, &body, &constraint);
    double final_angle = body.angle - anchor.angle;
    double coupled_value = constraint.ratio_distance * (final_distance - constraint.rest_distance) +
                          constraint.ratio_angle * (final_angle - constraint.rest_angle) -
                          constraint.target_offset;

    if (!isfinite(final_distance) || !isfinite(final_angle)) {
        fprintf(stderr, "Coupled constraint failed: non-finite state.\n");
        return 1;
    }

    if (fabs(coupled_value) > 0.008) {
        fprintf(stderr,
                "Coupled constraint failed: linear combination residual %.6f.\n",
                coupled_value);
        return 1;
    }

    double distance_delta = fabs(final_distance - constraint.rest_distance);
    double angle_delta = fabs(final_angle - constraint.rest_angle);
    if (distance_delta > 0.25 || angle_delta > 0.35) {
        fprintf(stderr,
                "Coupled constraint failed: state drift too large (dist=%.6f, angle=%.6f).\n",
                distance_delta,
                angle_delta);
        return 1;
    }

    if (fabs(body.linear_velocity[0]) > 0.18 || fabs(body.linear_velocity[1]) > 0.18) {
        fprintf(stderr,
                "Coupled constraint failed: residual linear velocity too high (vx=%.6f, vy=%.6f).\n",
                body.linear_velocity[0],
                body.linear_velocity[1]);
        return 1;
    }

    printf("Coupled constraint test passed.\n");
    return 0;
}
