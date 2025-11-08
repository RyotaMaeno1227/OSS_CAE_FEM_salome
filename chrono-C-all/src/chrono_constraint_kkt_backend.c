#include <math.h>
#include <stddef.h>
#include <string.h>

#include "../include/chrono_constraint_kkt_backend.h"
#include "../include/chrono_small_matrix.h"

typedef struct ChronoKKTBackendCacheEntry {
    int valid;
    int n;
    double pivot_epsilon;
    double matrix[CHRONO_COUPLED_KKT_MAX_EQ * CHRONO_COUPLED_KKT_MAX_EQ];
    ChronoKKTBackendResult_C result;
    unsigned long hits;
} ChronoKKTBackendCacheEntry;

static ChronoKKTBackendStats_C g_kkt_stats = {0, 0, 0, 0, 0, {0}};
static ChronoKKTBackendCacheEntry g_kkt_cache[CHRONO_COUPLED_KKT_MAX_EQ + 1];

void chrono_kkt_backend_reset_stats(void) {
    memset(&g_kkt_stats, 0, sizeof(g_kkt_stats));
    memset(g_kkt_cache, 0, sizeof(g_kkt_cache));
}

static int chrono_kkt_backend_cache_lookup(const double *src,
                                           int n,
                                           double pivot_epsilon,
                                           ChronoKKTBackendResult_C *result) {
    if (n < 3 || n > CHRONO_COUPLED_KKT_MAX_EQ) {
        return 0;
    }
    ChronoKKTBackendCacheEntry *entry = &g_kkt_cache[n];
    if (!entry->valid || entry->n != n) {
        return 0;
    }
    double eps_tol = fmax(1.0, fabs(pivot_epsilon)) * 1e-12;
    if (fabs(entry->pivot_epsilon - pivot_epsilon) > eps_tol) {
        return 0;
    }
    size_t span = (size_t)CHRONO_COUPLED_KKT_MAX_EQ * n;
    if (memcmp(entry->matrix, src, span * sizeof(double)) != 0) {
        return 0;
    }
    if (result) {
        *result = entry->result;
    }
    entry->hits += 1;
    g_kkt_stats.cache_hits += 1;
    return 1;
}

static void chrono_kkt_backend_cache_store(const double *src,
                                           int n,
                                           double pivot_epsilon,
                                           const ChronoKKTBackendResult_C *result) {
    if (n < 3 || n > CHRONO_COUPLED_KKT_MAX_EQ || !result) {
        return;
    }
    ChronoKKTBackendCacheEntry *entry = &g_kkt_cache[n];
    entry->n = n;
    entry->pivot_epsilon = pivot_epsilon;
    size_t span = (size_t)CHRONO_COUPLED_KKT_MAX_EQ * n;
    memcpy(entry->matrix, src, span * sizeof(double));
    entry->result = *result;
    entry->valid = 1;
    entry->hits = 0;
}

int chrono_kkt_backend_invert_small(const double *src,
                                    int n,
                                    double pivot_epsilon,
                                    ChronoKKTBackendResult_C *result) {
    g_kkt_stats.calls += 1;
    if (!src || !result || n <= 0 || n > CHRONO_COUPLED_KKT_MAX_EQ) {
        g_kkt_stats.fallback_calls += 1;
        return 0;
    }

    g_kkt_stats.size_histogram[n] += 1;

    if (n >= 3) {
        g_kkt_stats.cache_checks += 1;
        if (chrono_kkt_backend_cache_lookup(src, n, pivot_epsilon, result)) {
            return 1;
        }
        g_kkt_stats.cache_misses += 1;
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

    chrono_kkt_backend_cache_store(src, n, pivot_epsilon, result);
    return 1;
}

const ChronoKKTBackendStats_C *chrono_kkt_backend_get_stats(void) {
    return &g_kkt_stats;
}
