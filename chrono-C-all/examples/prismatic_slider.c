#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

typedef struct PrismaticDemoConfig {
    double dt;
    double total_time;
    double limit_lower;
    double limit_upper;
    double motor_speed_forward;
    double motor_speed_reverse;
    double motor_max_force;
    double damping;
    int switch_step;
    int sample_stride;
} PrismaticDemoConfig;

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = 0.0;
    anchor->position[1] = 0.0;
}

static void init_slider(ChronoBody2D_C *slider) {
    chrono_body2d_init(slider);
    chrono_body2d_set_mass(slider, 1.0, 0.25);
    chrono_body2d_set_circle_shape(slider, 0.18);
    slider->position[0] = -0.15;
    slider->position[1] = 0.0;
    slider->linear_velocity[0] = 0.0;
    slider->linear_velocity[1] = 0.0;
}

static double compute_translation(const ChronoBody2D_C *anchor,
                                  const ChronoBody2D_C *slider,
                                  const ChronoPrismaticConstraint2D_C *joint) {
    double axis_world[2] = {joint->local_axis_a[0], joint->local_axis_a[1]};
    if (anchor) {
        double angle = anchor->angle;
        double c = cos(angle);
        double s = sin(angle);
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

static void write_header(FILE *fp) {
    fprintf(fp,
            "time,x,y,vx,vy,translation,motor_impulse,limit_impulse,"
            "motor_speed,limit_state,limit_lower,limit_upper\n");
}

int main(int argc, char **argv) {
    const char *output_path = "prismatic_slider.csv";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        output_path = argv[1];
    }
    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file '%s'\n", output_path);
        return 1;
    }

    PrismaticDemoConfig config;
    config.dt = 0.002;
    config.total_time = 6.0;
    config.limit_lower = -0.4;
    config.limit_upper = 0.45;
    config.motor_speed_forward = 0.8;
    config.motor_speed_reverse = -0.5;
    config.motor_max_force = 18.0;
    config.damping = 0.999;
    config.sample_stride = 5;
    config.switch_step = (int)(2.8 / config.dt);

    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor);
    init_slider(&slider);

    ChronoPrismaticConstraint2D_C joint;
    double local_anchor[2] = {0.0, 0.0};
    double local_axis[2] = {1.0, 0.0};
    chrono_prismatic_constraint2d_init(&joint,
                                       &anchor,
                                       &slider,
                                       local_anchor,
                                       local_anchor,
                                       local_axis);
    chrono_prismatic_constraint2d_set_baumgarte(&joint, 0.3);
    chrono_prismatic_constraint2d_set_slop(&joint, 1e-4);
    chrono_prismatic_constraint2d_set_max_correction(&joint, 0.05);
    chrono_prismatic_constraint2d_enable_limit(&joint,
                                               1,
                                               config.limit_lower,
                                               config.limit_upper);
    chrono_prismatic_constraint2d_enable_motor(&joint,
                                               1,
                                               config.motor_speed_forward,
                                               config.motor_max_force);

    ChronoConstraint2DBase_C *constraints[1] = {&joint.base};
    ChronoConstraint2DBatchConfig_C solver_cfg;
    memset(&solver_cfg, 0, sizeof(solver_cfg));
    solver_cfg.velocity_iterations = 18;
    solver_cfg.position_iterations = 4;
    solver_cfg.enable_parallel = 0;

    write_header(fp);

    const int total_steps = (int)(config.total_time / config.dt);
    double time = 0.0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == config.switch_step) {
            chrono_prismatic_constraint2d_enable_motor(&joint,
                                                       1,
                                                       config.motor_speed_reverse,
                                                       config.motor_max_force);
        }

        double prev_motor_impulse = joint.accumulated_motor_impulse;
        double prev_limit_impulse = joint.limit_accumulated_impulse;

        chrono_constraint2d_batch_solve(constraints, 1, config.dt, &solver_cfg, NULL);

        double motor_impulse = (joint.accumulated_motor_impulse - prev_motor_impulse) / config.dt;
        double limit_impulse = (joint.limit_accumulated_impulse - prev_limit_impulse) / config.dt;

        slider.linear_velocity[0] *= config.damping;
        slider.linear_velocity[1] *= config.damping;

        chrono_body2d_integrate_explicit(&slider, config.dt);
        chrono_body2d_reset_forces(&slider);

        time += config.dt;
        if (step % config.sample_stride == 0) {
            double translation = compute_translation(&anchor, &slider, &joint);
            fprintf(fp,
                    "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f\n",
                    time,
                    slider.position[0],
                    slider.position[1],
                    slider.linear_velocity[0],
                    slider.linear_velocity[1],
                    translation,
                    motor_impulse,
                    limit_impulse,
                    joint.motor_speed,
                    joint.limit_state,
                    config.limit_lower,
                    config.limit_upper);
        }
    }

    fclose(fp);
    printf("Prismatic slider simulation complete. Data written to %s\n", output_path);
    return 0;
}
