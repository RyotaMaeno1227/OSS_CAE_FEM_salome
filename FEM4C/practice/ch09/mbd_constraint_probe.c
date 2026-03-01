#include <math.h>
#include <stdio.h>

#include "common/error.h"
#include "mbd/constraint2d.h"
#include "mbd/kkt2d.h"

#define FD_EPS 1.0e-7
#define RESIDUAL_TOL 1.0e-12
#define JACOBIAN_TOL 1.0e-6

typedef struct {
    const char *name;
    double distance_anchor_i[2];
    double distance_anchor_j[2];
    double distance_target;
    double revolute_anchor_i[2];
    double revolute_anchor_j[2];
    mbd_body_state2d_t state_i;
    mbd_body_state2d_t state_j;
} probe_case_t;

static void rotate_local(const double local[2], double theta, double out[2])
{
    const double c = cos(theta);
    const double s = sin(theta);
    out[0] = c * local[0] - s * local[1];
    out[1] = s * local[0] + c * local[1];
}

static void world_anchor(const mbd_body_state2d_t *state, const double local[2], double out[2])
{
    double rot[2];
    rotate_local(local, state->theta, rot);
    out[0] = state->x + rot[0];
    out[1] = state->y + rot[1];
}

static fem_error_t eval_residual_only(const mbd_constraint2d_t *c,
                                      const mbd_body_state2d_t *state_i,
                                      const mbd_body_state2d_t *state_j,
                                      double residual[MBD_CONSTRAINT2D_MAX_EQ],
                                      int *num_equations)
{
    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    return mbd_constraint_evaluate(c, state_i, state_j, residual, jac_i, jac_j, num_equations);
}

static void perturb_state(mbd_body_state2d_t *state, int dof_index, double delta)
{
    if (dof_index == 0) {
        state->x += delta;
        return;
    }
    if (dof_index == 1) {
        state->y += delta;
        return;
    }
    state->theta += delta;
}

static int check_distance_residual(const char *case_name,
                                   const mbd_constraint2d_t *constraint,
                                   const mbd_body_state2d_t *state_i,
                                   const mbd_body_state2d_t *state_j)
{
    double residual[MBD_CONSTRAINT2D_MAX_EQ] = {0.0};
    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    int num_equations = 0;
    double pi[2];
    double pj[2];
    double dx;
    double dy;
    double expected;
    double diff;
    fem_error_t err = mbd_constraint_evaluate(
        constraint, state_i, state_j, residual, jac_i, jac_j, &num_equations);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] distance evaluate failed: %s\n", case_name, error_get_message());
        return 1;
    }
    if (num_equations != 1) {
        fprintf(stderr, "[%s] distance equations mismatch: got=%d expected=1\n", case_name, num_equations);
        return 1;
    }

    world_anchor(state_i, constraint->anchor_i, pi);
    world_anchor(state_j, constraint->anchor_j, pj);
    dx = pi[0] - pj[0];
    dy = pi[1] - pj[1];
    expected = 0.5 * (dx * dx + dy * dy - constraint->target_value * constraint->target_value);
    diff = fabs(residual[0] - expected);
    if (diff > RESIDUAL_TOL) {
        fprintf(stderr,
                "[%s] distance residual mismatch: got=%.15e expected=%.15e diff=%.3e (tol=%.1e)\n",
                case_name,
                residual[0],
                expected,
                diff,
                RESIDUAL_TOL);
        return 1;
    }
    return 0;
}

static int check_revolute_residual(const char *case_name,
                                   const mbd_constraint2d_t *constraint,
                                   const mbd_body_state2d_t *state_i,
                                   const mbd_body_state2d_t *state_j)
{
    double residual[MBD_CONSTRAINT2D_MAX_EQ] = {0.0};
    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    int num_equations = 0;
    double pi[2];
    double pj[2];
    double expected[2];
    fem_error_t err = mbd_constraint_evaluate(
        constraint, state_i, state_j, residual, jac_i, jac_j, &num_equations);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] revolute evaluate failed: %s\n", case_name, error_get_message());
        return 1;
    }
    if (num_equations != 2) {
        fprintf(stderr, "[%s] revolute equations mismatch: got=%d expected=2\n", case_name, num_equations);
        return 1;
    }

    world_anchor(state_i, constraint->anchor_i, pi);
    world_anchor(state_j, constraint->anchor_j, pj);
    expected[0] = pi[0] - pj[0];
    expected[1] = pi[1] - pj[1];

    for (int eq = 0; eq < 2; ++eq) {
        double diff = fabs(residual[eq] - expected[eq]);
        if (diff > RESIDUAL_TOL) {
            fprintf(stderr,
                    "[%s] revolute residual[%d] mismatch: got=%.15e expected=%.15e diff=%.3e (tol=%.1e)\n",
                    case_name,
                    eq,
                    residual[eq],
                    expected[eq],
                    diff,
                    RESIDUAL_TOL);
            return 1;
        }
    }
    return 0;
}

static int check_jacobian_fd(const char *case_name,
                             const char *constraint_name,
                             const mbd_constraint2d_t *constraint,
                             const mbd_body_state2d_t *state_i,
                             const mbd_body_state2d_t *state_j,
                             double *max_diff_out)
{
    double residual[MBD_CONSTRAINT2D_MAX_EQ] = {0.0};
    double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF] = {{0.0}};
    int num_equations = 0;
    double max_diff = 0.0;
    fem_error_t err = mbd_constraint_evaluate(
        constraint, state_i, state_j, residual, jac_i, jac_j, &num_equations);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] %s evaluate failed: %s\n", case_name, constraint_name, error_get_message());
        return 1;
    }

    for (int body = 0; body < 2; ++body) {
        for (int dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
            mbd_body_state2d_t plus_i = *state_i;
            mbd_body_state2d_t minus_i = *state_i;
            mbd_body_state2d_t plus_j = *state_j;
            mbd_body_state2d_t minus_j = *state_j;
            double plus_res[MBD_CONSTRAINT2D_MAX_EQ] = {0.0};
            double minus_res[MBD_CONSTRAINT2D_MAX_EQ] = {0.0};
            int plus_eq = 0;
            int minus_eq = 0;

            if (body == 0) {
                perturb_state(&plus_i, dof, FD_EPS);
                perturb_state(&minus_i, dof, -FD_EPS);
            } else {
                perturb_state(&plus_j, dof, FD_EPS);
                perturb_state(&minus_j, dof, -FD_EPS);
            }

            err = eval_residual_only(constraint, &plus_i, &plus_j, plus_res, &plus_eq);
            if (err != FEM_SUCCESS) {
                fprintf(stderr,
                        "[%s] %s FD plus evaluate failed: %s\n",
                        case_name,
                        constraint_name,
                        error_get_message());
                return 1;
            }
            err = eval_residual_only(constraint, &minus_i, &minus_j, minus_res, &minus_eq);
            if (err != FEM_SUCCESS) {
                fprintf(stderr,
                        "[%s] %s FD minus evaluate failed: %s\n",
                        case_name,
                        constraint_name,
                        error_get_message());
                return 1;
            }
            if (plus_eq != num_equations || minus_eq != num_equations) {
                fprintf(stderr,
                        "[%s] %s FD equation mismatch: base=%d plus=%d minus=%d\n",
                        case_name,
                        constraint_name,
                        num_equations,
                        plus_eq,
                        minus_eq);
                return 1;
            }

            for (int eq = 0; eq < num_equations; ++eq) {
                const double fd = (plus_res[eq] - minus_res[eq]) / (2.0 * FD_EPS);
                const double analytic = (body == 0) ? jac_i[eq][dof] : jac_j[eq][dof];
                const double diff = fabs(fd - analytic);
                if (diff > max_diff) {
                    max_diff = diff;
                }
                if (diff > JACOBIAN_TOL) {
                    fprintf(stderr,
                            "[%s] %s jac mismatch body=%d dof=%d eq=%d: analytic=%.15e fd=%.15e diff=%.3e (tol=%.1e)\n",
                            case_name,
                            constraint_name,
                            body,
                            dof,
                            eq,
                            analytic,
                            fd,
                            diff,
                            JACOBIAN_TOL);
                    return 1;
                }
            }
        }
    }

    if (max_diff_out) {
        *max_diff_out = max_diff;
    }

    printf("[%s] %s jacobian check: max |analytic-fd| = %.3e (tol=%.1e, eps=%.1e)\n",
           case_name,
           constraint_name,
           max_diff,
           JACOBIAN_TOL,
           FD_EPS);
    return 0;
}

static int check_kkt_equation_count(const char *case_name,
                                    const mbd_constraint2d_t *distance_constraint,
                                    const mbd_constraint2d_t *revolute_constraint)
{
    mbd_constraint2d_t single[1];
    mbd_constraint2d_t pair[2];
    int num_equations = 0;
    fem_error_t err;

    single[0] = *revolute_constraint;
    err = mbd_kkt_count_constraint_equations(single, 1, &num_equations);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] kkt single count failed: %s\n", case_name, error_get_message());
        return 1;
    }
    if (num_equations != 2) {
        fprintf(stderr,
                "[%s] kkt revolute count mismatch: got=%d expected=2\n",
                case_name,
                num_equations);
        return 1;
    }

    pair[0] = *distance_constraint;
    pair[1] = *revolute_constraint;
    err = mbd_kkt_count_constraint_equations(pair, 2, &num_equations);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] kkt pair count failed: %s\n", case_name, error_get_message());
        return 1;
    }
    if (num_equations != 3) {
        fprintf(stderr,
                "[%s] kkt total count mismatch: got=%d expected=3\n",
                case_name,
                num_equations);
        return 1;
    }

    printf("[%s] kkt equation count check: revolute=2, distance+revolute=3\n", case_name);
    return 0;
}

static int run_probe_case(const probe_case_t *probe_case,
                          double *distance_max_diff,
                          double *revolute_max_diff)
{
    mbd_constraint2d_t distance_constraint;
    mbd_constraint2d_t revolute_constraint;
    fem_error_t err;

    err = mbd_constraint_init_distance(&distance_constraint,
                                       1,
                                       0,
                                       1,
                                       probe_case->distance_anchor_i,
                                       probe_case->distance_anchor_j,
                                       probe_case->distance_target);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] distance init failed: %s\n", probe_case->name, error_get_message());
        return 1;
    }

    err = mbd_constraint_init_revolute(&revolute_constraint,
                                       2,
                                       0,
                                       1,
                                       probe_case->revolute_anchor_i,
                                       probe_case->revolute_anchor_j);
    if (err != FEM_SUCCESS) {
        fprintf(stderr, "[%s] revolute init failed: %s\n", probe_case->name, error_get_message());
        return 1;
    }

    if (check_distance_residual(probe_case->name,
                                &distance_constraint,
                                &probe_case->state_i,
                                &probe_case->state_j) != 0) {
        return 1;
    }
    if (check_revolute_residual(probe_case->name,
                                &revolute_constraint,
                                &probe_case->state_i,
                                &probe_case->state_j) != 0) {
        return 1;
    }
    if (check_jacobian_fd(probe_case->name,
                          "distance",
                          &distance_constraint,
                          &probe_case->state_i,
                          &probe_case->state_j,
                          distance_max_diff) != 0) {
        return 1;
    }
    if (check_jacobian_fd(probe_case->name,
                          "revolute",
                          &revolute_constraint,
                          &probe_case->state_i,
                          &probe_case->state_j,
                          revolute_max_diff) != 0) {
        return 1;
    }
    if (check_kkt_equation_count(probe_case->name, &distance_constraint, &revolute_constraint) != 0) {
        return 1;
    }

    return 0;
}

int main(void)
{
    static const probe_case_t probe_cases[] = {
        {
            "case-1",
            {0.35, -0.20},
            {-0.15, 0.45},
            1.40,
            {0.25, 0.10},
            {-0.30, 0.20},
            {1.10, -0.40, 0.35},
            {-0.70, 0.80, -0.25},
        },
        {
            "case-2",
            {-0.40, 0.30},
            {0.60, -0.25},
            2.05,
            {-0.15, 0.50},
            {0.45, -0.35},
            {2.10, -1.20, 1.05},
            {-1.40, 0.65, -0.85},
        },
    };
    const int num_cases = (int)(sizeof(probe_cases) / sizeof(probe_cases[0]));
    double global_distance_max_diff = 0.0;
    double global_revolute_max_diff = 0.0;

    /* One-line reproduction:
     * make -C FEM4C mbd_probe
     */

    for (int i = 0; i < num_cases; ++i) {
        double distance_max_diff = 0.0;
        double revolute_max_diff = 0.0;

        if (run_probe_case(&probe_cases[i], &distance_max_diff, &revolute_max_diff) != 0) {
            return 1;
        }
        if (distance_max_diff > global_distance_max_diff) {
            global_distance_max_diff = distance_max_diff;
        }
        if (revolute_max_diff > global_revolute_max_diff) {
            global_revolute_max_diff = revolute_max_diff;
        }
    }

    printf("PASS: MBD constraint checks (%d cases, residual tol=%.1e, jacobian tol=%.1e, fd eps=%.1e, distance max=%.3e, revolute max=%.3e)\n",
           num_cases,
           RESIDUAL_TOL,
           JACOBIAN_TOL,
           FD_EPS,
           global_distance_max_diff,
           global_revolute_max_diff);
    return 0;
}
