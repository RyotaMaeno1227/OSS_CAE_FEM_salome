#include "coupled_run2d.h"

#include "../common/error.h"
#include "coupled_step_explicit2d.h"
#include "coupled_step_implicit2d.h"
#include "coupled_step_delayed_cosim2d.h"
#include "coupled_step_monolithic2d.h"
#include "coupled_step_oneway2d.h"

/* Owns coupled scheme-to-step-runner dispatch. Inputs are the coupled run
 * state, step index/status, and history/run context passed to the selected
 * runner. Output is the selected runner result and any runner-owned side
 * effects; side effects are limited to those performed by that selected step
 * runner. Route descriptors, writers, history allocation, flex model loading,
 * and snapshot artifact writing remain outside this file. This extraction is
 * for readability and maintainability, not behavior change.
 */

fem_error_t coupled_run2d_dispatch_step_by_scheme(
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
