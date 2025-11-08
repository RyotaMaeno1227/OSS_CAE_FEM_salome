#include <stdio.h>

#include "../include/chrono_constraint2d.h"

int main(void) {
    printf("Constraint common ABI check: size=%zu ops_offset=%zu\n",
           sizeof(ChronoConstraint2DBase_C),
           offsetof(ChronoConstraint2DBase_C, ops));
    return 0;
}
