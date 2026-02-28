#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static double compute_distance(const ChronoBody2D_C *anchor,
                               const ChronoBody2D_C *body,
                               const ChronoDistanceConstraint2D_C *constraint) {
    double pa[2];
    double pb[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, pa);
    chrono_body2d_local_to_world(body, constraint->local_anchor_b, pb);
    double dx = pb[0] - pa[0];
    double dy = pb[1] - pa[1];
    return sqrt(dx * dx + dy * dy);
}

static void init_anchor(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_static(body);
    body->position[0] = x;
    body->position[1] = y;
}

static void init_body(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.2, 0.3);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->position[0] = x;
    body->position[1] = y;
    body->linear_velocity[0] = 0.35;
    body->linear_velocity[1] = -0.25;
    body->angular_velocity = 0.45;
}

int main(void) {
    ChronoBody2D_C anchor_a;
    ChronoBody2D_C anchor_b;
    ChronoBody2D_C body;
    init_anchor(&anchor_a, -0.4, 0.1);
    init_anchor(&anchor_b, 0.6, -0.2);
    init_body(&body, 0.2, 0.5);

    ChronoDistanceConstraint2D_C constraint_a;
    ChronoDistanceConstraint2D_C constraint_b;

    chrono_distance_constraint2d_init(&constraint_a,
                                      &anchor_a,
                                      &body,
                                      (double[2]){0.0, 0.0},
                                      (double[2]){0.0, 0.0},
                                      compute_distance(&anchor_a, &body, &(ChronoDistanceConstraint2D_C){0}));
    chrono_distance_constraint2d_set_baumgarte(&constraint_a, 0.4);
    chrono_distance_constraint2d_set_softness_linear(&constraint_a, 0.005);
    chrono_distance_constraint2d_set_softness_angular(&constraint_a, 0.12);
    chrono_distance_constraint2d_set_slop(&constraint_a, 5e-4);
    chrono_distance_constraint2d_set_max_correction(&constraint_a, 0.07);

    chrono_distance_constraint2d_init(&constraint_b,
                                      &anchor_b,
                                      &body,
                                      (double[2]){0.0, 0.0},
                                      (double[2]){0.0, 0.0},
                                      compute_distance(&anchor_b, &body, &(ChronoDistanceConstraint2D_C){0}));
    chrono_distance_constraint2d_set_baumgarte(&constraint_b, 0.42);
    chrono_distance_constraint2d_set_softness_linear(&constraint_b, 0.005);
    chrono_distance_constraint2d_set_softness_angular(&constraint_b, 0.15);
    chrono_distance_constraint2d_set_slop(&constraint_b, 5e-4);
    chrono_distance_constraint2d_set_max_correction(&constraint_b, 0.07);
    chrono_distance_constraint2d_set_spring(&constraint_b, 32.0, 4.5);

    ChronoConstraint2DBase_C *constraints[2] = {&constraint_a.base, &constraint_b.base};

    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 18;
    cfg.position_iterations = 4;
    cfg.enable_parallel = 0;

    const double dt = 0.004;
    const int total_steps = 2200;
    double rest_a = constraint_a.rest_length;
    double rest_b = constraint_b.rest_length;
    double max_deviation_a = 0.0;
    double max_deviation_b = 0.0;
    double max_impulse_a = 0.0;
    double max_impulse_b = 0.0;

    for (int step = 0; step < total_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 2, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);

        double dist_a = compute_distance(&anchor_a, &body, &constraint_a);
        double dist_b = compute_distance(&anchor_b, &body, &constraint_b);
        double dev_a = fabs(dist_a - rest_a);
        double dev_b = fabs(dist_b - rest_b);
        if (dev_a > max_deviation_a) {
            max_deviation_a = dev_a;
        }
        if (dev_b > max_deviation_b) {
            max_deviation_b = dev_b;
        }
        if (fabs(constraint_a.last_impulse) > max_impulse_a) {
            max_impulse_a = fabs(constraint_a.last_impulse);
        }
        if (fabs(constraint_b.last_impulse) > max_impulse_b) {
            max_impulse_b = fabs(constraint_b.last_impulse);
        }
    }

    if (!isfinite(body.position[0]) || !isfinite(body.position[1]) || !isfinite(body.angle)) {
        fprintf(stderr, "Distance multi-constraint test failed: non-finite body state\n");
        return 1;
    }

    double final_dist_a = compute_distance(&anchor_a, &body, &constraint_a);
    double final_dist_b = compute_distance(&anchor_b, &body, &constraint_b);

    if (fabs(final_dist_a - rest_a) > 0.008 || fabs(final_dist_b - rest_b) > 0.008) {
        fprintf(stderr,
                "Distance multi-constraint test failed: rest lengths drifted (%.6f / %.6f)\n",
                final_dist_a - rest_a,
                final_dist_b - rest_b);
        return 1;
    }

    if (max_deviation_a > 0.015 || max_deviation_b > 0.015) {
        fprintf(stderr,
                "Distance multi-constraint test failed: excessive transient deviation (%.6f / %.6f)\n",
                max_deviation_a,
                max_deviation_b);
        return 1;
    }

    if (max_impulse_a < 1e-5 || max_impulse_b < 1e-5) {
        fprintf(stderr,
                "Distance multi-constraint test failed: insufficient impulses (%.6e / %.6e)\n",
                max_impulse_a,
                max_impulse_b);
        return 1;
    }

    printf("Distance multi-constraint test passed.\n");
    return 0;
}
