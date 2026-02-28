#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

typedef struct {
    double position[2];
    double angle;
    double linear_velocity[2];
    double angular_velocity;
} BodyState;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void init_ground(ChronoBody2D_C *ground) {
    chrono_body2d_init(ground);
    chrono_body2d_set_static(ground);
    double vertices[] = {
        -2.0, -0.05,
         2.0, -0.05,
         2.0,  0.05,
        -2.0,  0.05
    };
    chrono_body2d_set_polygon_shape(ground, vertices, 4);
    chrono_body2d_set_material(ground, &(ChronoMaterial2D_C){0.0, 0.8, 0.5});
}

static void init_block(ChronoBody2D_C *block) {
    chrono_body2d_init(block);
    chrono_body2d_set_mass(block, 1.5, 0.12);
    double vertices[] = {
        -0.4, -0.1,
         0.4, -0.1,
         0.4,  0.1,
        -0.4,  0.1
    };
    chrono_body2d_set_polygon_shape(block, vertices, 4);
    chrono_body2d_set_material(block, &(ChronoMaterial2D_C){0.15, 0.6, 0.4});
    block->position[0] = 0.0;
    block->position[1] = 0.6;
    block->angle = 5.0 * (M_PI / 180.0);
    block->linear_velocity[0] = 0.0;
    block->linear_velocity[1] = -1.5;
    block->angular_velocity = 0.0;
}

static void capture_state(const ChronoBody2D_C *body, BodyState *state) {
    state->position[0] = body->position[0];
    state->position[1] = body->position[1];
    state->angle = body->angle;
    state->linear_velocity[0] = body->linear_velocity[0];
    state->linear_velocity[1] = body->linear_velocity[1];
    state->angular_velocity = body->angular_velocity;
}

static double state_delta(const BodyState *a, const BodyState *b) {
    double sum = 0.0;
    sum += fabs(a->position[0] - b->position[0]);
    sum += fabs(a->position[1] - b->position[1]);
    sum += fabs(a->angle - b->angle);
    sum += fabs(a->linear_velocity[0] - b->linear_velocity[0]);
    sum += fabs(a->linear_velocity[1] - b->linear_velocity[1]);
    sum += fabs(a->angular_velocity - b->angular_velocity);
    return sum;
}

static int run_simulation(int enable_parallel, BodyState *state, int *max_contact_points) {
    ChronoBody2D_C ground;
    ChronoBody2D_C block;
    init_ground(&ground);
    init_block(&block);

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    ChronoIsland2DWorkspace_C workspace;
    chrono_island2d_workspace_init(&workspace);

    ChronoIsland2DSolveConfig_C config;
    memset(&config, 0, sizeof(config));
    config.constraint_config.velocity_iterations = 16;
    config.constraint_config.position_iterations = 5;
    config.constraint_config.enable_parallel = enable_parallel ? 1 : 0;
    config.enable_parallel = enable_parallel ? 1 : 0;

    const double dt = 0.005;
    int max_points = 0;

    for (int step = 0; step < 160; ++step) {
        chrono_contact_manager2d_begin_step(&manager);

        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_polygon_polygon(&block, &ground, &contact) != 0) {
            fprintf(stderr, "manifold test (%s): detection failed at step %d\n",
                    enable_parallel ? "parallel" : "serial",
                    step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        if (contact.has_contact) {
            if (!chrono_contact_manager2d_update_contact(&manager, &block, &ground, &contact)) {
                fprintf(stderr, "manifold test (%s): manager update failed at step %d\n",
                        enable_parallel ? "parallel" : "serial",
                        step);
                chrono_contact_manager2d_free(&manager);
                chrono_island2d_workspace_free(&workspace);
                return 1;
            }
        }

        size_t island_count =
            chrono_island2d_build(NULL, 0, manager.pairs, manager.count, &workspace);
        if (island_count > 1) {
            fprintf(stderr, "manifold test (%s): unexpected island count %zu at step %d\n",
                    enable_parallel ? "parallel" : "serial",
                    island_count,
                    step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        chrono_island2d_solve(&workspace, dt, &config);
        chrono_contact_manager2d_end_step(&manager);

        ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &block, &ground);
        if (manifold && manifold->num_points > max_points) {
            max_points = manifold->num_points;
        }

        chrono_body2d_integrate_explicit(&block, dt);
        chrono_body2d_reset_forces(&block);
        block.linear_velocity[0] *= 0.995;
        block.linear_velocity[1] *= 0.995;
        block.angular_velocity *= 0.995;
    }

    capture_state(&block, state);
    if (max_contact_points) {
        *max_contact_points = max_points;
    }

    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&workspace);
    return 0;
}

int main(void) {
    BodyState serial_state;
    BodyState parallel_state;
    int serial_points = 0;
    int parallel_points = 0;

    if (run_simulation(0, &serial_state, &serial_points) != 0) {
        return 1;
    }
    if (run_simulation(1, &parallel_state, &parallel_points) != 0) {
        return 1;
    }

    if (serial_points < 2 || parallel_points < 2) {
        fprintf(stderr,
                "manifold test: insufficient contact points (serial=%d, parallel=%d)\n",
                serial_points,
                parallel_points);
        return 1;
    }

    double delta = state_delta(&serial_state, &parallel_state);
    if (delta > 1e-6) {
        fprintf(stderr,
                "manifold test: state mismatch between serial and parallel (delta=%.12f)\n",
                delta);
        return 1;
    }

    printf("Island manifold parallel equivalence test passed.\n");
    return 0;
}
