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
    unsigned int flags; /**< CHRONO_COUPLED_DIAG_* bitmask (see docs/logs/kkt_descriptor_poc_e2e.md). */
    int rank;           /**< Active equation rank reported by KKT backend. */
    double condition_number;           /**< Condition estimate from the bound matrix. */
    double min_pivot;                  /**< Smallest pivot observed while factorising. */
    double max_pivot;                  /**< Largest pivot observed while factorising. */
    double condition_number_spectral;  /**< Spectral condition estimate. */
    double min_eigenvalue;
    double max_eigenvalue;
    double pivot_log[CHRONO_CONSTRAINT_DIAG_MAX_PIVOTS]; /**< First pivots recorded for debugging. */
    int pivot_log_count;
    ChronoLogLevel_C log_level_request; /**< User requested log level (policy). */
    ChronoLogLevel_C log_level_actual;  /**< Level emitted after throttling. */
    ChronoLogCategory_C log_category;   /**< Associated logging category. */
} ChronoConstraintDiagnostics_C;

typedef ChronoConstraintDiagnostics_C ChronoCoupledConstraintDiagnostics_C;

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_CONSTRAINT_COMMON_H */
