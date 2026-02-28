#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

typedef struct PlanarDemoConfig {
    double dt;
    double total_time;
    int sample_stride;
    double linear_damping;
    double angular_damping;
    double motor_max_force;
    double motor_gain;
    double motor_damping;
    double initial_target_x;
    double initial_target_y;
    double second_target_x;
    double second_target_y;
    double third_target_x;
    double third_target_y;
    double target_orientation_a;
    double target_orientation_b;
    double target_orientation_c;
    double y_limit_lower;
    double y_limit_upper;
    double y_limit_stiffness;
    double y_limit_damping;
    double switch_time_targets;
    double switch_time_orientation;
    double switch_time_second_targets;
    double switch_time_limit_start;
    double switch_time_limit_end;
} PlanarDemoConfig;

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = 0.0;
    anchor->position[1] = 0.0;
}

static void init_slider(ChronoBody2D_C *slider) {
    chrono_body2d_init(slider);
    chrono_body2d_set_mass(slider, 1.1, 0.28);
    const double verts[4][2] = {
        {-0.16, -0.09},
        {0.16, -0.09},
        {0.16, 0.09},
        {-0.16, 0.09}
    };
    chrono_body2d_set_polygon_shape(slider, &verts[0][0], 4);
    slider->position[0] = -0.12;
    slider->position[1] = 0.18;
    slider->linear_velocity[0] = 0.0;
    slider->linear_velocity[1] = 0.0;
    slider->angular_velocity = 0.0;
}

static double compute_axis_translation(const ChronoBody2D_C *anchor,
                                       const ChronoBody2D_C *slider,
                                       const ChronoPlanarConstraint2D_C *constraint,
                                       int axis_index) {
    double world_anchor[2];
    double world_slider[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, world_anchor);
    chrono_body2d_local_to_world(slider, constraint->local_anchor_b, world_slider);
    double delta[2] = {
        world_slider[0] - world_anchor[0],
        world_slider[1] - world_anchor[1]
    };
    const double *axis = constraint->axis_world[axis_index];
    return delta[0] * axis[0] + delta[1] * axis[1];
}

static double compute_orientation_error(const ChronoBody2D_C *anchor,
                                        const ChronoBody2D_C *slider,
                                        const ChronoPlanarConstraint2D_C *constraint) {
    double angle_a = anchor ? anchor->angle : 0.0;
    double angle_b = slider ? slider->angle : 0.0;
    return (angle_b - angle_a) - constraint->orientation_target;
}

static void write_header(FILE *fp) {
    fprintf(fp,
            "time,pos_x,pos_y,angle,vx,vy,omega,"
            "translation_x,translation_y,orientation_error,"
            "motor_force_x,motor_force_y,"
            "limit_force_x,limit_force_y,"
            "limit_spring_force_x,limit_spring_force_y,"
            "motor_mode_x,motor_mode_y,"
            "motor_target_x,motor_target_y,"
            "orientation_target,"
            "limit_state_x,limit_state_y,"
            "limit_lower_y,limit_upper_y,"
            "stage\n");
}

int main(int argc, char **argv) {
    const char *output_path = "planar_constraint.csv";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        output_path = argv[1];
    }

    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file '%s'\n", output_path);
        return 1;
    }

    PlanarDemoConfig config;
    config.dt = 0.004;
    config.total_time = 6.5;
    config.sample_stride = 5;
    config.linear_damping = 0.999;
    config.angular_damping = 0.999;
    config.motor_max_force = 18.0;
    config.motor_gain = 3.5;
    config.motor_damping = 1.2;
    config.initial_target_x = 0.24;
    config.initial_target_y = -0.18;
    config.second_target_x = -0.14;
    config.second_target_y = -0.04;
    config.third_target_x = 0.18;
    config.third_target_y = 0.20;
    config.target_orientation_a = 0.0;
    config.target_orientation_b = 0.22;
    config.target_orientation_c = -0.18;
    config.y_limit_lower = -0.30;
    config.y_limit_upper = 0.05;
    config.y_limit_stiffness = 55.0;
    config.y_limit_damping = 8.0;
    config.switch_time_targets = 1.4;
    config.switch_time_orientation = 2.5;
    config.switch_time_second_targets = 3.4;
    config.switch_time_limit_start = 4.6;
    config.switch_time_limit_end = 5.4;

    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor);
    init_slider(&slider);

    ChronoPlanarConstraint2D_C planar;
    double local_anchor[2] = {0.0, 0.0};
    double axis_x[2] = {1.0, 0.0};
    chrono_planar_constraint2d_init(&planar,
                                    &anchor,
                                    &slider,
                                    local_anchor,
                                    local_anchor,
                                    axis_x);
    chrono_planar_constraint2d_set_baumgarte(&planar, 0.18);
    chrono_planar_constraint2d_set_slop(&planar, 1e-4);
    chrono_planar_constraint2d_set_max_correction(&planar, 0.08);
    chrono_planar_constraint2d_set_orientation_target(&planar, config.target_orientation_a);

    chrono_planar_constraint2d_enable_motor(&planar,
                                            CHRONO_PLANAR_AXIS_X,
                                            1,
                                            0.0,
                                            config.motor_max_force);
    chrono_planar_constraint2d_enable_motor(&planar,
                                            CHRONO_PLANAR_AXIS_Y,
                                            1,
                                            0.0,
                                            config.motor_max_force);
    chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                         CHRONO_PLANAR_AXIS_X,
                                                         config.initial_target_x,
                                                         config.motor_gain,
                                                         config.motor_damping);
    chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                         CHRONO_PLANAR_AXIS_Y,
                                                         config.initial_target_y,
                                                         config.motor_gain,
                                                         config.motor_damping);

    ChronoConstraint2DBase_C *constraints[1] = {&planar.base};
    ChronoConstraint2DBatchConfig_C solver_cfg;
    memset(&solver_cfg, 0, sizeof(solver_cfg));
    solver_cfg.velocity_iterations = 22;
    solver_cfg.position_iterations = 5;
    solver_cfg.enable_parallel = 0;

    const int total_steps = (int)(config.total_time / config.dt);
    const int switch_targets_step = (int)(config.switch_time_targets / config.dt);
    const int switch_orientation_step = (int)(config.switch_time_orientation / config.dt);
    const int switch_second_targets_step = (int)(config.switch_time_second_targets / config.dt);
    const int switch_limit_start_step = (int)(config.switch_time_limit_start / config.dt);
    const int switch_limit_end_step = (int)(config.switch_time_limit_end / config.dt);

    int stage = 0;
    int limit_active = 0;

    write_header(fp);

    double time = 0.0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == switch_targets_step) {
            chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                                 CHRONO_PLANAR_AXIS_X,
                                                                 config.second_target_x,
                                                                 config.motor_gain,
                                                                 config.motor_damping);
            chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                                 CHRONO_PLANAR_AXIS_Y,
                                                                 config.second_target_y,
                                                                 config.motor_gain,
                                                                 config.motor_damping);
            chrono_planar_constraint2d_set_orientation_target(&planar, config.target_orientation_b);
            stage = 1;
        } else if (step == switch_orientation_step) {
            chrono_planar_constraint2d_set_orientation_target(&planar, config.target_orientation_c);
            stage = 2;
        } else if (step == switch_second_targets_step) {
            chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                                 CHRONO_PLANAR_AXIS_X,
                                                                 config.third_target_x,
                                                                 config.motor_gain,
                                                                 config.motor_damping);
            chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                                 CHRONO_PLANAR_AXIS_Y,
                                                                 config.third_target_y,
                                                                 config.motor_gain,
                                                                 config.motor_damping);
            stage = 3;
        } else if (step == switch_limit_start_step) {
            chrono_planar_constraint2d_enable_motor(&planar,
                                                    CHRONO_PLANAR_AXIS_Y,
                                                    0,
                                                    0.0,
                                                    0.0);
            chrono_planar_constraint2d_enable_limit(&planar,
                                                    CHRONO_PLANAR_AXIS_Y,
                                                    1,
                                                    config.y_limit_lower,
                                                    config.y_limit_upper);
            chrono_planar_constraint2d_set_limit_spring(&planar,
                                                        CHRONO_PLANAR_AXIS_Y,
                                                        config.y_limit_stiffness,
                                                        config.y_limit_damping);
            slider.linear_velocity[1] = -2.2;
            limit_active = 1;
            stage = 4;
        } else if (step == switch_limit_end_step) {
            chrono_planar_constraint2d_enable_limit(&planar,
                                                    CHRONO_PLANAR_AXIS_Y,
                                                    0,
                                                    0.0,
                                                    0.0);
            chrono_planar_constraint2d_enable_motor(&planar,
                                                    CHRONO_PLANAR_AXIS_Y,
                                                    1,
                                                    0.0,
                                                    config.motor_max_force);
            chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                                 CHRONO_PLANAR_AXIS_Y,
                                                                 0.05,
                                                                 config.motor_gain,
                                                                 config.motor_damping);
            limit_active = 0;
            stage = 5;
        }

        chrono_constraint2d_batch_solve(constraints, 1, config.dt, &solver_cfg, NULL);

        slider.linear_velocity[0] *= config.linear_damping;
        slider.linear_velocity[1] *= config.linear_damping;
        slider.angular_velocity *= config.angular_damping;

        if (limit_active) {
            slider.force[1] += -45.0;
        }

        chrono_body2d_integrate_explicit(&slider, config.dt);
        chrono_body2d_reset_forces(&slider);

        time += config.dt;
        if (step % config.sample_stride == 0) {
            double tx = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_X);
            double ty = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
            double orientation_error = compute_orientation_error(&anchor, &slider, &planar);
            fprintf(fp,
                    "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                    "%.6f,%.6f,%.6f,"
                    "%.6f,%.6f,"
                    "%.6f,%.6f,"
                    "%.6f,%.6f,"
                    "%d,%d,"
                    "%.6f,%.6f,"
                    "%.6f,"
                    "%d,%d,"
                    "%.6f,%.6f,"
                    "%d\n",
                    time,
                    slider.position[0],
                    slider.position[1],
                    slider.angle,
                    slider.linear_velocity[0],
                    slider.linear_velocity[1],
                    slider.angular_velocity,
                    tx,
                    ty,
                    orientation_error,
                    planar.last_motor_force[CHRONO_PLANAR_AXIS_X],
                    planar.last_motor_force[CHRONO_PLANAR_AXIS_Y],
                    planar.last_limit_force[CHRONO_PLANAR_AXIS_X],
                    planar.last_limit_force[CHRONO_PLANAR_AXIS_Y],
                    planar.last_limit_spring_force[CHRONO_PLANAR_AXIS_X],
                    planar.last_limit_spring_force[CHRONO_PLANAR_AXIS_Y],
                    planar.motor_mode[CHRONO_PLANAR_AXIS_X],
                    planar.motor_mode[CHRONO_PLANAR_AXIS_Y],
                    planar.motor_position_target[CHRONO_PLANAR_AXIS_X],
                    planar.motor_position_target[CHRONO_PLANAR_AXIS_Y],
                    planar.orientation_target,
                    planar.limit_state[CHRONO_PLANAR_AXIS_X],
                    planar.limit_state[CHRONO_PLANAR_AXIS_Y],
                    config.y_limit_lower,
                    config.y_limit_upper,
                    stage);
        }
    }

    fclose(fp);
    printf("Planar constraint demo complete. Data written to %s\n", output_path);
    return 0;
}
