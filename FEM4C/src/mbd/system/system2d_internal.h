#ifndef FEM4C_MBD_SYSTEM2D_INTERNAL_H
#define FEM4C_MBD_SYSTEM2D_INTERNAL_H

/*
 * Private constants shared by the system2d*.c implementation family.
 * This header is not part of the public MBD system API.
 */

#define MBD_SOURCE_DEFAULT "default"
#define MBD_SOURCE_ENV "env"
#define MBD_SOURCE_CLI "cli"
#define MBD_SOURCE_GROUND_AUTO "ground_auto"
#define MBD_SOURCE_ENV_INVALID_FALLBACK "env_invalid_fallback"
#define MBD_SOURCE_ENV_OUT_OF_RANGE_FALLBACK "env_out_of_range_fallback"

#define MBD_IMPLICIT_RESIDUAL_MODE_CONSTRAINT "constraint_residual_l2"
#define MBD_IMPLICIT_RESIDUAL_MODE_HHT_EFFECTIVE "hht_effective_residual_l2"
#define MBD_IMPLICIT_SCHEME_NOT_APPLICABLE "not_applicable"
#define MBD_IMPLICIT_SCHEME_NEWMARK_FREE "newmark_unconstrained_direct"
#define MBD_IMPLICIT_SCHEME_NEWMARK_KKT "newmark_constrained_single_kkt"
#define MBD_IMPLICIT_SCHEME_HHT_FREE "hht_unconstrained_direct"
#define MBD_IMPLICIT_SCHEME_HHT_MODIFIED_NEWTON "hht_modified_newton_effective"
#define MBD_IMPLICIT_REASON_NOT_RUN "not_run"
#define MBD_IMPLICIT_REASON_UNCONSTRAINED_DIRECT "unconstrained_direct"
#define MBD_IMPLICIT_REASON_RESIDUAL_TOLERANCE "residual_tolerance"
#define MBD_IMPLICIT_REASON_NO_EQUATIONS "no_equations"
#define MBD_IMPLICIT_REASON_ITERATION_CAP "iteration_cap"

#define MBD_HHT_FORCE_HISTORY_MODE_NOT_APPLICABLE "not_applicable"
#define MBD_HHT_FORCE_HISTORY_MODE_SYSTEM_OWNED_CURRENT_PREVIOUS "system_owned_current_previous"

#define MBD_PROJECTION_STOP_DISABLED "disabled"
#define MBD_PROJECTION_STOP_NO_CONSTRAINTS "no_constraints"
#define MBD_PROJECTION_STOP_NOT_APPLIED "not_applied"

#endif /* FEM4C_MBD_SYSTEM2D_INTERNAL_H */
