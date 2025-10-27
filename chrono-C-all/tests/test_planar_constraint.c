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
    chrono_body2d_set_mass(body, 1.2, 0.35);
    chrono_body2d_set_circle_shape(body, 0.18);
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

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C slider;
    init_anchor(&anchor, 0.0, 0.0);
    init_slider(&slider, -0.1, 0.2);
    slider.angular_velocity = 1.0;

    ChronoPlanarConstraint2D_C planar;
    double local_anchor[2] = {0.0, 0.0};
    double axis_x[2] = {1.0, 0.0};
    chrono_planar_constraint2d_init(&planar,
                                    &anchor,
                                    &slider,
                                    local_anchor,
                                    local_anchor,
                                    axis_x);
    chrono_planar_constraint2d_set_baumgarte(&planar, 0.3);
    chrono_planar_constraint2d_set_slop(&planar, 1e-4);
    chrono_planar_constraint2d_set_max_correction(&planar, 0.05);

    chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                          CHRONO_PLANAR_AXIS_X,
                                                          0.3,
                                                          6.0,
                                                          1.0);
    chrono_planar_constraint2d_enable_motor(&planar,
                                            CHRONO_PLANAR_AXIS_X,
                                            1,
                                            0.0,
                                            15.0);

    chrono_planar_constraint2d_set_motor_position_target(&planar,
                                                          CHRONO_PLANAR_AXIS_Y,
                                                          -0.2,
                                                          6.0,
                                                          1.2);
    chrono_planar_constraint2d_enable_motor(&planar,
                                            CHRONO_PLANAR_AXIS_Y,
                                            1,
                                            0.0,
                                            15.0);

    ChronoConstraint2DBase_C *constraints[1] = {&planar.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 18;
    cfg.position_iterations = 4;
    cfg.enable_parallel = 0;

    const double dt = 0.01;
    const int settle_steps = 300;

    for (int i = 0; i < settle_steps; ++i) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);
    }

    double translation_x = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_X);
    double translation_y = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);

    if (fabs((slider.angle - anchor.angle) - planar.orientation_target) > 0.05) {
        fprintf(stderr, "Planar constraint failed: orientation drifted (error=%.6f)\n",
                slider.angle - anchor.angle - planar.orientation_target);
        return 1;
    }

    if (fabs(translation_x - 0.3) > 0.03) {
        fprintf(stderr, "Planar constraint failed: X motor drifted (%.6f)\n", translation_x);
        return 1;
    }

    if (fabs(translation_y + 0.2) > 0.03) {
        fprintf(stderr, "Planar constraint failed: Y motor drifted (%.6f)\n", translation_y);
        return 1;
    }

    chrono_planar_constraint2d_enable_motor(&planar,
                                            CHRONO_PLANAR_AXIS_Y,
                                            0,
                                            0.0,
                                            0.0);
    chrono_planar_constraint2d_enable_limit(&planar,
                                            CHRONO_PLANAR_AXIS_Y,
                                            1,
                                            -0.25,
                                            0.05);
    chrono_planar_constraint2d_set_limit_spring(&planar,
                                                CHRONO_PLANAR_AXIS_Y,
                                                45.0,
                                                6.0);

    slider.linear_velocity[0] = 0.0;
    slider.linear_velocity[1] = -2.0;

    for (int i = 0; i < 200; ++i) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&slider, dt);
        chrono_body2d_reset_forces(&slider);
    }

    translation_y = compute_axis_translation(&anchor, &slider, &planar, CHRONO_PLANAR_AXIS_Y);
    if (translation_y < -0.26 || translation_y > 0.06) {
        fprintf(stderr, "Planar constraint failed: limit enforcement broke (%.6f)\n", translation_y);
        return 1;
    }

    printf("Planar constraint test passed.\n");
    return 0;
}
