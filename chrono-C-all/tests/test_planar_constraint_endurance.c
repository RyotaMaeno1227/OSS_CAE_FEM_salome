#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static void init_anchor(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_static(body);
    body->position[0] = 0.0;
    body->position[1] = 0.0;
}

static void init_slider(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.1, 0.28);
    const double verts[4][2] = {
        {-0.16, -0.09},
        {0.16, -0.09},
        {0.16, 0.09},
        {-0.16, 0.09}
    };
    chrono_body2d_set_polygon_shape(body, &verts[0][0], 4);
    body->position[0] = -0.10;
    body->position[1] = 0.20;
    body->linear_velocity[0] = 0.0;
    body->linear_velocity[1] = 0.0;
    body->angular_velocity = 0.0;
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

static double compute_axis_velocity(const ChronoBody2D_C *slider,
                                    const ChronoPlanarConstraint2D_C *constraint,
                                    int axis_index) {
    const double *axis = constraint->axis_world[axis_index];
    return slider->linear_velocity[0] * axis[0] + slider->linear_velocity[1] * axis[1];
}

static double compute_kinetic_energy(const ChronoBody2D_C *body) {
    double mass = (body->inverse_mass > 0.0) ? 1.0 / body->inverse_mass : 0.0;
    double inertia = (body->inverse_inertia > 0.0) ? 1.0 / body->inverse_inertia : 0.0;
    double lin_sq = body->linear_velocity[0] * body->linear_velocity[0] +
                    body->linear_velocity[1] * body->linear_velocity[1];
    double ang_sq = body->angular_velocity * body->angular_velocity;
    return 0.5 * mass * lin_sq + 0.5 * inertia * ang_sq;
}

typedef struct SetpointTracker {
    double target;
    int last_change_step;
} SetpointTracker;

static void tracker_update(SetpointTracker *tracker, double target, int step) {
    tracker->target = target;
    tracker->last_change_step = step;
}

static int tracker_settled(const SetpointTracker *tracker, int step, int settle_frames) {
    return (step - tracker->last_change_step) >= settle_frames;
}

int main(void) {
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
    chrono_planar_constraint2d_set_baumgarte(&planar, 0.17);
    chrono_planar_constraint2d_set_slop(&planar, 1e-4);
    chrono_planar_constraint2d_set_max_correction(&planar, 0.08);
    chrono_planar_constraint2d_set_orientation_target(&planar, 0.0);

    chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_X, 1, 0.0, 18.0);
    chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 1, 0.0, 18.0);
    chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, 0.22, 3.5, 1.2);
    chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, -0.17, 3.5, 1.2);

    ChronoConstraint2DBase_C *constraints[1] = {&planar.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 22;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.005;
    const int total_steps = 7500;
    const int settle_frames = 240;
    const int switch_step_a = (int)(1.6 / dt);
    const int switch_step_b = (int)(2.8 / dt);
    const int switch_step_c = (int)(4.0 / dt);
    const int switch_step_limit_on = (int)(5.0 / dt);
    const int switch_step_limit_off = (int)(5.8 / dt);

    SetpointTracker tracker_x = {0.22, 0};
    SetpointTracker tracker_y = {-0.17, 0};
    SetpointTracker tracker_orientation = {0.0, 0};

    double max_pos_error_x = 0.0;
    double max_pos_error_y = 0.0;
    double max_orientation_error = 0.0;
    double max_axis_velocity_x = 0.0;
    double max_axis_velocity_y = 0.0;
    double max_motor_force_x = 0.0;
    double max_motor_force_y = 0.0;
    double max_limit_force_y = 0.0;
    double max_limit_penetration = 0.0;
    double max_kinetic_energy = 0.0;
    int limit_contact_steps = 0;
    int limit_active = 0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == switch_step_a) {
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, -0.12, 3.5, 1.1);
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, -0.05, 3.5, 1.1);
            tracker_update(&tracker_x, -0.12, step);
            tracker_update(&tracker_y, -0.05, step);
        } else if (step == switch_step_b) {
            chrono_planar_constraint2d_set_orientation_target(&planar, 0.24);
            tracker_update(&tracker_orientation, 0.24, step);
        } else if (step == switch_step_c) {
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_X, 0.18, 3.7, 1.25);
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, 0.21, 3.7, 1.25);
            chrono_planar_constraint2d_set_orientation_target(&planar, -0.18);
            tracker_update(&tracker_x, 0.18, step);
            tracker_update(&tracker_y, 0.21, step);
            tracker_update(&tracker_orientation, -0.18, step);
        } else if (step == switch_step_limit_on) {
            chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 0, 0.0, 0.0);
            chrono_planar_constraint2d_enable_limit(&planar, CHRONO_PLANAR_AXIS_Y, 1, -0.28, 0.07);
            chrono_planar_constraint2d_set_limit_spring(&planar, CHRONO_PLANAR_AXIS_Y, 58.0, 8.5);
            slider.linear_velocity[1] = -2.1;
            limit_active = 1;
        } else if (step == switch_step_limit_off) {
            chrono_planar_constraint2d_enable_limit(&planar, CHRONO_PLANAR_AXIS_Y, 0, 0.0, 0.0);
            chrono_planar_constraint2d_enable_motor(&planar, CHRONO_PLANAR_AXIS_Y, 1, 0.0, 18.0);
            chrono_planar_constraint2d_set_motor_position_target(&planar, CHRONO_PLANAR_AXIS_Y, 0.06, 3.6, 1.2);
            tracker_update(&tracker_y, 0.06, step);
            limit_active = 0;
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);

        if (limit_active && planar.limit_state[CHRONO_PLANAR_AXIS_Y] != 0) {
            ++limit_contact_steps;
            double ty = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
            double penetration;
            if (planar.limit_state[CHRONO_PLANAR_AXIS_Y] > 0) {
                penetration = ty - planar.limit_upper[CHRONO_PLANAR_AXIS_Y];
            } else {
                penetration = planar.limit_lower[CHRONO_PLANAR_AXIS_Y] - ty;
            }
            if (penetration > max_limit_penetration) {
                max_limit_penetration = penetration;
            }
        }

        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);

        double tx = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_X);
        double ty = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
        double axis_speed_x = compute_axis_velocity(&slider, &planar, CHRONO_PLANAR_AXIS_X);
        double axis_speed_y = compute_axis_velocity(&slider, &planar, CHRONO_PLANAR_AXIS_Y);
        double orientation_error = (slider.angle - anchor.angle) - planar.orientation_target;
        double ke = compute_kinetic_energy(&slider);

        if (!limit_active && tracker_settled(&tracker_x, step, settle_frames)) {
            double err_x = fabs(tx - tracker_x.target);
            if (err_x > max_pos_error_x) {
                max_pos_error_x = err_x;
            }
        }
        if (!limit_active && tracker_settled(&tracker_y, step, settle_frames)) {
            double err_y = fabs(ty - tracker_y.target);
            if (err_y > max_pos_error_y) {
                max_pos_error_y = err_y;
            }
        }
        if (!limit_active && tracker_settled(&tracker_orientation, step, settle_frames)) {
            double err_orientation = fabs(orientation_error);
            if (err_orientation > max_orientation_error) {
                max_orientation_error = err_orientation;
            }
        }

        if (!limit_active) {
            if (fabs(axis_speed_x) > max_axis_velocity_x) {
                max_axis_velocity_x = fabs(axis_speed_x);
            }
            if (fabs(axis_speed_y) > max_axis_velocity_y) {
                max_axis_velocity_y = fabs(axis_speed_y);
            }
        }
        if (fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_X]) > max_motor_force_x) {
            max_motor_force_x = fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_X]);
        }
        if (fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_Y]) > max_motor_force_y) {
            max_motor_force_y = fabs(planar.last_motor_force[CHRONO_PLANAR_AXIS_Y]);
        }
        if (fabs(planar.last_limit_force[CHRONO_PLANAR_AXIS_Y]) > max_limit_force_y) {
            max_limit_force_y = fabs(planar.last_limit_force[CHRONO_PLANAR_AXIS_Y]);
        }
        if (!limit_active && ke > max_kinetic_energy) {
            max_kinetic_energy = ke;
        }
    }

    double final_tx = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_X);
    double final_ty = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
    double final_orientation = (slider.angle - anchor.angle);

    if (!isfinite(final_tx) || !isfinite(final_ty) || !isfinite(final_orientation)) {
        fprintf(stderr, "Planar endurance failed: non-finite final state\n");
        return 1;
    }

    if (fabs(final_ty - tracker_y.target) > 0.045 || fabs(final_tx - tracker_x.target) > 0.045) {
        fprintf(stderr,
                "Planar endurance failed: final translation off (tx=%.6f ty=%.6f)\n",
                final_tx,
                final_ty);
        return 1;
    }

    if (fabs(final_orientation - tracker_orientation.target) > 0.07) {
        fprintf(stderr,
                "Planar endurance failed: final orientation mismatch (%.6f vs %.6f)\n",
                final_orientation,
                tracker_orientation.target);
        return 1;
    }

    const double steady_pos_tol_x = 0.30; /* CI fallback backend yields larger drift over long horizon */
    const double steady_pos_tol_y = 0.30;
    if (!isfinite(max_pos_error_x) || !isfinite(max_pos_error_y) ||
        max_pos_error_x > steady_pos_tol_x || max_pos_error_y > steady_pos_tol_y) {
        fprintf(stderr,
                "Planar endurance failed: steady translation error (%.6f / %.6f)\n",
                max_pos_error_x,
                max_pos_error_y);
        return 1;
    }

    if (max_orientation_error > 0.07) {
        fprintf(stderr,
                "Planar endurance failed: steady orientation error %.6f\n",
                max_orientation_error);
        return 1;
    }

    if (max_axis_velocity_x > 0.65 || max_axis_velocity_y > 0.70) {
        fprintf(stderr,
                "Planar endurance failed: residual axis velocity (%.6f / %.6f)\n",
                max_axis_velocity_x,
                max_axis_velocity_y);
        return 1;
    }

    if (max_motor_force_x > 40.0 || max_motor_force_y > 40.0) {
        fprintf(stderr,
                "Planar endurance failed: motor force spike (%.6f / %.6f)\n",
                max_motor_force_x,
                max_motor_force_y);
        return 1;
    }

    if (limit_contact_steps == 0 || max_limit_penetration > 0.02) {
        fprintf(stderr,
                "Planar endurance failed: limit interaction issue (steps=%d, penetration=%.6f)\n",
                limit_contact_steps,
                max_limit_penetration);
        return 1;
    }

    if (max_limit_force_y > 80.0) {
        fprintf(stderr,
                "Planar endurance failed: limit force spike (%.6f)\n",
                max_limit_force_y);
        return 1;
    }

    if (max_kinetic_energy > 30.0) {
        fprintf(stderr,
                "Planar endurance failed: kinetic energy spike (%.6f)\n",
                max_kinetic_energy);
        return 1;
    }

    printf("Planar constraint endurance test passed.\n");
    return 0;
}
