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
#include "../include/chrono_constraint_kkt_backend.h"
#include "../include/chrono_logging.h"

typedef struct {
    int eq_count;
    double epsilon;
    double solver_omega;
    double solver_sharpness;
    double solver_tolerance;
    double max_condition;
    double avg_condition;
    double max_condition_spectral;
    double avg_condition_spectral;
    double max_condition_gap;
    double avg_condition_gap;
    double min_pivot;
    double max_pivot;
    double avg_solve_time_us;
    int drop_events;
    unsigned int drop_index_mask;
    int recovery_events;
    double avg_recovery_steps;
    int max_recovery_steps;
    int unrecovered_drops;
    int max_pending_steps;
    const char *scenario;
} CoupledBenchResult;

typedef void (*BenchSetupFunc)(ChronoCoupledConstraint2D_C *constraint, int equation_count, double epsilon);

static int write_stats_json(const char *path);

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

static void setup_default_case(ChronoCoupledConstraint2D_C *constraint, int equation_count, double epsilon) {
    configure_equations(constraint, equation_count, epsilon);
    configure_warning_policy(constraint, equation_count);
}

static void setup_spectral_stress_case(ChronoCoupledConstraint2D_C *constraint, int equation_count, double epsilon) {
    (void)equation_count;
    (void)epsilon;

    chrono_coupled_constraint2d_clear_equations(constraint);

    chrono_coupled_constraint2d_set_softness_distance(constraint, 1e-6);
    chrono_coupled_constraint2d_set_softness_angle(constraint, 1e-6);
    chrono_coupled_constraint2d_set_distance_spring(constraint, 0.0, 0.0);
    chrono_coupled_constraint2d_set_angle_spring(constraint, 0.0, 0.0);

    ChronoCoupledConstraint2DEquationDesc_C desc;

    memset(&desc, 0, sizeof(desc));
    desc.ratio_distance = 1.0;
    desc.ratio_angle = 0.0;
    desc.softness_distance = 1e-12;
    desc.softness_angle = 1e-12;
    chrono_coupled_constraint2d_add_equation(constraint, &desc);

    memset(&desc, 0, sizeof(desc));
    desc.ratio_distance = 1.0e9;
    desc.ratio_angle = 0.0;
    desc.softness_distance = 1e-12;
    desc.softness_angle = 1e-12;
    chrono_coupled_constraint2d_add_equation(constraint, &desc);

    memset(&desc, 0, sizeof(desc));
    desc.ratio_distance = 0.0;
    desc.ratio_angle = 1.0;
    desc.softness_distance = 1e-12;
    desc.softness_angle = 1e-12;
    chrono_coupled_constraint2d_add_equation(constraint, &desc);

    memset(&desc, 0, sizeof(desc));
    desc.ratio_distance = 1.0e9;
    desc.ratio_angle = -1.0e3;
    desc.softness_distance = 1e-10;
    desc.softness_angle = 1e-10;
    chrono_coupled_constraint2d_add_equation(constraint, &desc);

    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(constraint, &policy);
    policy.enable_logging = 0;
    policy.enable_auto_recover = 0;
    policy.max_drop = 0;
    chrono_coupled_constraint2d_set_condition_warning_policy(constraint, &policy);
}

static CoupledBenchResult bench_case(int equation_count,
                                     double epsilon,
                                     const char *scenario,
                                     BenchSetupFunc setup,
                                     double solver_omega) {
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
    if (setup) {
        setup(&constraint, equation_count, epsilon);
    } else {
        setup_default_case(&constraint, equation_count, epsilon);
    }

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 6;
    cfg.enable_parallel = 0;
    cfg.iterative.omega = solver_omega;
    cfg.iterative.sharpness = 1.0;
    cfg.iterative.tolerance = 1e-6;
    cfg.iterative.enable_warm_start = 1;
    cfg.iterative.record_violation_history = 0;
    cfg.iterative.max_iterations_override = 0;

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
    double max_condition_spectral = 0.0;
    double sum_condition_spectral = 0.0;
    double max_condition_gap = 0.0;
    double sum_condition_gap = 0.0;
    double min_pivot = 0.0;
    double max_pivot = 0.0;
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
            double condition_bound = diag->condition_number;
            double condition_spectral = diag->condition_number_spectral;
            if (condition_spectral <= 0.0) {
                condition_spectral = condition_bound;
            }
            double condition_gap = fabs(condition_spectral - condition_bound);
            max_condition = fmax(max_condition, condition_bound);
            max_condition_spectral = fmax(max_condition_spectral, condition_spectral);
            max_condition_gap = fmax(max_condition_gap, condition_gap);
            sum_condition += condition_bound;
            sum_condition_spectral += condition_spectral;
            sum_condition_gap += condition_gap;
            condition_samples += 1;
            if (diag->min_pivot > 0.0) {
                if (min_pivot == 0.0 || diag->min_pivot < min_pivot) {
                    min_pivot = diag->min_pivot;
                }
            }
            if (diag->max_pivot > 0.0) {
                max_pivot = fmax(max_pivot, diag->max_pivot);
            }
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
    double avg_condition_spectral =
        condition_samples > 0 ? sum_condition_spectral / (double)condition_samples : 0.0;
    double avg_condition_gap =
        condition_samples > 0 ? sum_condition_gap / (double)condition_samples : 0.0;
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
    result.solver_omega = solver_omega;
    result.solver_sharpness = cfg.iterative.sharpness;
    result.solver_tolerance = cfg.iterative.tolerance;
    result.max_condition = max_condition;
    result.avg_condition = avg_condition;
    result.max_condition_spectral = max_condition_spectral;
    result.avg_condition_spectral = avg_condition_spectral;
    result.max_condition_gap = max_condition_gap;
    result.avg_condition_gap = avg_condition_gap;
    result.min_pivot = min_pivot;
    result.max_pivot = max_pivot;
    result.avg_solve_time_us = avg_time_us;
    result.drop_events = drop_events;
    result.drop_index_mask = drop_index_mask;
    result.recovery_events = recovery_events;
    result.avg_recovery_steps =
        (recovery_events > 0) ? (recovery_steps_total / (double)recovery_events) : 0.0;
    result.max_recovery_steps = max_recovery_steps;
    result.unrecovered_drops = unrecovered;
    result.max_pending_steps = max_pending_steps;
    result.scenario = scenario ? scenario : "default";
    return result;
}

static void write_result(FILE *out, const CoupledBenchResult *result) {
    fprintf(out,
            "%d,%.1e,%.3f,%.3f,%.3e,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e,%.3f,%d,%u,%d,%.3f,%d,%d,%d,%s\n",
            result->eq_count,
            result->epsilon,
            result->solver_omega,
            result->solver_sharpness,
            result->solver_tolerance,
            result->max_condition,
            result->avg_condition,
            result->max_condition_spectral,
            result->avg_condition_spectral,
            result->max_condition_gap,
            result->avg_condition_gap,
            result->min_pivot,
            result->max_pivot,
            result->avg_solve_time_us,
            result->drop_events,
            result->drop_index_mask,
            result->recovery_events,
            result->avg_recovery_steps,
            result->max_recovery_steps,
            result->unrecovered_drops,
            result->max_pending_steps,
            result->scenario ? result->scenario : "default");
}

static int write_stats_json(const char *path) {
    if (!path) {
        return 1;
    }
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return 0;
    }
    const ChronoKKTBackendStats_C *stats = chrono_kkt_backend_get_stats();
    double hit_rate = 0.0;
    if (stats->cache_checks > 0) {
        hit_rate = (double)stats->cache_hits / (double)stats->cache_checks;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "  \"calls\": %lu,\n", stats->calls);
    fprintf(fp, "  \"fallback_calls\": %lu,\n", stats->fallback_calls);
    fprintf(fp, "  \"cache_hits\": %lu,\n", stats->cache_hits);
    fprintf(fp, "  \"cache_misses\": %lu,\n", stats->cache_misses);
    fprintf(fp, "  \"cache_checks\": %lu,\n", stats->cache_checks);
    fprintf(fp, "  \"cache_hit_rate\": %.6f,\n", hit_rate);
    fprintf(fp, "  \"size_histogram\": [");
    for (int i = 0; i <= CHRONO_COUPLED_KKT_MAX_EQ; ++i) {
        fprintf(fp, "%s%lu", (i == 0 ? "" : ", "), stats->size_histogram[i]);
    }
    fprintf(fp, "]\n}\n");
    fclose(fp);
    return 1;
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [--output path] [--omega value] [--stats-json path]\n", program);
    fprintf(stderr, "  --omega value   Append a solver over-relaxation factor (can be repeated).\n");
    fprintf(stderr, "  --stats-json    Dump Chrono KKT backend stats to the given JSON file.\n");
}

int main(int argc, char **argv) {
    const char *output_path = NULL;
    const char *stats_json_path = NULL;
    double omega_values[8];
    size_t omega_count = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            output_path = argv[++i];
        } else if (strncmp(argv[i], "--omega=", 8) == 0) {
            const char *value = argv[i] + 8;
            if (!*value) {
                fprintf(stderr, "Missing value for --omega\n");
                return 1;
            }
            if (omega_count >= sizeof(omega_values) / sizeof(omega_values[0])) {
                fprintf(stderr, "Too many --omega values (max %zu)\n",
                        sizeof(omega_values) / sizeof(omega_values[0]));
                return 1;
            }
            char *end_ptr = NULL;
            double parsed = strtod(value, &end_ptr);
            if (!end_ptr || *end_ptr != '\0' || parsed <= 0.0) {
                fprintf(stderr, "Invalid omega value: %s\n", value);
                return 1;
            }
            omega_values[omega_count++] = parsed;
        } else if (strcmp(argv[i], "--omega") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            const char *value = argv[++i];
            if (omega_count >= sizeof(omega_values) / sizeof(omega_values[0])) {
                fprintf(stderr, "Too many --omega values (max %zu)\n",
                        sizeof(omega_values) / sizeof(omega_values[0]));
                return 1;
            }
            char *end_ptr = NULL;
            double parsed = strtod(value, &end_ptr);
            if (!end_ptr || *end_ptr != '\0' || parsed <= 0.0) {
                fprintf(stderr, "Invalid omega value: %s\n", value);
                return 1;
            }
            omega_values[omega_count++] = parsed;
        } else if (strcmp(argv[i], "--stats-json") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            stats_json_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (omega_count == 0) {
        omega_values[0] = 1.0;
        omega_count = 1;
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
            "eq_count,epsilon,omega,sharpness,tolerance,max_condition,avg_condition,max_condition_spectral,"
            "avg_condition_spectral,max_condition_gap,avg_condition_gap,min_pivot,max_pivot,avg_solve_time_us,drop_events,"
            "drop_index_mask,recovery_events,avg_recovery_steps,max_recovery_steps,unrecovered_drops,"
            "max_pending_steps,scenario\n");

    const int equation_counts[] = {1, 2, 3, 4};
    const size_t eq_count_total = sizeof(equation_counts) / sizeof(equation_counts[0]);
    const double epsilons[] = {0.0, 1e-4, 1e-6, 1e-8};
    const size_t epsilon_total = sizeof(epsilons) / sizeof(epsilons[0]);

    chrono_log_set_level(CHRONO_LOG_LEVEL_ERROR);
    chrono_kkt_backend_reset_stats();

    for (size_t omega_idx = 0; omega_idx < omega_count; ++omega_idx) {
        double omega = omega_values[omega_idx];
        for (size_t i = 0; i < eq_count_total; ++i) {
            int eq_count = equation_counts[i];
            for (size_t j = 0; j < epsilon_total; ++j) {
                double epsilon = epsilons[j];
                if (eq_count == 1 && epsilon != 0.0) {
                    continue;
                }
                CoupledBenchResult result = bench_case(eq_count, epsilon, "default", NULL, omega);
                write_result(out, &result);
            }
        }

        CoupledBenchResult stress =
            bench_case(4, 0.0, "spectral_stress", setup_spectral_stress_case, omega);
        write_result(out, &stress);
    }

    if (stats_json_path && !write_stats_json(stats_json_path)) {
        fprintf(stderr, "Failed to write KKT stats JSON to %s\n", stats_json_path);
        if (out != stdout) {
            fclose(out);
        }
        return 1;
    }

    if (out != stdout) {
        fclose(out);
    }

    return 0;
}
