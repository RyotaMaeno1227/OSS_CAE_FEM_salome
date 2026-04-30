#include "flex_body2d.h"
#include "flex_bc2d.h"
#include "flex_reaction2d.h"
#include "flex_solver2d.h"

#include "../common/error.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int flex_body2d_find_node_index(const fem_model2d_t *model, int node_id)
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

static fem_error_t flex_body2d_ensure_set_local_coords(node_set_t *set,
                                                       const fem_model2d_t *model,
                                                       const char *set_name)
{
    if (!set || !model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Local coordinate setup inputs must not be NULL");
    }
    if (set->count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s count must be non-negative",
                         set_name);
    }
    if (set->count == 0) {
        return FEM_SUCCESS;
    }
    if (!set->node_ids) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s node ids are required",
                         set_name);
    }

    return node_set_local_coordinates(set, model);
}

static fem_error_t flex_body2d_apply_bc_list(flex_body2d_t *body,
                                             const flex_bc2d_list_t *bc_list)
{
    int i;

    if (!body || !bc_list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Rigid BC application inputs must not be NULL");
    }
    if (body->u_local_size < body->model.num_nodes * 2) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body u_local is smaller than nodal 2D dof");
    }

    for (i = 0; i < bc_list->count; ++i) {
        int node_index = flex_body2d_find_node_index(&body->model,
                                                     bc_list->entries[i].node_id);
        int dof_index;

        if (node_index < 0) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "BC node id %d is not present in model",
                             bc_list->entries[i].node_id);
        }
        if (bc_list->entries[i].dof < 0 || bc_list->entries[i].dof > 1) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "BC dof must be 0 or 1");
        }

        dof_index = node_index * 2 + bc_list->entries[i].dof;
        body->u_local[dof_index] = bc_list->entries[i].value;
    }

    return FEM_SUCCESS;
}

static fem_error_t flex_body2d_build_set_rigid_bc(flex_body2d_t *body,
                                                  node_set_t *set,
                                                  const double marker_disp[3],
                                                  const char *set_name,
                                                  flex_bc2d_list_t *bc_list)
{
    if (!body || !set || !marker_disp || !bc_list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Rigid BC build inputs must not be NULL");
    }

    if (flex_body2d_ensure_set_local_coords(set, &body->model, set_name) != FEM_SUCCESS) {
        return error_get_last();
    }

    return flex_bc2d_build_node_set_entries(set, marker_disp, bc_list);
}

static fem_error_t flex_body2d_compute_set_marker_disp(
    const flex_body2d_t *body,
    const node_set_t *set,
    const double reference_pose[3],
    const double current_pose[3],
    const char *set_name,
    double marker_disp[3])
{
    double center[2] = {0.0, 0.0};
    double delta_world[2] = {0.0, 0.0};
    double delta_local[2] = {0.0, 0.0};
    double theta_delta;
    double c_ref;
    double s_ref;
    double c_delta;
    double s_delta;

    CHECK_NULL(body, "Flexible body");
    CHECK_NULL(set, "Interface node set");
    CHECK_NULL(reference_pose, "Reference pose");
    CHECK_NULL(current_pose, "Current pose");
    CHECK_NULL(set_name, "Interface node set name");
    CHECK_NULL(marker_disp, "Interface marker displacement");

    if (set->count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s requires at least one node",
                         set_name);
    }

    CHECK_ERROR(node_set_center(set, &body->model, center));

    theta_delta = current_pose[2] - reference_pose[2];
    delta_world[0] = current_pose[0] - reference_pose[0];
    delta_world[1] = current_pose[1] - reference_pose[1];

    c_ref = cos(reference_pose[2]);
    s_ref = sin(reference_pose[2]);
    delta_local[0] = c_ref * delta_world[0] + s_ref * delta_world[1];
    delta_local[1] = -s_ref * delta_world[0] + c_ref * delta_world[1];

    c_delta = cos(theta_delta);
    s_delta = sin(theta_delta);
    marker_disp[0] = delta_local[0]
        + (c_delta - 1.0) * center[0]
        - s_delta * center[1];
    marker_disp[1] = delta_local[1]
        + s_delta * center[0]
        + (c_delta - 1.0) * center[1];
    marker_disp[2] = theta_delta;
    return FEM_SUCCESS;
}

static fem_error_t flex_body2d_compute_set_center_local(
    const flex_body2d_t *body,
    const node_set_t *set,
    const char *set_name,
    double center_local[2])
{
    double sum_x = 0.0;
    double sum_y = 0.0;
    int i;

    CHECK_NULL(body, "Flexible body");
    CHECK_NULL(set, "Interface node set");
    CHECK_NULL(set_name, "Interface node set name");
    CHECK_NULL(center_local, "Interface center local position");

    if (set->count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s requires at least one node",
                         set_name);
    }
    if (!body->model.node_coords) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body model node coordinates are required");
    }
    if (body->u_local_size < body->model.num_nodes * 2) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body u_local is smaller than nodal 2D dof");
    }

    for (i = 0; i < set->count; ++i) {
        const int node_index = flex_body2d_find_node_index(&body->model, set->node_ids[i]);
        double x_ref;
        double y_ref;

        if (node_index < 0) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "%s node id %d is not present in model",
                             set_name,
                             set->node_ids[i]);
        }

        x_ref = body->model.node_coords[node_index][0];
        y_ref = body->model.node_coords[node_index][1];

        sum_x += x_ref + body->u_local[2 * node_index];
        sum_y += y_ref + body->u_local[2 * node_index + 1];
    }

    center_local[0] = sum_x / (double)set->count;
    center_local[1] = sum_y / (double)set->count;
    return FEM_SUCCESS;
}

static fem_error_t flex_body2d_compute_set_center_world(
    const flex_body2d_t *body,
    const node_set_t *set,
    const char *set_name,
    const double body_pose[3],
    double center_world[2])
{
    double center_local[2] = {0.0, 0.0};
    const double c = cos(body_pose[2]);
    const double s = sin(body_pose[2]);

    CHECK_NULL(body_pose, "Flexible body pose");
    CHECK_NULL(center_world, "Interface center world position");
    CHECK_ERROR(flex_body2d_compute_set_center_local(body,
                                                     set,
                                                     set_name,
                                                     center_local));

    center_world[0] = body_pose[0]
                    + c * center_local[0]
                    - s * center_local[1];
    center_world[1] = body_pose[1]
                    + s * center_local[0]
                    + c * center_local[1];
    return FEM_SUCCESS;
}

static fem_error_t flex_body2d_collect_interface_bc(flex_body2d_t *body,
                                                    const double root_marker_disp[3],
                                                    const double tip_marker_disp[3],
                                                    flex_bc2d_list_t *bc_list)
{
    fem_error_t err;

    if (!body || !root_marker_disp || !tip_marker_disp || !bc_list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Interface BC inputs must not be NULL");
    }

    flex_bc2d_list_clear(bc_list);

    err = flex_body2d_build_root_rigid_bc(body, root_marker_disp, bc_list);
    if (err != FEM_SUCCESS) {
        return err;
    }

    return flex_body2d_build_tip_rigid_bc(body, tip_marker_disp, bc_list);
}

static void flex_body2d_copy_model_displacement(const fem_model2d_t *model,
                                                double *u_local,
                                                int u_local_size)
{
    int i;
    int dof_count;

    if (!model || !u_local || u_local_size <= 0) {
        return;
    }

    dof_count = model->total_dof;
    if (dof_count <= 0) {
        dof_count = model->num_nodes * 2;
    }
    if (dof_count > u_local_size) {
        dof_count = u_local_size;
    }

    if (model->global_displ) {
        memcpy(u_local, model->global_displ, (size_t)dof_count * sizeof(*u_local));
        return;
    }

    if (model->node_displ) {
        memset(u_local, 0, (size_t)u_local_size * sizeof(*u_local));
        for (i = 0; i < model->num_nodes && (2 * i + 1) < u_local_size; ++i) {
            u_local[2 * i] = model->node_displ[i][0];
            u_local[2 * i + 1] = model->node_displ[i][1];
        }
    }
}

void fem_model2d_zero(fem_model2d_t *model)
{
    fem_model_zero(model);
}

fem_error_t fem_model2d_clone(fem_model2d_t *dst, const fem_model2d_t *src)
{
    return fem_model_clone(dst, src);
}

void fem_model2d_free(fem_model2d_t *model)
{
    fem_model_free(model);
}

void flex_body2d_zero(flex_body2d_t *body)
{
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
}

fem_error_t flex_body2d_init(flex_body2d_t *body,
                             int body_id,
                             const fem_model2d_t *model,
                             const node_set_t *root_set,
                             const node_set_t *tip_set)
{
    int u_local_size = 0;

    CHECK_NULL(body, "Flexible body");

    if (body_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body id must be non-negative");
    }
    if (!model || !root_set || !tip_set) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body init inputs must not be NULL");
    }

    flex_body2d_zero(body);
    body->body_id = body_id;

    if (fem_model2d_clone(&body->model, model) != FEM_SUCCESS) {
        flex_body2d_free(body);
        return error_get_last();
    }
    if (node_set_clone(&body->root_set, root_set) != FEM_SUCCESS) {
        flex_body2d_free(body);
        return error_get_last();
    }
    if (node_set_clone(&body->tip_set, tip_set) != FEM_SUCCESS) {
        flex_body2d_free(body);
        return error_get_last();
    }
    if (flex_body2d_ensure_set_local_coords(&body->root_set,
                                            &body->model,
                                            "Root set") != FEM_SUCCESS) {
        flex_body2d_free(body);
        return error_get_last();
    }
    if (flex_body2d_ensure_set_local_coords(&body->tip_set,
                                            &body->model,
                                            "Tip set") != FEM_SUCCESS) {
        flex_body2d_free(body);
        return error_get_last();
    }

    u_local_size = body->model.total_dof;
    if (u_local_size == 0 && body->model.num_nodes > 0) {
        u_local_size = body->model.num_nodes * 2;
    }
    if (u_local_size < 0) {
        flex_body2d_free(body);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Flexible body u_local size must be non-negative");
    }

    body->u_local_size = u_local_size;
    if (body->u_local_size > 0) {
        body->u_local = calloc((size_t)body->u_local_size,
                               sizeof(*body->u_local));
        if (!body->u_local) {
            flex_body2d_free(body);
            return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                             "Flexible body local displacement");
        }
        flex_body2d_copy_model_displacement(&body->model,
                                            body->u_local,
                                            body->u_local_size);
    }

    memset(body->reaction_root, 0, sizeof(body->reaction_root));
    memset(body->reaction_tip, 0, sizeof(body->reaction_tip));
    return FEM_SUCCESS;
}

fem_error_t flex_body2d_build_root_rigid_bc(flex_body2d_t *body,
                                            const double marker_disp[3],
                                            flex_bc2d_list_t *bc_list)
{
    CHECK_NULL(body, "Flexible body");

    return flex_body2d_build_set_rigid_bc(body,
                                          &body->root_set,
                                          marker_disp,
                                          "Root set",
                                          bc_list);
}

fem_error_t flex_body2d_build_tip_rigid_bc(flex_body2d_t *body,
                                           const double marker_disp[3],
                                           flex_bc2d_list_t *bc_list)
{
    CHECK_NULL(body, "Flexible body");

    return flex_body2d_build_set_rigid_bc(body,
                                          &body->tip_set,
                                          marker_disp,
                                          "Tip set",
                                          bc_list);
}

fem_error_t flex_body2d_compute_root_marker_disp(
    const flex_body2d_t *body,
    const double reference_pose[3],
    const double current_pose[3],
    double marker_disp[3])
{
    return flex_body2d_compute_set_marker_disp(body,
                                               &body->root_set,
                                               reference_pose,
                                               current_pose,
                                               "Root set",
                                               marker_disp);
}

fem_error_t flex_body2d_compute_tip_marker_disp(
    const flex_body2d_t *body,
    const double reference_pose[3],
    const double current_pose[3],
    double marker_disp[3])
{
    return flex_body2d_compute_set_marker_disp(body,
                                               &body->tip_set,
                                               reference_pose,
                                               current_pose,
                                               "Tip set",
                                               marker_disp);
}

fem_error_t flex_body2d_compute_root_center_local(const flex_body2d_t *body,
                                                  double center_local[2])
{
    return flex_body2d_compute_set_center_local(body,
                                                &body->root_set,
                                                "Root set",
                                                center_local);
}

fem_error_t flex_body2d_compute_tip_center_local(const flex_body2d_t *body,
                                                 double center_local[2])
{
    return flex_body2d_compute_set_center_local(body,
                                                &body->tip_set,
                                                "Tip set",
                                                center_local);
}

fem_error_t flex_body2d_compute_root_center_world(const flex_body2d_t *body,
                                                  const double body_pose[3],
                                                  double center_world[2])
{
    return flex_body2d_compute_set_center_world(body,
                                                &body->root_set,
                                                "Root set",
                                                body_pose,
                                                center_world);
}

fem_error_t flex_body2d_compute_tip_center_world(const flex_body2d_t *body,
                                                 const double body_pose[3],
                                                 double center_world[2])
{
    return flex_body2d_compute_set_center_world(body,
                                                &body->tip_set,
                                                "Tip set",
                                                body_pose,
                                                center_world);
}

fem_error_t flex_body2d_compute_interface_body_force(
    const flex_body2d_t *body,
    double root_body_force[3],
    double tip_body_force[3],
    double total_body_force[3])
{
    CHECK_NULL(body, "Flexible body");
    CHECK_NULL(root_body_force, "Flexible root body force");
    CHECK_NULL(tip_body_force, "Flexible tip body force");
    CHECK_NULL(total_body_force, "Flexible total body force");

    flex_reaction2d_to_root_body_force(body->reaction_root, root_body_force);
    flex_reaction2d_to_tip_body_force(body->reaction_tip, tip_body_force);
    flex_reaction2d_sum_interface_forces(root_body_force,
                                         tip_body_force,
                                         total_body_force);
    return FEM_SUCCESS;
}

fem_error_t flex_body2d_solve_snapshot(flex_body2d_t *body,
                                       const double root_marker_disp[3],
                                       const double tip_marker_disp[3],
                                       const double inertial_load[3])
{
    flex_bc2d_list_t bc_list;
    fem_model2d_t *work_model = NULL;
    fem_model2d_t *assembled_model = NULL;
    double *residual = NULL;
    fem_error_t err;

    CHECK_NULL(body, "Flexible body");
    if (!root_marker_disp || !tip_marker_disp) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Snapshot solve requires root and tip marker displacements");
    }

    flex_bc2d_list_zero(&bc_list);
    work_model = calloc(1, sizeof(*work_model));
    assembled_model = calloc(1, sizeof(*assembled_model));
    if (!work_model || !assembled_model) {
        err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                        "Flexible body snapshot models");
        goto cleanup;
    }
    fem_model2d_zero(work_model);
    fem_model2d_zero(assembled_model);

    err = flex_body2d_collect_interface_bc(body,
                                           root_marker_disp,
                                           tip_marker_disp,
                                           &bc_list);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = fem_model2d_clone(work_model, &body->model);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    if (inertial_load) {
        flex_inertial_load2d_t inertial_spec;

        memset(&inertial_spec, 0, sizeof(inertial_spec));
        inertial_spec.translational_accel[0] = inertial_load[0];
        inertial_spec.translational_accel[1] = inertial_load[1];
        inertial_spec.density = inertial_load[2];
        err = flex_solver2d_set_inertial_loads(work_model, &inertial_spec);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    } else {
        err = flex_solver2d_clear_inertial_loads(work_model);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    err = flex_solver2d_apply_bc_entries(work_model, &bc_list);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = flex_solver2d_reassemble_and_solve(work_model, assembled_model);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    flex_body2d_copy_model_displacement(work_model,
                                        body->u_local,
                                        body->u_local_size);

    residual = calloc((size_t)body->u_local_size, sizeof(*residual));
    if (!residual) {
        err = error_set(FEM_ERROR_MEMORY_ALLOCATION,
                        "Flexible body residual workspace");
        goto cleanup;
    }

    err = flex_solver2d_compute_residual(assembled_model,
                                         body->u_local,
                                         body->u_local_size,
                                         residual,
                                         body->u_local_size);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = flex_reaction2d_from_node_set(&body->root_set,
                                        work_model,
                                        residual,
                                        body->u_local_size,
                                        body->reaction_root);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = flex_reaction2d_from_node_set(&body->tip_set,
                                        work_model,
                                        residual,
                                        body->u_local_size,
                                        body->reaction_tip);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    fem_model2d_free(&body->model);
    body->model = *work_model;
    fem_model2d_zero(work_model);

cleanup:
    free(residual);
    flex_bc2d_list_free(&bc_list);
    if (work_model) {
        fem_model2d_free(work_model);
        free(work_model);
    }
    if (assembled_model) {
        fem_model2d_free(assembled_model);
        free(assembled_model);
    }
    return err;
}

fem_error_t flex_body2d_apply_root_rigid_displacement(
    flex_body2d_t *body,
    const double marker_disp[3])
{
    flex_bc2d_list_t bc_list;
    fem_error_t err;

    CHECK_NULL(body, "Flexible body");
    flex_bc2d_list_zero(&bc_list);

    err = flex_body2d_build_root_rigid_bc(body, marker_disp, &bc_list);
    if (err == FEM_SUCCESS) {
        err = flex_body2d_apply_bc_list(body, &bc_list);
    }

    flex_bc2d_list_free(&bc_list);
    return err;
}

fem_error_t flex_body2d_apply_tip_rigid_displacement(
    flex_body2d_t *body,
    const double marker_disp[3])
{
    flex_bc2d_list_t bc_list;
    fem_error_t err;

    CHECK_NULL(body, "Flexible body");
    flex_bc2d_list_zero(&bc_list);

    err = flex_body2d_build_tip_rigid_bc(body, marker_disp, &bc_list);
    if (err == FEM_SUCCESS) {
        err = flex_body2d_apply_bc_list(body, &bc_list);
    }

    flex_bc2d_list_free(&bc_list);
    return err;
}

void flex_body2d_free(flex_body2d_t *body)
{
    if (!body) {
        return;
    }

    free(body->u_local);
    body->u_local = NULL;
    fem_model2d_free(&body->model);
    node_set_free(&body->root_set);
    node_set_free(&body->tip_set);
    flex_body2d_zero(body);
}
