#include "flex_solver2d.h"
#include "flex_bc2d.h"

#include "../common/error.h"
#include "../numerics/cg/cg_solver.h"
#include "../domain/fem/assembly/assembly.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double base_body_force[3];
    int has_base_body_force;
    int has_inertial_load;
    flex_inertial_load2d_t inertial_load;
} flex_solver2d_load_state_t;

static fem_error_t flex_solver2d_model_total_dof(const fem_model_t *model, int *total_dof)
{
    if (!model || !total_dof) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_model_total_dof requires non-NULL arguments");
    }

    if (model->total_dof > 0) {
        *total_dof = model->total_dof;
        return FEM_SUCCESS;
    }

    if (model->num_nodes < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Model has invalid node count %d", model->num_nodes);
    }

    *total_dof = model->num_nodes * 2;
    return FEM_SUCCESS;
}

static int flex_solver2d_find_node_index(const fem_model_t *model, int node_id)
{
    int i;

    if (!model) {
        return -1;
    }
    if (node_id >= 0 &&
        node_id < model->node_id_capacity &&
        model->node_id_to_index &&
        model->node_id_to_index[node_id] >= 0 &&
        model->node_id_to_index[node_id] < model->num_nodes) {
        return model->node_id_to_index[node_id];
    }
    if (!model->node_ids) {
        return -1;
    }

    for (i = 0; i < model->num_nodes; ++i) {
        if (model->node_ids[i] == node_id) {
            return i;
        }
    }

    return -1;
}

static fem_model_t *flex_solver2d_alloc_model(const char *label)
{
    fem_model_t *model = calloc(1, sizeof(*model));

    if (!model) {
        error_set(FEM_ERROR_MEMORY_ALLOCATION,
                  "%s",
                  label ? label : "flex_solver2d model allocation failed");
        return NULL;
    }

    fem_model_zero(model);
    return model;
}

static void flex_solver2d_free_model(fem_model_t *model)
{
    if (!model) {
        return;
    }

    fem_model_free(model);
    free(model);
}

static void flex_solver2d_detach_system_arrays(fem_model_t *model)
{
    if (!model) {
        return;
    }

    model->global_force = NULL;
    model->global_displ = NULL;
    model->global_stiffness_values = NULL;
    model->stiffness_profile = NULL;
    model->stiffness_offsets = NULL;
    model->stiffness_value_count = 0;
    model->stiffness_bandwidth = 0;
}

static fem_error_t flex_solver2d_sync_prescribed_displacements(fem_model_t *model)
{
    int total_dof = 0;
    int node_index;
    int dof;
    fem_error_t err;

    if (!model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_sync_prescribed_displacements requires a model");
    }
    if (!model->node_bc_flags || !model->node_displ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Model boundary arrays are not initialized");
    }

    err = flex_solver2d_model_total_dof(model, &total_dof);
    if (err != FEM_SUCCESS) {
        return err;
    }
    if (total_dof <= 0) {
        return FEM_SUCCESS;
    }
    if (!model->global_displ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Model global displacement array is not initialized");
    }

    for (node_index = 0; node_index < model->num_nodes; ++node_index) {
        for (dof = 0; dof < 2; ++dof) {
            int global_dof = node_index * 2 + dof;

            if (global_dof >= total_dof) {
                continue;
            }
            if (model->node_bc_flags[node_index][dof] == 1) {
                model->global_displ[global_dof] = model->node_displ[node_index][dof];
            }
        }
    }

    return FEM_SUCCESS;
}

static int flex_solver2d_body_force_is_nonzero(const double body_force[3])
{
    if (!body_force) {
        return 0;
    }

    return fabs(body_force[0]) > 0.0 ||
           fabs(body_force[1]) > 0.0 ||
           fabs(body_force[2]) > 0.0;
}

static void flex_solver2d_capture_load_state(const fem_model_t *model,
                                             flex_solver2d_load_state_t *state)
{
    if (!model || !state) {
        return;
    }

    memcpy(state->base_body_force,
           model->base_body_force,
           sizeof(state->base_body_force));
    state->has_base_body_force = model->has_base_body_force;
    state->has_inertial_load = model->has_inertial_load;
    state->inertial_load = model->inertial_load;
}

static void flex_solver2d_refresh_effective_body_force(fem_model_t *model)
{
    double inertial_body_force[3] = {0.0, 0.0, 0.0};
    int has_inertial_body_force = 0;

    if (!model) {
        return;
    }

    if (model->has_inertial_load) {
        inertial_body_force[0] =
            -model->inertial_load.density * model->inertial_load.translational_accel[0];
        inertial_body_force[1] =
            -model->inertial_load.density * model->inertial_load.translational_accel[1];
        inertial_body_force[2] = 0.0;
        has_inertial_body_force =
            flex_solver2d_body_force_is_nonzero(inertial_body_force);
    }

    model->body_force[0] = model->base_body_force[0] + inertial_body_force[0];
    model->body_force[1] = model->base_body_force[1] + inertial_body_force[1];
    model->body_force[2] = model->base_body_force[2] + inertial_body_force[2];
    model->has_body_force = model->has_base_body_force || has_inertial_body_force;
}

static void flex_solver2d_restore_load_state(fem_model_t *model,
                                             const flex_solver2d_load_state_t *state)
{
    if (!model || !state) {
        return;
    }

    memcpy(model->base_body_force,
           state->base_body_force,
           sizeof(model->base_body_force));
    model->has_base_body_force = state->has_base_body_force;
    model->has_inertial_load = state->has_inertial_load;
    model->inertial_load = state->inertial_load;
    flex_solver2d_refresh_effective_body_force(model);
}

static fem_error_t flex_solver2d_restore_model_to_globals(fem_model_t *model)
{
    fem_error_t err;

    err = fem_model_restore_globals(model);
    if (err != FEM_SUCCESS) {
        return err;
    }

    /* Globals now own these buffers and may reallocate/free them during assembly. */
    flex_solver2d_detach_system_arrays(model);
    return FEM_SUCCESS;
}

static fem_error_t flex_solver2d_restore_host(
    fem_model_t *host_snapshot,
    fem_error_t work_err)
{
    fem_error_t restore_err = FEM_SUCCESS;

    if (host_snapshot) {
        restore_err = fem_model_restore_globals(host_snapshot);
    }
    flex_solver2d_free_model(host_snapshot);
    if (work_err == FEM_SUCCESS && restore_err != FEM_SUCCESS) {
        return restore_err;
    }
    return work_err;
}

fem_error_t flex_solver2d_prepare_model(fem_model_t *model)
{
    fem_model_t *host_snapshot = NULL;
    fem_error_t err;
    int total_dof = 0;
    int full_reassembly_count = 0;
    int static_solve_count = 0;
    flex_solver2d_load_state_t load_state = {0};

    if (!model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_prepare_model requires a model");
    }

    err = flex_solver2d_model_total_dof(model, &total_dof);
    if (err != FEM_SUCCESS) {
        return err;
    }
    full_reassembly_count = model->full_reassembly_count;
    static_solve_count = model->static_solve_count;
    flex_solver2d_capture_load_state(model, &load_state);

    host_snapshot = flex_solver2d_alloc_model("flex_solver2d host snapshot");
    CHECK_NULL(host_snapshot, "flex_solver2d host snapshot");

    err = fem_model_clone_from_globals(host_snapshot);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    err = flex_solver2d_restore_model_to_globals(model);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    g_total_dof = total_dof;
    err = globals_allocate_system_arrays(total_dof);
    if (err == FEM_SUCCESS) {
        err = fem_model_clone_from_globals(model);
    }
    if (err == FEM_SUCCESS) {
        model->full_reassembly_count = full_reassembly_count;
        model->static_solve_count = static_solve_count;
        flex_solver2d_restore_load_state(model, &load_state);
        err = flex_solver2d_sync_prescribed_displacements(model);
    }

    return flex_solver2d_restore_host(host_snapshot, err);
}

fem_error_t flex_solver2d_assemble_full_mesh(fem_model_t *model)
{
    fem_model_t *host_snapshot = NULL;
    fem_error_t err;
    int total_dof = 0;
    int next_full_reassembly_count = 0;
    int static_solve_count = 0;
    flex_solver2d_load_state_t load_state = {0};

    if (!model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_assemble_full_mesh requires a model");
    }

    err = flex_solver2d_model_total_dof(model, &total_dof);
    if (err != FEM_SUCCESS) {
        return err;
    }
    next_full_reassembly_count = model->full_reassembly_count + 1;
    static_solve_count = model->static_solve_count;
    flex_solver2d_capture_load_state(model, &load_state);

    host_snapshot = flex_solver2d_alloc_model("flex_solver2d host snapshot");
    CHECK_NULL(host_snapshot, "flex_solver2d host snapshot");

    err = fem_model_clone_from_globals(host_snapshot);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    err = flex_solver2d_restore_model_to_globals(model);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    g_total_dof = total_dof;

    err = assembly_global_stiffness_matrix();
    if (err == FEM_SUCCESS) {
        err = assembly_global_force_vector();
    }
    if (err == FEM_SUCCESS) {
        err = fem_model_clone_from_globals(model);
    }
    if (err == FEM_SUCCESS) {
        model->full_reassembly_count = next_full_reassembly_count;
        model->static_solve_count = static_solve_count;
        flex_solver2d_restore_load_state(model, &load_state);
        err = flex_solver2d_sync_prescribed_displacements(model);
    }

    return flex_solver2d_restore_host(host_snapshot, err);
}

fem_error_t flex_solver2d_set_inertial_loads(fem_model_t *model,
                                             const flex_inertial_load2d_t *load)
{
    CHECK_NULL(model, "flex_solver2d model");
    CHECK_NULL(load, "flex inertial load");

    model->has_inertial_load = 1;
    model->inertial_load = *load;
    flex_solver2d_refresh_effective_body_force(model);
    return FEM_SUCCESS;
}

fem_error_t flex_solver2d_clear_inertial_loads(fem_model_t *model)
{
    CHECK_NULL(model, "flex_solver2d model");

    model->has_inertial_load = 0;
    memset(&model->inertial_load, 0, sizeof(model->inertial_load));
    flex_solver2d_refresh_effective_body_force(model);
    return FEM_SUCCESS;
}

fem_error_t flex_solver2d_apply_bc_entries(fem_model_t *model,
                                           const flex_bc2d_list_t *bc_list)
{
    int i;

    if (!model || !bc_list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_apply_bc_entries requires non-NULL inputs");
    }
    if (!model->node_bc_flags || !model->node_displ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Model boundary arrays are not initialized");
    }

    for (i = 0; i < bc_list->count; ++i) {
        int node_index = flex_solver2d_find_node_index(model,
                                                       bc_list->entries[i].node_id);
        int dof = bc_list->entries[i].dof;

        if (node_index < 0) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "BC node id %d is not present in model",
                             bc_list->entries[i].node_id);
        }
        if (dof < 0 || dof > 1) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "BC dof must be 0 or 1");
        }

        model->node_bc_flags[node_index][dof] = 1;
        model->node_displ[node_index][dof] = bc_list->entries[i].value;
        if (model->global_displ && node_index * 2 + dof < model->total_dof) {
            model->global_displ[node_index * 2 + dof] = bc_list->entries[i].value;
        }
    }

    return FEM_SUCCESS;
}

fem_error_t flex_solver2d_reassemble_and_solve(fem_model_t *model,
                                               fem_model_t *assembled_model)
{
    fem_model_t *host_snapshot = NULL;
    fem_error_t err;
    int total_dof = 0;
    int next_full_reassembly_count = 0;
    int next_static_solve_count = 0;
    flex_solver2d_load_state_t load_state = {0};

    if (!model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_reassemble_and_solve requires a model");
    }

    err = flex_solver2d_model_total_dof(model, &total_dof);
    if (err != FEM_SUCCESS) {
        return err;
    }
    next_full_reassembly_count = model->full_reassembly_count + 1;
    next_static_solve_count = model->static_solve_count + 1;
    flex_solver2d_capture_load_state(model, &load_state);

    host_snapshot = flex_solver2d_alloc_model("flex_solver2d host snapshot");
    CHECK_NULL(host_snapshot, "flex_solver2d host snapshot");

    err = fem_model_clone_from_globals(host_snapshot);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    err = flex_solver2d_restore_model_to_globals(model);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    g_total_dof = total_dof;

    err = assembly_global_stiffness_matrix();
    if (err == FEM_SUCCESS) {
        err = assembly_global_force_vector();
    }
    if (err == FEM_SUCCESS && assembled_model) {
        fem_model_free(assembled_model);
        err = fem_model_clone_from_globals(assembled_model);
    }
    if (err == FEM_SUCCESS && assembled_model) {
        flex_solver2d_restore_load_state(assembled_model, &load_state);
        assembled_model->full_reassembly_count = next_full_reassembly_count;
        assembled_model->static_solve_count = model->static_solve_count;
        err = flex_solver2d_sync_prescribed_displacements(assembled_model);
    }
    if (err == FEM_SUCCESS) {
        err = assembly_apply_boundary_conditions();
    }
    if (err == FEM_SUCCESS) {
        err = cg_solve_system();
    }
    if (err == FEM_SUCCESS) {
        err = fem_model_clone_from_globals(model);
    }
    if (err == FEM_SUCCESS) {
        flex_solver2d_restore_load_state(model, &load_state);
        model->full_reassembly_count = next_full_reassembly_count;
        model->static_solve_count = next_static_solve_count;
        err = flex_solver2d_sync_prescribed_displacements(model);
    }

    return flex_solver2d_restore_host(host_snapshot, err);
}

fem_error_t flex_solver2d_compute_residual(const fem_model_t *assembled_model,
                                           const double *u,
                                           int u_size,
                                           double *residual,
                                           int residual_size)
{
    fem_model_t *host_snapshot = NULL;
    fem_model_t *work_model = NULL;
    fem_error_t err;
    int i;
    int total_dof = 0;

    if (!assembled_model || !u || !residual) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_solver2d_compute_residual requires non-NULL inputs");
    }

    err = flex_solver2d_model_total_dof(assembled_model, &total_dof);
    if (err != FEM_SUCCESS) {
        return err;
    }
    if (u_size < total_dof || residual_size < total_dof) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Residual vectors are smaller than total DOF");
    }

    host_snapshot = flex_solver2d_alloc_model("flex_solver2d host snapshot");
    CHECK_NULL(host_snapshot, "flex_solver2d host snapshot");
    work_model = flex_solver2d_alloc_model("flex_solver2d work model");
    if (!work_model) {
        flex_solver2d_free_model(host_snapshot);
        return error_get_last();
    }

    err = fem_model_clone_from_globals(host_snapshot);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(work_model);
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    err = fem_model_clone(work_model, assembled_model);
    if (err != FEM_SUCCESS) {
        flex_solver2d_free_model(work_model);
        flex_solver2d_free_model(host_snapshot);
        return err;
    }

    err = flex_solver2d_restore_model_to_globals(work_model);
    if (err == FEM_SUCCESS) {
        err = cg_matrix_vector_multiply(NULL, (double *)u, residual, total_dof);
    }
    if (err == FEM_SUCCESS) {
        for (i = 0; i < total_dof; ++i) {
            residual[i] -= g_global_force[i];
        }
    }

    flex_solver2d_free_model(work_model);
    return flex_solver2d_restore_host(host_snapshot, err);
}
