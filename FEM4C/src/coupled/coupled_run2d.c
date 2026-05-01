#include "coupled_run2d.h"

#include "../common/error.h"
#include "../common/globals.h"
#include "../io/input.h"
#include "coupled_step_explicit2d.h"
#include "coupled_step_implicit2d.h"
#include "coupled_step_delayed_cosim2d.h"
#include "coupled_step_monolithic2d.h"
#include "coupled_step_oneway2d.h"
#include "flex_body2d.h"
#include "flex_snapshot2d.h"
#include "flex_solver2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void coupled_time_control_set_defaults(coupled_time_control_t *time);
coupled_scheme_t coupled_scheme_legacy_default_from_integrator(
    coupled_integrator_t integrator);
fem_error_t coupled_time_control_validate_contract(
    const coupled_time_control_t *time);
static fem_error_t coupled_run2d_load_master_input(coupled_run2d_t *run,
                                                   const char *input_filename);
static fem_error_t coupled_run2d_validate_flex_case(const coupled_run2d_t *run);
static fem_error_t coupled_run2d_load_flex_models(coupled_run2d_t *run);
static fem_error_t coupled_run2d_load_single_flex_model(fem_model_t *model,
                                                        const char *input_filename);
static fem_error_t coupled_run2d_write_step_snapshots(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history,
    const char *output_filename);
static fem_error_t coupled_run2d_capture_interface_centers(
    const coupled_case2d_flex_body_t *case_body,
    const fem_model_t *model,
    const double marker_pose[3],
    double root_center_local[2],
    double tip_center_local[2],
    double root_center_world[2],
    double tip_center_world[2]);
static fem_error_t coupled_run2d_write_output(const coupled_run2d_t *run,
                                              const coupled_step_history2d_t *history,
                                              int history_count,
                                              const char *output_filename);
static fem_error_t coupled_legacy_no_flex_fallback_error(
    const coupled_run2d_t *run);
static fem_error_t coupled_run2d_capture_step_flex_counters(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history);
static fem_error_t coupled_run2d_capture_marker_pose(const mbd_body2d_t *body,
                                                     double marker_pose[3]);
const char *coupled_run2d_summary_title_from_scheme(
    coupled_scheme_t scheme);
const char *coupled_run2d_step_runner_name_from_scheme(
    coupled_scheme_t scheme);
int coupled_run2d_feedback_to_mbd_from_scheme(coupled_scheme_t scheme);
const char *coupled_run2d_path_class_from_scheme(coupled_scheme_t scheme);
const char *coupled_run2d_role_from_scheme(coupled_scheme_t scheme);
const char *coupled_run2d_comparison_role_from_scheme(
    coupled_scheme_t scheme);
int coupled_run2d_body_count_from_run(const coupled_run2d_t *run);
int coupled_run2d_interface_count_from_run(const coupled_run2d_t *run);
const char *coupled_run2d_solver_route_class_from_run(
    const coupled_run2d_t *run);
const char *coupled_run2d_delay_buffer_scope_from_run(
    const coupled_run2d_t *run);
const char *coupled_run2d_coupling_metric_from_scheme(
    coupled_scheme_t scheme);
const char *coupled_run2d_delay_semantics_status_from_scheme(
    coupled_scheme_t scheme);
int coupled_run2d_scheme_uses_strong_metrics(coupled_scheme_t scheme);
int coupled_run2d_scheme_uses_year1_compare_schema(
    coupled_scheme_t scheme);
int coupled_run2d_compare_step_iteration_count(
    const coupled_step_history2d_t *history);
int coupled_run2d_compare_step_coupling_converged(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history);
int coupled_run2d_compare_step_exchange_lag_steps(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history);
int coupled_run2d_compare_step_sample_hold_active(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history);
int coupled_run2d_compare_step_delayed_snapshot_step(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history);
const char *coupled_run2d_step_flex_iteration_column_name(
    coupled_scheme_t scheme);
const char *coupled_run2d_step_coupling_reason(
    const coupled_step_history2d_t *history);
static fem_error_t coupled_run2d_dispatch_step_by_scheme(
    coupled_run2d_t *run,
    int step_index,
    coupled_step_history2d_t *history);

void coupled_run2d_zero(coupled_run2d_t *run)
{
    if (!run) {
        return;
    }

    memset(run, 0, sizeof(*run));
    coupled_case2d_zero(&run->case_data);
    mbd_system2d_zero(&run->mbd_system);
    coupled_time_control_set_defaults(&run->time);
}

void coupled_run2d_free_dynamic_buffers(coupled_run2d_t *run)
{
    int i;

    if (!run) {
        return;
    }

    for (i = 0; i < run->flex_model_capacity; ++i) {
        fem_model_free(&run->flex_models[i]);
    }
    free(run->flex_models);
    run->flex_models = NULL;
    run->flex_model_capacity = 0;
}

fem_error_t coupled_run2d_reserve_flex_model_storage(coupled_run2d_t *run,
                                                     int required_capacity)
{
    fem_model_t *new_models = NULL;

    CHECK_NULL(run, "coupled_run2d");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled flex model capacity %d must be positive",
                         required_capacity);
    }
    if (run->flex_model_capacity >= required_capacity &&
        run->flex_models) {
        return FEM_SUCCESS;
    }

    new_models = calloc((size_t)required_capacity, sizeof(*new_models));
    CHECK_NULL(new_models, "coupled flex model storage");
    if (run->flex_model_count > 0 && run->flex_models) {
        memcpy(new_models,
               run->flex_models,
               (size_t)run->flex_model_count * sizeof(*new_models));
    }

    free(run->flex_models);
    run->flex_models = new_models;
    run->flex_model_capacity = required_capacity;
    return FEM_SUCCESS;
}

void coupled_run2d_free(coupled_run2d_t *run)
{
    if (!run) {
        return;
    }

    coupled_run2d_free_dynamic_buffers(run);
    coupled_case2d_free(&run->case_data);
    coupled_run2d_zero(run);
}

static fem_error_t coupled_run2d_load_master_input(coupled_run2d_t *run,
                                                   const char *input_filename)
{
    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(input_filename, "coupled input filename");

    CHECK_ERROR(input_read_data(input_filename));

    run->master_fem.analysis = &g_analysis;
    run->master_fem.num_nodes = g_num_nodes;
    run->master_fem.num_elements = g_num_elements;
    run->master_fem.num_materials = g_num_materials;

    CHECK_ERROR(mbd_system2d_load(&run->mbd_system, input_filename));
    CHECK_ERROR(coupled_case2d_clone(&run->case_data, coupled_case2d_view()));
    if (run->case_data.num_flex_bodies > 0) {
        CHECK_ERROR(coupled_run2d_reserve_flex_model_storage(
            run,
            run->case_data.num_flex_bodies));
    }
    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_validate_flex_case(const coupled_run2d_t *run)
{
    int i;

    CHECK_NULL(run, "coupled_run2d");

    if (run->case_data.num_flex_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled run requires at least one COUPLED_FLEX_BODY");
    }

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];
        int body_index = -1;

        if (body->fem_input_path[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires fem input path for body_id %d",
                             body->body_id);
        }
        if (mbd_system2d_find_body_index_by_id(&run->mbd_system,
                                               body->body_id,
                                               &body_index) != FEM_SUCCESS) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "COUPLED_FLEX_BODY body_id %d is not present in MBD system",
                             body->body_id);
        }
        if (body->num_root_nodes <= 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires COUPLED_FLEX_ROOT_SET for body_id %d",
                             body->body_id);
        }
        if (body->num_tip_nodes <= 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires COUPLED_FLEX_TIP_SET for body_id %d",
                             body->body_id);
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_load_single_flex_model(fem_model_t *model,
                                                        const char *input_filename)
{
    CHECK_NULL(model, "flex model");
    CHECK_NULL(input_filename, "flex input filename");

    CHECK_ERROR(input_read_data(input_filename));
    CHECK_ERROR(fem_model_clone_from_globals(model));
    CHECK_ERROR(flex_solver2d_prepare_model(model));
    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_load_flex_models(coupled_run2d_t *run)
{
    int i;
    int loaded = 0;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_ERROR(coupled_run2d_reserve_flex_model_storage(run,
                                                         run->case_data.num_flex_bodies));

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];

        fem_model_free(&run->flex_models[i]);
        fem_model_zero(&run->flex_models[i]);
        CHECK_ERROR(coupled_run2d_load_single_flex_model(&run->flex_models[i],
                                                         body->fem_input_path));
        ++loaded;
    }

    run->flex_model_count = loaded;
    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_write_output(const coupled_run2d_t *run,
                                              const coupled_step_history2d_t *history,
                                              int history_count,
                                              const char *output_filename)
{
    FILE *out = NULL;
    int feedback_to_mbd = 0;
    int uses_strong_metrics = 0;
    int i;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled history");
    CHECK_NULL(output_filename, "coupled output filename");

    feedback_to_mbd = coupled_run2d_feedback_to_mbd_from_scheme(run->time.scheme);
    uses_strong_metrics = coupled_run2d_scheme_uses_strong_metrics(run->time.scheme);

    out = fopen(output_filename, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot open coupled output file: %s",
                         output_filename);
    }

    fprintf(out, "# FEM4C coupled output\n");
    fprintf(out, "integrator,%s\n", coupled_integrator_to_string(run->time.integrator));
    fprintf(out, "mbd_integrator,%s\n",
            coupled_integrator_to_string(run->time.integrator));
    fprintf(out, "coupling_scheme,%s\n", coupled_scheme_to_string(run->time.scheme));
    fprintf(out, "coupling_path_class,%s\n",
            coupled_run2d_path_class_from_scheme(run->time.scheme));
    fprintf(out, "coupling_scheme_source,%s\n",
            run->time.scheme_is_legacy_default ? "legacy_default" : "explicit");
    fprintf(out, "coupling_role,%s\n",
            coupled_run2d_role_from_scheme(run->time.scheme));
    fprintf(out, "comparison_role,%s\n",
            coupled_run2d_comparison_role_from_scheme(run->time.scheme));
    fprintf(out, "feedback_to_mbd,%d\n", feedback_to_mbd);
    fprintf(out, "solver_route_class,%s\n",
            coupled_run2d_solver_route_class_from_run(run));
    fprintf(out, "delay_semantics_status,%s\n",
            coupled_run2d_delay_semantics_status_from_scheme(run->time.scheme));
    fprintf(out, "v2_decision_state,undecided\n");
    fprintf(out, "artifact_route_class,product_adjacent\n");
    fprintf(out, "artifact_family,flex_reference_v1\n");
    fprintf(out, "artifact_preferred_compare_source,summary_csv\n");
    fprintf(out, "artifact_aux_exports,interface_centers_csv|reaction_map_csv|observation_points_csv\n");
    fprintf(out, "steps_requested,%d\n", run->time.num_steps);
    fprintf(out, "steps_executed,%d\n", history_count);
    fprintf(out, "flex_body_count,%d\n", run->flex_model_count);
    if (coupled_run2d_scheme_uses_year1_compare_schema(run->time.scheme)) {
        fprintf(out, "body_count,%d\n", coupled_run2d_body_count_from_run(run));
        fprintf(out, "interface_count,%d\n",
                coupled_run2d_interface_count_from_run(run));
        if (run->time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
            fprintf(out, "iteration_metric_name,monolithic_iteration\n");
        }
    }
    fprintf(out, "snapshot_policy,accepted_steps_only\n");
    if (run->time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        fprintf(out, "exchange_lag_steps,1\n");
        fprintf(out, "sample_hold_contract,accepted_previous_step_force_hold\n");
        fprintf(out, "snapshot_provenance,accepted_pose_end_of_step\n");
        fprintf(out, "delay_buffer_scope,%s\n",
                coupled_run2d_delay_buffer_scope_from_run(run));
    }
    if (uses_strong_metrics) {
        fprintf(out, "max_coupling_iterations,%d\n", run->time.max_coupling_iterations);
        fprintf(out, "residual_tolerance,%.16e\n", run->time.residual_tolerance);
        fprintf(out, "marker_relaxation,%.16e\n", run->time.marker_relaxation);
        fprintf(out, "coupling_metric,%s\n",
                coupled_run2d_coupling_metric_from_scheme(run->time.scheme));
        fprintf(out, "step_columns,step_index,time,constraint_residual_l2,coupling_residual_l2,flex_solves,fixed_point_iterations,coupling_converged\n");
        if (run->time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
            fprintf(out, "physical_status_columns,step_index,physical_residual_l2,constraint_residual_l2\n");
            fprintf(out, "step_status_columns,step_index,coupling_converged,coupling_iterations,coupling_reason\n");
        }
    } else if (run->time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        fprintf(out, "step_columns,step_index,time,constraint_residual_l2,flex_solves,exchange_lag_steps,sample_hold_active,delayed_snapshot_step\n");
    } else {
        fprintf(out, "step_columns,step_index,time,constraint_residual_l2,flex_solves\n");
    }
    if (coupled_run2d_scheme_uses_year1_compare_schema(run->time.scheme)) {
        fprintf(out,
                "compare_schema_version,%s\n",
                COUPLED_RUN2D_COMPARE_SCHEMA_YEAR1_2LINK_V1);
        fprintf(out,
                "compare_step_columns,%s\n",
                COUPLED_RUN2D_COMPARE_STEP_COLUMNS_YEAR1_2LINK_V1_CSV);
    }
    fprintf(out, "flex_body_counter_columns,body_id,full_reassembly_count,static_solve_count\n");
    fprintf(out, "step_flex_counter_columns,step_index,%s,body_id,full_reassembly_count,static_solve_count\n",
            coupled_run2d_step_flex_iteration_column_name(run->time.scheme));
    fprintf(out, "snapshot_columns,step_index,body_id,iteration_index,path\n");
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];

        fprintf(out,
                "flex_body,%d,%s,%d,%d,%d,%d,%d\n",
                body->body_id,
                body->fem_input_path,
                body->num_root_nodes,
                body->num_tip_nodes,
                run->flex_models[i].num_nodes,
                run->flex_models[i].num_elements,
                run->flex_models[i].num_materials);
        fprintf(out,
                "flex_body_counter,%d,%d,%d\n",
                body->body_id,
                run->flex_models[i].full_reassembly_count,
                run->flex_models[i].static_solve_count);
    }
    for (i = 0; i < history_count; ++i) {
        int j;

        if (uses_strong_metrics) {
            fprintf(out,
                    "step,%d,%.16e,%.16e,%.16e,%d,%d,%d\n",
                    history[i].step_index,
                    history[i].time,
                    history[i].constraint_residual_l2,
                    history[i].coupling_residual_l2,
                    history[i].flex_solves,
                    history[i].fixed_point_iterations,
                    history[i].coupling_converged);
            if (run->time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
                fprintf(out,
                        "physical_status,%d,%.16e,%.16e\n",
                        history[i].step_index,
                        history[i].coupling_residual_l2,
                        history[i].constraint_residual_l2);
                fprintf(out,
                        "step_status,%d,%d,%d,%s\n",
                        history[i].step_index,
                        history[i].coupling_converged,
                        history[i].fixed_point_iterations,
                        coupled_run2d_step_coupling_reason(&history[i]));
            }
        } else if (run->time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
            fprintf(out,
                    "step,%d,%.16e,%.16e,%d,%d,%d,%d\n",
                    history[i].step_index,
                    history[i].time,
                    history[i].constraint_residual_l2,
                    history[i].flex_solves,
                    history[i].exchange_lag_steps,
                    history[i].sample_hold_active,
                    history[i].delayed_snapshot_step);
        } else {
            fprintf(out,
                    "step,%d,%.16e,%.16e,%d\n",
                    history[i].step_index,
                    history[i].time,
                    history[i].constraint_residual_l2,
                    history[i].flex_solves);
        }
        if (coupled_run2d_scheme_uses_year1_compare_schema(run->time.scheme)) {
            fprintf(out,
                    "compare_step,%d,%.16e,%.16e,%.16e,%d,%d,%d,%d,%d,%d\n",
                    history[i].step_index,
                    history[i].time,
                    history[i].constraint_residual_l2,
                    history[i].coupling_residual_l2,
                    history[i].flex_solves,
                    coupled_run2d_compare_step_iteration_count(&history[i]),
                    coupled_run2d_compare_step_coupling_converged(
                        run->time.scheme, &history[i]),
                    coupled_run2d_compare_step_exchange_lag_steps(
                        run->time.scheme, &history[i]),
                    coupled_run2d_compare_step_sample_hold_active(
                        run->time.scheme, &history[i]),
                    coupled_run2d_compare_step_delayed_snapshot_step(
                        run->time.scheme, &history[i]));
        }
        for (j = 0; j < history[i].flex_body_count; ++j) {
            fprintf(out,
                    "step_flex_counter,%d,%d,%d,%d,%d\n",
                    history[i].step_index,
                    history[i].fixed_point_iterations > 0
                        ? history[i].fixed_point_iterations
                        : 1,
                    history[i].flex_body_ids[j],
                    history[i].flex_body_full_reassembly_count[j],
                    history[i].flex_body_static_solve_count[j]);
        }
        for (j = 0; j < history[i].snapshot_record_count; ++j) {
            fprintf(out,
                    "snapshot_record,%d,%d,%d,%s\n",
                    history[i].step_index,
                    history[i].snapshot_body_ids[j],
                    history[i].snapshot_iteration_indices[j],
                    history[i].snapshot_paths[j]);
        }
    }

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Cannot close coupled output file: %s",
                         output_filename);
    }

    return FEM_SUCCESS;
}

static fem_error_t coupled_run2d_capture_step_flex_counters(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history)
{
    int i;

    if (!run || !history) {
        return FEM_SUCCESS;
    }

    CHECK_ERROR(coupled_step_history2d_reserve_flex_body_storage(
        history,
        run->case_data.num_flex_bodies));
    history->flex_body_count = 0;
    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];
        const int slot = history->flex_body_count;

        history->flex_body_ids[slot] = body->body_id;
        history->flex_body_full_reassembly_count[slot] =
            run->flex_models[i].full_reassembly_count;
        history->flex_body_static_solve_count[slot] =
            run->flex_models[i].static_solve_count;
        history->flex_body_count += 1;
    }
    return FEM_SUCCESS;
}

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

static fem_error_t coupled_run2d_dispatch_step_by_scheme(
    coupled_run2d_t *run,
    int step_index,
    coupled_step_history2d_t *history)
{
    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(history, "coupled step history");

    switch (run->time.scheme) {
    case COUPLED_SCHEME_ONEWAY_SNAPSHOT:
        return coupled_step_oneway2d_run(run, step_index, history);
    case COUPLED_SCHEME_STAGGERED_EXPLICIT:
        return coupled_step_explicit2d_run(run, step_index, history);
    case COUPLED_SCHEME_FIXED_POINT_STRONG:
        return coupled_step_implicit2d_run(run, step_index, history);
    case COUPLED_SCHEME_MONOLITHIC_STRONG_V1:
        return coupled_step_monolithic2d_run(run, step_index, history);
    case COUPLED_SCHEME_DELAYED_COSIM_V1_5:
        return coupled_step_delayed_cosim2d_run(run, step_index, history);
    default:
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Unsupported coupled scheme dispatch: %d",
                         (int)run->time.scheme);
    }
}

static fem_error_t coupled_run2d_write_step_snapshots(
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

static fem_error_t coupled_legacy_no_flex_fallback_error(
    const coupled_run2d_t *run)
{
    const coupled_integrator_t integrator =
        run ? run->time.integrator : COUPLED_INTEGRATOR_NEWMARK_BETA;

    CHECK_NULL(run, "coupled_run2d");

    printf("Coupled mode contract snapshot (stub):\n");
    printf("  fem: nodes=%d elements=%d materials=%d analysis_ptr=%p\n",
           run->master_fem.num_nodes,
           run->master_fem.num_elements,
           run->master_fem.num_materials,
           (const void *)run->master_fem.analysis);
    printf("  mbd: bodies=%d constraints=%d bodies_ptr=%p constraints_ptr=%p\n",
           run->mbd_system.num_bodies,
           run->mbd_system.num_constraints,
           (const void *)run->mbd_system.bodies,
           (const void *)run->mbd_system.constraints);
    printf("  time: dt=%.6e steps=%d max_iter=%d residual_tol=%.6e\n",
           run->time.dt,
           run->time.num_steps,
           run->time.max_coupling_iterations,
           run->time.residual_tolerance);
    printf("  legacy_stub_role=non_default_no_flex_fallback\n");
    printf("  default_path_requires_flex_bodies=1\n");
    printf("  integrator=%s\n", coupled_integrator_to_string(integrator));
    printf("  coupling_scheme=%s\n", coupled_scheme_to_string(run->time.scheme));
    if (run->time.scheme_is_legacy_default) {
        printf("  coupling_scheme_source=legacy_default via integrator=%s\n",
               coupled_integrator_to_string(integrator));
    } else {
        printf("  coupling_scheme_source=explicit_request\n");
    }
    printf("  integrator_params: newmark_beta=%.6e newmark_gamma=%.6e hht_alpha=%.6e marker_relaxation=%.6e\n",
           run->time.newmark_beta,
           run->time.newmark_gamma,
           run->time.hht_alpha,
           run->time.marker_relaxation);

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Coupled FEM+MBD mode is not wired yet; legacy no-flex fallback is kept only for non-default stub checks");
}

fem_error_t coupled_run2d(const char *input_filename,
                          const char *output_filename)
{
    coupled_run2d_t run;
    coupled_step_history2d_t *history = NULL;
    fem_error_t err = FEM_SUCCESS;
    int globals_ready = 0;
    int step = 0;

    CHECK_NULL(input_filename, "coupled input filename");
    CHECK_NULL(output_filename, "coupled output filename");

    coupled_run2d_zero(&run);

    err = coupled_time_control_from_env(&run.time);
    if (err != FEM_SUCCESS) {
        return err;
    }

    err = globals_initialize();
    if (err != FEM_SUCCESS) {
        coupled_run2d_free(&run);
        return err;
    }
    globals_ready = 1;

    err = coupled_run2d_load_master_input(&run, input_filename);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    if (run.time.scheme == COUPLED_SCHEME_FIXED_POINT_STRONG &&
        run.case_data.num_flex_bodies <= 0) {
        err = coupled_legacy_no_flex_fallback_error(&run);
        goto cleanup;
    }

    err = coupled_run2d_validate_flex_case(&run);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    err = coupled_run2d_load_flex_models(&run);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    history = calloc((size_t)run.time.num_steps, sizeof(*history));
    CHECK_NULL(history, "coupled explicit history");

    printf("%s\n", coupled_run2d_summary_title_from_scheme(run.time.scheme));
    printf("  scheme=%s\n", coupled_scheme_to_string(run.time.scheme));
    printf("  path_class=%s\n",
           coupled_run2d_path_class_from_scheme(run.time.scheme));
    printf("  step_dispatch_basis=coupling_scheme\n");
    printf("  step_runner=%s\n",
           coupled_run2d_step_runner_name_from_scheme(run.time.scheme));
    if (run.time.scheme_is_legacy_default) {
        printf("  scheme_source=legacy_default via integrator=%s\n",
               coupled_integrator_to_string(run.time.integrator));
    } else {
        printf("  scheme_source=explicit_request\n");
    }
    printf("  integrator=%s\n",
           coupled_integrator_to_string(run.time.integrator));
    printf("  coupling_role=%s\n",
           coupled_run2d_role_from_scheme(run.time.scheme));
    printf("  comparison_role=%s\n",
           coupled_run2d_comparison_role_from_scheme(run.time.scheme));
    printf("  solver_route_class=%s\n",
           coupled_run2d_solver_route_class_from_run(&run));
    printf("  delay_semantics_status=%s\n",
           coupled_run2d_delay_semantics_status_from_scheme(run.time.scheme));
    printf("  v2_decision_state=undecided\n");
    if (run.time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        printf("  year1_experimental_only=1\n");
        printf("  fixed_point_strong!=monolithic_strong_v1\n");
    }
    if (run.time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        printf("  year1_experimental_only=1\n");
        printf("  monolithic_strong_v1!=delayed_cosim_v1_5\n");
    }
    if (run.time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        printf("  hht_alpha=%.6e\n", run.time.hht_alpha);
    }
    printf("  feedback_to_mbd=%d\n",
           coupled_run2d_feedback_to_mbd_from_scheme(run.time.scheme));
    printf("  flex_bodies=%d\n", run.flex_model_count);
    if (run.time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1 ||
        run.time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        printf("  body_count=%d\n", coupled_run2d_body_count_from_run(&run));
        printf("  interface_count=%d\n",
               coupled_run2d_interface_count_from_run(&run));
    }
    printf("  time: dt=%.6e steps=%d max_iter=%d residual_tol=%.6e marker_relaxation=%.6e\n",
           run.time.dt,
           run.time.num_steps,
           run.time.max_coupling_iterations,
           run.time.residual_tolerance,
           run.time.marker_relaxation);

    for (step = 0; step < run.time.num_steps; ++step) {
        err = coupled_run2d_dispatch_step_by_scheme(&run,
                                                    step + 1,
                                                    &history[step]);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }
        err = coupled_run2d_capture_step_flex_counters(&run, &history[step]);
        if (err != FEM_SUCCESS) {
            goto cleanup;
        }

        if (coupled_run2d_scheme_uses_strong_metrics(run.time.scheme) &&
            !history[step].coupling_converged) {
            printf("  warning: coupled step %d reached max_iter=%d without convergence (coupling_residual_l2=%.6e)\n",
                   step + 1,
                   run.time.max_coupling_iterations,
                   history[step].coupling_residual_l2);
            printf("  snapshot_skip: step=%d reason=not_accepted_due_to_nonconvergence\n",
                   step + 1);
        } else {
            err = coupled_run2d_write_step_snapshots(&run,
                                                     &history[step],
                                                     output_filename);
            if (err != FEM_SUCCESS) {
                goto cleanup;
            }
        }
    }

    err = coupled_run2d_write_output(&run,
                                     history,
                                     run.time.num_steps,
                                     output_filename);

cleanup:
    if (history) {
        for (step = 0; step < run.time.num_steps; ++step) {
            coupled_step_history2d_free_dynamic_buffers(&history[step]);
        }
    }
    free(history);
    if (globals_ready) {
        (void)globals_finalize();
    }
    coupled_run2d_free(&run);
    return err;
}
