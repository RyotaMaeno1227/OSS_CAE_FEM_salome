#include "coupled_run2d.h"

#include "../common/error.h"
#include "flex_body2d.h"
#include "flex_snapshot2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Owns per-step snapshot artifact writing and the dedicated capture helpers
 * needed by that writer. Inputs are coupled run state/history, accepted step
 * status, configured output paths, and flex body state. Outputs are snapshot
 * artifact files and updated snapshot records inside the step history. Side
 * effects are file creation/writing/closing through flex snapshot writers,
 * capture buffer updates, record-count updates, and related error reporting.
 * Route execution, solver work, summary output writing, descriptor selection,
 * and dynamic history buffer allocation remain outside this file. This
 * extraction is for readability and maintainability, not behavior change.
 */

static fem_error_t coupled_run2d_capture_marker_pose(const mbd_body2d_t *body,
                                                     double marker_pose[3])
{
    double origin[2];
    double theta = 0.0;

    CHECK_NULL(body, "MBD body");
    CHECK_NULL(marker_pose, "coupled marker pose");
    CHECK_ERROR(mbd_body2d_get_current_pose(body, origin, &theta));

    marker_pose[0] = origin[0];
    marker_pose[1] = origin[1];
    marker_pose[2] = theta;
    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_capture_interface_centers(
    const coupled_case2d_flex_body_t *case_body,
    const fem_model_t *model,
    const double marker_pose[3],
    double root_center_local[2],
    double tip_center_local[2],
    double root_center_world[2],
    double tip_center_world[2])
{
    flex_body2d_t *flex_body = NULL;
    node_set_t root_set;
    node_set_t tip_set;
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(case_body, "coupled case body");
    CHECK_NULL(model, "coupled flex snapshot model");
    CHECK_NULL(marker_pose, "coupled marker pose");
    CHECK_NULL(root_center_local, "coupled root center local");
    CHECK_NULL(tip_center_local, "coupled tip center local");
    CHECK_NULL(root_center_world, "coupled root center world");
    CHECK_NULL(tip_center_world, "coupled tip center world");

    flex_body = calloc(1, sizeof(*flex_body));
    if (!flex_body) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate coupled interface flex body");
    }

    flex_body2d_zero(flex_body);
    node_set_zero(&root_set);
    node_set_zero(&tip_set);

    err = coupled_case2d_build_root_node_set(case_body, &root_set);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = coupled_case2d_build_tip_node_set(case_body, &tip_set);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = flex_body2d_init(flex_body,
                           case_body->body_id,
                           model,
                           &root_set,
                           &tip_set);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = flex_body2d_compute_root_center_local(flex_body, root_center_local);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = flex_body2d_compute_tip_center_local(flex_body, tip_center_local);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = flex_body2d_compute_root_center_world(flex_body,
                                                marker_pose,
                                                root_center_world);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = flex_body2d_compute_tip_center_world(flex_body,
                                               marker_pose,
                                               tip_center_world);

cleanup:
    if (flex_body) {
        flex_body2d_free(flex_body);
        free(flex_body);
    }
    node_set_free(&root_set);
    node_set_free(&tip_set);
    return err;
}

fem_error_t coupled_run2d_write_step_snapshots(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history,
    const char *output_filename)
{
    int i;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled step history");
    CHECK_NULL(output_filename, "coupled output filename");

    CHECK_ERROR(coupled_step_history2d_reserve_snapshot_storage(
        history,
        run->case_data.num_flex_bodies));
    history->snapshot_record_count = 0;
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];
        double marker_pose[3];
        double root_center_local[2] = {0.0, 0.0};
        double tip_center_local[2] = {0.0, 0.0};
        double root_center_world[2] = {0.0, 0.0};
        double tip_center_world[2] = {0.0, 0.0};
        double root_reaction_local[3] = {0.0, 0.0, 0.0};
        double tip_reaction_local[3] = {0.0, 0.0, 0.0};
        double root_body_force[3] = {0.0, 0.0, 0.0};
        double tip_body_force[3] = {0.0, 0.0, 0.0};
        double total_body_force[3] = {0.0, 0.0, 0.0};
        char snapshot_path[MAX_FILENAME_LEN];
        const mbd_body2d_t *mbd_body = NULL;
        CHECK_ERROR(mbd_system2d_get_body_const(&run->mbd_system,
                                                body->body_id,
                                                &mbd_body));
        CHECK_ERROR(coupled_run2d_capture_marker_pose(mbd_body, marker_pose));
        CHECK_ERROR(coupled_run2d_capture_interface_centers(body,
                                                            &run->flex_models[i],
                                                            marker_pose,
                                                            root_center_local,
                                                            tip_center_local,
                                                            root_center_world,
                                                            tip_center_world));
        memcpy(root_reaction_local,
               history->flex_body_reaction_root[i],
               sizeof(root_reaction_local));
        memcpy(tip_reaction_local,
               history->flex_body_reaction_tip[i],
               sizeof(tip_reaction_local));
        memcpy(root_body_force,
               history->flex_body_root_force[i],
               sizeof(root_body_force));
        memcpy(tip_body_force,
               history->flex_body_tip_force[i],
               sizeof(tip_body_force));
        memcpy(total_body_force,
               history->flex_body_total_force[i],
               sizeof(total_body_force));
        CHECK_ERROR(flex_snapshot2d_build_output_path(
            snapshot_path,
            output_filename,
            body->body_id,
            history->step_index,
            history->fixed_point_iterations > 0
                ? history->fixed_point_iterations
                : 1,
            history->time));
        CHECK_ERROR(flex_snapshot2d_write_csv_with_interface_centers(
            &run->flex_models[i],
            body->body_id,
            history->step_index,
            history->fixed_point_iterations > 0
                ? history->fixed_point_iterations
                : 1,
            history->time,
            marker_pose,
            root_center_local,
            tip_center_local,
            root_center_world,
            tip_center_world,
            root_reaction_local,
            tip_reaction_local,
            root_body_force,
            tip_body_force,
            total_body_force,
            output_filename));
        history->snapshot_body_ids[history->snapshot_record_count] = body->body_id;
        history->snapshot_iteration_indices[history->snapshot_record_count] =
            history->fixed_point_iterations > 0
                ? history->fixed_point_iterations
                : 1;
        snprintf(history->snapshot_paths[history->snapshot_record_count],
                 MAX_FILENAME_LEN,
                 "%s",
                 snapshot_path);
        history->snapshot_record_count += 1;
    }

    return FEM_SUCCESS;
}
