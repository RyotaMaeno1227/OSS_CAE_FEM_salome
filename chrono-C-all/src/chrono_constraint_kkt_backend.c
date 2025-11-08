#include <math.h>
#include <stddef.h>
#include <string.h>

#include "../include/chrono_constraint_kkt_backend.h"
#include "../include/chrono_small_matrix.h"

static ChronoKKTBackendStats_C g_kkt_stats = {0, 0};

int chrono_kkt_backend_invert_small(const double *src,
                                    int n,
                                    double pivot_epsilon,
                                    ChronoKKTBackendResult_C *result) {
    g_kkt_stats.calls += 1;
    if (!src || !result || n <= 0 || n > CHRONO_COUPLED_KKT_MAX_EQ) {
        g_kkt_stats.fallback_calls += 1;
        return 0;
    }

    ChronoSmallMatInversionResult_C helper_result;
    if (!chrono_smallmat_invert_with_history(src, n, pivot_epsilon, &helper_result)) {
        g_kkt_stats.fallback_calls += 1;
        memset(result, 0, sizeof(*result));
        return 0;
    }

    memset(result, 0, sizeof(*result));
    for (int i = 0; i < n * CHRONO_COUPLED_KKT_MAX_EQ; ++i) {
        result->inverse[i] = helper_result.inverse[i];
    }
    result->min_pivot = helper_result.min_pivot;
    result->max_pivot = helper_result.max_pivot;
    result->rank = helper_result.rank;
    result->pivot_count = helper_result.pivot_count;
    memcpy(result->pivot_history,
           helper_result.pivot_history,
           sizeof(result->pivot_history));
    result->success = 1;
    return 1;
}

const ChronoKKTBackendStats_C *chrono_kkt_backend_get_stats(void) {
    return &g_kkt_stats;
}
