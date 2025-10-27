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

static void init_slider(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.3);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->position[0] = x;
    body->position[1] = y;
}

static double compute_axis_translation(const ChronoBody2D_C *anchor,
                                       const ChronoBody2D_C *slider,
                                       const ChronoPlanarConstraint2D_C *constraint,
                                       int axis_index) {
    double axis[2] = {constraint->axis_world[axis_index][0], constraint->axis_world[axis_index][1]};
    double world_anchor[2];
    double world_slider[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, world_anchor);
    chrono_body2d_local_to_world(slider, constraint->local_anchor_b, world_slider);
    double delta[2] = {world_slider[0] - world_anchor[0], world_slider[1] - world_anchor[1]};
    return delta[0] * axis[0] + delta[1] * axis[1];
}

static double compute_axis_velocity(const ChronoBody2D_C *slider,
                                    const ChronoPlanarConstraint2D_C *constraint,
                                    int axis_index) {
    const double *axis = constraint->axis_world[axis_index];
    return slider->linear_velocity[0] * axis[0] + slider->linear_velocity[1] * axis[1];
}

static double compute_kinetic_energy(const ChronoBody2D_C *body) {
    double mass = (body->inverse_mass > 0.0) ? 1.0 / body->inverse_mass : 0.0;
    double inertia = (body->inverse_inertia > 0.0) ? 1.0 / body->inverse_inertia : 0.0;
    double linear = body->linear_velocity[0] * body->linear_velocity[0] +
                    body->linear_velocity[1] * body->linear_velocity[1];
    double rotational = body->angular_velocity * body->angular_velocity;
    return 0.5 * mass * linear + 0.5 * inertia * rotational;
}

typedef struct SetpointTracker {
    double target;
    int last_change_step;
} SetpointTracker;

static void update_setpoint(SetpointTracker *tracker, double value, int step) {
    tracker->target = value;
    tracker->last_change_step = step;
}

static int settled(const SetpointTracker *tracker, int current_step, int settle_frames) {
    return (current_step - tracker->last_change_step) >= settle_frames;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor, 0.0, 0.0);
    init_slider(&slider, -0.05, 0.25);

    ChronoPlanarConstraint2D_C planar;
    double local_anchor[2] = {0.0, 0.0};
    double axis_x[2] = {1.0, 0.0};
    chrono_planar_constraint2d_init(&planar, &anchor, &slider, local_anchor, local_anchor, axis_x);
    chrono_planar_constraint2d_set_baumgarte(&planar, 0.15);
    chrono_planar_constraint2d_set_slop(&planar, 1e-4);
    chrono_planar_constraint2d_set_max_correction(&planar, 0.08);
    chrono_planar_constraint2d_set_orientation_target(&planar, 0.0);

    const double motor_frequency = 3.5;
    const double motor_damping = 1.2;
    chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_X, 1, 0.0, 18.0);
    chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 1, 0.0, 18.0);
    chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, 0.25, motor_frequency, motor_damping);
    chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, -0.18, motor_frequency, motor_damping);

    ChronoConstraint2DBase_C *constraints[1] = {&planar.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.01;
    const int total_steps = 6000;
    const int settle_frames = 220;

    SetpointTracker tracker_x = {0.25, 0};
    SetpointTracker tracker_y = {-0.18, 0};
    SetpointTracker tracker_angle = {0.0, 0};

    double max_pos_error_x = 0.0;
    double max_pos_error_y = 0.0;
    double max_angle_error = 0.0;
    double max_axis_velocity_x = 0.0;
    double max_axis_velocity_y = 0.0;
    double max_kinetic_energy = 0.0;
    double max_motor_force_x = 0.0;
    double max_motor_force_y = 0.0;

    int limit_active = 0;
    double max_limit_penetration = 0.0;

    const int change_step_pos = 1000;
    const int change_step_angle = 2200;
    const int change_step_second = 3200;
    const int limit_start = 3600;
    const int limit_end = 4700;

    for (int step = 0; step < total_steps; ++step) {
        if (step == change_step_pos) {
            double new_target_x = -0.12;
            double new_target_y = -0.05;
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, new_target_x, motor_frequency, motor_damping);
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, new_target_y, motor_frequency, motor_damping);
            update_setpoint(&tracker_x, new_target_x, step);
            update_setpoint(&tracker_y, new_target_y, step);
        } else if (step == change_step_angle) {
            double new_angle = 0.25;
            chrono_planar_constraint2d_set_orientation_target(&planar, new_angle);
            update_setpoint(&tracker_angle, new_angle, step);
        } else if (step == change_step_second) {
            double new_target_x = 0.15;
            double new_target_y = 0.20;
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, new_target_x, motor_frequency, motor_damping);
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, new_target_y, motor_frequency, motor_damping);
            chrono_planar_constraint2d_set_orientation_target(&planar, -0.2);
            update_setpoint(&tracker_x, new_target_x, step);
            update_setpoint(&tracker_y, new_target_y, step);
            update_setpoint(&tracker_angle, -0.2, step);
        } else if (step == limit_start) {
            chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 0, 0.0, 0.0);
            chrono_planar_constraint2d_enable_limit(&planar, CHRONO_PLANAR_AXIS_Y, 1, -0.3, 0.04);
            chrono_planar_constraint2d_set_limit_spring(&planar, CHRONO_PLANAR_AXIS_Y, 55.0, 8.0);
            slider.linear_velocity[1] = -2.5;
            update_setpoint(&tracker_y, -0.3, step);
            limit_active = 1;
        } else if (step == limit_end) {
            chrono_planar_constraint2d_enable_limit(&planar, CHRONO_PLANAR_AXIS_Y, 0, 0.0, 0.0);
            chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 1, 0.0, 18.0);
            double new_target_y = 0.05;
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, new_target_y, motor_frequency, motor_damping);
            update_setpoint(&tracker_y, new_target_y, step);
            limit_active = 0;
        }

        if (limit_active) {
            slider.force[1] += -30.0;
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);

        double translation_x = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_X);
        double translation_y = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
        double axis_vel_x = compute_axis_velocity(&slider, &planar, CHRONO_PLANAR_AXIS_X);
        double axis_vel_y = compute_axis_velocity(&slider, &planar, CHRONO_PLANAR_AXIS_Y);
        double angle_error = (slider.angle - anchor.angle) - planar.orientation_target;
        double kinetic = compute_kinetic_energy(&slider);

        if (settled(&tracker_x, step, settle_frames)) {
            double err_x = fabs(translation_x - tracker_x.target);
            if (err_x > max_pos_error_x) {
                max_pos_error_x = err_x;
            }
            double abs_vx = fabs(axis_vel_x);
            if (abs_vx > max_axis_velocity_x) {
                max_axis_velocity_x = abs_vx;
            }
        }

        if (!limit_active && settled(&tracker_y, step, settle_frames)) {
            double err_y = fabs(translation_y - tracker_y.target);
            if (err_y > max_pos_error_y) {
                max_pos_error_y = err_y;
            }
            double abs_vy = fabs(axis_vel_y);
            if (abs_vy > max_axis_velocity_y) {
                max_axis_velocity_y = abs_vy;
            }
        }

        if (!limit_active && settled(&tracker_angle, step, settle_frames)) {
            double abs_angle = fabs(angle_error);
            if (abs_angle > max_angle_error) {
                max_angle_error = abs_angle;
            }
        }

        if (kinetic > max_kinetic_energy) {
            max_kinetic_energy = kinetic;
        }

        if (!limit_active) {
            if (fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_X]) > max_motor_force_x) {
                max_motor_force_x = fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_X]);
            }
            if (fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_Y]) > max_motor_force_y) {
                max_motor_force_y = fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_Y]);
            }
        } else {
            double penetration = 0.0;
            if (translation_y < planar.limit_lower[CHRONO_PLANAR_AXIS_Y]) {
                penetration = planar.limit_lower[CHRONO_PLANAR_AXIS_Y] - translation_y;
            } else if (translation_y > planar.limit_upper[CHRONO_PLANAR_AXIS_Y]) {
                penetration = translation_y - planar.limit_upper[CHRONO_PLANAR_AXIS_Y];
            }
            if (penetration > max_limit_penetration) {
                max_limit_penetration = penetration;
            }
        }
    }

    if (max_pos_error_x > 0.05) {
        fprintf(stderr, "Planar longrun failed: X-axis steady error too high (%.6f)\n", max_pos_error_x);
        return 1;
    }
    if (max_pos_error_y > 0.05) {
        fprintf(stderr, "Planar longrun failed: Y-axis steady error too high (%.6f)\n", max_pos_error_y);
        return 1;
    }
    if (max_angle_error > 0.06) {
        fprintf(stderr, "Planar longrun failed: orientation error too high (%.6f)\n", max_angle_error);
        return 1;
    }
    if (max_axis_velocity_x > 0.6) {
        fprintf(stderr, "Planar longrun failed: residual X velocity too high (%.6f)\n", max_axis_velocity_x);
        return 1;
    }
    if (max_axis_velocity_y > 0.6) {
        fprintf(stderr, "Planar longrun failed: residual Y velocity too high (%.6f)\n", max_axis_velocity_y);
        return 1;
    }
    if (max_kinetic_energy > 7.0) {
        fprintf(stderr, "Planar longrun failed: kinetic energy spiked (%.6f)\n", max_kinetic_energy);
        return 1;
    }
    if (max_motor_force_x > 40.0 || max_motor_force_y > 40.0) {
        fprintf(stderr, "Planar longrun failed: motor force out of range (%.6f / %.6f)\n",
                max_motor_force_x, max_motor_force_y);
        return 1;
    }
    if (max_limit_penetration > 0.02) {
        fprintf(stderr, "Planar longrun failed: limit penetration too large (%.6f)\n", max_limit_penetration);
        return 1;
    }

    printf("Planar constraint longrun test passed.\n");
    printf("Max steady errors: X=%.5f Y=%.5f angle=%.5f rad\n", max_pos_error_x, max_pos_error_y, max_angle_error);
    printf("Max residual velocities: X=%.5f Y=%.5f\n", max_axis_velocity_x, max_axis_velocity_y);
    printf("Max kinetic energy: %.5f, peak motor force X=%.5f Y=%.5f\n",
           max_kinetic_energy, max_motor_force_x, max_motor_force_y);
    printf("Max limit penetration: %.5f\n", max_limit_penetration);
    return 0;
}
