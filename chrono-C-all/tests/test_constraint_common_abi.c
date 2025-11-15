#include <stdio.h>
#include <stddef.h>

#include "../include/chrono_constraint2d.h"
#include "../include/chrono_constraint_common.h"

int main(void) {
    printf("Constraint common ABI check: size=%zu ops_offset=%zu body_a_offset=%zu body_b_offset=%zu\n",
           sizeof(ChronoConstraint2DBase_C),
           offsetof(ChronoConstraint2DBase_C, ops),
           offsetof(ChronoConstraint2DBase_C, body_a),
           offsetof(ChronoConstraint2DBase_C, body_b));

    printf("Diagnostics ABI check: size=%zu flags_offset=%zu pivot_log_offset=%zu log_level_actual_offset=%zu\n",
           sizeof(ChronoConstraintDiagnostics_C),
           offsetof(ChronoConstraintDiagnostics_C, flags),
           offsetof(ChronoConstraintDiagnostics_C, pivot_log),
           offsetof(ChronoConstraintDiagnostics_C, log_level_actual));
    return 0;
}
