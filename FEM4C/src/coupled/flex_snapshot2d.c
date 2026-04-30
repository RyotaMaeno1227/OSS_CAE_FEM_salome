#include "flex_snapshot2d.h"

#include "../common/constants.h"
#include "../common/error.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

fem_error_t flex_snapshot2d_write_csv_with_interface_centers(
    const fem_model_t *model,
    int body_id,
    int step_index,
    int iteration_index,
    double time,
    const double marker_pose[3],
    const double root_center_local[2],
    const double tip_center_local[2],
    const double root_center_world[2],
    const double tip_center_world[2],
    const double root_reaction_local[3],
    const double tip_reaction_local[3],
    const double root_body_force[3],
    const double tip_body_force[3],
    const double total_body_force[3],
    const char *output_filename);

static void flex_snapshot2d_rotate_point(const double local_point[2],
                                         double theta,
                                         double world_offset[2])
{
    const double c = cos(theta);
    const double s = sin(theta);

    if (!local_point || !world_offset) {
        return;
    }

    world_offset[0] = c * local_point[0] - s * local_point[1];
    world_offset[1] = s * local_point[0] + c * local_point[1];
}

static void flex_snapshot2d_rotate_vector(const double local_vector[2],
                                          double theta,
                                          double world_vector[2])
{
    const double c = cos(theta);
    const double s = sin(theta);

    if (!local_vector || !world_vector) {
        return;
    }

    world_vector[0] = c * local_vector[0] - s * local_vector[1];
    world_vector[1] = s * local_vector[0] + c * local_vector[1];
}

static void flex_snapshot2d_compute_centroid(const fem_model_t *model,
                                             int include_displacement,
                                             double centroid_local[2])
{
    int i;
    double sum_x = 0.0;
    double sum_y = 0.0;

    if (!centroid_local) {
        return;
    }

    centroid_local[0] = 0.0;
    centroid_local[1] = 0.0;
    if (!model || model->num_nodes <= 0 || !model->node_coords) {
        return;
    }

    for (i = 0; i < model->num_nodes; ++i) {
        const double ux = (include_displacement && model->node_displ)
                              ? model->node_displ[i][0]
                              : 0.0;
        const double uy = (include_displacement && model->node_displ)
                              ? model->node_displ[i][1]
                              : 0.0;
        sum_x += model->node_coords[i][0] + ux;
        sum_y += model->node_coords[i][1] + uy;
    }

    centroid_local[0] = sum_x / (double)model->num_nodes;
    centroid_local[1] = sum_y / (double)model->num_nodes;
}

static void flex_snapshot2d_compute_load_resultant(const fem_model_t *model,
                                                   const double marker_pose[3],
                                                   double load_local[3],
                                                   double load_world[3])
{
    int i;
    double local_force[2] = {0.0, 0.0};
    double world_force[2] = {0.0, 0.0};

    if (load_local) {
        memset(load_local, 0, sizeof(double) * 3);
    }
    if (load_world) {
        memset(load_world, 0, sizeof(double) * 3);
    }
    if (!model || !model->node_force || !model->node_coords) {
        return;
    }

    for (i = 0; i < model->num_nodes; ++i) {
        const double fx = model->node_force[i][0];
        const double fy = model->node_force[i][1];

        local_force[0] += fx;
        local_force[1] += fy;
        if (load_local) {
            load_local[2] += model->node_coords[i][0] * fy
                             - model->node_coords[i][1] * fx;
        }
    }

    if (load_local) {
        load_local[0] = local_force[0];
        load_local[1] = local_force[1];
    }

    if (!load_world || !marker_pose) {
        return;
    }

    flex_snapshot2d_rotate_vector(local_force, marker_pose[2], world_force);
    load_world[0] = world_force[0];
    load_world[1] = world_force[1];
    load_world[2] = load_local ? load_local[2] : 0.0;
}

fem_error_t flex_snapshot2d_build_output_path(char path[MAX_FILENAME_LEN],
                                              const char *output_filename,
                                              int body_id,
                                              int step_index,
                                              int iteration_index,
                                              double time)
{
    char stem[MAX_FILENAME_LEN];
    char *dot = NULL;

    CHECK_NULL(path, "flex snapshot path");
    CHECK_NULL(output_filename, "flex snapshot output filename");

    strncpy(stem, output_filename, sizeof(stem) - 1);
    stem[sizeof(stem) - 1] = '\0';

    dot = strrchr(stem, '.');
    if (dot) {
        *dot = '\0';
    }

    if (snprintf(path,
                 MAX_FILENAME_LEN,
                 "%s_body%d_step%04d_iter%02d_t%.6e.csv",
                 stem,
                 body_id,
                 step_index,
                 iteration_index,
                 time) >= MAX_FILENAME_LEN) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex snapshot filename exceeds max length for body %d step %d iter %d",
                         body_id,
                         step_index,
                         iteration_index);
    }

    return FEM_SUCCESS;
}

fem_error_t flex_snapshot2d_write_csv(const fem_model_t *model,
                                      int body_id,
                                      int step_index,
                                      int iteration_index,
                                      double time,
                                      const double marker_pose[3],
                                      const char *output_filename)
{
    return flex_snapshot2d_write_csv_with_interface_centers(model,
                                                            body_id,
                                                            step_index,
                                                            iteration_index,
                                                            time,
                                                            marker_pose,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            output_filename);
}

fem_error_t flex_snapshot2d_write_csv_with_interface_centers(
    const fem_model_t *model,
    int body_id,
    int step_index,
    int iteration_index,
    double time,
    const double marker_pose[3],
    const double root_center_local[2],
    const double tip_center_local[2],
    const double root_center_world[2],
    const double tip_center_world[2],
    const double root_reaction_local[3],
    const double tip_reaction_local[3],
    const double root_body_force[3],
    const double tip_body_force[3],
    const double total_body_force[3],
    const char *output_filename)
{
    char snapshot_filename[MAX_FILENAME_LEN];
    FILE *out = NULL;
    int i;
    double observation_ref_local[2] = {0.0, 0.0};
    double observation_local[2] = {0.0, 0.0};
    double observation_disp_local[2] = {0.0, 0.0};
    double observation_ref_world_offset[2] = {0.0, 0.0};
    double observation_world_offset[2] = {0.0, 0.0};
    double observation_disp_world[2] = {0.0, 0.0};
    double load_resultant_local[3] = {0.0, 0.0, 0.0};
    double load_resultant_world[3] = {0.0, 0.0, 0.0};

    CHECK_NULL(model, "flex snapshot model");
    CHECK_NULL(marker_pose, "flex snapshot marker pose");
    CHECK_NULL(output_filename, "flex snapshot output filename");

    CHECK_ERROR(flex_snapshot2d_build_output_path(snapshot_filename,
                                                  output_filename,
                                                  body_id,
                                                  step_index,
                                                  iteration_index,
                                                  time));

    out = fopen(snapshot_filename, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open flex snapshot file: %s",
                         snapshot_filename);
    }

    fprintf(out, "# FEM4C flexible snapshot\n");
    fprintf(out, "body_id,%d\n", body_id);
    fprintf(out, "step_index,%d\n", step_index);
    fprintf(out, "iteration_index,%d\n", iteration_index);
    fprintf(out, "time,%.16e\n", time);
    fprintf(out, "marker_pose,%.16e,%.16e,%.16e\n",
            marker_pose[0], marker_pose[1], marker_pose[2]);
    if (root_center_local) {
        fprintf(out, "root_center_local,%.16e,%.16e\n",
                root_center_local[0], root_center_local[1]);
    }
    if (tip_center_local) {
        fprintf(out, "tip_center_local,%.16e,%.16e\n",
                tip_center_local[0], tip_center_local[1]);
    }
    if (root_center_world) {
        fprintf(out, "root_center_world,%.16e,%.16e\n",
                root_center_world[0], root_center_world[1]);
    }
    if (tip_center_world) {
        fprintf(out, "tip_center_world,%.16e,%.16e\n",
                tip_center_world[0], tip_center_world[1]);
    }
    if (root_reaction_local) {
        fprintf(out, "root_reaction_local,%.16e,%.16e,%.16e\n",
                root_reaction_local[0],
                root_reaction_local[1],
                root_reaction_local[2]);
    }
    if (tip_reaction_local) {
        fprintf(out, "tip_reaction_local,%.16e,%.16e,%.16e\n",
                tip_reaction_local[0],
                tip_reaction_local[1],
                tip_reaction_local[2]);
    }
    if (root_body_force) {
        fprintf(out, "root_body_force,%.16e,%.16e,%.16e\n",
                root_body_force[0],
                root_body_force[1],
                root_body_force[2]);
    }
    if (tip_body_force) {
        fprintf(out, "tip_body_force,%.16e,%.16e,%.16e\n",
                tip_body_force[0],
                tip_body_force[1],
                tip_body_force[2]);
    }
    if (total_body_force) {
        fprintf(out, "total_body_force,%.16e,%.16e,%.16e\n",
                total_body_force[0],
                total_body_force[1],
                total_body_force[2]);
    }
    flex_snapshot2d_compute_centroid(model, 0, observation_ref_local);
    flex_snapshot2d_compute_centroid(model, 1, observation_local);
    observation_disp_local[0] = observation_local[0] - observation_ref_local[0];
    observation_disp_local[1] = observation_local[1] - observation_ref_local[1];
    flex_snapshot2d_rotate_point(observation_ref_local,
                                 marker_pose[2],
                                 observation_ref_world_offset);
    flex_snapshot2d_rotate_point(observation_local,
                                 marker_pose[2],
                                 observation_world_offset);
    flex_snapshot2d_rotate_vector(observation_disp_local,
                                  marker_pose[2],
                                  observation_disp_world);
    fprintf(out, "observation_point_label,model_centroid\n");
    fprintf(out, "observation_point_ref_local,%.16e,%.16e\n",
            observation_ref_local[0], observation_ref_local[1]);
    fprintf(out, "observation_point_local,%.16e,%.16e\n",
            observation_local[0], observation_local[1]);
    fprintf(out, "observation_point_world,%.16e,%.16e\n",
            marker_pose[0] + observation_world_offset[0],
            marker_pose[1] + observation_world_offset[1]);
    fprintf(out, "observation_point_disp_local,%.16e,%.16e\n",
            observation_disp_local[0], observation_disp_local[1]);
    fprintf(out, "observation_point_disp_world,%.16e,%.16e\n",
            observation_disp_world[0], observation_disp_world[1]);
    flex_snapshot2d_compute_load_resultant(model,
                                           marker_pose,
                                           load_resultant_local,
                                           load_resultant_world);
    fprintf(out, "load_resultant_local,%.16e,%.16e,%.16e\n",
            load_resultant_local[0],
            load_resultant_local[1],
            load_resultant_local[2]);
    fprintf(out, "load_resultant_world,%.16e,%.16e,%.16e\n",
            load_resultant_world[0],
            load_resultant_world[1],
            load_resultant_world[2]);
    fprintf(out,
            "node_id,x_local,y_local,ux_local,uy_local,x_local_def,y_local_def,"
            "x_world_ref,y_world_ref,ux_world,uy_world,x_world,y_world\n");

    for (i = 0; i < model->num_nodes; ++i) {
        const int node_id = model->node_ids ? model->node_ids[i] : i + 1;
        const double x_local = model->node_coords ? model->node_coords[i][0] : 0.0;
        const double y_local = model->node_coords ? model->node_coords[i][1] : 0.0;
        const double ux_local = model->node_displ ? model->node_displ[i][0] : 0.0;
        const double uy_local = model->node_displ ? model->node_displ[i][1] : 0.0;
        const double local_deformed[2] = {x_local + ux_local, y_local + uy_local};
        const double local_reference[2] = {x_local, y_local};
        const double local_displacement[2] = {ux_local, uy_local};
        double world_ref[2] = {0.0, 0.0};
        double world_displ[2] = {0.0, 0.0};
        double world_offset[2] = {0.0, 0.0};

        flex_snapshot2d_rotate_point(local_reference, marker_pose[2], world_ref);
        flex_snapshot2d_rotate_point(local_displacement, marker_pose[2], world_displ);
        flex_snapshot2d_rotate_point(local_deformed, marker_pose[2], world_offset);
        fprintf(out,
                "%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e\n",
                node_id,
                x_local,
                y_local,
                ux_local,
                uy_local,
                local_deformed[0],
                local_deformed[1],
                marker_pose[0] + world_ref[0],
                marker_pose[1] + world_ref[1],
                world_displ[0],
                world_displ[1],
                marker_pose[0] + world_offset[0],
                marker_pose[1] + world_offset[1]);
    }

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot close flex snapshot file: %s",
                         snapshot_filename);
    }

    return FEM_SUCCESS;
}
