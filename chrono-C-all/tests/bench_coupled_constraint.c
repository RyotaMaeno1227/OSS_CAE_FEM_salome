#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_logging.h"

typedef struct {
    int eq_count;
    double epsilon;
    double max_condition;
    double avg_condition;
    double avg_solve_time_us;
    int drop_events;
    unsigned int drop_index_mask;
    int recovery_events;
    double avg_recovery_steps;
    int max_recovery_steps;
    int unrecovered_drops;
    int max_pending_steps;
} CoupledBenchResult;

static double bench_now(void) {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = -0.2;
    anchor->position[1] = 0.1;
}

static void init_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.35);
    chrono_body2d_set_circle_shape(body, 0.25);
    body->position[0] = 0.45;
    body->position[1] = 0.6;
    body->angle = 0.3;
    body->linear_velocity[0] = 0.35;
    body->linear_velocity[1] = -0.25;
    body->angular_velocity = 0.45;
}

static void configure_warning_policy(ChronoCoupledConstraint2D_C *constraint, int equation_count) {
    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(constraint, &policy);
    policy.enable_logging = 0;
    policy.enable_auto_recover = 1;
    policy.max_drop = equation_count > 0 ? equation_count : 1;
    chrono_coupled_constraint2d_set_condition_warning_policy(constraint, &policy);
}

static void configure_equations(ChronoCoupledConstraint2D_C *constraint,
                                int equation_count,
                                double epsilon) {
    chrono_coupled_constraint2d_set_softness_distance(constraint, 0.015);
    chrono_coupled_constraint2d_set_softness_angle(constraint, 0.03);
    chrono_coupled_constraint2d_set_distance_spring(constraint, 32.0, 2.5);
    chrono_coupled_constraint2d_set_angle_spring(constraint, 14.0, 0.8);

    for (int index = 1; index < equation_count; ++index) {
        ChronoCoupledConstraint2DEquationDesc_C desc;
        memset(&desc, 0, sizeof(desc));
        double scale = 1.0 + (double)index;
        desc.ratio_distance = 1.0 + epsilon * scale;
        desc.ratio_angle = epsilon * scale;
        desc.softness_distance = 0.015;
        desc.softness_angle = 0.03;
        if (chrono_coupled_constraint2d_add_equation(constraint, &desc) < 0) {
            fprintf(stderr, "Failed to add equation %d for epsilon %.2e\n", index, epsilon);
        }
    }
}

static void reset_bodies(ChronoBody2D_C *anchor, ChronoBody2D_C *body) {
    init_anchor(anchor);
    init_body(body);
}

static CoupledBenchResult bench_case(int equation_count, double epsilon) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    reset_bodies(&anchor, &body);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};
    double rest_distance = 0.5;
    double rest_angle = 0.0;

    ChronoCoupledConstraint2D_C constraint;
    chrono_coupled_constraint2d_init(&constraint,
                                     &anchor,
                                     &body,
                                     local_anchor,
                                     local_anchor,
                                     axis_local,
                                     rest_distance,
                                     rest_angle,
                                     1.0,
                                     0.0,
                                     0.0);
    configure_equations(&constraint, equation_count, epsilon);
    configure_warning_policy(&constraint, equation_count);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 6;
    cfg.enable_parallel = 0;

    const double dt = 0.0025;
    const int warmup_steps = 40;
    const int measure_steps = 400;
    const int total_steps = warmup_steps + measure_steps;

    int total_eq = chrono_coupled_constraint2d_get_equation_count(&constraint);
    if (total_eq < 1) {
        total_eq = 1;
    }

    int prev_active[CHRONO_COUPLED_MAX_EQ];
    int pending_drop_step[CHRONO_COUPLED_MAX_EQ];
    for (int i = 0; i < total_eq; ++i) {
        prev_active[i] = constraint.equation_active[i];
        pending_drop_step[i] = -1;
    }

    int drop_events = 0;
    unsigned int drop_index_mask = 0u;
    int recovery_events = 0;
    double recovery_steps_total = 0.0;
    int max_recovery_steps = 0;
    int max_pending_steps = 0;

    double max_condition = 0.0;
    double sum_condition = 0.0;
    int condition_samples = 0;

    double start_time = 0.0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == warmup_steps) {
            start_time = bench_now();
            sum_condition = 0.0;
            condition_samples = 0;
            max_condition = 0.0;
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);

        const ChronoCoupledConstraint2DDiagnostics_C *diag =
            chrono_coupled_constraint2d_get_diagnostics(&constraint);
        if (diag && step >= warmup_steps) {
            double condition = diag->condition_number;
            max_condition = fmax(max_condition, condition);
            sum_condition += condition;
            condition_samples += 1;
        }

        for (int eq = 0; eq < total_eq; ++eq) {
            int current_active = constraint.equation_active[eq];
            if (prev_active[eq] && !current_active) {
                drop_events += 1;
                drop_index_mask |= (1u << eq);
                pending_drop_step[eq] = step;
            } else if (!prev_active[eq] && current_active) {
                recovery_events += 1;
                if (pending_drop_step[eq] >= 0) {
                    int recovery_steps = step - pending_drop_step[eq];
                    recovery_steps_total += (double)recovery_steps;
                    if (recovery_steps > max_recovery_steps) {
                        max_recovery_steps = recovery_steps;
                    }
                    pending_drop_step[eq] = -1;
                }
            }
            prev_active[eq] = current_active;
        }

        for (int eq = 0; eq < total_eq; ++eq) {
            if (pending_drop_step[eq] >= 0) {
                int pending_duration = step - pending_drop_step[eq];
                if (pending_duration > max_pending_steps) {
                    max_pending_steps = pending_duration;
                }
            }
        }
    }

    double end_time = bench_now();
    double avg_condition = condition_samples > 0 ? sum_condition / (double)condition_samples : 0.0;
    double avg_time_us = ((end_time - start_time) / (double)measure_steps) * 1e6;

    int unrecovered = 0;
    for (int eq = 0; eq < total_eq; ++eq) {
        if (pending_drop_step[eq] >= 0) {
            unrecovered += 1;
            int pending_duration = total_steps - pending_drop_step[eq];
            if (pending_duration > max_pending_steps) {
                max_pending_steps = pending_duration;
            }
        }
    }

    CoupledBenchResult result;
    result.eq_count = equation_count;
    result.epsilon = epsilon;
    result.max_condition = max_condition;
    result.avg_condition = avg_condition;
    result.avg_solve_time_us = avg_time_us;
    result.drop_events = drop_events;
    result.drop_index_mask = drop_index_mask;
    result.recovery_events = recovery_events;
    result.avg_recovery_steps =
        (recovery_events > 0) ? (recovery_steps_total / (double)recovery_events) : 0.0;
    result.max_recovery_steps = max_recovery_steps;
    result.unrecovered_drops = unrecovered;
    result.max_pending_steps = max_pending_steps;
    return result;
}

static void write_result(FILE *out, const CoupledBenchResult *result) {
    fprintf(out,
            "%d,%.1e,%.6e,%.6e,%.3f,%d,%u,%d,%.3f,%d,%d,%d\n",
            result->eq_count,
            result->epsilon,
            result->max_condition,
            result->avg_condition,
            result->avg_solve_time_us,
            result->drop_events,
            result->drop_index_mask,
            result->recovery_events,
            result->avg_recovery_steps,
            result->max_recovery_steps,
            result->unrecovered_drops,
            result->max_pending_steps);
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [--output path]\n", program);
}

int main(int argc, char **argv) {
    const char *output_path = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "Failed to open output file: %s\n", output_path);
            return 1;
        }
    }

    fprintf(out,
            "eq_count,epsilon,max_condition,avg_condition,avg_solve_time_us,"
            "drop_events,drop_index_mask,recovery_events,avg_recovery_steps,"
            "max_recovery_steps,unrecovered_drops,max_pending_steps\n");

    const int equation_counts[] = {1, 2, 3, 4};
    const size_t eq_count_total = sizeof(equation_counts) / sizeof(equation_counts[0]);
    const double epsilons[] = {0.0, 1e-4, 1e-6, 1e-8};
    const size_t epsilon_total = sizeof(epsilons) / sizeof(epsilons[0]);

    chrono_log_set_level(CHRONO_LOG_LEVEL_ERROR);

    for (size_t i = 0; i < eq_count_total; ++i) {
        int eq_count = equation_counts[i];
        for (size_t j = 0; j < epsilon_total; ++j) {
            double epsilon = epsilons[j];
            if (eq_count == 1 && epsilon != 0.0) {
                continue;
            }
            CoupledBenchResult result = bench_case(eq_count, epsilon);
            write_result(out, &result);
        }
    }

    if (out != stdout) {
        fclose(out);
    }

    return 0;
}
