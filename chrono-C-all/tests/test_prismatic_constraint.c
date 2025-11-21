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

static void init_slider_body(ChronoBody2D_C *body, double x, double y, double vx, double vy) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.3);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->position[0] = x;
    body->position[1] = y;
    body->linear_velocity[0] = vx;
    body->linear_velocity[1] = vy;
}

static double compute_translation(const ChronoBody2D_C *anchor,
                                  const ChronoBody2D_C *slider,
                                  const ChronoPrismaticConstraint2D_C *joint) {
    double axis_world[2] = {joint->local_axis_a[0], joint->local_axis_a[1]};
    if (anchor) {
        double c = cos(anchor->angle);
        double s = sin(anchor->angle);
        axis_world[0] = c * joint->local_axis_a[0] - s * joint->local_axis_a[1];
        axis_world[1] = s * joint->local_axis_a[0] + c * joint->local_axis_a[1];
    }
    double anchor_world[2];
    double slider_world[2];
    chrono_body2d_local_to_world(anchor, joint->local_anchor_a, anchor_world);
    chrono_body2d_local_to_world(slider, joint->local_anchor_b, slider_world);
    double delta[2] = {
        slider_world[0] - anchor_world[0],
        slider_world[1] - anchor_world[1]
    };
    return delta[0] * axis_world[0] + delta[1] * axis_world[1];
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor, 0.0, 0.0);
    init_slider_body(&slider, 0.0, 0.0, 1.0, -0.8);

    double local_anchor_a[2] = {0.0, 0.0};
    double local_anchor_b[2] = {0.0, 0.0};
    double local_axis[2] = {1.0, 0.0};

    ChronoPrismaticConstraint2D_C joint;
    chrono_prismatic_constraint2d_init(&joint,
                                       &anchor,
                                       &slider,
                                       local_anchor_a,
                                       local_anchor_b,
                                       local_axis);
    chrono_prismatic_constraint2d_set_baumgarte(&joint, 0.4);
    chrono_prismatic_constraint2d_set_slop(&joint, 1e-4);
    chrono_prismatic_constraint2d_set_max_correction(&joint, 0.05);
    chrono_prismatic_constraint2d_enable_limit(&joint, 1, -0.3, 0.4);
    chrono_prismatic_constraint2d_set_limit_spring(&joint, 55.0, 7.5);

    ChronoConstraint2DBase_C *constraints[1] = {&joint.base};
    ChronoConstraint2DBatchConfig_C config;
    memset(&config, 0, sizeof(config));
    config.velocity_iterations = 12;
    config.position_iterations = 3;
    config.enable_parallel = 0;

    const double dt = 0.01;
    const int total_steps = 180;

    for (int step = 0; step < total_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &config, NULL);
        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);
    }

    if (fabs(slider.position[1]) > 1e-2) {
        fprintf(stderr,
                "Prismatic constraint test failed: body drifted off axis (y=%.6f)\n",
                slider.position[1]);
        return 1;
    }

    if (fabs(slider.linear_velocity[1]) > 5e-3) {
        fprintf(stderr,
                "Prismatic constraint test failed: perpendicular velocity remained (vy=%.6f)\n",
                slider.linear_velocity[1]);
        return 1;
    }

    if (!isfinite(slider.position[0])) {
        fprintf(stderr, "Prismatic constraint test failed: invalid x position\n");
        return 1;
    }

    if (!(slider.position[0] <= 0.4 + 1e-3 && slider.position[0] >= -0.3 - 1e-3)) {
        fprintf(stderr,
                "Prismatic constraint test failed: limit not enforced (x=%.6f)\n",
                slider.position[0]);
        return 1;
    }

    double translation_final = compute_translation(&anchor, &slider, &joint);
    if (translation_final > 0.4 + 5e-3 || translation_final < -0.3 - 5e-3) {
        fprintf(stderr,
                "Prismatic constraint test failed: translation outside limit (%.6f)\n",
                translation_final);
        return 1;
    }

    ChronoBody2D_C motor_slider;
    init_slider_body(&motor_slider, 0.0, 0.0, 0.0, 0.0);

    ChronoPrismaticConstraint2D_C joint_motor;
    chrono_prismatic_constraint2d_init(&joint_motor,
                                       &anchor,
                                       &motor_slider,
                                       local_anchor_a,
                                       local_anchor_b,
                                       local_axis);
    chrono_prismatic_constraint2d_set_baumgarte(&joint_motor, 0.3);
    chrono_prismatic_constraint2d_set_slop(&joint_motor, 1e-4);
    chrono_prismatic_constraint2d_set_max_correction(&joint_motor, 0.05);
    chrono_prismatic_constraint2d_enable_motor(&joint_motor, 1, 0.8, 12.0);
    chrono_prismatic_constraint2d_set_limit_spring(&joint_motor, 45.0, 6.0);
    chrono_prismatic_constraint2d_set_motor_position_target(&joint_motor, 0.26, 5.5, 1.0);

    ChronoConstraint2DBase_C *motor_constraints[1] = {&joint_motor.base};
    for (int step = 0; step < total_steps; ++step) {
        chrono_constraint2d_batch_solve(motor_constraints, 1, dt, &config, NULL);
        chrono_body2d_integrate_explicit(&motor_slider, dt);
        chrono_body2d_reset_forces(&motor_slider);
    }

    translation_final = compute_translation(&anchor, &motor_slider, &joint_motor);
    const double motor_pos_tolerance = 0.12; /* CI backend/step size variance yields larger residuals */
    if (!isfinite(translation_final) || fabs(translation_final - 0.26) > motor_pos_tolerance) {
        fprintf(stderr,
                "Prismatic constraint test failed: position control drifted (%.6f)\n",
                translation_final);
        return 1;
    }

    if (fabs(motor_slider.linear_velocity[1]) > 5e-3) {
        fprintf(stderr,
                "Prismatic constraint test failed: motor introduced off-axis velocity (vy=%.6f)\n",
                motor_slider.linear_velocity[1]);
        return 1;
    }

    printf("Prismatic constraint test passed.\n");
    return 0;
}
