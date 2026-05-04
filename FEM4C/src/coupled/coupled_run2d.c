#include "coupled_run2d.h"

#include "../common/error.h"
#include "../common/globals.h"
#include "../io/input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

coupled_scheme_t coupled_scheme_legacy_default_from_integrator(
    coupled_integrator_t integrator);
fem_error_t coupled_time_control_validate_contract(
    const coupled_time_control_t *time);
fem_error_t coupled_run2d_load_master_input(coupled_run2d_t *run,
                                            const char *input_filename);
fem_error_t coupled_run2d_validate_flex_case(const coupled_run2d_t *run);
fem_error_t coupled_run2d_load_flex_models(coupled_run2d_t *run);
fem_error_t coupled_run2d_write_step_snapshots(
    const coupled_run2d_t *run,
    coupled_step_history2d_t *history,
    const char *output_filename);
fem_error_t coupled_run2d_write_output(const coupled_run2d_t *run,
                                       const coupled_step_history2d_t *history,
                                       int history_count,
                                       const char *output_filename);
void coupled_run2d_print_startup_summary(const coupled_run2d_t *run);
fem_error_t coupled_legacy_no_flex_fallback_error(
    const coupled_run2d_t *run);
fem_error_t coupled_run2d_capture_step_flex_counters(
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
fem_error_t coupled_run2d_dispatch_step_by_scheme(
    coupled_run2d_t *run,
    int step_index,
    coupled_step_history2d_t *history);

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

    coupled_run2d_print_startup_summary(&run);

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
