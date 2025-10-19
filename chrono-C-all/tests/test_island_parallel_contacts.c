#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

static void init_static_circle(ChronoBody2D_C *body, double x) {
    chrono_body2d_init(body);
    chrono_body2d_set_static(body);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.0);
    chrono_body2d_set_friction_static(body, 0.3);
    chrono_body2d_set_friction_dynamic(body, 0.2);
    body->position[0] = x;
    body->position[1] = 0.0;
}

static void init_dynamic_circle(ChronoBody2D_C *body, double x) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.0);
    chrono_body2d_set_friction_static(body, 0.4);
    chrono_body2d_set_friction_dynamic(body, 0.25);
    body->position[0] = x;
    body->position[1] = 0.0;
    body->linear_velocity[0] = 0.0;
    body->linear_velocity[1] = 0.0;
}

int main(void) {
    ChronoBody2D_C anchors[2];
    ChronoBody2D_C statics[2];
    ChronoBody2D_C dynamics[2];

    init_static_circle(&anchors[0], -1.0);
    init_static_circle(&anchors[1], 1.0);

    init_static_circle(&statics[0], -0.1);
    init_static_circle(&statics[1], 0.9);

    init_dynamic_circle(&dynamics[0], -0.5);
    init_dynamic_circle(&dynamics[1], 0.5);

    ChronoDistanceConstraint2D_C constraints[2];
    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(&constraints[0],
                                      &anchors[0],
                                      &dynamics[0],
                                      local_anchor,
                                      local_anchor,
                                      fabs(anchors[0].position[0] - dynamics[0].position[0]));
    chrono_distance_constraint2d_set_baumgarte(&constraints[0], 0.3);
    chrono_distance_constraint2d_set_max_correction(&constraints[0], 0.05);

    chrono_distance_constraint2d_init(&constraints[1],
                                      &anchors[1],
                                      &dynamics[1],
                                      local_anchor,
                                      local_anchor,
                                      fabs(anchors[1].position[0] - dynamics[1].position[0]));
    chrono_distance_constraint2d_set_baumgarte(&constraints[1], 0.3);
    chrono_distance_constraint2d_set_max_correction(&constraints[1], 0.05);

    ChronoConstraint2DBase_C *constraint_ptrs[2] = {
        &constraints[0].base,
        &constraints[1].base
    };

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    ChronoIsland2DWorkspace_C workspace;
    chrono_island2d_workspace_init(&workspace);

    ChronoIsland2DSolveConfig_C config;
    memset(&config, 0, sizeof(config));
    config.constraint_config.velocity_iterations = 6;
    config.constraint_config.position_iterations = 2;
    config.constraint_config.enable_parallel = 0;
    config.enable_parallel = 1;

    const double dt = 0.01;
    size_t last_island_count = 0;

    for (int step = 0; step < 40; ++step) {
        chrono_contact_manager2d_begin_step(&manager);

        ChronoContact2D_C contact01;
        if (chrono_collision2d_detect_circle_circle(&dynamics[0], &statics[0], &contact01) == 0 &&
            contact01.has_contact) {
            if (!chrono_contact_manager2d_update_circle_circle(&manager, &dynamics[0], &statics[0], &contact01)) {
                fprintf(stderr, "parallel island test: unable to update contact pair 0 at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                chrono_island2d_workspace_free(&workspace);
                return 1;
            }
        } else {
            fprintf(stderr, "parallel island test: contact pair 0 missing at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        ChronoContact2D_C contact23;
        if (chrono_collision2d_detect_circle_circle(&dynamics[1], &statics[1], &contact23) == 0 &&
            contact23.has_contact) {
            if (!chrono_contact_manager2d_update_circle_circle(&manager, &dynamics[1], &statics[1], &contact23)) {
                fprintf(stderr, "parallel island test: unable to update contact pair 1 at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                chrono_island2d_workspace_free(&workspace);
                return 1;
            }
        } else {
            fprintf(stderr, "parallel island test: contact pair 1 missing at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        size_t island_count = chrono_island2d_build(constraint_ptrs,
                                                    2,
                                                    manager.pairs,
                                                    manager.count,
                                                    &workspace);
        if (island_count != 2) {
            fprintf(stderr, "parallel island test: expected 2 islands, got %zu at step %d\n",
                    island_count, step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        chrono_island2d_solve(&workspace, dt, &config);
        last_island_count = island_count;

        chrono_contact_manager2d_end_step(&manager);

        for (int i = 0; i < 2; ++i) {
            chrono_body2d_integrate_explicit(&dynamics[i], dt);
            chrono_body2d_reset_forces(&dynamics[i]);
            dynamics[i].linear_velocity[0] *= 0.8;
            dynamics[i].linear_velocity[1] *= 0.8;
        }
    }

    if (last_island_count != 2 || manager.count != 2) {
        fprintf(stderr, "parallel island test: final island/contact counts unexpected (islands=%zu, contacts=%zu)\n",
                last_island_count, manager.count);
        chrono_contact_manager2d_free(&manager);
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }

    for (size_t i = 0; i < manager.count; ++i) {
        ChronoContactPair2D_C *pair = &manager.pairs[i];
        ChronoContactManifold2D_C *manifold = &pair->manifold;
        if (manifold->num_points <= 0) {
            fprintf(stderr, "parallel island test: manifold %zu has no points\n", i);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }
        int active_found = 0;
        for (int j = 0; j < manifold->num_points; ++j) {
            if (manifold->points[j].is_active) {
                active_found = 1;
                break;
            }
        }
        if (!active_found) {
            fprintf(stderr, "parallel island test: manifold %zu lacks active points\n", i);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }
        ChronoContact2D_C final_contact;
        if (chrono_collision2d_detect_circle_circle(pair->body_a, pair->body_b, &final_contact) != 0 ||
            !final_contact.has_contact) {
            fprintf(stderr, "parallel island test: manifold %zu lost contact state\n", i);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }
    }

    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&workspace);

    printf("Island parallel contact stress test passed.\n");
    return 0;
}
