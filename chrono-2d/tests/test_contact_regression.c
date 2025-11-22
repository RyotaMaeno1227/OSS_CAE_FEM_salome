#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

static const ConstraintCase *find_case(const SolveResult *res, const char *name) {
    for (int i = 0; i < res->count; ++i) {
        if (strcmp(res->cases[i].name, name) == 0) {
            return &res->cases[i];
        }
    }
    return NULL;
}

int main(void) {
    SolveResult res = run_contact_regression();
    if (res.count != 2) {
        fprintf(stderr, "Expected 2 contact cases, got %d\n", res.count);
        return 1;
    }
    const ConstraintCase *stick = find_case(&res, "contact_stick");
    const ConstraintCase *slip = find_case(&res, "contact_slip");
    if (!stick || !slip) {
        fprintf(stderr, "Missing stick/slip contact cases\n");
        return 1;
    }
    if (stick->condition_bound <= 0.0 || slip->condition_bound <= 0.0) {
        fprintf(stderr, "Invalid condition numbers\n");
        return 1;
    }
    if (slip->min_pivot >= stick->min_pivot) {
        fprintf(stderr, "Slip pivot should be smaller than stick (less effective friction)\n");
        return 1;
    }
    if (slip->condition_bound <= stick->condition_bound) {
        fprintf(stderr, "Slip should be less conditioned than stick\n");
        return 1;
    }
    printf("Contact regression test passed.\n");
    return 0;
}
