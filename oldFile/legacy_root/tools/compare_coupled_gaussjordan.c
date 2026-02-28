#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../chrono-C-all/include/chrono_body2d.h"
#include "../chrono-C-all/include/chrono_constraint2d.h"

#define MAX_EQ CHRONO_COUPLED_MAX_EQ
#define PIVOT_EPSILON 1e-12

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

static void configure_warning_policy(ChronoCoupledConstraint2D_C *constraint, int equation_count, int enable_auto_recover) {
    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(constraint, &policy);
    policy.enable_logging = 0;
    policy.enable_auto_recover = enable_auto_recover ? 1 : 0;
    policy.max_drop = equation_count > 0 ? equation_count : 1;
    if (!enable_auto_recover) {
        policy.max_drop = 0;
    }
    chrono_coupled_constraint2d_set_condition_warning_policy(constraint, &policy);
}

static void configure_equations_default(ChronoCoupledConstraint2D_C *constraint,
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
        chrono_coupled_constraint2d_add_equation(constraint, &desc);
    }
    configure_warning_policy(constraint, equation_count, 1);
}

static void configure_spectral_stress(ChronoCoupledConstraint2D_C *constraint) {
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

    configure_warning_policy(constraint, 4, 0);
}

static int gauss_jordan_unscaled(const double *src, double *dst, int n, double pivot_epsilon) {
    double a[MAX_EQ][MAX_EQ];
    double inv[MAX_EQ][MAX_EQ];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[i][j] = src[i * MAX_EQ + j];
            inv[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
    for (int col = 0; col < n; ++col) {
        int pivot_row = col;
        double pivot_val = fabs(a[pivot_row][col]);
        for (int row = col + 1; row < n; ++row) {
            double val = fabs(a[row][col]);
            if (val > pivot_val) {
                pivot_val = val;
                pivot_row = row;
            }
        }
        if (pivot_val < pivot_epsilon) {
            return 0;
        }
        if (pivot_row != col) {
            for (int k = 0; k < n; ++k) {
                double tmp = a[col][k];
                a[col][k] = a[pivot_row][k];
                a[pivot_row][k] = tmp;
                tmp = inv[col][k];
                inv[col][k] = inv[pivot_row][k];
                inv[pivot_row][k] = tmp;
            }
        }
        double pivot = a[col][col];
        double inv_pivot = 1.0 / pivot;
        for (int k = 0; k < n; ++k) {
            a[col][k] *= inv_pivot;
            inv[col][k] *= inv_pivot;
        }
        for (int row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            double factor = a[row][col];
            if (factor == 0.0) {
                continue;
            }
            for (int k = 0; k < n; ++k) {
                a[row][k] -= factor * a[col][k];
                inv[row][k] -= factor * inv[col][k];
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dst[i * MAX_EQ + j] = inv[i][j];
        }
    }
    return 1;
}

static int gauss_jordan_scaled(const double *src, double *dst, int n, double pivot_epsilon) {
    double a[MAX_EQ][MAX_EQ];
    double inv[MAX_EQ][MAX_EQ];
    double scale[MAX_EQ];
    for (int i = 0; i < n; ++i) {
        double max_row = 0.0;
        for (int j = 0; j < n; ++j) {
            a[i][j] = src[i * MAX_EQ + j];
            inv[i][j] = (i == j) ? 1.0 : 0.0;
            if (fabs(a[i][j]) > max_row) {
                max_row = fabs(a[i][j]);
            }
        }
        if (max_row < pivot_epsilon) {
            max_row = 1.0;
        }
        scale[i] = max_row;
    }

    for (int col = 0; col < n; ++col) {
        int pivot_row = col;
        double pivot_metric = -1.0;
        for (int row = col; row < n; ++row) {
            double val = fabs(a[row][col]);
            double metric = val / scale[row];
            if (metric > pivot_metric) {
                pivot_metric = metric;
                pivot_row = row;
            }
        }
        double pivot_val = fabs(a[pivot_row][col]);
        if (pivot_val < pivot_epsilon) {
            return 0;
        }
        if (pivot_row != col) {
            for (int k = 0; k < n; ++k) {
                double tmp = a[col][k];
                a[col][k] = a[pivot_row][k];
                a[pivot_row][k] = tmp;
                tmp = inv[col][k];
                inv[col][k] = inv[pivot_row][k];
                inv[pivot_row][k] = tmp;
            }
            double tmp_scale = scale[col];
            scale[col] = scale[pivot_row];
            scale[pivot_row] = tmp_scale;
        }
        double pivot = a[col][col];
        double inv_pivot = 1.0 / pivot;
        for (int k = 0; k < n; ++k) {
            a[col][k] *= inv_pivot;
            inv[col][k] *= inv_pivot;
        }
        for (int row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            double factor = a[row][col];
            if (factor == 0.0) {
                continue;
            }
            for (int k = 0; k < n; ++k) {
                a[row][k] -= factor * a[col][k];
                inv[row][k] -= factor * inv[col][k];
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dst[i * MAX_EQ + j] = inv[i][j];
        }
    }
    return 1;
}

static double compute_residual(const double *matrix, const double *inverse, int n) {
    double max_err = 0.0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += matrix[i * MAX_EQ + k] * inverse[k * MAX_EQ + j];
            }
            double target = (i == j) ? 1.0 : 0.0;
            double err = fabs(sum - target);
            if (err > max_err) {
                max_err = err;
            }
        }
    }
    return max_err;
}

static void emit_json(const char *scenario,
                      int n,
                      const double *matrix,
                      double residual_unscaled,
                      double residual_scaled) {
    printf("{\"scenario\":\"%s\",\"n\":%d,\"matrix\":[", scenario, n);
    for (int i = 0; i < n; ++i) {
        printf("[");
        for (int j = 0; j < n; ++j) {
            printf("%.17e", matrix[i * MAX_EQ + j]);
            if (j + 1 < n) {
                printf(",");
            }
        }
        printf("]");
        if (i + 1 < n) {
            printf(",");
        }
    }
    printf("],\"residual_unscaled\":%.17e,\"residual_scaled\":%.17e}\n",
           residual_unscaled,
           residual_scaled);
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;

    const double dt = 0.0025;

    struct ScenarioSpec {
        const char *name;
        int eq_count;
        double epsilon;
        void (*config)(ChronoCoupledConstraint2D_C *, int, double);
        void (*custom)(ChronoCoupledConstraint2D_C *);
    } scenarios[] = {
        {"default_eq4", 4, 0.0, configure_equations_default, NULL},
        {"spectral_stress", 0, 0.0, NULL, configure_spectral_stress},
    };

    const size_t scenario_count = sizeof(scenarios) / sizeof(scenarios[0]);

    for (size_t idx = 0; idx < scenario_count; ++idx) {
        struct ScenarioSpec spec = scenarios[idx];

        init_anchor(&anchor);
        init_body(&body);

        ChronoCoupledConstraint2D_C constraint;
        double local_anchor[2] = {0.0, 0.0};
        double axis_local[2] = {1.0, 0.0};
        chrono_coupled_constraint2d_init(&constraint,
                                         &anchor,
                                         &body,
                                         local_anchor,
                                         local_anchor,
                                         axis_local,
                                         0.5,
                                         0.0,
                                         1.0,
                                         0.0,
                                         0.0);

        if (spec.config) {
            spec.config(&constraint, spec.eq_count, spec.epsilon);
        } else if (spec.custom) {
            spec.custom(&constraint);
        }

        chrono_coupled_constraint2d_prepare(&constraint, dt);

        int n = 0;
        for (int i = 0; i < constraint.equation_count; ++i) {
            if (constraint.equation_active[i]) {
                ++n;
            }
        }
        if (n == 0) {
            fprintf(stderr, "Scenario %s has no active equations.\n", spec.name);
            continue;
        }

        double matrix[MAX_EQ * MAX_EQ] = {0.0};
        int row_idx = 0;
        for (int i = 0; i < constraint.equation_count; ++i) {
            if (!constraint.equation_active[i]) {
                continue;
            }
            int col_idx = 0;
            for (int j = 0; j < constraint.equation_count; ++j) {
                if (!constraint.equation_active[j]) {
                    continue;
                }
                matrix[row_idx * MAX_EQ + col_idx] = constraint.system_matrix[i][j];
                ++col_idx;
            }
            ++row_idx;
        }

        double inv_unscaled[MAX_EQ * MAX_EQ] = {0.0};
        double inv_scaled[MAX_EQ * MAX_EQ] = {0.0};

        if (!gauss_jordan_unscaled(matrix, inv_unscaled, n, PIVOT_EPSILON)) {
            fprintf(stderr, "Unscaled Gauss-Jordan failed for %s\n", spec.name);
            continue;
        }
        if (!gauss_jordan_scaled(matrix, inv_scaled, n, PIVOT_EPSILON)) {
            fprintf(stderr, "Scaled Gauss-Jordan failed for %s\n", spec.name);
            continue;
        }

        double res_unscaled = compute_residual(matrix, inv_unscaled, n);
        double res_scaled = compute_residual(matrix, inv_scaled, n);

        emit_json(spec.name, n, matrix, res_unscaled, res_scaled);
    }

    return 0;
}
