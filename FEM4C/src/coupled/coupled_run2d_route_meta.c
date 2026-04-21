#include "coupled_run2d.h"

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
