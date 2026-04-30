#include "contact_patch_load2d.h"

#include "flex_snapshot2d.h"
#include "flex_solver2d.h"

#include "../common/error.h"
#include "../common/globals.h"
#include "../io/input.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static fem_error_t contact_patch_load2d_validate_patch(const contact_patch2d_t *patch);
static fem_error_t contact_patch_load2d_load_fixture_model(const char *fixture_path,
                                                           fem_model_t *model);
static fem_error_t contact_patch_load2d_validate_frame(const contact_patch2d_t *patch);
static fem_error_t contact_patch_load2d_build_q4_shape_distribution(
    const fem_model_t *model,
    const double point_local[2],
    int node_ids[CONTACT_PATCH_LOAD2D_MAX_NODES],
    double weights[CONTACT_PATCH_LOAD2D_MAX_NODES]);
static fem_error_t contact_patch_load2d_compute_reaction_resultant(
    const contact_patch2d_t *patch,
    const fem_model_t *assembled_model,
    const fem_model_t *solved_model,
    double reaction_resultant_local[3]);
static void contact_patch_load2d_compute_displacement_summary(
    const fem_model_t *model,
    double *max_norm_out,
    double centroid_local[2]);
static fem_error_t contact_patch_load2d_write_summary_json(
    const contact_patch2d_t *patch,
    const contact_patch_load2d_t *load,
    const contact_patch_load2d_result_t *result);

void contact_patch_load2d_zero(contact_patch_load2d_t *load)
{
    if (!load) {
        return;
    }
    memset(load, 0, sizeof(*load));
    for (int i = 0; i < CONTACT_PATCH_LOAD2D_MAX_NODES; ++i) {
        load->node_ids[i] = -1;
    }
}

void contact_patch_load2d_result_zero(contact_patch_load2d_result_t *result)
{
    if (!result) {
        return;
    }
    memset(result, 0, sizeof(*result));
}

fem_error_t contact_patch_load2d_build_equivalent_nodal_load(
    const contact_patch2d_t *patch,
    const fem_model_t *model,
    contact_patch_load2d_t *load)
{
    int node_ids[CONTACT_PATCH_LOAD2D_MAX_NODES] = {-1, -1, -1, -1};
    double node_weights[CONTACT_PATCH_LOAD2D_MAX_NODES] = {0.0, 0.0, 0.0, 0.0};
    double force_world[2];
    double force_local[2];
    double force_sum_local[2] = {0.0, 0.0};
    int i;

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(model, "contact patch fixture model");
    CHECK_NULL(load, "contact patch load");
    CHECK_ERROR(contact_patch_load2d_validate_patch(patch));

    if (model->num_nodes < CONTACT_PATCH_LOAD2D_MAX_NODES || !model->node_coords) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch fixture requires at least %d nodes",
                         CONTACT_PATCH_LOAD2D_MAX_NODES);
    }

    contact_patch_load2d_zero(load);
    force_world[0] = -patch->fn_macro * patch->normal_world[0];
    force_world[1] = -patch->fn_macro * patch->normal_world[1];
    force_local[0] = force_world[0] * patch->tangent_world[0]
                     + force_world[1] * patch->tangent_world[1];
    force_local[1] = force_world[0] * patch->normal_world[0]
                     + force_world[1] * patch->normal_world[1];

    load->total_force_world[0] = force_world[0];
    load->total_force_world[1] = force_world[1];
    load->total_force_local[0] = force_local[0];
    load->total_force_local[1] = force_local[1];
    load->application_point_local[0] = patch->contact_point_world[0];
    load->application_point_local[1] = patch->contact_point_world[1];

    CHECK_ERROR(contact_patch_load2d_build_q4_shape_distribution(model,
                                                                 patch->contact_point_world,
                                                                 node_ids,
                                                                 node_weights));

    load->node_count = CONTACT_PATCH_LOAD2D_MAX_NODES;
    for (i = 0; i < CONTACT_PATCH_LOAD2D_MAX_NODES; ++i) {
        load->node_ids[i] = node_ids[i];
        load->nodal_force_local[i][0] = node_weights[i] * force_local[0];
        load->nodal_force_local[i][1] = node_weights[i] * force_local[1];
        force_sum_local[0] += load->nodal_force_local[i][0];
        force_sum_local[1] += load->nodal_force_local[i][1];
    }

    if (fabs(force_sum_local[0] - force_local[0]) > 1.0e-12 ||
        fabs(force_sum_local[1] - force_local[1]) > 1.0e-12) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch Q4 load distribution did not preserve total force");
    }

    return FEM_SUCCESS;
}

fem_error_t contact_patch_load2d_apply_to_model(
    fem_model_t *model,
    const contact_patch_load2d_t *load)
{
    int i;

    CHECK_NULL(model, "contact patch fixture model");
    CHECK_NULL(load, "contact patch load");

    if (!model->node_force || !model->node_ids) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch fixture model node force arrays are not initialized");
    }

    for (i = 0; i < load->node_count; ++i) {
        int node_index = -1;
        int j;

        for (j = 0; j < model->num_nodes; ++j) {
            if (model->node_ids[j] == load->node_ids[i]) {
                node_index = j;
                break;
            }
        }
        if (node_index < 0) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "contact patch load node id %d is not present in fixture model",
                             load->node_ids[i]);
        }

        model->node_force[node_index][0] += load->nodal_force_local[i][0];
        model->node_force[node_index][1] += load->nodal_force_local[i][1];
    }

    return FEM_SUCCESS;
}

fem_error_t contact_patch_load2d_run_fixture_static(
    const contact_patch2d_t *patch,
    contact_patch_load2d_result_t *result_out)
{
    fem_model_t fixture_model;
    fem_model_t assembled_model;
    contact_patch_load2d_t load;
    contact_patch_load2d_result_t result;
    double marker_pose[3] = {0.0, 0.0, 0.0};
    fem_error_t err;

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(result_out, "contact patch receiver result");
    CHECK_ERROR(contact_patch_load2d_validate_patch(patch));

    fem_model_zero(&fixture_model);
    fem_model_zero(&assembled_model);
    contact_patch_load2d_zero(&load);
    contact_patch_load2d_result_zero(&result);

    CHECK_ERROR(contact_patch_load2d_load_fixture_model(patch->mesh_path, &fixture_model));

    err = globals_initialize();
    if (err != FEM_SUCCESS) {
        fem_model_free(&fixture_model);
        return err;
    }

    err = contact_patch_load2d_build_equivalent_nodal_load(patch, &fixture_model, &load);
    if (err == FEM_SUCCESS) {
        err = flex_solver2d_prepare_model(&fixture_model);
    }
    if (err == FEM_SUCCESS) {
        err = contact_patch_load2d_apply_to_model(&fixture_model, &load);
    }
    if (err == FEM_SUCCESS) {
        err = flex_solver2d_reassemble_and_solve(&fixture_model, &assembled_model);
    }
    if (err == FEM_SUCCESS) {
        err = flex_snapshot2d_build_output_path(result.deformed_output_path,
                                                patch->output_path,
                                                patch->body_id,
                                                patch->step,
                                                0,
                                                patch->time);
    }
    if (err == FEM_SUCCESS) {
        err = flex_snapshot2d_write_csv(&fixture_model,
                                        patch->body_id,
                                        patch->step,
                                        0,
                                        patch->time,
                                        marker_pose,
                                        patch->output_path);
    }
    if (err == FEM_SUCCESS) {
        snprintf(result.summary_path,
                 sizeof(result.summary_path),
                 "%s",
                 patch->output_path);
        contact_patch_load2d_compute_displacement_summary(
            &fixture_model,
            &result.displacement_max_norm,
            result.displacement_centroid_local);
        result.applied_force_local[0] = load.total_force_local[0];
        result.applied_force_local[1] = load.total_force_local[1];
        result.applied_force_world[0] = load.total_force_world[0];
        result.applied_force_world[1] = load.total_force_world[1];
        err = contact_patch_load2d_compute_reaction_resultant(
            patch,
            &assembled_model,
            &fixture_model,
            result.reaction_resultant_local);
    }
    if (err == FEM_SUCCESS) {
        err = contact_patch_load2d_write_summary_json(patch, &load, &result);
    }

    fem_model_free(&assembled_model);
    fem_model_free(&fixture_model);
    globals_finalize();
    if (err != FEM_SUCCESS) {
        return err;
    }

    *result_out = result;
    return FEM_SUCCESS;
}

static fem_error_t contact_patch_load2d_validate_patch(const contact_patch2d_t *patch)
{
    const double contact_point_norm2 = patch->contact_point_world[0] * patch->contact_point_world[0]
                                       + patch->contact_point_world[1] * patch->contact_point_world[1];
    const int has_radius_body = patch->radius_body > 0.0;
    const int has_patch_size = patch->patch_size > 0.0;

    CHECK_NULL(patch, "contact patch");

    if (patch->mesh_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch mesh_path must point to a fixture mesh");
    }
    if (patch->output_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch output_path must not be empty");
    }
    if (!isfinite(patch->fn_macro) || patch->fn_macro < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch fn_macro must be finite and non-negative");
    }
    if (!isfinite(patch->radius_body) || patch->radius_body < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch radius_body must be finite and non-negative");
    }
    if (!isfinite(patch->thickness) || patch->thickness <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch thickness must be finite and positive");
    }
    if (!isfinite(patch->patch_size) || patch->patch_size < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch patch_size must be finite and non-negative");
    }
    if (has_radius_body != has_patch_size) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch radius_body and patch_size must both be set or both be legacy-unset");
    }
    if (has_radius_body && has_patch_size &&
        patch->patch_size > 2.0 * patch->radius_body) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch patch_size must not exceed the body diameter (patch_size=%.16e radius_body=%.16e)",
                         patch->patch_size,
                         patch->radius_body);
    }
    if (!isfinite(patch->contact_point_world[0]) ||
        !isfinite(patch->contact_point_world[1]) ||
        !isfinite(patch->normal_world[0]) ||
        !isfinite(patch->normal_world[1]) ||
        !isfinite(patch->tangent_world[0]) ||
        !isfinite(patch->tangent_world[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch receiver inputs must be finite");
    }
    if (!isfinite(contact_point_norm2)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch contact_point_world must be finite");
    }

    return contact_patch_load2d_validate_frame(patch);
}

static fem_error_t contact_patch_load2d_load_fixture_model(const char *fixture_path,
                                                           fem_model_t *model)
{
    fem_error_t err;

    CHECK_NULL(fixture_path, "contact patch fixture path");
    CHECK_NULL(model, "contact patch fixture model");

    err = globals_initialize();
    if (err != FEM_SUCCESS) {
        return err;
    }

    err = input_read_data(fixture_path);
    if (err == FEM_SUCCESS) {
        err = fem_model_clone_from_globals(model);
    }
    globals_finalize();
    return err;
}

static fem_error_t contact_patch_load2d_validate_frame(const contact_patch2d_t *patch)
{
    const double normal_norm = sqrt(patch->normal_world[0] * patch->normal_world[0]
                                    + patch->normal_world[1] * patch->normal_world[1]);
    const double tangent_norm = sqrt(patch->tangent_world[0] * patch->tangent_world[0]
                                     + patch->tangent_world[1] * patch->tangent_world[1]);
    const double dot = patch->normal_world[0] * patch->tangent_world[0]
                       + patch->normal_world[1] * patch->tangent_world[1];

    CHECK_NULL(patch, "contact patch");

    if (!isfinite(normal_norm) || normal_norm <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch normal_world must have positive finite norm");
    }
    if (!isfinite(tangent_norm) || tangent_norm <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch tangent_world must have positive finite norm");
    }
    if (fabs(normal_norm - 1.0) > 1.0e-8) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch normal_world must be unit length (norm=%.16e)",
                         normal_norm);
    }
    if (fabs(tangent_norm - 1.0) > 1.0e-8) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch tangent_world must be unit length (norm=%.16e)",
                         tangent_norm);
    }
    if (!isfinite(dot) || fabs(dot) > 1.0e-8) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch normal_world and tangent_world must be orthogonal (dot=%.16e)",
                         dot);
    }
    return FEM_SUCCESS;
}

static fem_error_t contact_patch_load2d_build_q4_shape_distribution(
    const fem_model_t *model,
    const double point_local[2],
    int node_ids[CONTACT_PATCH_LOAD2D_MAX_NODES],
    double weights[CONTACT_PATCH_LOAD2D_MAX_NODES])
{
    double coords[CONTACT_PATCH_LOAD2D_MAX_NODES][2];
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double tol_x;
    double tol_y;
    double xi;
    double eta;
    double weight_sum;
    int i;

    CHECK_NULL(model, "contact patch fixture model");
    CHECK_NULL(point_local, "contact patch point");
    CHECK_NULL(node_ids, "contact patch load node ids");
    CHECK_NULL(weights, "contact patch load node weights");

    if (model->num_elements != 1 || !model->element_nodes || !model->element_type) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch receiver requires a single-element fixture");
    }
    if (model->element_type[0] != ELEMENT_Q4) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "contact patch receiver requires a Q4 fixture element");
    }
    if (!model->node_coords) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch fixture model node coordinates are not initialized");
    }

    for (i = 0; i < CONTACT_PATCH_LOAD2D_MAX_NODES; ++i) {
        const int node_index = model->element_nodes[0][i];
        if (node_index < 0 || node_index >= model->num_nodes) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "contact patch Q4 fixture has invalid node index at local node %d",
                             i);
        }
        coords[i][0] = model->node_coords[node_index][0];
        coords[i][1] = model->node_coords[node_index][1];
        node_ids[i] = model->node_ids ? model->node_ids[node_index] : node_index + 1;
    }

    x_min = coords[0][0];
    x_max = coords[0][0];
    y_min = coords[0][1];
    y_max = coords[0][1];
    for (i = 1; i < CONTACT_PATCH_LOAD2D_MAX_NODES; ++i) {
        if (coords[i][0] < x_min) x_min = coords[i][0];
        if (coords[i][0] > x_max) x_max = coords[i][0];
        if (coords[i][1] < y_min) y_min = coords[i][1];
        if (coords[i][1] > y_max) y_max = coords[i][1];
    }
    if (!(x_max > x_min) || !(y_max > y_min)) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "contact patch Q4 fixture must span a positive rectangular area");
    }

    tol_x = 1.0e-9 * fmax(1.0, fabs(x_max - x_min));
    tol_y = 1.0e-9 * fmax(1.0, fabs(y_max - y_min));
    if (point_local[0] < x_min - tol_x || point_local[0] > x_max + tol_x ||
        point_local[1] < y_min - tol_y || point_local[1] > y_max + tol_y) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact point (%.16e, %.16e) is outside the Q4 patch bounds",
                         point_local[0],
                         point_local[1]);
    }

    xi = 2.0 * (point_local[0] - x_min) / (x_max - x_min) - 1.0;
    eta = 2.0 * (point_local[1] - y_min) / (y_max - y_min) - 1.0;
    if (xi < -1.0 - 1.0e-9 || xi > 1.0 + 1.0e-9 ||
        eta < -1.0 - 1.0e-9 || eta > 1.0 + 1.0e-9) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact point maps outside the Q4 natural coordinates");
    }

    xi = fmax(-1.0, fmin(1.0, xi));
    eta = fmax(-1.0, fmin(1.0, eta));
    weights[0] = 0.25 * (1.0 - xi) * (1.0 - eta);
    weights[1] = 0.25 * (1.0 + xi) * (1.0 - eta);
    weights[2] = 0.25 * (1.0 + xi) * (1.0 + eta);
    weights[3] = 0.25 * (1.0 - xi) * (1.0 + eta);
    weight_sum = 0.0;
    for (i = 0; i < CONTACT_PATCH_LOAD2D_MAX_NODES; ++i) {
        if (weights[i] < -1.0e-12) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "contact patch Q4 shape weight became negative at local node %d",
                             i);
        }
        if (weights[i] < 0.0) {
            weights[i] = 0.0;
        }
        weight_sum += weights[i];
    }
    if (fabs(weight_sum - 1.0) > 1.0e-12) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch Q4 shape weights did not sum to one");
    }

    return FEM_SUCCESS;
}

static fem_error_t contact_patch_load2d_compute_reaction_resultant(
    const contact_patch2d_t *patch,
    const fem_model_t *assembled_model,
    const fem_model_t *solved_model,
    double reaction_resultant_local[3])
{
    double *residual = NULL;
    fem_error_t err;
    int i;

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(assembled_model, "contact patch assembled model");
    CHECK_NULL(solved_model, "contact patch solved model");
    CHECK_NULL(reaction_resultant_local, "contact patch reaction resultant");

    if (assembled_model->total_dof <= 0 || !solved_model->global_displ) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch solved model does not have a valid displacement vector");
    }

    residual = calloc((size_t)assembled_model->total_dof, sizeof(*residual));
    CHECK_NULL(residual, "contact patch reaction residual");

    err = flex_solver2d_compute_residual(assembled_model,
                                         solved_model->global_displ,
                                         assembled_model->total_dof,
                                         residual,
                                         assembled_model->total_dof);
    if (err != FEM_SUCCESS) {
        free(residual);
        return err;
    }

    reaction_resultant_local[0] = 0.0;
    reaction_resultant_local[1] = 0.0;
    reaction_resultant_local[2] = 0.0;
    for (i = 0; i < solved_model->num_nodes; ++i) {
        const int dof_u = i * 2;
        const int dof_v = dof_u + 1;
        const int constrained_u = solved_model->node_bc_flags
                                      ? solved_model->node_bc_flags[i][0]
                                      : 0;
        const int constrained_v = solved_model->node_bc_flags
                                      ? solved_model->node_bc_flags[i][1]
                                      : 0;
        const double x = solved_model->node_coords ? solved_model->node_coords[i][0] : 0.0;
        const double y = solved_model->node_coords ? solved_model->node_coords[i][1] : 0.0;
        const double rx = constrained_u ? residual[dof_u] : 0.0;
        const double ry = constrained_v ? residual[dof_v] : 0.0;
        const double rel_x = x - patch->contact_point_world[0];
        const double rel_y = y - patch->contact_point_world[1];

        reaction_resultant_local[0] += rx;
        reaction_resultant_local[1] += ry;
        reaction_resultant_local[2] += rel_x * ry - rel_y * rx;
    }

    free(residual);
    return FEM_SUCCESS;
}

static void contact_patch_load2d_compute_displacement_summary(
    const fem_model_t *model,
    double *max_norm_out,
    double centroid_local[2])
{
    int i;
    double max_norm = 0.0;
    double sum_x = 0.0;
    double sum_y = 0.0;

    if (max_norm_out) {
        *max_norm_out = 0.0;
    }
    if (centroid_local) {
        centroid_local[0] = 0.0;
        centroid_local[1] = 0.0;
    }
    if (!model || model->num_nodes <= 0 || !model->node_displ) {
        return;
    }

    for (i = 0; i < model->num_nodes; ++i) {
        const double ux = model->node_displ[i][0];
        const double uy = model->node_displ[i][1];
        const double norm = sqrt(ux * ux + uy * uy);

        if (norm > max_norm) {
            max_norm = norm;
        }
        sum_x += ux;
        sum_y += uy;
    }

    if (max_norm_out) {
        *max_norm_out = max_norm;
    }
    if (centroid_local) {
        centroid_local[0] = sum_x / (double)model->num_nodes;
        centroid_local[1] = sum_y / (double)model->num_nodes;
    }
}

static fem_error_t contact_patch_load2d_write_summary_json(
    const contact_patch2d_t *patch,
    const contact_patch_load2d_t *load,
    const contact_patch_load2d_result_t *result)
{
    FILE *out = NULL;

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(load, "contact patch load");
    CHECK_NULL(result, "contact patch result");

    out = fopen(result->summary_path, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "cannot open contact patch receiver summary: %s",
                         result->summary_path);
    }

    if (fprintf(out,
                "{\n"
                "  \"body_id\": %d,\n"
                "  \"pair_id\": %d,\n"
                "  \"step\": %d,\n"
                "  \"time\": %.16e,\n"
                "  \"fixture_mesh_path\": \"%s\",\n"
                "  \"contact_point_local\": [%.16e, %.16e],\n"
                "  \"normal_world\": [%.16e, %.16e],\n"
                "  \"fn_macro\": %.16e,\n"
                "  \"applied_force_world\": [%.16e, %.16e],\n"
                "  \"applied_force_local\": [%.16e, %.16e],\n"
                "  \"load_node_ids\": [%d, %d, %d, %d],\n"
                "  \"displacement_max_norm\": %.16e,\n"
                "  \"displacement_centroid_local\": [%.16e, %.16e],\n"
                "  \"reaction_resultant_local\": [%.16e, %.16e, %.16e],\n"
                "  \"deformed_output_path\": \"%s\"\n"
                "}\n",
                patch->body_id,
                patch->pair_id,
                patch->step,
                patch->time,
                patch->mesh_path,
                patch->contact_point_world[0],
                patch->contact_point_world[1],
                patch->normal_world[0],
                patch->normal_world[1],
                patch->fn_macro,
                result->applied_force_world[0],
                result->applied_force_world[1],
                result->applied_force_local[0],
                result->applied_force_local[1],
                load->node_ids[0],
                load->node_ids[1],
                load->node_ids[2],
                load->node_ids[3],
                result->displacement_max_norm,
                result->displacement_centroid_local[0],
                result->displacement_centroid_local[1],
                result->reaction_resultant_local[0],
                result->reaction_resultant_local[1],
                result->reaction_resultant_local[2],
                result->deformed_output_path) < 0) {
        fclose(out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to write contact patch receiver summary: %s",
                         result->summary_path);
    }

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to close contact patch receiver summary: %s",
                         result->summary_path);
    }
    return FEM_SUCCESS;
}
