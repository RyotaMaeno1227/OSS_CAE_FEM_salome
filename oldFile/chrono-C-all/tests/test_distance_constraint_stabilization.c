#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static void run_step(ChronoDistanceConstraint2D_C *constraint, double dt, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        chrono_distance_constraint2d_prepare(constraint, dt);
        if (i == 0) {
            chrono_distance_constraint2d_apply_warm_start(constraint);
        }
        chrono_distance_constraint2d_solve_velocity(constraint);
    }
    for (int i = 0; i < iterations; ++i) {
        chrono_distance_constraint2d_prepare(constraint, dt);
        chrono_distance_constraint2d_solve_position(constraint);
    }
}

int main(void) {
    ChronoBody2D_C anchor;
    chrono_body2d_init(&anchor);
    chrono_body2d_set_static(&anchor);

    ChronoBody2D_C body;
    chrono_body2d_init(&body);
    chrono_body2d_set_mass(&body, 1.0, 1.0);
    body.position[0] = 2.0;
    body.position[1] = 0.0;

    double local_a[2] = {0.0, 0.0};
    double local_b[2] = {0.0, 0.0};

    ChronoDistanceConstraint2D_C constraint;
    chrono_distance_constraint2d_init(&constraint, &anchor, &body, local_a, local_b, 1.0);
    chrono_distance_constraint2d_set_baumgarte(&constraint, 0.65);
    chrono_distance_constraint2d_set_softness(&constraint, 0.02);
    chrono_distance_constraint2d_set_slop(&constraint, 0.0005);
    chrono_distance_constraint2d_set_max_correction(&constraint, 0.2);

    double dt = 0.016;

    for (int step = 0; step < 180; ++step) {
        run_step(&constraint, dt, 5);
        double pa[2];
        chrono_body2d_local_to_world(&anchor, local_a, pa);
        double pb[2];
        chrono_body2d_local_to_world(&body, local_b, pb);
        double dx = pb[0] - pa[0];
        double dy = pb[1] - pa[1];
        double distance = sqrt(dx * dx + dy * dy);
        printf("step=%d distance=%.6f\n", step, distance);
        if (fabs(distance - 1.0) <= 1e-3) {
            printf("Constraint stabilized within tolerance after %d steps.\n", step);
            break;
        }
    }

    return 0;
}

