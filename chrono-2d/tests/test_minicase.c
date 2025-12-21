#include <math.h>
#include <stdio.h>

#include "../include/solver.h"

static int almost_equal(double a, double b, double tol) {
    return fabs(a - b) <= tol;
}

int main(void) {
    Body2D a, b;
    body_init(&a, 1.0, 1.0, 0.0, 0.0);
    body_init(&b, 1.0, 1.0, 0.0, 0.0);

    Constraint2D gear = {0};
    gear.name = "minicase_gear";
    gear.type = CONSTRAINT_GEAR;
    gear.a = &a;
    gear.b = &b;

    ConstraintStats stats = compute_stats(&gear);
    if (!almost_equal(stats.min_pivot, 0.5, 1e-9) ||
        !almost_equal(stats.max_pivot, 0.5, 1e-9)) {
        fprintf(stderr, "Gear pivot mismatch: min=%.9f max=%.9f\n",
                stats.min_pivot, stats.max_pivot);
        return 1;
    }
    if (!almost_equal(stats.condition_bound, 1.0, 1e-9)) {
        fprintf(stderr, "Gear condition mismatch: %.9f\n", stats.condition_bound);
        return 1;
    }
    printf("Mini-case test passed.\n");
    return 0;
}
