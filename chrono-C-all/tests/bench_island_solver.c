#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

#ifdef _OPENMP
#include <omp.h>
#endif

typedef struct {
    ChronoBody2D_C anchor;
    ChronoBody2D_C static_body;
    ChronoBody2D_C dynamic_body;
    ChronoDistanceConstraint2D_C constraint;
    double base_x;
} IslandFixture;

typedef struct {
    IslandFixture *fixtures;
    ChronoConstraint2DBase_C **constraint_ptrs;
    int count;
} BenchWorld;

static double wall_time(void) {
#ifdef _OPENMP
    return omp_get_wtime();
#else
#ifdef CLOCK_MONOTONIC
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
#endif
}

static void init_fixture(IslandFixture *fixture, double base_x, int index) {
    fixture->base_x = base_x;

    chrono_body2d_init(&fixture->anchor);
    chrono_body2d_set_static(&fixture->anchor);
    fixture->anchor.position[0] = base_x;
    fixture->anchor.position[1] = 0.0;
    chrono_body2d_set_circle_shape(&fixture->anchor, 0.15);

    chrono_body2d_init(&fixture->static_body);
    chrono_body2d_set_static(&fixture->static_body);
    chrono_body2d_set_circle_shape(&fixture->static_body, 0.3);
    chrono_body2d_set_restitution(&fixture->static_body, 0.05);
    chrono_body2d_set_friction_static(&fixture->static_body, 0.5);
    chrono_body2d_set_friction_dynamic(&fixture->static_body, 0.35);
    fixture->static_body.position[0] = base_x + 0.6;
    fixture->static_body.position[1] = 0.0;

    chrono_body2d_init(&fixture->dynamic_body);
    chrono_body2d_set_mass(&fixture->dynamic_body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(&fixture->dynamic_body, 0.3);
    chrono_body2d_set_restitution(&fixture->dynamic_body, 0.1);
    chrono_body2d_set_friction_static(&fixture->dynamic_body, 0.6);
    chrono_body2d_set_friction_dynamic(&fixture->dynamic_body, 0.4);
    fixture->dynamic_body.position[0] = base_x + 0.35;
    fixture->dynamic_body.position[1] = 0.0;
    fixture->dynamic_body.linear_velocity[0] = 0.0;
    fixture->dynamic_body.linear_velocity[1] = ((index & 1) ? 1.0 : -1.0) * 0.05;

    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(&fixture->constraint,
                                      &fixture->anchor,
                                      &fixture->dynamic_body,
                                      local_anchor,
                                      local_anchor,
                                      fabs(fixture->dynamic_body.position[0] - fixture->anchor.position[0]));
    chrono_distance_constraint2d_set_baumgarte(&fixture->constraint, 0.3);
    chrono_distance_constraint2d_set_max_correction(&fixture->constraint, 0.05);
    chrono_distance_constraint2d_set_softness(&fixture->constraint, 0.0);
}

static int benchmark_world_init(BenchWorld *world, int num_islands) {
    if (!world || num_islands <= 0) {
        return 0;
    }

    world->fixtures = (IslandFixture *)calloc((size_t)num_islands, sizeof(IslandFixture));
    world->constraint_ptrs =
        (ChronoConstraint2DBase_C **)calloc((size_t)num_islands, sizeof(ChronoConstraint2DBase_C *));
    if (!world->fixtures || !world->constraint_ptrs) {
        free(world->fixtures);
        free(world->constraint_ptrs);
        memset(world, 0, sizeof(*world));
        return 0;
    }

    world->count = num_islands;

    for (int i = 0; i < num_islands; ++i) {
        double base_x = (double)i * 2.5;
        init_fixture(&world->fixtures[i], base_x, i);
        world->constraint_ptrs[i] = &world->fixtures[i].constraint.base;
    }

    return 1;
}

static void benchmark_world_free(BenchWorld *world) {
    if (!world) {
        return;
    }
    free(world->fixtures);
    free(world->constraint_ptrs);
    memset(world, 0, sizeof(*world));
}

static void clamp_dynamic(IslandFixture *fixture) {
    if (!fixture) {
        return;
    }
    double *pos = fixture->dynamic_body.position;
    double *vel = fixture->dynamic_body.linear_velocity;
    const double target_x = fixture->base_x + 0.35;
    pos[0] = target_x;
    vel[0] = 0.0;
    const double max_offset = 0.25;
    if (pos[1] > max_offset) {
        pos[1] = max_offset;
        if (vel[1] > 0.0) {
            vel[1] = 0.0;
        }
    } else if (pos[1] < -max_offset) {
        pos[1] = -max_offset;
        if (vel[1] < 0.0) {
            vel[1] = 0.0;
        }
    }
}

static int run_benchmark_iteration(int num_islands,
                                   int steps,
                                   int thread_count,
                                   double dt,
                                   double *pos_out,
                                   double *vel_out,
                                   double *elapsed_out,
                                   ChronoIslandSchedulerBackend_C scheduler) {
    BenchWorld world = {0};
    ChronoIsland2DWorkspace_C workspace;
    ChronoContactManager2D_C manager;
    double start_time;
    double end_time;

    if (!benchmark_world_init(&world, num_islands)) {
        fprintf(stderr, "[bench] failed to initialize world (islands=%d)\n", num_islands);
        return 0;
    }

    chrono_island2d_workspace_init(&workspace);
    chrono_contact_manager2d_init(&manager);

#ifdef _OPENMP
    if (thread_count > 0) {
        omp_set_num_threads(thread_count);
    }
#endif

    ChronoIsland2DSolveConfig_C config;
    memset(&config, 0, sizeof(config));
    config.constraint_config.velocity_iterations = 8;
    config.constraint_config.position_iterations = 2;
    config.constraint_config.enable_parallel = 0;
#ifdef _OPENMP
    config.enable_parallel = thread_count > 1 ? 1 : 0;
#else
    (void)thread_count;
    config.enable_parallel = 0;
#endif
    config.scheduler = scheduler;

    start_time = wall_time();

    for (int step = 0; step < steps; ++step) {
        chrono_contact_manager2d_begin_step(&manager);

        for (int i = 0; i < num_islands; ++i) {
            IslandFixture *fixture = &world.fixtures[i];
            ChronoContact2D_C contact;
            if (chrono_collision2d_detect_circle_circle(&fixture->dynamic_body,
                                                        &fixture->static_body,
                                                        &contact) != 0 || !contact.has_contact) {
                fprintf(stderr, "[bench] contact detection failed at island %d step %d\n", i, step);
                chrono_contact_manager2d_free(&manager);
                chrono_island2d_workspace_free(&workspace);
                benchmark_world_free(&world);
                return 0;
            }
            if (!chrono_contact_manager2d_update_circle_circle(&manager,
                                                               &fixture->dynamic_body,
                                                               &fixture->static_body,
                                                               &contact)) {
                fprintf(stderr, "[bench] contact update failed at island %d step %d\n", i, step);
                chrono_contact_manager2d_free(&manager);
                chrono_island2d_workspace_free(&workspace);
                benchmark_world_free(&world);
                return 0;
            }
        }

        size_t island_count = chrono_island2d_build(world.constraint_ptrs,
                                                    (size_t)num_islands,
                                                    manager.pairs,
                                                    manager.count,
                                                    &workspace);
        if (island_count != (size_t)num_islands) {
            fprintf(stderr,
                    "[bench] unexpected island count (expected %d, got %zu) at step %d\n",
                    num_islands,
                    island_count,
                    step);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            benchmark_world_free(&world);
            return 0;
        }

        chrono_island2d_solve(&workspace, dt, &config);

        chrono_contact_manager2d_end_step(&manager);

        for (int i = 0; i < num_islands; ++i) {
            IslandFixture *fixture = &world.fixtures[i];
            double impulse_y = ((step + i) & 1) ? 0.015 : -0.015;
            double world_point[2] = {
                fixture->dynamic_body.position[0],
                fixture->dynamic_body.position[1]
            };
            double impulse[2] = {0.0, impulse_y};
            chrono_body2d_apply_impulse(&fixture->dynamic_body, impulse, world_point);
            chrono_body2d_integrate_explicit(&fixture->dynamic_body, dt);
            chrono_body2d_reset_forces(&fixture->dynamic_body);
            clamp_dynamic(fixture);
        }
    }

    end_time = wall_time();

    if (elapsed_out) {
        *elapsed_out = end_time - start_time;
    }

    if (pos_out) {
        for (int i = 0; i < num_islands; ++i) {
            pos_out[2 * i] = world.fixtures[i].dynamic_body.position[0];
            pos_out[2 * i + 1] = world.fixtures[i].dynamic_body.position[1];
        }
    }
    if (vel_out) {
        for (int i = 0; i < num_islands; ++i) {
            vel_out[2 * i] = world.fixtures[i].dynamic_body.linear_velocity[0];
            vel_out[2 * i + 1] = world.fixtures[i].dynamic_body.linear_velocity[1];
        }
    }

    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&workspace);
    benchmark_world_free(&world);

    return 1;
}

static int compare_results(const double *baseline,
                           const double *candidate,
                           int count,
                           double tolerance,
                           const char *label) {
    for (int i = 0; i < count; ++i) {
        double diff = fabs(baseline[i] - candidate[i]);
        if (diff > tolerance) {
            fprintf(stderr,
                    "[bench] mismatch on %s[%d]: baseline=%.9f candidate=%.9f diff=%.9f tolerance=%.9f\n",
                    label,
                    i,
                    baseline[i],
                    candidate[i],
                    diff,
                    tolerance);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    int num_islands = 64;
    int steps = 200;
    int max_threads = 4;
    double dt = 0.01;
    ChronoIslandSchedulerBackend_C scheduler = CHRONO_ISLAND_SCHED_AUTO;

    if (argc > 1) {
        num_islands = atoi(argv[1]);
    }
    if (argc > 2) {
        steps = atoi(argv[2]);
    }
    if (argc > 3) {
        max_threads = atoi(argv[3]);
    }
    if (argc > 4) {
        dt = atof(argv[4]);
    }
    if (argc > 5) {
        const char *backend = argv[5];
        if (strcmp(backend, "tbb") == 0) {
            scheduler = CHRONO_ISLAND_SCHED_TBB;
        } else if (strcmp(backend, "openmp") == 0) {
            scheduler = CHRONO_ISLAND_SCHED_OPENMP;
        } else if (strcmp(backend, "auto") == 0) {
            scheduler = CHRONO_ISLAND_SCHED_AUTO;
        } else {
            fprintf(stderr, "Unknown scheduler '%s' (expected auto|openmp|tbb)\n", backend);
            return 1;
        }
    }

    if (num_islands <= 0 || steps <= 0) {
        fprintf(stderr,
                "Usage: %s [islands] [steps] [max_threads] [dt] [scheduler(auto|openmp|tbb)]\n",
                argv[0]);
        return 1;
    }

#ifndef _OPENMP
    max_threads = 1;
#else
    if (max_threads <= 0) {
        max_threads = omp_get_max_threads();
    } else {
        int hw_threads = omp_get_max_threads();
        if (max_threads > hw_threads) {
            max_threads = hw_threads;
        }
    }
#endif

    double *baseline_positions = (double *)calloc((size_t)num_islands * 2, sizeof(double));
    double *baseline_velocities = (double *)calloc((size_t)num_islands * 2, sizeof(double));
    double *trial_positions = (double *)calloc((size_t)num_islands * 2, sizeof(double));
    double *trial_velocities = (double *)calloc((size_t)num_islands * 2, sizeof(double));

    if (!baseline_positions || !baseline_velocities || !trial_positions || !trial_velocities) {
        fprintf(stderr, "Failed to allocate benchmark buffers.\n");
        free(baseline_positions);
        free(baseline_velocities);
        free(trial_positions);
        free(trial_velocities);
        return 1;
    }

    printf("Island solver benchmark: islands=%d steps=%d dt=%.4f max_threads=%d scheduler=%d\n",
           num_islands,
           steps,
           dt,
           max_threads,
           (int)scheduler);

    double baseline_time = 0.0;
    if (!run_benchmark_iteration(num_islands,
                                 steps,
                                 1,
                                 dt,
                                 baseline_positions,
                                 baseline_velocities,
                                 &baseline_time,
                                 scheduler)) {
        free(baseline_positions);
        free(baseline_velocities);
        free(trial_positions);
        free(trial_velocities);
        return 1;
    }
    printf(" threads=1 elapsed=%.6f s\n", baseline_time);

    for (int threads = 2; threads <= max_threads; ++threads) {
        double elapsed = 0.0;
        if (!run_benchmark_iteration(num_islands,
                                     steps,
                                     threads,
                                     dt,
                                     trial_positions,
                                     trial_velocities,
                                     &elapsed,
                                     scheduler)) {
            free(baseline_positions);
            free(baseline_velocities);
            free(trial_positions);
            free(trial_velocities);
            return 1;
        }

        if (!compare_results(baseline_positions,
                             trial_positions,
                             num_islands * 2,
                             1e-6,
                             "position") ||
            !compare_results(baseline_velocities,
                             trial_velocities,
                             num_islands * 2,
                             1e-6,
                             "velocity")) {
            fprintf(stderr, "[bench] thread safety regression detected for %d threads\n", threads);
            free(baseline_positions);
            free(baseline_velocities);
            free(trial_positions);
            free(trial_velocities);
            return 1;
        }

        double speedup = baseline_time > 0.0 ? baseline_time / elapsed : 0.0;
        printf(" threads=%d elapsed=%.6f s speedup=%.3f\n", threads, elapsed, speedup);
    }

    free(baseline_positions);
    free(baseline_velocities);
    free(trial_positions);
    free(trial_velocities);

    printf("Benchmark completed successfully.\n");
    return 0;
}
