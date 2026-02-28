#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static void init_body(ChronoBody2D_C *body, double angle, double inertia) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, inertia);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->angle = angle;
}

int main(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    init_body(&body_a, 0.0, 0.35);
    init_body(&body_b, 0.2, 0.45);

    body_a.angular_velocity = 1.5;
    body_b.angular_velocity = -0.75;

    ChronoGearConstraint2D_C gear;
    chrono_gear_constraint2d_init(&gear, &body_a, &body_b, 2.0, 0.0);
    chrono_gear_constraint2d_set_baumgarte(&gear, 0.3);
    chrono_gear_constraint2d_set_softness(&gear, 0.0);

    ChronoConstraint2DBase_C *constraints[1] = {&gear.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 15;
    cfg.position_iterations = 3;
    cfg.enable_parallel = 0;

    const double dt = 0.01;
    const int steps = 200;

    for (int i = 0; i < steps; ++i) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body_a, dt);
        chrono_body2d_integrate_explicit(&body_b, dt);
        chrono_body2d_reset_forces(&body_a);
        chrono_body2d_reset_forces(&body_b);
    }

    double relation = body_a.angular_velocity + 2.0 * body_b.angular_velocity;
    if (fabs(relation) > 5e-2) {
        fprintf(stderr, "Gear constraint failed: angular velocity relation drifted (%.6f)\n", relation);
        return 1;
    }

    double angle_relation = (body_a.angle + 2.0 * body_b.angle);
    if (fabs(angle_relation) > 0.08) {
        fprintf(stderr, "Gear constraint failed: angle relation drifted (%.6f)\n", angle_relation);
        return 1;
    }

    printf("Gear constraint test passed.\n");
    return 0;
}
