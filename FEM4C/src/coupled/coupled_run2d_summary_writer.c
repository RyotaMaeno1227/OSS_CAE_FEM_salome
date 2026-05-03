#include "coupled_run2d.h"

#include "../common/error.h"

#include <stdio.h>

/*
 * Owns coupled-run summary/reporting output responsibilities, including
 * startup console summary printing and summary file writing. Startup console
 * output reads the already-prepared run configuration/state. Summary file
 * writing writes existing report artifacts through configured paths. Side
 * effects are console printing and file output in the writer routines. Route
 * dispatch, solver steps, flex loading, validation, snapshots, history
 * allocation/capture, and route descriptor selection remain outside this file.
 * This extraction is for readability and maintainability, not behavior change.
 */

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

void coupled_run2d_print_startup_summary(const coupled_run2d_t *run)
{
    printf("%s\n", coupled_run2d_summary_title_from_scheme(run->time.scheme));
    printf("  scheme=%s\n", coupled_scheme_to_string(run->time.scheme));
    printf("  path_class=%s\n",
           coupled_run2d_path_class_from_scheme(run->time.scheme));
    printf("  step_dispatch_basis=coupling_scheme\n");
    printf("  step_runner=%s\n",
           coupled_run2d_step_runner_name_from_scheme(run->time.scheme));
    if (run->time.scheme_is_legacy_default) {
        printf("  scheme_source=legacy_default via integrator=%s\n",
               coupled_integrator_to_string(run->time.integrator));
    } else {
        printf("  scheme_source=explicit_request\n");
    }
    printf("  integrator=%s\n",
           coupled_integrator_to_string(run->time.integrator));
    printf("  coupling_role=%s\n",
           coupled_run2d_role_from_scheme(run->time.scheme));
    printf("  comparison_role=%s\n",
           coupled_run2d_comparison_role_from_scheme(run->time.scheme));
    printf("  solver_route_class=%s\n",
           coupled_run2d_solver_route_class_from_run(run));
    printf("  delay_semantics_status=%s\n",
           coupled_run2d_delay_semantics_status_from_scheme(run->time.scheme));
    printf("  v2_decision_state=undecided\n");
    if (run->time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        printf("  year1_experimental_only=1\n");
        printf("  fixed_point_strong!=monolithic_strong_v1\n");
    }
    if (run->time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        printf("  year1_experimental_only=1\n");
        printf("  monolithic_strong_v1!=delayed_cosim_v1_5\n");
    }
    if (run->time.integrator == COUPLED_INTEGRATOR_HHT_ALPHA) {
        printf("  hht_alpha=%.6e\n", run->time.hht_alpha);
    }
    printf("  feedback_to_mbd=%d\n",
           coupled_run2d_feedback_to_mbd_from_scheme(run->time.scheme));
    printf("  flex_bodies=%d\n", run->flex_model_count);
    if (run->time.scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1 ||
        run->time.scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        printf("  body_count=%d\n", coupled_run2d_body_count_from_run(run));
        printf("  interface_count=%d\n",
               coupled_run2d_interface_count_from_run(run));
    }
    printf("  time: dt=%.6e steps=%d max_iter=%d residual_tol=%.6e marker_relaxation=%.6e\n",
           run->time.dt,
           run->time.num_steps,
           run->time.max_coupling_iterations,
           run->time.residual_tolerance,
           run->time.marker_relaxation);
}

fem_error_t coupled_run2d_write_output(const coupled_run2d_t *run,
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
