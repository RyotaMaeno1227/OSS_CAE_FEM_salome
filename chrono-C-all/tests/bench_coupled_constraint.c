#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_logging.h"

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

static void configure_warning_policy(ChronoCoupledConstraint2D_C *constraint) {
    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(constraint, &policy);
    policy.enable_logging = 0;
    policy.enable_auto_recover = 0;
    policy.max_drop = 1;
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

static void bench_case(int equation_count, double epsilon) {
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
    configure_warning_policy(&constraint);
    configure_equations(&constraint, equation_count, epsilon);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 6;
    cfg.enable_parallel = 0;

    const double dt = 0.0025;
    const int warmup_steps = 40;
    const int measure_steps = 400;

    for (int step = 0; step < warmup_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
    }

    double max_condition = 0.0;
    double sum_condition = 0.0;
    int condition_samples = 0;

    double start_time = bench_now();
    for (int step = 0; step < measure_steps; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
        const ChronoCoupledConstraint2DDiagnostics_C *diag =
            chrono_coupled_constraint2d_get_diagnostics(&constraint);
        if (diag) {
            double condition = diag->condition_number;
            max_condition = fmax(max_condition, condition);
            sum_condition += condition;
            condition_samples += 1;
        }
    }
    double end_time = bench_now();

    double avg_condition = condition_samples > 0 ? sum_condition / (double)condition_samples : 0.0;
    double avg_time_us = ((end_time - start_time) / (double)measure_steps) * 1e6;

    printf("%d,%.1e,%.6e,%.6e,%.3f\n",
           equation_count,
           epsilon,
           max_condition,
           avg_condition,
           avg_time_us);
}

int main(void) {
    const int equation_counts[] = {1, 2, 3, 4};
    const size_t eq_count_total = sizeof(equation_counts) / sizeof(equation_counts[0]);
    const double epsilons[] = {0.0, 1e-4, 1e-6, 1e-8};
    const size_t epsilon_total = sizeof(epsilons) / sizeof(epsilons[0]);

    chrono_log_set_level(CHRONO_LOG_LEVEL_WARNING);
    printf("eq_count,epsilon,max_condition,avg_condition,avg_solve_time_us\n");

    for (size_t i = 0; i < eq_count_total; ++i) {
        int eq_count = equation_counts[i];
        for (size_t j = 0; j < epsilon_total; ++j) {
            double epsilon = epsilons[j];
            if (eq_count == 1 && epsilon != 0.0) {
                continue;
            }
            bench_case(eq_count, epsilon);
        }
    }

    return 0;
}
