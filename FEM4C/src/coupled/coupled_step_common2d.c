#include "coupled_step_common2d.h"

#include "../common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int coupled_step_common2d_count_defined_flex_bodies(const coupled_run2d_t *run)
{
    if (!run) {
        return 0;
    }

    return run->case_data.num_flex_bodies;
}

fem_error_t coupled_step_common2d_get_body_const_for_slot(
    const coupled_run2d_t *run,
    int flex_slot,
    const coupled_case2d_flex_body_t **case_body_out,
    const mbd_body2d_t **body_out)
{
    const coupled_case2d_flex_body_t *case_body = NULL;
    const mbd_body2d_t *body = NULL;

    CHECK_NULL(run, "coupled_run2d");

    if (flex_slot < 0 || flex_slot >= run->case_data.num_flex_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Primary flex slot %d is out of range",
                         flex_slot);
    }

    case_body = &run->case_data.flex_bodies[flex_slot];
    if (!case_body->is_defined) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Primary flex slot %d is not defined",
                         flex_slot);
    }

    CHECK_ERROR(mbd_system2d_get_body_const(&run->mbd_system,
                                            case_body->body_id,
                                            &body));

    if (case_body_out) {
        *case_body_out = case_body;
    }
    if (body_out) {
        *body_out = body;
    }
    return FEM_SUCCESS;
}

fem_error_t coupled_step_common2d_init_flex_body(
    flex_body2d_t *flex_body,
    const coupled_case2d_flex_body_t *case_body,
    const fem_model_t *model)
{
    node_set_t root_set;
    node_set_t tip_set;
    fem_error_t err;

    CHECK_NULL(flex_body, "coupled flex body");
    CHECK_NULL(case_body, "coupled flex body case");
    CHECK_NULL(model, "coupled flex model");

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

cleanup:
    node_set_free(&root_set);
    node_set_free(&tip_set);
    return err;
}

fem_error_t coupled_step_common2d_record_flex_snapshot_result(
    coupled_run2d_t *run,
    int flex_slot,
    flex_body2d_t *flex_body,
    int feedback_to_mbd,
    double total_body_force_out[3],
    coupled_step_history2d_t *history)
{
    double root_body_force[3] = {0.0, 0.0, 0.0};
    double tip_body_force[3] = {0.0, 0.0, 0.0};
    double total_body_force[3] = {0.0, 0.0, 0.0};
    fem_error_t err;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(flex_body, "coupled flex body");
    CHECK_NULL(history, "coupled step history");

    if (flex_slot < 0 || flex_slot >= run->case_data.num_flex_bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Primary flex slot %d is out of range",
                         flex_slot);
    }
    if (flex_slot >= history->flex_body_storage_capacity) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled step history storage missing for flex slot %d",
                         flex_slot);
    }

    err = flex_body2d_compute_interface_body_force(flex_body,
                                                   root_body_force,
                                                   tip_body_force,
                                                   total_body_force);
    if (err != FEM_SUCCESS) {
        return err;
    }
    if (feedback_to_mbd) {
        err = mbd_system2d_add_flexible_generalized_force(
            &run->mbd_system,
            run->case_data.flex_bodies[flex_slot].body_id,
            total_body_force);
        if (err != FEM_SUCCESS) {
            return err;
        }
    }

    history->flex_solves += 1;
    memcpy(history->flex_body_reaction_root[flex_slot],
           flex_body->reaction_root,
           sizeof(history->flex_body_reaction_root[flex_slot]));
    memcpy(history->flex_body_reaction_tip[flex_slot],
           flex_body->reaction_tip,
           sizeof(history->flex_body_reaction_tip[flex_slot]));
    memcpy(history->flex_body_root_force[flex_slot],
           root_body_force,
           sizeof(history->flex_body_root_force[flex_slot]));
    memcpy(history->flex_body_tip_force[flex_slot],
           tip_body_force,
           sizeof(history->flex_body_tip_force[flex_slot]));
    memcpy(history->flex_body_total_force[flex_slot],
           total_body_force,
           sizeof(history->flex_body_total_force[flex_slot]));

    if (total_body_force_out) {
        memcpy(total_body_force_out, total_body_force, sizeof(total_body_force));
    }

    fem_model_free(&run->flex_models[flex_slot]);
    run->flex_models[flex_slot] = flex_body->model;
    fem_model_zero(&flex_body->model);
    return FEM_SUCCESS;
}

fem_error_t coupled_step_common2d_solve_flex_snapshot_for_slot(
    coupled_run2d_t *run,
    int flex_slot,
    const double current_pose_override[3],
    int feedback_to_mbd,
    const char *line_prefix,
    const char *detail_prefix,
    const char *current_pose_label,
    const char *force_label,
    double total_body_force_out[3],
    coupled_step_history2d_t *history)
{
    const coupled_case2d_flex_body_t *case_body = NULL;
    const mbd_body2d_t *body = NULL;
    flex_body2d_t *flex_body = NULL;
    double reference_pose[3] = {0.0, 0.0, 0.0};
    double current_pose[3] = {0.0, 0.0, 0.0};
    double root_marker_disp[3] = {0.0, 0.0, 0.0};
    double tip_marker_disp[3] = {0.0, 0.0, 0.0};
    double total_body_force[3] = {0.0, 0.0, 0.0};
    fem_error_t err = FEM_SUCCESS;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled step history");

    CHECK_ERROR(coupled_step_common2d_get_body_const_for_slot(run,
                                                              flex_slot,
                                                              &case_body,
                                                              &body));

    flex_body = calloc(1, sizeof(*flex_body));
    if (!flex_body) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate coupled flex body snapshot");
    }

    flex_body2d_zero(flex_body);
    err = coupled_step_common2d_init_flex_body(flex_body,
                                               case_body,
                                               &run->flex_models[flex_slot]);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = coupled_step_common2d_capture_reference_pose(body, reference_pose);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    if (current_pose_override) {
        memcpy(current_pose, current_pose_override, sizeof(current_pose));
    } else {
        err = coupled_step_common2d_capture_current_pose(body, current_pose);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
    }

    err = coupled_step_common2d_compute_interface_marker_disp(flex_body,
                                                              reference_pose,
                                                              current_pose,
                                                              root_marker_disp,
                                                              tip_marker_disp);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = flex_body2d_solve_snapshot(flex_body,
                                     root_marker_disp,
                                     tip_marker_disp,
                                     NULL);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = coupled_step_common2d_record_flex_snapshot_result(run,
                                                            flex_slot,
                                                            flex_body,
                                                            feedback_to_mbd,
                                                            total_body_force,
                                                            history);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    coupled_step_common2d_log_flex_body_step(line_prefix,
                                             detail_prefix,
                                             flex_slot,
                                             case_body->body_id,
                                             reference_pose,
                                             current_pose,
                                             root_marker_disp,
                                             tip_marker_disp,
                                             case_body->num_root_nodes,
                                             case_body->num_tip_nodes,
                                             flex_body,
                                             total_body_force,
                                             current_pose_label,
                                             force_label,
                                             feedback_to_mbd);

    if (total_body_force_out) {
        memcpy(total_body_force_out, total_body_force, sizeof(total_body_force));
    }

cleanup:
    if (flex_body) {
        flex_body2d_free(flex_body);
        free(flex_body);
    }
    return err;
}

fem_error_t coupled_step_common2d_compute_interface_marker_disp(
    const flex_body2d_t *flex_body,
    const double reference_pose[3],
    const double current_pose[3],
    double root_marker_disp[3],
    double tip_marker_disp[3])
{
    CHECK_NULL(flex_body, "coupled flex body");
    CHECK_NULL(reference_pose, "coupled reference pose");
    CHECK_NULL(current_pose, "coupled current pose");
    CHECK_NULL(root_marker_disp, "coupled root marker displacement");
    CHECK_NULL(tip_marker_disp, "coupled tip marker displacement");

    CHECK_ERROR(flex_body2d_compute_root_marker_disp(flex_body,
                                                     reference_pose,
                                                     current_pose,
                                                     root_marker_disp));
    CHECK_ERROR(flex_body2d_compute_tip_marker_disp(flex_body,
                                                    reference_pose,
                                                    current_pose,
                                                    tip_marker_disp));
    return FEM_SUCCESS;
}

void coupled_step_common2d_log_flex_body_step(
    const char *line_prefix,
    const char *detail_prefix,
    int flex_slot,
    int body_id,
    const double reference_pose[3],
    const double current_pose[3],
    const double root_marker_disp[3],
    const double tip_marker_disp[3],
    int num_root_nodes,
    int num_tip_nodes,
    const flex_body2d_t *flex_body,
    const double total_body_force[3],
    const char *current_pose_label,
    const char *force_label,
    int feedback_to_mbd)
{
    const char *line = line_prefix ? line_prefix : "";
    const char *detail = detail_prefix ? detail_prefix : line;
    const char *pose_label = current_pose_label ? current_pose_label : "current";
    const char *force_name = force_label
        ? force_label
        : (feedback_to_mbd ? "mbd_force_increment" : "snapshot_force_observed");

    if (!reference_pose || !current_pose || !root_marker_disp || !tip_marker_disp
        || !flex_body || !total_body_force) {
        return;
    }

    printf("%sflex_body[%d]: body_id=%d reference=(%.6e,%.6e,%.6e) %s=(%.6e,%.6e,%.6e) root_marker=(%.6e,%.6e,%.6e) tip_marker=(%.6e,%.6e,%.6e) root_nodes=%d tip_nodes=%d\n",
           line,
           flex_slot,
           body_id,
           reference_pose[0],
           reference_pose[1],
           reference_pose[2],
           pose_label,
           current_pose[0],
           current_pose[1],
           current_pose[2],
           root_marker_disp[0],
           root_marker_disp[1],
           root_marker_disp[2],
           tip_marker_disp[0],
           tip_marker_disp[1],
           tip_marker_disp[2],
           num_root_nodes,
           num_tip_nodes);
    printf("%sreaction_root=(%.6e,%.6e,%.6e) reaction_tip=(%.6e,%.6e,%.6e)\n",
           detail,
           flex_body->reaction_root[0],
           flex_body->reaction_root[1],
           flex_body->reaction_root[2],
           flex_body->reaction_tip[0],
           flex_body->reaction_tip[1],
           flex_body->reaction_tip[2]);
    printf("%s%s=(%.6e,%.6e,%.6e) feedback_to_mbd=%d\n",
           detail,
           force_name,
           total_body_force[0],
           total_body_force[1],
           total_body_force[2],
           feedback_to_mbd ? 1 : 0);
}

fem_error_t coupled_step_common2d_resolve_mbd_integrator(
    const coupled_run2d_t *run,
    mbd_integrator2d_t *integrator_out)
{
    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(integrator_out, "mbd integrator output");

    if (run->time.integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        *integrator_out = MBD_INTEGRATOR2D_EXPLICIT;
        return FEM_SUCCESS;
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_NEWMARK_BETA) {
        *integrator_out = MBD_INTEGRATOR2D_NEWMARK_BETA;
        return FEM_SUCCESS;
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        *integrator_out = MBD_INTEGRATOR2D_HHT_ALPHA;
        return FEM_SUCCESS;
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Unsupported coupled integrator %d for MBD bridge",
                     (int)run->time.integrator);
}

fem_error_t coupled_step_common2d_do_mbd_step(coupled_run2d_t *run)
{
    CHECK_NULL(run, "coupled_run2d");

    if (run->time.integrator == COUPLED_INTEGRATOR_EXPLICIT) {
        return mbd_system2d_do_explicit_step(&run->mbd_system);
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        return mbd_system2d_do_hht_step(&run->mbd_system);
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_NEWMARK_BETA) {
        return mbd_system2d_do_newmark_step(&run->mbd_system);
    }

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Coupled step requires explicit, newmark_beta, or hht_alpha integrator");
}

void coupled_step_common2d_sync_mbd_time(coupled_run2d_t *run,
                                         mbd_integrator2d_t integrator)
{
    if (!run) {
        return;
    }

    run->mbd_system.time.dt = run->time.dt;
    run->mbd_system.time.num_steps = run->time.num_steps;
    run->mbd_system.time.steps_requested = run->time.num_steps;
    run->mbd_system.time.integrator = integrator;
    run->mbd_system.time.newmark_beta = run->time.newmark_beta;
    run->mbd_system.time.newmark_gamma = run->time.newmark_gamma;
    run->mbd_system.time.hht_alpha = run->time.hht_alpha;
}

fem_error_t coupled_step_common2d_capture_current_pose_for_slot(
    const coupled_run2d_t *run,
    int flex_slot,
    double current_pose[3])
{
    const mbd_body2d_t *body = NULL;

    CHECK_NULL(current_pose, "coupled current pose");
    CHECK_ERROR(coupled_step_common2d_get_body_const_for_slot(run,
                                                              flex_slot,
                                                              NULL,
                                                              &body));
    return coupled_step_common2d_capture_current_pose(body, current_pose);
}

fem_error_t coupled_step_common2d_capture_reference_pose(
    const mbd_body2d_t *body,
    double reference_pose[3])
{
    double origin[2];
    double theta = 0.0;

    CHECK_NULL(body, "MBD body");
    CHECK_NULL(reference_pose, "coupled reference pose");
    CHECK_ERROR(mbd_body2d_get_reference_frame(body, origin, &theta));

    reference_pose[0] = origin[0];
    reference_pose[1] = origin[1];
    reference_pose[2] = theta;
    return FEM_SUCCESS;
}

fem_error_t coupled_step_common2d_capture_current_pose(
    const mbd_body2d_t *body,
    double current_pose[3])
{
    double origin[2];
    double theta = 0.0;

    CHECK_NULL(body, "MBD body");
    CHECK_NULL(current_pose, "coupled current pose");
    CHECK_ERROR(mbd_body2d_get_current_pose(body, origin, &theta));

    current_pose[0] = origin[0];
    current_pose[1] = origin[1];
    current_pose[2] = theta;
    return FEM_SUCCESS;
}
