#ifndef CHRONO_CONSTRAINT_KKT_BACKEND_H
#define CHRONO_CONSTRAINT_KKT_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHRONO_COUPLED_KKT_MAX_EQ 4

typedef struct ChronoKKTBackendResult_C {
    double inverse[CHRONO_COUPLED_KKT_MAX_EQ * CHRONO_COUPLED_KKT_MAX_EQ];
    double min_pivot;
    double max_pivot;
    double pivot_history[CHRONO_COUPLED_KKT_MAX_EQ];
    int pivot_count;
    int rank;
    int success;
} ChronoKKTBackendResult_C;

typedef struct ChronoKKTBackendStats_C {
    unsigned long calls;
    unsigned long fallback_calls;
    unsigned long cache_hits;
    unsigned long cache_misses;
    unsigned long cache_checks;
    unsigned long size_histogram[CHRONO_COUPLED_KKT_MAX_EQ + 1];
} ChronoKKTBackendStats_C;

int chrono_kkt_backend_invert_small(const double *src,
                                    int n,
                                    double pivot_epsilon,
                                    ChronoKKTBackendResult_C *result);

void chrono_kkt_backend_set_debug_label(const char *label);

const ChronoKKTBackendStats_C *chrono_kkt_backend_get_stats(void);
void chrono_kkt_backend_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_CONSTRAINT_KKT_BACKEND_H */
