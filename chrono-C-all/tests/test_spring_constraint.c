#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static void init_anchor(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_static(body);
    body->position[0] = x;
    body->position[1] = y;
}

static void init_mass(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(body, 0.15);
    body->position[0] = x;
    body->position[1] = y;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C mass;
    init_anchor(&anchor, 0.0, 0.0);
    init_mass(&mass, 0.0, -1.4);

    double local_anchor_a[2] = {0.0, 0.0};
    double local_anchor_b[2] = {0.0, 0.0};

    ChronoSpringConstraint2D_C spring;
    chrono_spring_constraint2d_init(&spring,
                                    &anchor,
                                    &mass,
                                    local_anchor_a,
                                    local_anchor_b,
                                    1.0,
                                    45.0,
                                    8.0);

    ChronoConstraint2DBase_C *constraints[1] = {&spring.base};
    ChronoConstraint2DBatchConfig_C config;
    memset(&config, 0, sizeof(config));
    config.velocity_iterations = 8;
    config.position_iterations = 1;
    config.enable_parallel = 0;

    const double dt = 0.01;
    const int total_steps = 400;

    for (int step = 0; step < total_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &config, NULL);
        chrono_body2d_integrate_explicit(&mass, dt);
        chrono_body2d_reset_forces(&mass);
    }

    double displacement = fabs(mass.position[1] + 1.0);
    if (!isfinite(displacement) || displacement > 0.03) {
        fprintf(stderr,
                "Spring constraint test failed: displacement remained high (%.6f)\n",
                displacement);
        return 1;
    }

    if (fabs(mass.linear_velocity[1]) > 0.1) {
        fprintf(stderr,
                "Spring constraint test failed: velocity not damped (vy=%.6f)\n",
                mass.linear_velocity[1]);
        return 1;
    }

    printf("Spring constraint test passed.\n");
    return 0;
}
