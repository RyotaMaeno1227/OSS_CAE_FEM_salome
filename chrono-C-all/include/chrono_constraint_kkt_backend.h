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
    int rank;
    int success;
} ChronoKKTBackendResult_C;

typedef struct ChronoKKTBackendStats_C {
    unsigned long calls;
    unsigned long fallback_calls;
} ChronoKKTBackendStats_C;

int chrono_kkt_backend_invert_small(const double *src,
                                    int n,
                                    double pivot_epsilon,
                                    ChronoKKTBackendResult_C *result);

const ChronoKKTBackendStats_C *chrono_kkt_backend_get_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_CONSTRAINT_KKT_BACKEND_H */
