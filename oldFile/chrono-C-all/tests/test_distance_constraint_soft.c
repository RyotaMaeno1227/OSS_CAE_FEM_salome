#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static double compute_distance(const ChronoBody2D_C *anchor,
                               const ChronoBody2D_C *body,
                               const double local_anchor_a[2],
                               const double local_anchor_b[2]) {
    double world_a[2];
    double world_b[2];
    chrono_body2d_local_to_world(anchor, local_anchor_a, world_a);
    chrono_body2d_local_to_world(body, local_anchor_b, world_b);
    double dx = world_b[0] - world_a[0];
    double dy = world_b[1] - world_a[1];
    return sqrt(dx * dx + dy * dy);
}

int main(void) {
    ChronoBody2D_C anchor;
    chrono_body2d_init(&anchor);
    chrono_body2d_set_static(&anchor);
    anchor.position[0] = 0.0;
    anchor.position[1] = 0.0;

    ChronoBody2D_C body;
    chrono_body2d_init(&body);
    chrono_body2d_set_mass(&body, 1.0, 0.2);
    body.position[0] = 1.3;
    body.position[1] = 0.0;

    double local_anchor_a[2] = {0.0, 0.0};
    double local_anchor_b[2] = {0.0, 0.0};

    ChronoDistanceConstraint2D_C constraint;
    chrono_distance_constraint2d_init(&constraint, &anchor, &body, local_anchor_a, local_anchor_b, 1.0);
    chrono_distance_constraint2d_set_baumgarte(&constraint, 0.0);
    chrono_distance_constraint2d_set_slop(&constraint, 1e-4);
    chrono_distance_constraint2d_set_max_correction(&constraint, 0.0);
    chrono_distance_constraint2d_set_softness(&constraint, 3.5);
    chrono_distance_constraint2d_set_spring(&constraint, 35.0, 6.0);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 12;
    cfg.position_iterations = 1;
    cfg.enable_parallel = 0;

    const double dt = 0.01;
    double distance = compute_distance(&anchor, &body, local_anchor_a, local_anchor_b);
    double max_force = 0.0;

    for (int step = 0; step < 400; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);

        distance = compute_distance(&anchor, &body, local_anchor_a, local_anchor_b);
        if (fabs(constraint.last_spring_force) > max_force) {
            max_force = fabs(constraint.last_spring_force);
        }
    }

    if (!isfinite(distance) || !isfinite(max_force)) {
        fprintf(stderr, "Distance constraint soft test failed: non-finite result.\n");
        return 1;
    }

    if (distance < 0.95 || distance > 1.05) {
        fprintf(stderr,
                "Distance constraint soft test failed: distance did not converge (distance=%.6f).\n",
                distance);
        return 1;
    }

    if (max_force < 1e-3) {
        fprintf(stderr, "Distance constraint soft test failed: spring force remained near zero.\n");
        return 1;
    }

    printf("Distance constraint soft test passed (distance=%.6f, max spring force=%.4f).\n",
           distance,
           max_force);
    return 0;
}
