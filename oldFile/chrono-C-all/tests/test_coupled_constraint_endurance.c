#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"

static double compute_distance_raw(const ChronoBody2D_C *anchor,
                                   const ChronoBody2D_C *body) {
    double dx = body->position[0] - anchor->position[0];
    double dy = body->position[1] - anchor->position[1];
    return sqrt(dx * dx + dy * dy);
}

static double compute_distance_constraint(const ChronoBody2D_C *anchor,
                                          const ChronoBody2D_C *body,
                                          const ChronoCoupledConstraint2D_C *constraint) {
    double world_a[2];
    double world_b[2];
    chrono_body2d_local_to_world(anchor, constraint->local_anchor_a, world_a);
    chrono_body2d_local_to_world(body, constraint->local_anchor_b, world_b);
    double dx = world_b[0] - world_a[0];
    double dy = world_b[1] - world_a[1];
    return sqrt(dx * dx + dy * dy);
}

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = -0.15;
    anchor->position[1] = 0.1;
}

static void init_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.1, 0.32);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->position[0] = 0.5;
    body->position[1] = 0.45;
    body->angle = 0.35;
    body->linear_velocity[0] = 0.35;
    body->linear_velocity[1] = -0.28;
    body->angular_velocity = 0.6;
}

static FILE *open_csv(void) {
    FILE *csv = fopen("../data/coupled_constraint_endurance.csv", "w");
    if (!csv) {
        csv = fopen("coupled_constraint_endurance.csv", "w");
    }
    return csv;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    init_anchor(&anchor);
    init_body(&body);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};

    double initial_distance = compute_distance_raw(&anchor, &body);
    double initial_angle = body.angle - anchor.angle;

    ChronoCoupledConstraint2D_C constraint;
    chrono_coupled_constraint2d_init(&constraint,
                                     &anchor,
                                     &body,
                                     local_anchor,
                                     local_anchor,
                                     axis_local,
                                     initial_distance,
                                     initial_angle,
                                     1.0,
                                     0.4,
                                     0.0);
    chrono_coupled_constraint2d_set_baumgarte(&constraint, 0.38);
    chrono_coupled_constraint2d_set_softness_distance(&constraint, 0.014);
    chrono_coupled_constraint2d_set_softness_angle(&constraint, 0.028);
    chrono_coupled_constraint2d_set_distance_spring(&constraint, 36.0, 2.8);
    chrono_coupled_constraint2d_set_angle_spring(&constraint, 16.0, 0.9);
    chrono_coupled_constraint2d_set_slop(&constraint, 7e-4);
    chrono_coupled_constraint2d_set_max_correction(&constraint, 0.09);

    ChronoCoupledConstraint2DEquationDesc_C eq1;
    memset(&eq1, 0, sizeof(eq1));
    eq1.ratio_distance = 0.6;
    eq1.ratio_angle = -0.35;
    eq1.target_offset = 0.015;
    eq1.softness_distance = 0.02;
    eq1.softness_angle = 0.018;
    eq1.spring_distance_stiffness = 22.0;
    eq1.spring_distance_damping = 2.1;
    eq1.spring_angle_stiffness = 11.0;
    eq1.spring_angle_damping = 0.65;
    chrono_coupled_constraint2d_add_equation(&constraint, &eq1);

    ChronoCoupledConstraint2DEquationDesc_C eq2;
    memset(&eq2, 0, sizeof(eq2));
    eq2.ratio_distance = -0.25;
    eq2.ratio_angle = 1.0;
    eq2.target_offset = -0.02;
    eq2.softness_distance = 0.012;
    eq2.softness_angle = 0.04;
    eq2.spring_distance_stiffness = 15.0;
    eq2.spring_distance_damping = 1.8;
    eq2.spring_angle_stiffness = 20.0;
    eq2.spring_angle_damping = 1.0;
    chrono_coupled_constraint2d_add_equation(&constraint, &eq2);

    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(&constraint, &policy);
    policy.enable_logging = 0;
    policy.enable_auto_recover = 1;
    policy.max_drop = chrono_coupled_constraint2d_get_equation_count(&constraint);
    chrono_coupled_constraint2d_set_condition_warning_policy(&constraint, &policy);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    FILE *csv = open_csv();
    if (!csv) {
        fprintf(stderr, "Failed to open CSV for coupled endurance test.\n");
        return 1;
    }
    fprintf(csv,
            "step,time,distance,angle,eq0_force_distance,eq1_force_distance,eq2_force_distance,"
            "eq0_force_angle,eq1_force_angle,eq2_force_angle,eq0_impulse,eq1_impulse,eq2_impulse,"
            "diagnostics_flags,condition_number,condition_number_spectral,condition_gap,min_eigenvalue,"
            "max_eigenvalue,active_equations,drop_events_step,drop_index_mask_step,drop_events_total,"
            "recovery_events_step,recovery_steps_step,recovery_events_total,pending_drop_count,"
            "max_pending_steps_running\n");

    const double dt = 0.0035;
    const int total_steps = 7200;
    const int switch_step_a = 1800;
    const int switch_step_b = 3600;
    const int switch_step_c = 5400;

    double max_distance_error = 0.0;
    double max_angle_error = 0.0;
    double max_distance_force = 0.0;
    double max_angle_force = 0.0;
    double max_condition_bound = 0.0;
    double max_condition_spectral = 0.0;
    double max_condition_gap = 0.0;
    double max_condition_effective = 0.0;
    double min_eigen_observed = DBL_MAX;
    double max_eigen_observed = -DBL_MAX;
    unsigned int accumulated_flags = 0u;

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
    int drop_events_total = 0;
    int recovery_events_total = 0;
    int max_pending_steps_running = 0;

    for (int step = 0; step < total_steps; ++step) {
        if (step == switch_step_a) {
            chrono_coupled_constraint2d_set_rest_distance(&constraint, initial_distance - 0.08);
            chrono_coupled_constraint2d_set_rest_angle(&constraint, initial_angle + 0.18);
        } else if (step == switch_step_b) {
            chrono_coupled_constraint2d_set_rest_distance(&constraint, initial_distance - 0.03);
            chrono_coupled_constraint2d_set_rest_angle(&constraint, initial_angle - 0.12);
            chrono_coupled_constraint2d_set_target_offset(&constraint, 0.01);
        } else if (step == switch_step_c) {
            eq1.target_offset = -0.012;
            eq1.ratio_distance = 0.45;
            eq1.ratio_angle = -0.28;
            chrono_coupled_constraint2d_set_equation(&constraint, 1, &eq1);
            eq2.target_offset = 0.018;
            chrono_coupled_constraint2d_set_equation(&constraint, 2, &eq2);
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);

        body.linear_velocity[0] *= 0.997;
        body.linear_velocity[1] *= 0.997;
        body.angular_velocity *= 0.997;

        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);

        double distance = compute_distance_constraint(&anchor, &body, &constraint);
        double angle = body.angle - anchor.angle;

        double distance_error = fabs(distance - constraint.rest_distance);
        double angle_error = fabs(angle - constraint.rest_angle);
        if (distance_error > max_distance_error) {
            max_distance_error = distance_error;
        }
        if (angle_error > max_angle_error) {
            max_angle_error = angle_error;
        }

        for (int i = 0; i < chrono_coupled_constraint2d_get_equation_count(&constraint); ++i) {
            double df = fabs(constraint.last_distance_force_eq[i]);
            double af = fabs(constraint.last_angle_force_eq[i]);
            if (df > max_distance_force) {
                max_distance_force = df;
            }
            if (af > max_angle_force) {
                max_angle_force = af;
            }
        }

        const ChronoCoupledConstraint2DDiagnostics_C *diag =
            chrono_coupled_constraint2d_get_diagnostics(&constraint);
        unsigned int diag_flags = diag ? diag->flags : 0u;
        double condition_number = diag ? diag->condition_number : 0.0;
        double condition_spectral = diag ? diag->condition_number_spectral : 0.0;
        double condition_gap = fabs(condition_spectral - condition_number);
        double eigen_min = diag ? diag->min_eigenvalue : 0.0;
        double eigen_max = diag ? diag->max_eigenvalue : 0.0;
        double condition_effective = condition_number;
        if (condition_spectral > 0.0) {
            condition_effective = fmax(condition_effective, condition_spectral);
        }
        accumulated_flags |= diag_flags;
        if (condition_number > max_condition_bound) {
            max_condition_bound = condition_number;
        }
        if (condition_spectral > max_condition_spectral) {
            max_condition_spectral = condition_spectral;
        }
        if (condition_gap > max_condition_gap) {
            max_condition_gap = condition_gap;
        }
        if (condition_effective > max_condition_effective) {
            max_condition_effective = condition_effective;
        }
        if (diag) {
            if (eigen_min < min_eigen_observed) {
                min_eigen_observed = eigen_min;
            }
            if (eigen_max > max_eigen_observed) {
                max_eigen_observed = eigen_max;
            }
        }

        int drop_events_step = 0;
        unsigned int drop_index_mask_step = 0u;
        int recovery_events_step = 0;
        int recovery_steps_step = -1;
        for (int eq = 0; eq < total_eq; ++eq) {
            int current_active = constraint.equation_active[eq];
            if (prev_active[eq] && !current_active) {
                drop_events_step += 1;
                drop_index_mask_step |= (1u << eq);
                pending_drop_step[eq] = step;
            } else if (!prev_active[eq] && current_active) {
                recovery_events_step += 1;
                if (pending_drop_step[eq] >= 0) {
                    int steps_to_recover = step - pending_drop_step[eq];
                    if (steps_to_recover > recovery_steps_step) {
                        recovery_steps_step = steps_to_recover;
                    }
                    pending_drop_step[eq] = -1;
                }
            }
            prev_active[eq] = current_active;
        }
        drop_events_total += drop_events_step;
        recovery_events_total += recovery_events_step;

        int pending_drop_count = 0;
        for (int eq = 0; eq < total_eq; ++eq) {
            if (pending_drop_step[eq] >= 0) {
                pending_drop_count += 1;
                int pending_duration = step - pending_drop_step[eq];
                if (pending_duration > max_pending_steps_running) {
                    max_pending_steps_running = pending_duration;
                }
            }
        }

        int active_equations = 0;
        for (int eq = 0; eq < total_eq; ++eq) {
            if (constraint.equation_active[eq]) {
                active_equations += 1;
            }
        }

        double time = step * dt;
        double fd0 = constraint.last_distance_force_eq[0];
        double fd1 = constraint.last_distance_force_eq[1];
        double fd2 = constraint.last_distance_force_eq[2];
        double fa0 = constraint.last_angle_force_eq[0];
        double fa1 = constraint.last_angle_force_eq[1];
        double fa2 = constraint.last_angle_force_eq[2];
        double li0 = constraint.last_distance_impulse_eq[0];
        double li1 = constraint.last_distance_impulse_eq[1];
        double li2 = constraint.last_distance_impulse_eq[2];

        fprintf(csv,
                "%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%.6e,%.6e,%.6e,%.6e,%.6e,%d,%d,%u,%d,%d,%d,%d,%d,%d\n",
                step,
                time,
                distance,
                angle,
                fd0,
                fd1,
                fd2,
                fa0,
                fa1,
                fa2,
                li0,
                li1,
                li2,
                diag_flags,
                condition_number,
                condition_spectral,
                condition_gap,
                eigen_min,
                eigen_max,
                active_equations,
                drop_events_step,
                drop_index_mask_step,
                drop_events_total,
                recovery_events_step,
                recovery_steps_step,
                recovery_events_total,
                pending_drop_count,
                max_pending_steps_running);
    }

    fclose(csv);

    double final_distance = compute_distance_constraint(&anchor, &body, &constraint);
    double final_angle = body.angle - anchor.angle;

    if (!isfinite(final_distance) || !isfinite(final_angle)) {
        fprintf(stderr, "Coupled endurance failed: non-finite final state.\n");
        return 1;
    }

    if (fabs(final_distance - constraint.rest_distance) > 0.018 ||
        fabs(final_angle - constraint.rest_angle) > 0.03) {
        fprintf(stderr,
                "Coupled endurance failed: final mismatch (dist=%.6f, angle=%.6f).\n",
                final_distance - constraint.rest_distance,
                final_angle - constraint.rest_angle);
        return 1;
    }

    if (max_distance_error > 0.28 || max_angle_error > 0.42) {
        fprintf(stderr,
                "Coupled endurance failed: transient error too large (dist=%.6f, angle=%.6f).\n",
                max_distance_error,
                max_angle_error);
        return 1;
    }

    if (max_distance_force > 115.0 || max_angle_force > 65.0) {
        fprintf(stderr,
                "Coupled endurance failed: force spikes (dist=%.6f, angle=%.6f).\n",
                max_distance_force,
                max_angle_force);
        return 1;
    }

    if ((accumulated_flags & CHRONO_COUPLED_DIAG_RANK_DEFICIENT) != 0) {
        fprintf(stderr, "Coupled endurance failed: rank deficiency detected in diagnostics.\n");
        return 1;
    }

    if (max_condition_effective > 5e9) {
        fprintf(stderr,
                "Coupled endurance failed: condition number too high (bound=%.6e, spectral=%.6e).\n",
                max_condition_bound,
                max_condition_spectral);
        return 1;
    }

    printf("Coupled constraint endurance test passed.\n");
    return 0;
}
