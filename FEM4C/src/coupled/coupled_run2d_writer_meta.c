#include "coupled_run2d.h"

const char *coupled_run2d_step_flex_iteration_column_name(
    coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "snapshot_iteration_index";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "communication_iteration_index";
    }
    return "coupling_iteration_index";
}
