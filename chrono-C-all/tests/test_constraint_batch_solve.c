#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

#define CONSTRAINT_COUNT 3

static size_t run_batch(int enable_parallel,
                        double distances_out[CONSTRAINT_COUNT],
                        int island_ids_out[CONSTRAINT_COUNT],
                        double rest_lengths_out[CONSTRAINT_COUNT]) {
    ChronoBody2D_C anchor0;
    ChronoBody2D_C anchor2;
    ChronoBody2D_C body_shared;
    ChronoBody2D_C body_chain;
    ChronoBody2D_C body_isolated;

    chrono_body2d_init(&anchor0);
    chrono_body2d_set_static(&anchor0);
    anchor0.position[0] = 0.0;
    anchor0.position[1] = 0.0;

    chrono_body2d_init(&anchor2);
    chrono_body2d_set_static(&anchor2);
    anchor2.position[0] = 0.0;
    anchor2.position[1] = 2.0;

    chrono_body2d_init(&body_shared);
    chrono_body2d_set_mass(&body_shared, 1.0, 0.4);
    body_shared.position[0] = 1.0;
    body_shared.position[1] = 0.4;

    chrono_body2d_init(&body_chain);
    chrono_body2d_set_mass(&body_chain, 1.1, 0.45);
    body_chain.position[0] = 2.1;
    body_chain.position[1] = 0.6;

    chrono_body2d_init(&body_isolated);
    chrono_body2d_set_mass(&body_isolated, 0.9, 0.35);
    body_isolated.position[0] = 1.2;
    body_isolated.position[1] = 2.7;

    ChronoDistanceConstraint2D_C constraints[CONSTRAINT_COUNT];
    double local_anchor[2] = {0.0, 0.0};

    double rest_lengths[CONSTRAINT_COUNT];
    rest_lengths[0] = 1.05;
    rest_lengths[1] = 1.1;
    rest_lengths[2] = 1.0;

    chrono_distance_constraint2d_init(&constraints[0],
                                      &anchor0,
                                      &body_shared,
                                      local_anchor,
                                      local_anchor,
                                      rest_lengths[0]);
    chrono_distance_constraint2d_set_baumgarte(&constraints[0], 0.5);
    chrono_distance_constraint2d_set_softness(&constraints[0], 0.01);

    chrono_distance_constraint2d_init(&constraints[1],
                                      &body_shared,
                                      &body_chain,
                                      local_anchor,
                                      local_anchor,
                                      rest_lengths[1]);
    chrono_distance_constraint2d_set_baumgarte(&constraints[1], 0.45);
    chrono_distance_constraint2d_set_softness(&constraints[1], 0.02);

    chrono_distance_constraint2d_init(&constraints[2],
                                      &anchor2,
                                      &body_isolated,
                                      local_anchor,
                                      local_anchor,
                                      rest_lengths[2]);
    chrono_distance_constraint2d_set_baumgarte(&constraints[2], 0.55);
    chrono_distance_constraint2d_set_softness(&constraints[2], 0.015);

    ChronoConstraint2DBase_C *ptrs[CONSTRAINT_COUNT];
    for (int i = 0; i < CONSTRAINT_COUNT; ++i) {
        ptrs[i] = &constraints[i].base;
    }

    size_t island_count = chrono_constraint2d_build_islands(ptrs, CONSTRAINT_COUNT, island_ids_out);

    ChronoConstraint2DBatchConfig_C cfg;
    cfg.velocity_iterations = 25;
    cfg.position_iterations = 12;
    cfg.enable_parallel = enable_parallel;

    double dt = 0.016;
    chrono_constraint2d_batch_solve(ptrs, CONSTRAINT_COUNT, dt, &cfg);

    for (int i = 0; i < CONSTRAINT_COUNT; ++i) {
        ChronoBody2D_C *body_a = ptrs[i]->body_a;
        ChronoBody2D_C *body_b = ptrs[i]->body_b;
        double wa[2];
        double wb[2];
        chrono_body2d_local_to_world(body_a, local_anchor, wa);
        chrono_body2d_local_to_world(body_b, local_anchor, wb);
        double dx = wb[0] - wa[0];
        double dy = wb[1] - wa[1];
        distances_out[i] = sqrt(dx * dx + dy * dy);
        if (rest_lengths_out) {
            rest_lengths_out[i] = rest_lengths[i];
        }
    }

    return island_count;
}

int main(void) {
    double sequential_distances[CONSTRAINT_COUNT];
    double parallel_distances[CONSTRAINT_COUNT];
    double sequential_rest_lengths[CONSTRAINT_COUNT];
    double parallel_rest_lengths[CONSTRAINT_COUNT];
    int island_ids[CONSTRAINT_COUNT];

    size_t island_count = run_batch(0, sequential_distances, island_ids, sequential_rest_lengths);
    run_batch(1, parallel_distances, NULL, parallel_rest_lengths);

    int success = 1;
    if (island_count != 2) {
        fprintf(stderr, "Expected 2 islands, got %zu\n", island_count);
        success = 0;
    }
    if (island_ids[0] != island_ids[1] || island_ids[2] == island_ids[0]) {
        fprintf(stderr, "Island grouping incorrect: [%d, %d, %d]\n",
                island_ids[0], island_ids[1], island_ids[2]);
        success = 0;
    }

    for (int i = 0; i < CONSTRAINT_COUNT; ++i) {
        double rest_length = sequential_rest_lengths[i];
        double seq_error = fabs(sequential_distances[i] - rest_length);
        double diff = fabs(sequential_distances[i] - parallel_distances[i]);
        if (seq_error > 5e-3) {
            fprintf(stderr, "Constraint %d did not reach rest length: distance=%.6f rest=%.6f\n",
                    i, sequential_distances[i], rest_length);
            success = 0;
        }
        if (diff > 1e-4) {
            fprintf(stderr, "Parallel and sequential results diverged for constraint %d (diff=%.6e)\n",
                    i, diff);
            success = 0;
        }
    }

    if (!success) {
        return 1;
    }

    printf("Constraint batch solve test passed (islands + parallel vs sequential).\n");
    return 0;
}
