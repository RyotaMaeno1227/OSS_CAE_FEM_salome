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

static void init_slider(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.28);
    chrono_body2d_set_circle_shape(body, 0.18);
    body->position[0] = x;
    body->position[1] = y;
}

static double compute_translation(const ChronoBody2D_C *anchor,
                                  const ChronoBody2D_C *slider,
                                  const ChronoPrismaticConstraint2D_C *joint) {
    double axis_world[2] = {joint->local_axis_a[0], joint->local_axis_a[1]};
    double anchor_world[2];
    double slider_world[2];
    chrono_body2d_local_to_world(anchor, joint->local_anchor_a, anchor_world);
    chrono_body2d_local_to_world(slider, joint->local_anchor_b, slider_world);
    double delta[2] = {
        slider_world[0] - anchor_world[0],
        slider_world[1] - anchor_world[1]
    };
    double c = cos(anchor->angle);
    double s = sin(anchor->angle);
    axis_world[0] = c * joint->local_axis_a[0] - s * joint->local_axis_a[1];
    axis_world[1] = s * joint->local_axis_a[0] + c * joint->local_axis_a[1];
    return delta[0] * axis_world[0] + delta[1] * axis_world[1];
}

typedef struct SetpointTracker {
    double target;
    int last_change_step;
} SetpointTracker;

static void tracker_update(SetpointTracker *tracker, double value, int step) {
    tracker->target = value;
    tracker->last_change_step = step;
}

static int tracker_settled(const SetpointTracker *tracker, int step, int settle_frames) {
    return (step - tracker->last_change_step) >= settle_frames;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor);
    init_slider(&slider, -0.2, 0.0);

    double local_anchor[2] = {0.0, 0.0};
    double local_axis[2] = {1.0, 0.0};

    ChronoPrismaticConstraint2D_C joint;
    chrono_prismatic_constraint2d_init(&joint,
                                       &anchor,
                                       &slider,
                                       local_anchor,
                                       local_anchor,
                                       local_axis);
    chrono_prismatic_constraint2d_set_baumgarte(&joint, 0.3);
    chrono_prismatic_constraint2d_set_slop(&joint, 1e-4);
    chrono_prismatic_constraint2d_set_max_correction(&joint, 0.06);
    chrono_prismatic_constraint2d_set_softness(&joint, 0.02);
    chrono_prismatic_constraint2d_enable_motor(&joint, 1, 0.0, 16.0);
    chrono_prismatic_constraint2d_set_motor_position_target(&joint, 0.22, 4.8, 1.1);

    ChronoConstraint2DBase_C *constraints[1] = {&joint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 20;
    cfg.position_iterations = 4;
    cfg.enable_parallel = 0;

    const double dt = 0.005;
    const int total_steps = 9000;
    const int settle_frames = 240;
    const int switch_step_a = (int)(1.8 / dt);
    const int switch_step_b = (int)(3.6 / dt);
    const int switch_step_limit = (int)(5.0 / dt);
    const int switch_step_post_limit = (int)(6.3 / dt);

    SetpointTracker tracker = {0.22, 0};
    double max_translation_error = 0.0;
    double max_velocity_drift = 0.0;
    double max_motor_force = 0.0;
    double max_limit_penetration = 0.0;
    double max_limit_spring = 0.0;
    int limit_contact_steps = 0;
    int limit_active = 0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == switch_step_a) {
            chrono_prismatic_constraint2d_set_motor_position_target(&joint, -0.18, 4.8, 1.1);
            tracker_update(&tracker, -0.18, step);
        } else if (step == switch_step_b) {
            chrono_prismatic_constraint2d_set_motor_position_target(&joint, 0.30, 5.5, 1.25);
            tracker_update(&tracker, 0.30, step);
        } else if (step == switch_step_limit) {
            chrono_prismatic_constraint2d_enable_motor(&joint, 0, 0.0, 0.0);
            chrono_prismatic_constraint2d_enable_limit(&joint, 1, -0.26, 0.34);
            chrono_prismatic_constraint2d_set_limit_spring(&joint, 60.0, 7.5);
            slider.linear_velocity[0] = 2.2;
            limit_active = 1;
        } else if (step == switch_step_post_limit) {
            chrono_prismatic_constraint2d_enable_limit(&joint, 0, 0.0, 0.0);
            chrono_prismatic_constraint2d_enable_motor(&joint, 1, 0.0, 18.0);
            chrono_prismatic_constraint2d_set_motor_position_target(&joint, 0.10, 4.2, 1.2);
            tracker_update(&tracker, 0.10, step);
            limit_active = 0;
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);

        if (limit_active) {
            if (joint.limit_state != 0) {
                ++limit_contact_steps;
                double translation = compute_translation(&anchor, &slider, &joint);
                double penetration = translation - (joint.limit_state > 0 ? joint.limit_upper : joint.limit_lower);
                if (penetration > max_limit_penetration) {
                    max_limit_penetration = penetration;
                }
            }
        }

        if (fabs(joint.last_limit_spring_force) > max_limit_spring) {
            max_limit_spring = fabs(joint.last_limit_spring_force);
        }
        if (fabs(joint.last_motor_force) > max_motor_force) {
            max_motor_force = fabs(joint.last_motor_force);
        }

        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);

        double translation = compute_translation(&anchor, &slider, &joint);
        if (!limit_active && tracker_settled(&tracker, step, settle_frames)) {
            double err = fabs(translation - tracker.target);
            if (err > max_translation_error) {
                max_translation_error = err;
            }
        }

        double vy = slider.linear_velocity[1];
        if (fabs(vy) > max_velocity_drift) {
            max_velocity_drift = fabs(vy);
        }
    }

    double final_translation = compute_translation(&anchor, &slider, &joint);

    if (!isfinite(final_translation) || fabs(final_translation - tracker.target) > 0.045) {
        fprintf(stderr,
                "Prismatic endurance failed: final translation off (%.6f vs target %.6f)\n",
                final_translation,
                tracker.target);
        return 1;
    }

    if (max_translation_error > 0.055) {
        fprintf(stderr,
                "Prismatic endurance failed: steady translation error too high (%.6f)\n",
                max_translation_error);
        return 1;
    }

    if (max_velocity_drift > 0.12) {
        fprintf(stderr,
                "Prismatic endurance failed: off-axis velocity drift (%.6f)\n",
                max_velocity_drift);
        return 1;
    }

    if (max_motor_force > 38.0) {
        fprintf(stderr,
                "Prismatic endurance failed: motor force spike (%.6f)\n",
                max_motor_force);
        return 1;
    }

    if (limit_contact_steps == 0 || max_limit_penetration > 0.02) {
        fprintf(stderr,
                "Prismatic endurance failed: limit handling issue (steps=%d, penetration=%.6f)\n",
                limit_contact_steps,
                max_limit_penetration);
        return 1;
    }

    if (max_limit_spring > 150.0) {
        fprintf(stderr,
                "Prismatic endurance failed: limit spring force too large (%.6f)\n",
                max_limit_spring);
        return 1;
    }

    if (fabs(slider.position[1]) > 0.015) {
        fprintf(stderr,
                "Prismatic endurance failed: body drifted off constraint axis (y=%.6f)\n",
                slider.position[1]);
        return 1;
    }

    printf("Prismatic constraint endurance test passed.\n");
    return 0;
}
