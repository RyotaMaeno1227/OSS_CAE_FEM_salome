#ifndef CHRONO_CONSTRAINT_COMMON_H
#define CHRONO_CONSTRAINT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ChronoCoupledConstraintDiagnostics_C {
    unsigned int flags;
    int rank;
    double condition_number;
    double min_pivot;
    double max_pivot;
    double condition_number_spectral;
    double min_eigenvalue;
    double max_eigenvalue;
} ChronoCoupledConstraintDiagnostics_C;

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_CONSTRAINT_COMMON_H */
