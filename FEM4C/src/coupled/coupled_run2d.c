#include "coupled_run2d.h"

#include "../common/error.h"
#include "../common/globals.h"
#include "../io/input.h"
#include "coupled_step_explicit2d.h"
#include "coupled_step_implicit2d.h"
#include "coupled_step_delayed_cosim2d.h"
#include "coupled_step_monolithic2d.h"
#include "coupled_step_oneway2d.h"

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
fem_error_t coupled_run2d_load_flex_models(coupled_run2d_t *run);
fem_error_t coupled_run2d_write_step_snapshots(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history,
    const char *output_filename);
fem_error_t coupled_run2d_write_output(const coupled_run2d_t *run,
                                       const coupled_step_history2d_t *history,
                                       int history_count,
                                       const char *output_filename);
static fem_error_t coupled_legacy_no_flex_fallback_error(
    const coupled_run2d_t *run);
static fem_error_t coupled_run2d_capture_step_flex_counters(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history);
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
