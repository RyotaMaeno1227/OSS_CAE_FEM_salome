#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

static void init_ground(ChronoBody2D_C *ground) {
    chrono_body2d_init(ground);
    chrono_body2d_set_static(ground);
    double half_width = 2.0;
    double half_height = 0.2;
    double verts[] = {
        -half_width, -half_height,
         half_width, -half_height,
         half_width,  half_height,
        -half_width,  half_height
    };
    chrono_body2d_set_polygon_shape(ground, verts, 4);
    chrono_body2d_set_material(ground, &(ChronoMaterial2D_C){0.0, 0.9, 0.7});
}

static void init_box(ChronoBody2D_C *body, double x, double y, double angle) {
    chrono_body2d_init(body);
    double verts[] = {
        -0.3, -0.25,
         0.3, -0.25,
         0.3,  0.25,
        -0.3,  0.25
    };
    chrono_body2d_set_polygon_shape_with_density(body, verts, 4, 2.0);
    body->position[0] = x;
    body->position[1] = y;
    body->angle = angle;
    chrono_body2d_set_material(body, &(ChronoMaterial2D_C){0.1, 0.6, 0.4});
}

static void apply_damping(ChronoBody2D_C *body, double dt) {
    if (!body || body->is_static) {
        return;
    }
    (void)dt;
    body->linear_velocity[0] *= 0.999;
    body->linear_velocity[1] *= 0.999;
    body->angular_velocity *= 0.999;
}

static void clamp_velocity(ChronoBody2D_C *body, double max_linear, double max_angular) {
    if (!body || body->is_static) {
        return;
    }
    if (body->linear_velocity[0] > max_linear) body->linear_velocity[0] = max_linear;
    if (body->linear_velocity[0] < -max_linear) body->linear_velocity[0] = -max_linear;
    if (body->linear_velocity[1] > max_linear) body->linear_velocity[1] = max_linear;
    if (body->linear_velocity[1] < -max_linear) body->linear_velocity[1] = -max_linear;
    if (body->angular_velocity > max_angular) body->angular_velocity = max_angular;
    if (body->angular_velocity < -max_angular) body->angular_velocity = -max_angular;
}

int main(void) {
    ChronoBody2D_C ground;
    init_ground(&ground);

    ChronoBody2D_C box_a;
    ChronoBody2D_C box_b;
    init_box(&box_a, -0.4, 0.8, 0.05);
    init_box(&box_b, 0.4, 0.8, -0.03);
    box_a.linear_velocity[0] = 0.6;
    box_a.linear_velocity[1] = 0.2;
    box_a.angular_velocity = 1.2;
    box_b.linear_velocity[0] = -0.4;
    box_b.linear_velocity[1] = -0.15;
    box_b.angular_velocity = -1.0;

    ChronoDistanceConstraint2D_C constraint_chain;
    double local_anchor[2] = {0.0, 0.0};
    double dx0 = box_b.position[0] - box_a.position[0];
    double dy0 = box_b.position[1] - box_a.position[1];
    double rest_length = sqrt(dx0 * dx0 + dy0 * dy0);
    chrono_distance_constraint2d_init(&constraint_chain,
                                      &box_a,
                                      &box_b,
                                      local_anchor,
                                      local_anchor,
                                      rest_length);
    chrono_distance_constraint2d_set_baumgarte(&constraint_chain, 0.35);
    chrono_distance_constraint2d_set_max_correction(&constraint_chain, 0.1);

    ChronoBody2D_C anchor;
    chrono_body2d_init(&anchor);
    chrono_body2d_set_static(&anchor);
    anchor.position[0] = -0.4;
    anchor.position[1] = 1.2;

    ChronoDistanceConstraint2D_C constraint_anchor;
    chrono_distance_constraint2d_init(&constraint_anchor,
                                      &anchor,
                                      &box_a,
                                      local_anchor,
                                      local_anchor,
                                      0.4);
    chrono_distance_constraint2d_set_baumgarte(&constraint_anchor, 0.3);
    chrono_distance_constraint2d_set_max_correction(&constraint_anchor, 0.08);

    ChronoConstraint2DBase_C *constraints[2] = {
        &constraint_chain.base,
        &constraint_anchor.base
    };

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    ChronoIsland2DWorkspace_C island_workspace;
    chrono_island2d_workspace_init(&island_workspace);

    ChronoIsland2DSolveConfig_C solve_config;
    memset(&solve_config, 0, sizeof(solve_config));
    solve_config.constraint_config.velocity_iterations = 12;
    solve_config.constraint_config.position_iterations = 3;
    solve_config.enable_parallel = 0;

    const double dt = 0.01;
    double max_length = 0.0;
    double max_angle = 0.0;

    for (int step = 0; step < 80; ++step) {
        apply_damping(&box_a, dt);
        apply_damping(&box_b, dt);
        clamp_velocity(&box_a, 4.0, 6.0);
        clamp_velocity(&box_b, 4.0, 6.0);

        chrono_contact_manager2d_begin_step(&manager);

        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_polygon_polygon(&box_a, &ground, &contact) == 0 && contact.has_contact) {
            if (!chrono_contact_manager2d_update_contact(&manager, &box_a, &ground, &contact)) {
                fprintf(stderr, "polygon island: update A-ground failed at step %d\n", step);
                goto failure;
            }
        }
        if (chrono_collision2d_detect_polygon_polygon(&box_b, &ground, &contact) == 0 && contact.has_contact) {
            if (!chrono_contact_manager2d_update_contact(&manager, &box_b, &ground, &contact)) {
                fprintf(stderr, "polygon island: update B-ground failed at step %d\n", step);
                goto failure;
            }
        }
        if (chrono_collision2d_detect_polygon_polygon(&box_a, &box_b, &contact) == 0 && contact.has_contact) {
            if (!chrono_contact_manager2d_update_contact(&manager, &box_a, &box_b, &contact)) {
                fprintf(stderr, "polygon island: update A-B failed at step %d\n", step);
                goto failure;
            }
        }

        chrono_island2d_workspace_reset(&island_workspace);
        size_t islands = chrono_island2d_build(constraints,
                                               2,
                                               manager.pairs,
                                               manager.count,
                                               &island_workspace);
        if (islands > 0) {
            chrono_island2d_solve(&island_workspace, dt, &solve_config);
        } else {
            chrono_constraint2d_batch_solve(constraints, 2, dt, &solve_config.constraint_config, NULL);
        }

        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&box_a, dt);
        chrono_body2d_reset_forces(&box_a);
        chrono_body2d_integrate_explicit(&box_b, dt);
        chrono_body2d_reset_forces(&box_b);
        clamp_velocity(&box_a, 4.0, 6.0);
        clamp_velocity(&box_b, 4.0, 6.0);

        double dx = box_b.position[0] - box_a.position[0];
        double dy = box_b.position[1] - box_a.position[1];
        double current_length = sqrt(dx * dx + dy * dy);
        if (current_length > max_length) {
            max_length = current_length;
        }
        double angle_a = fabs(box_a.angle);
        double angle_b = fabs(box_b.angle);
        if (angle_a > max_angle) {
            max_angle = angle_a;
        }
        if (angle_b > max_angle) {
            max_angle = angle_b;
        }
    }

    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&island_workspace);

    if (!isfinite(box_a.position[0]) || !isfinite(box_a.position[1]) ||
        !isfinite(box_b.position[0]) || !isfinite(box_b.position[1])) {
        fprintf(stderr, "polygon island: non-finite body position detected\n");
        return 1;
    }

    if (fabs(box_a.position[1]) > 3.0 || fabs(box_b.position[1]) > 3.0) {
        fprintf(stderr, "polygon island: bodies left vertical bounds (%.6f, %.6f)\n",
                box_a.position[1], box_b.position[1]);
        return 1;
    }

    if (!isfinite(max_length) || max_length > rest_length * 1.8) {
        fprintf(stderr, "polygon island: constraint stretched excessively (%.6f)\n", max_length);
        return 1;
    }
    if (max_angle > 1.2) {
        fprintf(stderr, "polygon island: excessive rotation (max angle=%.6f)\n", max_angle);
        return 1;
    }

    printf("Island polygon long-run test passed.\n");
    return 0;

failure:
    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&island_workspace);
    return 1;
}
