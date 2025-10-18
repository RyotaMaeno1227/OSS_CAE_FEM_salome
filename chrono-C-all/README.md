# chrono-C-all 2D Constraint Stabilization

This directory contains a minimal C implementation of the 2D rigid body and distance
constraint utilities used for experimentation within `chrono-C-all`.  The
implementation focuses on providing a stabilized distance constraint solver that
combines Baumgarte velocity biasing, warm starting, and a soft constraint term to
control compliance.

The accompanying test `tests/test_distance_constraint_stabilization.c` can be built
with a standard C compiler:

```bash
gcc -std=c99 -Iinclude src/chrono_body2d.c src/chrono_constraint2d.c \
    tests/test_distance_constraint_stabilization.c -lm \
    -o tests/test_distance_constraint_stabilization
./tests/test_distance_constraint_stabilization
```

The test connects a dynamic body to a static anchor and verifies that the solver
converges to the target rest length within a tolerance of 1 mm while reporting
intermediate constraint distances.
