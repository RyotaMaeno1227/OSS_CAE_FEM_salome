#ifndef CHRONO_CONSTRAINT_COMMON_H
#define CHRONO_CONSTRAINT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "chrono_logging.h"

typedef struct ChronoConstraintCommon_C {
    const void *ops;
    void *body_a;
    void *body_b;
    double accumulated_impulse;
    double effective_mass;
} ChronoConstraintCommon_C;

#define CHRONO_CONSTRAINT_DIAG_MAX_PIVOTS 8

typedef struct ChronoConstraintDiagnostics_C {
    unsigned int flags;
    int rank;
    double condition_number;
    double min_pivot;
    double max_pivot;
    double condition_number_spectral;
    double min_eigenvalue;
    double max_eigenvalue;
    double pivot_log[CHRONO_CONSTRAINT_DIAG_MAX_PIVOTS];
    int pivot_log_count;
    ChronoLogLevel_C log_level_request;
    ChronoLogLevel_C log_level_actual;
    ChronoLogCategory_C log_category;
} ChronoConstraintDiagnostics_C;

typedef ChronoConstraintDiagnostics_C ChronoCoupledConstraintDiagnostics_C;

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_CONSTRAINT_COMMON_H */
