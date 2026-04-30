#include "assembler2d.h"
#include "forces2d.h"
#include "../common/error.h"
#include <stdlib.h>
#include <string.h>

static fem_error_t mbd_dense_kkt2d_constraint_row_offsets(const mbd_constraint2d_t *constraints,
                                                          int num_constraints,
                                                          int *offsets,
                                                          int *num_equations)
{
    int i;
    int total = 0;

    CHECK_NULL(offsets, "constraint_row_offsets");
    CHECK_NULL(num_equations, "num_equations");

    if (num_constraints < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "num_constraints %d must be non-negative",
                         num_constraints);
    }
    if (num_constraints > 0) {
        CHECK_NULL(constraints, "constraints");
    }

    offsets[0] = 0;
    for (i = 0; i < num_constraints; ++i) {
        CHECK_ERROR(mbd_constraint_validate(&constraints[i]));
        total += mbd_constraint_equation_count(&constraints[i]);
        offsets[i + 1] = total;
    }

    *num_equations = total;
    return FEM_SUCCESS;
}

static fem_error_t mbd_dense_kkt2d_add_body_block(const mbd_system2d_t *system,
                                                  mbd_dense_kkt2d_t *kkt)
{
    int body_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(kkt, "mbd_dense_kkt2d");

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        const int row = body_index * MBD_BODY2D_DOF;
        const mbd_body2d_t *body = &system->bodies[body_index];

        kkt->matrix[row + 0][row + 0] = body->mass;
        kkt->matrix[row + 1][row + 1] = body->mass;
        kkt->matrix[row + 2][row + 2] = body->inertia;
    }

    CHECK_ERROR(mbd_forces2d_build_rhs_vector(system, kkt->rhs, kkt->layout.body_dof));
    return FEM_SUCCESS;
}

static fem_error_t mbd_dense_kkt2d_add_constraint_block(const mbd_system2d_t *system,
                                                        mbd_dense_kkt2d_t *kkt)
{
    int constraint_index;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(kkt, "mbd_dense_kkt2d");

    for (constraint_index = 0; constraint_index < system->num_constraints; ++constraint_index) {
        const mbd_constraint2d_t *constraint = &system->constraints[constraint_index];
        const int lambda_row0 = kkt->layout.body_dof + kkt->constraint_row_offsets[constraint_index];
        const int body_i_col0 = constraint->body_i * MBD_BODY2D_DOF;
        const int body_j_col0 = constraint->body_j * MBD_BODY2D_DOF;
        const mbd_body2d_t *body_i = &system->bodies[constraint->body_i];
        const mbd_body2d_t *body_j = &system->bodies[constraint->body_j];
        mbd_constraint_eval2d_t eval;
        mbd_body_state2d_t state_i_view;
        mbd_body_state2d_t state_j_view;
        int r;
        int c;

        if (constraint->body_i < 0 || constraint->body_i >= system->num_bodies ||
            constraint->body_j < 0 || constraint->body_j >= system->num_bodies) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "constraint body index out of range (id=%d, body_i=%d, body_j=%d)",
                             constraint->id, constraint->body_i, constraint->body_j);
        }

        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_i], &state_i_view));
        CHECK_ERROR(mbd_body2d_to_state_view(&system->bodies[constraint->body_j], &state_j_view));
        CHECK_ERROR(mbd_constraint_evaluate_accel_rhs(constraint,
                                                      &state_i_view,
                                                      &state_j_view,
                                                      body_i->v,
                                                      body_j->v,
                                                      MBD_CONSTRAINT2D_BAUMGARTE_ALPHA_DEFAULT,
                                                      MBD_CONSTRAINT2D_BAUMGARTE_BETA_DEFAULT,
                                                      &eval));

        for (r = 0; r < eval.num_equations; ++r) {
            const int lambda_row = lambda_row0 + r;
            const int lambda_index = kkt->constraint_row_offsets[constraint_index] + r;
            kkt->constraint_residual[lambda_index] = eval.residual[r];
            kkt->constraint_phi_dot[lambda_index] = eval.phi_dot[r];
            kkt->constraint_gamma_rhs[lambda_index] = eval.gamma_rhs[r];
            kkt->rhs[lambda_row] = eval.gamma_rhs[r];
            for (c = 0; c < MBD_BODY2D_DOF; ++c) {
                const double gij = eval.jac_i[r][c];
                const double gjj = eval.jac_j[r][c];
                kkt->matrix[lambda_row][body_i_col0 + c] = gij;
                kkt->matrix[body_i_col0 + c][lambda_row] = gij;
                kkt->matrix[lambda_row][body_j_col0 + c] = gjj;
                kkt->matrix[body_j_col0 + c][lambda_row] = gjj;
            }
        }
    }

    return FEM_SUCCESS;
}

void mbd_dense_kkt2d_zero(mbd_dense_kkt2d_t *kkt)
{
    if (!kkt) {
        return;
    }
    memset(kkt, 0, sizeof(*kkt));
}

void mbd_dense_kkt2d_free(mbd_dense_kkt2d_t *kkt)
{
    if (!kkt) {
        return;
    }

    free(kkt->matrix_storage);
    free(kkt->matrix);
    free(kkt->rhs);
    free(kkt->constraint_row_offsets);
    free(kkt->constraint_residual);
    free(kkt->constraint_phi_dot);
    free(kkt->constraint_gamma_rhs);
    memset(kkt, 0, sizeof(*kkt));
}

fem_error_t mbd_dense_kkt2d_reserve_dense_storage(mbd_dense_kkt2d_t *kkt,
                                                  int total_dof)
{
    double *matrix_storage = NULL;
    double **matrix = NULL;
    double *rhs = NULL;
    size_t total_dof_size = 0;
    size_t matrix_entries = 0;
    const size_t max_double_count = ((size_t) -1) / sizeof(double);
    int row = 0;

    CHECK_NULL(kkt, "mbd_dense_kkt2d");
    if (total_dof <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense KKT total_dof %d must be positive",
                         total_dof);
    }
    if (kkt->total_dof_capacity >= total_dof &&
        kkt->matrix_storage &&
        kkt->matrix &&
        kkt->rhs) {
        return FEM_SUCCESS;
    }

    total_dof_size = (size_t) total_dof;
    if (total_dof_size > max_double_count ||
        total_dof_size > max_double_count / total_dof_size) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "dense KKT total_dof %d overflows dense matrix allocation",
                         total_dof);
    }
    matrix_entries = total_dof_size * total_dof_size;
    matrix_storage = (double *) calloc(matrix_entries, sizeof(*matrix_storage));
    matrix = (double **) calloc(total_dof_size, sizeof(*matrix));
    rhs = (double *) calloc(total_dof_size, sizeof(*rhs));
    if (!matrix_storage || !matrix || !rhs) {
        free(rhs);
        free(matrix);
        free(matrix_storage);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate dense KKT storage for total_dof=%d",
                         total_dof);
    }

    for (row = 0; row < total_dof; ++row) {
        matrix[row] = matrix_storage + (size_t) row * total_dof_size;
    }

    free(kkt->matrix_storage);
    free(kkt->matrix);
    free(kkt->rhs);
    kkt->matrix_storage = matrix_storage;
    kkt->matrix = matrix;
    kkt->rhs = rhs;
    kkt->total_dof_capacity = total_dof;
    return FEM_SUCCESS;
}

fem_error_t mbd_dense_kkt2d_reserve_constraint_scratch(mbd_dense_kkt2d_t *kkt,
                                                       int num_constraints,
                                                       int lambda_dof)
{
    int *constraint_row_offsets = NULL;
    double *constraint_residual = NULL;
    double *constraint_phi_dot = NULL;
    double *constraint_gamma_rhs = NULL;
    size_t row_count = 0;
    size_t lambda_count = 0;

    CHECK_NULL(kkt, "mbd_dense_kkt2d");
    if (num_constraints < 0 || lambda_dof < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "constraint scratch sizes must be non-negative (constraints=%d, lambda_dof=%d)",
                         num_constraints,
                         lambda_dof);
    }
    if (kkt->constraint_row_offset_capacity >= num_constraints + 1 &&
        kkt->lambda_capacity >= lambda_dof &&
        kkt->constraint_row_offsets &&
        (lambda_dof == 0 ||
         (kkt->constraint_residual &&
          kkt->constraint_phi_dot &&
          kkt->constraint_gamma_rhs))) {
        return FEM_SUCCESS;
    }

    row_count = (size_t) num_constraints + 1U;
    constraint_row_offsets = (int *) calloc(row_count, sizeof(*constraint_row_offsets));
    if (!constraint_row_offsets) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "failed to allocate dense KKT row offsets for %d constraints",
                         num_constraints);
    }

    if (lambda_dof > 0) {
        lambda_count = (size_t) lambda_dof;
        constraint_residual =
            (double *) calloc(lambda_count, sizeof(*constraint_residual));
        constraint_phi_dot =
            (double *) calloc(lambda_count, sizeof(*constraint_phi_dot));
        constraint_gamma_rhs =
            (double *) calloc(lambda_count, sizeof(*constraint_gamma_rhs));
        if (!constraint_residual || !constraint_phi_dot || !constraint_gamma_rhs) {
            free(constraint_row_offsets);
            free(constraint_residual);
            free(constraint_phi_dot);
            free(constraint_gamma_rhs);
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "failed to allocate dense KKT lambda scratch for %d equations",
                             lambda_dof);
        }
    }

    free(kkt->constraint_row_offsets);
    free(kkt->constraint_residual);
    free(kkt->constraint_phi_dot);
    free(kkt->constraint_gamma_rhs);
    kkt->constraint_row_offsets = constraint_row_offsets;
    kkt->constraint_residual = constraint_residual;
    kkt->constraint_phi_dot = constraint_phi_dot;
    kkt->constraint_gamma_rhs = constraint_gamma_rhs;
    kkt->constraint_row_offset_capacity = num_constraints + 1;
    kkt->lambda_capacity = lambda_dof;
    return FEM_SUCCESS;
}

fem_error_t mbd_dense_kkt2d_copy_compact(const mbd_dense_kkt2d_t *kkt,
                                         double *matrix_out)
{
    int row;
    int col;

    CHECK_NULL(kkt, "mbd_dense_kkt2d");
    CHECK_NULL(matrix_out, "compact dense matrix");
    CHECK_NULL(kkt->matrix, "dense KKT matrix");

    for (row = 0; row < kkt->layout.total_dof; ++row) {
        for (col = 0; col < kkt->layout.total_dof; ++col) {
            matrix_out[row * kkt->layout.total_dof + col] = kkt->matrix[row][col];
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_dense_kkt2d_assemble(const mbd_system2d_t *system,
                                     mbd_dense_kkt2d_t *kkt)
{
    int num_equations = 0;
    int max_lambda_dof = 0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(kkt, "mbd_dense_kkt2d");

    if (system->num_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "num_bodies %d outside supported range [1,+inf)",
                         system->num_bodies);
    }

    max_lambda_dof = system->num_constraints * MBD_CONSTRAINT2D_MAX_EQ;
    CHECK_ERROR(mbd_dense_kkt2d_reserve_constraint_scratch(kkt,
                                                           system->num_constraints,
                                                           max_lambda_dof));
    memset(&kkt->layout, 0, sizeof(kkt->layout));
    memset(kkt->constraint_row_offsets,
           0,
           (size_t) (system->num_constraints + 1) * sizeof(*kkt->constraint_row_offsets));
    if (kkt->lambda_capacity > 0) {
        memset(kkt->constraint_residual,
               0,
               (size_t) kkt->lambda_capacity * sizeof(*kkt->constraint_residual));
        memset(kkt->constraint_phi_dot,
               0,
               (size_t) kkt->lambda_capacity * sizeof(*kkt->constraint_phi_dot));
        memset(kkt->constraint_gamma_rhs,
               0,
               (size_t) kkt->lambda_capacity * sizeof(*kkt->constraint_gamma_rhs));
    }
    CHECK_ERROR(mbd_dense_kkt2d_constraint_row_offsets(system->constraints,
                                                       system->num_constraints,
                                                       kkt->constraint_row_offsets,
                                                       &num_equations));
    CHECK_ERROR(mbd_kkt_compute_layout(system->num_bodies, num_equations, &kkt->layout));
    CHECK_ERROR(mbd_dense_kkt2d_reserve_dense_storage(kkt, kkt->layout.total_dof));
    memset(kkt->matrix_storage,
           0,
           (size_t) kkt->layout.total_dof * (size_t) kkt->layout.total_dof * sizeof(*kkt->matrix_storage));
    memset(kkt->rhs,
           0,
           (size_t) kkt->layout.total_dof * sizeof(*kkt->rhs));

    CHECK_ERROR(mbd_dense_kkt2d_add_body_block(system, kkt));
    CHECK_ERROR(mbd_dense_kkt2d_add_constraint_block(system, kkt));
    return FEM_SUCCESS;
}
