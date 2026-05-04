#include "projection2d.h"
#include "../numerics/dense/linear_solver_dense.h"
#include "../common/error.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MBD_PROJECTION2D_DENSE_RETRY_PIVOT_TOL 1.0e-16

static double mbd_projection2d_vector_l2(const double *values, int count)
{
    double sum = 0.0;
    int index;

    if (!values || count <= 0) {
        return 0.0;
    }

    for (index = 0; index < count; ++index) {
        sum += values[index] * values[index];
    }
    return sqrt(sum);
}

static fem_error_t mbd_projection2d_constraint_row_offsets(
    const mbd_constraint2d_t *constraints,
    int num_constraints,
    int *offsets,
    int *num_equations)
{
    int constraint_index;
    int total = 0;

    CHECK_NULL(offsets, "projection row offsets");
    CHECK_NULL(num_equations, "projection equation count");

    if (num_constraints < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection num_constraints %d must be non-negative",
                         num_constraints);
    }
    if (num_constraints > 0) {
        CHECK_NULL(constraints, "projection constraints");
    }

    offsets[0] = 0;
    for (constraint_index = 0; constraint_index < num_constraints; ++constraint_index) {
        CHECK_ERROR(mbd_constraint_validate(&constraints[constraint_index]));
        total += mbd_constraint_equation_count(&constraints[constraint_index]);
        offsets[constraint_index + 1] = total;
    }

    *num_equations = total;
    return FEM_SUCCESS;
}

static fem_error_t mbd_projection2d_build_kkt(const mbd_system2d_t *system,
                                              const mbd_projection2d_report_t *report,
                                              const int *constraint_row_offsets,
                                              double *matrix,
                                              double *rhs)
{
    const int total_dof = report->layout.total_dof;
    int body_dof_index;
    int constraint_index;

    CHECK_NULL(system, "projection system");
    CHECK_NULL(report, "projection report");
    CHECK_NULL(constraint_row_offsets, "projection row offsets");
    CHECK_NULL(matrix, "projection matrix");
    CHECK_NULL(rhs, "projection rhs");

    memset(matrix, 0, sizeof(double) * (size_t) total_dof * (size_t) total_dof);
    memset(rhs, 0, sizeof(double) * (size_t) total_dof);

    for (body_dof_index = 0; body_dof_index < report->layout.body_dof; ++body_dof_index) {
        matrix[body_dof_index * total_dof + body_dof_index] = 1.0;
    }

    for (constraint_index = 0; constraint_index < system->num_constraints; ++constraint_index) {
        const mbd_constraint2d_t *constraint = &system->constraints[constraint_index];
        const int body_i_col0 = constraint->body_i * MBD_BODY2D_DOF;
        const int body_j_col0 = constraint->body_j * MBD_BODY2D_DOF;
        const int body_i_is_ground = system->bodies[constraint->body_i].is_ground;
        const int body_j_is_ground = system->bodies[constraint->body_j].is_ground;
        mbd_body_state2d_t state_i;
        mbd_body_state2d_t state_j;
        double residual[MBD_CONSTRAINT2D_MAX_EQ];
        double jac_i[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF];
        double jac_j[MBD_CONSTRAINT2D_MAX_EQ][MBD_BODY2D_DOF];
        int num_equations = 0;
        int equation_index;
        int dof;

        if (constraint->body_i < 0 || constraint->body_i >= system->num_bodies ||
            constraint->body_j < 0 || constraint->body_j >= system->num_bodies) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "projection constraint body index out of range (id=%d, body_i=%d, body_j=%d)",
                             constraint->id,
                             constraint->body_i,
                             constraint->body_j);
        }

        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_i], &state_i));
        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_j], &state_j));
        CHECK_ERROR(mbd_constraint_evaluate(constraint,
                                            &state_i,
                                            &state_j,
                                            residual,
                                            jac_i,
                                            jac_j,
                                            &num_equations));

        for (equation_index = 0; equation_index < num_equations; ++equation_index) {
            const int lambda_row = report->layout.body_dof
                                 + constraint_row_offsets[constraint_index]
                                 + equation_index;

            rhs[lambda_row] = -residual[equation_index];
            if (!body_i_is_ground) {
                for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
                    matrix[lambda_row * total_dof + body_i_col0 + dof] = jac_i[equation_index][dof];
                    matrix[(body_i_col0 + dof) * total_dof + lambda_row] = jac_i[equation_index][dof];
                }
            }
            if (!body_j_is_ground) {
                for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
                    matrix[lambda_row * total_dof + body_j_col0 + dof] = jac_j[equation_index][dof];
                    matrix[(body_j_col0 + dof) * total_dof + lambda_row] = jac_j[equation_index][dof];
                }
            }
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_projection2d_velocity_violation_l2(
    const mbd_system2d_t *system,
    const int *constraint_row_offsets,
    int num_equations,
    double *phi_dot_l2)
{
    double *phi_dot_all = NULL;
    int constraint_index;
    double sum = 0.0;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "projection system");
    CHECK_NULL(constraint_row_offsets, "projection row offsets");
    CHECK_NULL(phi_dot_l2, "projection phi_dot_l2");
    if (num_equations < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection num_equations %d must be non-negative",
                         num_equations);
    }
    if (num_equations > 0) {
        phi_dot_all = (double *) calloc((size_t) num_equations, sizeof(*phi_dot_all));
        if (!phi_dot_all) {
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Failed to allocate projection phi_dot scratch for %d equations",
                             num_equations);
        }
    }

    for (constraint_index = 0; constraint_index < system->num_constraints; ++constraint_index) {
        const mbd_constraint2d_t *constraint = &system->constraints[constraint_index];
        mbd_constraint_eval2d_t eval;
        mbd_body_state2d_t state_i;
        mbd_body_state2d_t state_j;
        int row0 = constraint_row_offsets[constraint_index];
        int eq;

        err = mbd_body2d_to_state_view(&system->bodies[constraint->body_i], &state_i);
        if (err != FEM_SUCCESS) {
            free(phi_dot_all);
            return err;
        }
        err = mbd_body2d_to_state_view(&system->bodies[constraint->body_j], &state_j);
        if (err != FEM_SUCCESS) {
            free(phi_dot_all);
            return err;
        }
        err = mbd_constraint_evaluate_accel_rhs(constraint,
                                                &state_i,
                                                &state_j,
                                                system->bodies[constraint->body_i].v,
                                                system->bodies[constraint->body_j].v,
                                                0.0,
                                                0.0,
                                                &eval);
        if (err != FEM_SUCCESS) {
            free(phi_dot_all);
            return err;
        }
        for (eq = 0; eq < eval.num_equations; ++eq) {
            phi_dot_all[row0 + eq] = eval.phi_dot[eq];
        }
    }

    for (constraint_index = 0; constraint_index < num_equations; ++constraint_index) {
        sum += phi_dot_all[constraint_index] * phi_dot_all[constraint_index];
    }
    *phi_dot_l2 = sqrt(sum);
    free(phi_dot_all);
    return FEM_SUCCESS;
}

static fem_error_t mbd_projection2d_build_velocity_kkt(
    const mbd_system2d_t *system,
    const mbd_projection2d_report_t *report,
    const int *constraint_row_offsets,
    double *matrix,
    double *rhs)
{
    const int total_dof = report->layout.total_dof;
    int body_dof_index;
    int constraint_index;

    CHECK_NULL(system, "projection system");
    CHECK_NULL(report, "projection report");
    CHECK_NULL(constraint_row_offsets, "projection row offsets");
    CHECK_NULL(matrix, "projection velocity matrix");
    CHECK_NULL(rhs, "projection velocity rhs");

    memset(matrix, 0, sizeof(double) * (size_t) total_dof * (size_t) total_dof);
    memset(rhs, 0, sizeof(double) * (size_t) total_dof);

    for (body_dof_index = 0; body_dof_index < report->layout.body_dof; ++body_dof_index) {
        matrix[body_dof_index * total_dof + body_dof_index] = 1.0;
    }

    for (constraint_index = 0; constraint_index < system->num_constraints; ++constraint_index) {
        const mbd_constraint2d_t *constraint = &system->constraints[constraint_index];
        const int body_i_col0 = constraint->body_i * MBD_BODY2D_DOF;
        const int body_j_col0 = constraint->body_j * MBD_BODY2D_DOF;
        const int body_i_is_ground = system->bodies[constraint->body_i].is_ground;
        const int body_j_is_ground = system->bodies[constraint->body_j].is_ground;
        mbd_constraint_eval2d_t eval;
        mbd_body_state2d_t state_i;
        mbd_body_state2d_t state_j;
        int equation_index;
        int dof;

        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_i], &state_i));
        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_j], &state_j));
        CHECK_ERROR(mbd_constraint_evaluate_accel_rhs(constraint,
                                                     &state_i,
                                                     &state_j,
                                                     system->bodies[constraint->body_i].v,
                                                     system->bodies[constraint->body_j].v,
                                                     0.0,
                                                     0.0,
                                                     &eval));

        for (equation_index = 0; equation_index < eval.num_equations; ++equation_index) {
            const int lambda_row = report->layout.body_dof
                                 + constraint_row_offsets[constraint_index]
                                 + equation_index;

            rhs[lambda_row] = -eval.phi_dot[equation_index];
            if (!body_i_is_ground) {
                for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
                    matrix[lambda_row * total_dof + body_i_col0 + dof] =
                        eval.jac_i[equation_index][dof];
                    matrix[(body_i_col0 + dof) * total_dof + lambda_row] =
                        eval.jac_i[equation_index][dof];
                }
            }
            if (!body_j_is_ground) {
                for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
                    matrix[lambda_row * total_dof + body_j_col0 + dof] =
                        eval.jac_j[equation_index][dof];
                    matrix[(body_j_col0 + dof) * total_dof + lambda_row] =
                        eval.jac_j[equation_index][dof];
                }
            }
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_projection2d_dense_solve(const double *matrix,
                                                const double *rhs,
                                                int n,
                                                double *solution)
{
    fem_error_t err = mbd_linear_solver_dense_solve(matrix,
                                                    rhs,
                                                    n,
                                                    MBD_LINEAR_SOLVER_DENSE_DEFAULT_PIVOT_TOL,
                                                    solution);

    if (err == FEM_SUCCESS || err != FEM_ERROR_SINGULAR_MATRIX) {
        return err;
    }

    error_clear();
    return mbd_linear_solver_dense_solve(matrix,
                                         rhs,
                                         n,
                                         MBD_PROJECTION2D_DENSE_RETRY_PIVOT_TOL,
                                         solution);
}

static fem_error_t mbd_projection2d_allocate_dense_workspace(int total_dof,
                                                             double **matrix,
                                                             double **rhs,
                                                             double **solution)
{
    const size_t total_dof_size = (size_t) total_dof;
    const size_t max_double_count = ((size_t) -1) / sizeof(double);
    size_t matrix_entries;

    CHECK_NULL(matrix, "projection matrix pointer");
    CHECK_NULL(rhs, "projection rhs pointer");
    CHECK_NULL(solution, "projection solution pointer");

    *matrix = NULL;
    *rhs = NULL;
    *solution = NULL;

    if (total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection total dof %d must be positive",
                         total_dof);
    }
    if (total_dof_size > max_double_count ||
        total_dof_size > max_double_count / total_dof_size) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection total dof %d overflows dense workspace sizing",
                         total_dof);
    }

    matrix_entries = total_dof_size * total_dof_size;
    *matrix = (double *) calloc(matrix_entries, sizeof(**matrix));
    *rhs = (double *) calloc(total_dof_size, sizeof(**rhs));
    *solution = (double *) calloc(total_dof_size, sizeof(**solution));
    if (!*matrix || !*rhs || !*solution) {
        free(*solution);
        free(*rhs);
        free(*matrix);
        *solution = NULL;
        *rhs = NULL;
        *matrix = NULL;
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate projection dense workspace for total dof %d",
                         total_dof);
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_projection2d_apply_velocity_projection(
    mbd_system2d_t *system,
    mbd_projection2d_report_t *report,
    const int *constraint_row_offsets)
{
    double *matrix = NULL;
    double *rhs = NULL;
    double *solution = NULL;
    double (*saved_v)[MBD_BODY2D_DOF] = NULL;
    double phi_dot_before = 0.0;
    double phi_dot_after = 0.0;
    int body_index;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "projection system");
    CHECK_NULL(report, "projection report");
    CHECK_NULL(constraint_row_offsets, "projection row offsets");

    if (report->num_equations <= 0) {
        return FEM_SUCCESS;
    }

    CHECK_ERROR(mbd_projection2d_velocity_violation_l2(system,
                                                       constraint_row_offsets,
                                                       report->num_equations,
                                                       &phi_dot_before));
    report->velocity_residual_l2_before = phi_dot_before;
    if (!(phi_dot_before > 0.0)) {
        report->velocity_residual_l2_after = phi_dot_before;
        report->velocity_residual_reduction_ratio = 0.0;
        return FEM_SUCCESS;
    }
    err = mbd_projection2d_allocate_dense_workspace(report->layout.total_dof,
                                                    &matrix,
                                                    &rhs,
                                                    &solution);
    if (err != FEM_SUCCESS) {
        return err;
    }
    if (system->num_bodies > 0) {
        saved_v = (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                      sizeof(*saved_v));
        if (!saved_v) {
            err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                            "Failed to allocate projection velocity backup for %d bodies",
                            system->num_bodies);
            goto cleanup;
        }
    }

    err = mbd_projection2d_build_velocity_kkt(system,
                                              report,
                                              constraint_row_offsets,
                                              matrix,
                                              rhs);
    if (err != FEM_SUCCESS) {
        error_print(err);
        goto cleanup;
    }
    err = mbd_projection2d_dense_solve(matrix,
                                       rhs,
                                       report->layout.total_dof,
                                       solution);
    if (err != FEM_SUCCESS) {
        error_print(err);
        goto cleanup;
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        memcpy(saved_v[body_index], system->bodies[body_index].v, sizeof(saved_v[body_index]));
    }
    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        const int row = body_index * MBD_BODY2D_DOF;

        if (system->bodies[body_index].is_ground) {
            continue;
        }
        system->bodies[body_index].v[0] += solution[row + 0];
        system->bodies[body_index].v[1] += solution[row + 1];
        system->bodies[body_index].v[2] += solution[row + 2];
    }
    err = mbd_system2d_sync_body_states(system);
    if (err != FEM_SUCCESS) {
        error_print(err);
        goto cleanup;
    }
    err = mbd_projection2d_velocity_violation_l2(system,
                                                 constraint_row_offsets,
                                                 report->num_equations,
                                                 &phi_dot_after);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    if (phi_dot_after > phi_dot_before) {
        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            memcpy(system->bodies[body_index].v, saved_v[body_index], sizeof(saved_v[body_index]));
        }
        err = mbd_system2d_sync_body_states(system);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
        phi_dot_after = phi_dot_before;
    }
    report->velocity_residual_l2_after = phi_dot_after;
    report->velocity_residual_reduction_ratio =
        phi_dot_before > 0.0 ? phi_dot_after / phi_dot_before : 0.0;

cleanup:
    free(solution);
    free(rhs);
    free(matrix);
    free(saved_v);
    return err;
}

void mbd_projection2d_options_set_defaults(mbd_projection2d_options_t *options)
{
    if (!options) {
        return;
    }

    options->max_iterations = MBD_PROJECTION2D_DEFAULT_MAX_ITERS;
    options->residual_tolerance = MBD_PROJECTION2D_DEFAULT_RESIDUAL_TOL;
}

void mbd_projection2d_report_zero(mbd_projection2d_report_t *report)
{
    if (!report) {
        return;
    }

    memset(report, 0, sizeof(*report));
    report->stop_reason = "not_started";
}

fem_error_t mbd_projection2d_apply_with_options(
    mbd_system2d_t *system,
    const mbd_projection2d_options_t *options,
    mbd_projection2d_report_t *report)
{
    double *matrix = NULL;
    double *rhs = NULL;
    double *solution = NULL;
    double (*saved_q)[MBD_BODY2D_DOF] = NULL;
    int *constraint_row_offsets = NULL;
    mbd_projection2d_report_t local_report;
    mbd_projection2d_options_t local_options;
    const mbd_projection2d_options_t *active_options = options;
    double residual_current;
    double correction_l2_sq = 0.0;
    int iteration;
    int body_index;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(system, "projection system");
    if (!active_options) {
        mbd_projection2d_options_set_defaults(&local_options);
        active_options = &local_options;
    }
    if (active_options->max_iterations <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection max_iterations %d must be positive",
                         active_options->max_iterations);
    }
    if (!isfinite(active_options->residual_tolerance) ||
        active_options->residual_tolerance < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "projection residual_tolerance %.16e must be finite and non-negative",
                         active_options->residual_tolerance);
    }

    if (!report) {
        report = &local_report;
    }
    mbd_projection2d_report_zero(report);
    constraint_row_offsets = (int *) calloc((size_t) system->num_constraints + 1U,
                                            sizeof(*constraint_row_offsets));
    if (!constraint_row_offsets) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate projection row offsets for %d constraints",
                         system->num_constraints);
    }

    err = mbd_projection2d_constraint_row_offsets(system->constraints,
                                                  system->num_constraints,
                                                  constraint_row_offsets,
                                                  &report->num_equations);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = mbd_kkt_compute_layout(system->num_bodies,
                                 report->num_equations,
                                 &report->layout);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = mbd_system2d_compute_constraint_residual_l2(system,
                                                      &report->residual_l2_before,
                                                      &report->num_equations);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    if (report->layout.total_dof <= 0 || report->num_equations <= 0) {
        report->residual_l2_after = report->residual_l2_before;
        report->residual_reduction_ratio = 0.0;
        report->target_reached = 1;
        report->stop_reason = "no_constraints";
        err = FEM_SUCCESS;
        goto cleanup;
    }
    if (report->residual_l2_before <= 0.0) {
        report->residual_l2_after = report->residual_l2_before;
        report->residual_reduction_ratio = 0.0;
        report->target_reached = 1;
        report->stop_reason = "already_within_tolerance";
        err = FEM_SUCCESS;
        goto cleanup;
    }
    err = mbd_projection2d_allocate_dense_workspace(report->layout.total_dof,
                                                    &matrix,
                                                    &rhs,
                                                    &solution);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    if (system->num_bodies > 0) {
        saved_q = (double (*)[MBD_BODY2D_DOF]) calloc((size_t) system->num_bodies,
                                                      sizeof(*saved_q));
        if (!saved_q) {
            err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                            "Failed to allocate projection position backup for %d bodies",
                            system->num_bodies);
            goto cleanup;
        }
    }

    residual_current = report->residual_l2_before;
    report->residual_l2_after = residual_current;
    for (iteration = 0; iteration < active_options->max_iterations; ++iteration) {
        err = mbd_projection2d_build_kkt(system,
                                         report,
                                         constraint_row_offsets,
                                         matrix,
                                         rhs);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
        err = mbd_projection2d_dense_solve(matrix,
                                           rhs,
                                           report->layout.total_dof,
                                           solution);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            memcpy(saved_q[body_index], system->bodies[body_index].q, sizeof(saved_q[body_index]));
        }
        for (body_index = 0; body_index < system->num_bodies; ++body_index) {
            const int row = body_index * MBD_BODY2D_DOF;

            if (system->bodies[body_index].is_ground) {
                continue;
            }
            system->bodies[body_index].q[0] += solution[row + 0];
            system->bodies[body_index].q[1] += solution[row + 1];
            system->bodies[body_index].q[2] += solution[row + 2];
        }

        err = mbd_system2d_sync_body_states(system);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
        err = mbd_system2d_compute_constraint_residual_l2(system,
                                                          &report->residual_l2_after,
                                                          &report->num_equations);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
        if (report->residual_l2_after >= residual_current) {
            for (body_index = 0; body_index < system->num_bodies; ++body_index) {
                memcpy(system->bodies[body_index].q, saved_q[body_index], sizeof(saved_q[body_index]));
            }
            err = mbd_system2d_sync_body_states(system);
            if (err != FEM_SUCCESS) {
                error_print(err);
                goto cleanup;
            }
            report->residual_l2_after = residual_current;
            report->stop_reason = "no_improvement";
            break;
        }

        correction_l2_sq += pow(mbd_projection2d_vector_l2(solution, report->layout.body_dof), 2.0);
        report->applied = 1;
        report->iterations_applied = iteration + 1;
        residual_current = report->residual_l2_after;
        if (residual_current <= active_options->residual_tolerance) {
            report->target_reached = 1;
            report->stop_reason = "residual_tolerance";
            break;
        }
        report->stop_reason = "iteration_cap";
    }

    report->residual_reduction_ratio =
        report->residual_l2_before > 0.0
            ? report->residual_l2_after / report->residual_l2_before
            : 0.0;
    report->correction_l2 = sqrt(correction_l2_sq);
    if (report->applied) {
        err = mbd_projection2d_apply_velocity_projection(system,
                                                         report,
                                                         constraint_row_offsets);
        if (err != FEM_SUCCESS) {
            error_print(err);
            goto cleanup;
        }
    }
    err = FEM_SUCCESS;

cleanup:
    free(constraint_row_offsets);
    free(solution);
    free(rhs);
    free(matrix);
    free(saved_q);
    return err;
}

fem_error_t mbd_projection2d_apply(mbd_system2d_t *system,
                                   mbd_projection2d_report_t *report)
{
    return mbd_projection2d_apply_with_options(system, NULL, report);
}
