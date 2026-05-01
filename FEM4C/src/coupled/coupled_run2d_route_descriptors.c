#include "coupled_run2d.h"

/*
 * Route descriptor helpers own coupled route descriptor strings,
 * classification helpers, count helpers, and reason normalization. Inputs are
 * coupled scheme, run, and step-history state; outputs are strings, counts,
 * booleans, and normalized reason labels. They have no side effects. Route
 * execution, dispatch, file writing, solver work, and buffer allocation remain
 * outside this file. This extraction is for readability and maintainability,
 * not behavior change.
 */
const char *coupled_run2d_summary_title_from_scheme(
    coupled_scheme_t scheme)
{
    switch (scheme) {
    case COUPLED_SCHEME_ONEWAY_SNAPSHOT:
        return "Coupled oneway_snapshot run summary:";
    case COUPLED_SCHEME_STAGGERED_EXPLICIT:
        return "Coupled staggered_explicit run summary:";
    case COUPLED_SCHEME_FIXED_POINT_STRONG:
        return "Coupled fixed_point_strong run summary:";
    case COUPLED_SCHEME_MONOLITHIC_STRONG_V1:
        return "Coupled monolithic_strong_v1 run summary:";
    case COUPLED_SCHEME_DELAYED_COSIM_V1_5:
        return "Coupled delayed_cosim_v1_5 run summary:";
    default:
        return "Coupled unknown-scheme run summary:";
    }
}

const char *coupled_run2d_step_runner_name_from_scheme(
    coupled_scheme_t scheme)
{
    switch (scheme) {
    case COUPLED_SCHEME_ONEWAY_SNAPSHOT:
        return "coupled_step_oneway2d_run";
    case COUPLED_SCHEME_STAGGERED_EXPLICIT:
        return "coupled_step_explicit2d_run";
    case COUPLED_SCHEME_FIXED_POINT_STRONG:
        return "coupled_step_implicit2d_run";
    case COUPLED_SCHEME_MONOLITHIC_STRONG_V1:
        return "coupled_step_monolithic2d_run";
    case COUPLED_SCHEME_DELAYED_COSIM_V1_5:
        return "coupled_step_delayed_cosim2d_run";
    default:
        return "unknown";
    }
}

int coupled_run2d_feedback_to_mbd_from_scheme(coupled_scheme_t scheme)
{
    return scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT ? 0 : 1;
}

const char *coupled_run2d_path_class_from_scheme(coupled_scheme_t scheme)
{
    return scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT ? "official" : "experimental";
}

const char *coupled_run2d_role_from_scheme(coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "official one-way snapshot/replay baseline with no FEM-to-MBD feedback";
    }
    if (scheme == COUPLED_SCHEME_STAGGERED_EXPLICIT) {
        return "experimental two-way staggered path preserved outside official mainline acceptance";
    }
    if (scheme == COUPLED_SCHEME_FIXED_POINT_STRONG) {
        return "experimental strong same-step path preserved outside official mainline acceptance";
    }
    if (scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        return "year1 experimental monolithic strong comparison lane; fixed_point_strong remains a separate experimental legacy path";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "year1 experimental delayed co-simulation comparison lane with lag-1 sample-hold skeleton and topology-dependent route metadata; still distinct from monolithic_strong_v1";
    }
    return "unknown";
}

const char *coupled_run2d_comparison_role_from_scheme(
    coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "official_reference";
    }
    if (scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        return "monolithic_strong";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "co_simulation";
    }
    return "legacy_experimental";
}

int coupled_run2d_body_count_from_run(const coupled_run2d_t *run)
{
    if (!run) {
        return 0;
    }

    if (run->case_data.num_flex_bodies > 0) {
        return run->case_data.num_flex_bodies;
    }

    return run->flex_model_count;
}

int coupled_run2d_interface_count_from_run(const coupled_run2d_t *run)
{
    return coupled_run2d_body_count_from_run(run);
}

const char *coupled_run2d_solver_route_class_from_run(
    const coupled_run2d_t *run)
{
    const coupled_scheme_t scheme =
        run ? run->time.scheme : COUPLED_SCHEME_ONEWAY_SNAPSHOT;

    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "accepted_snapshot_replay";
    }
    if (scheme == COUPLED_SCHEME_STAGGERED_EXPLICIT) {
        return "partitioned_staggered_two_way";
    }
    if (scheme == COUPLED_SCHEME_FIXED_POINT_STRONG) {
        return "same_step_fixed_point";
    }
    if (scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        if (coupled_run2d_body_count_from_run(run) >= 2) {
            return "single_coupled_system_2link_body_interface_skeleton";
        }
        return "single_coupled_system_1link_newton_minimal_proof";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        if (coupled_run2d_body_count_from_run(run) >= 2) {
            return "partitioned_delayed_cosim_sample_hold_2link_body_interface_skeleton";
        }
        return "partitioned_delayed_cosim_sample_hold_1link_skeleton";
    }
    return "unknown";
}

const char *coupled_run2d_delay_buffer_scope_from_run(
    const coupled_run2d_t *run)
{
    if (coupled_run2d_body_count_from_run(run) >= 2) {
        return "2link_body_interface_skeleton";
    }
    return "1link_minimal_skeleton";
}

const char *coupled_run2d_step_coupling_reason(
    const coupled_step_history2d_t *history)
{
    if (!history || history->coupling_reason[0] == '\0') {
        return "not_reported";
    }

    return history->coupling_reason;
}

const char *coupled_run2d_coupling_metric_from_scheme(
    coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1) {
        return "monolithic_reduced_residual_l2";
    }
    if (scheme == COUPLED_SCHEME_FIXED_POINT_STRONG) {
        return "qflex_l2";
    }
    return "not_applicable";
}

const char *coupled_run2d_delay_semantics_status_from_scheme(
    coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "lag1_sample_hold_accepted_previous_step_skeleton";
    }
    return "not_applicable";
}

int coupled_run2d_scheme_uses_strong_metrics(coupled_scheme_t scheme)
{
    return scheme == COUPLED_SCHEME_FIXED_POINT_STRONG ||
           scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1;
}

int coupled_run2d_scheme_uses_year1_compare_schema(
    coupled_scheme_t scheme)
{
    return scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT ||
           scheme == COUPLED_SCHEME_MONOLITHIC_STRONG_V1 ||
           scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5;
}

int coupled_run2d_compare_step_iteration_count(
    const coupled_step_history2d_t *history)
{
    if (!history) {
        return 0;
    }

    return history->fixed_point_iterations > 0
        ? history->fixed_point_iterations
        : 1;
}

int coupled_run2d_compare_step_coupling_converged(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history)
{
    if (!history) {
        return 0;
    }

    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return 1;
    }

    return history->coupling_converged;
}

int coupled_run2d_compare_step_exchange_lag_steps(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history)
{
    if (!history) {
        return 0;
    }

    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return history->exchange_lag_steps;
    }

    return 0;
}

int coupled_run2d_compare_step_sample_hold_active(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history)
{
    if (!history) {
        return 0;
    }

    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return history->sample_hold_active;
    }

    return 0;
}

int coupled_run2d_compare_step_delayed_snapshot_step(
    coupled_scheme_t scheme,
    const coupled_step_history2d_t *history)
{
    if (!history) {
        return 0;
    }

    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return history->delayed_snapshot_step;
    }

    return 0;
}
