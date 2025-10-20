# chrono-C-all 2D Constraint Stabilization

This directory contains a minimal C implementation of the 2D rigid body and distance
constraint utilities used for experimentation within `chrono-C-all`.  The
implementation focuses on providing a stabilized distance constraint solver that
combines Baumgarte velocity biasing, warm starting, and a soft constraint term to
control compliance.

## Key APIs

- `chrono_body2d_set_circle_shape(body, radius)`: assign a circular shape for a body.  Inverse
  mass / inertia must be set manually via `chrono_body2d_set_mass`.
- `chrono_body2d_set_polygon_shape(body, vertices, count)`: register a convex polygon (local coordinates).
- `chrono_body2d_set_polygon_shape_with_density(body, vertices, count, density)`: convenience helper that
  stores the polygon shape _and_ computes mass / inertia for a uniform density polygon.  Setting `density <= 0`
  leaves the body static.
- `chrono_collision2d_detect_* / resolve_*`: collision routines covering circle–circle, circle–polygon, and
  polygon–polygon (convex) pairs.  Each detection function fills a `ChronoContact2D_C` struct that can store up
  to two contact points for use with `chrono_collision2d_resolve_contact` and the contact manager.

## Building the tests

The distance constraint regression `tests/test_distance_constraint_stabilization.c` can be built with a standard C compiler:

```bash
gcc -std=c99 -Iinclude src/chrono_body2d.c src/chrono_constraint2d.c \
    tests/test_distance_constraint_stabilization.c -lm \
    -o tests/test_distance_constraint_stabilization
./tests/test_distance_constraint_stabilization
```

The test connects a dynamic body to a static anchor and verifies that the solver
converges to the target rest length within a tolerance of 1 mm while reporting
intermediate constraint distances.

Additional regression tests are available via `make test` (see the top-level `Makefile`).  Notable examples:

- `tests/test_polygon_collision.c`: circle vs polygon and polygon vs polygon collision regression.
- `tests/test_polygon_mass_properties.c`: verifies mass / inertia output from `chrono_body2d_set_polygon_shape_with_density`.
- `tests/test_polygon_slope_friction.c`: block sliding on an inclined plane with friction.
- `tests/test_polygon_spin_collision.c`: counter-rotating convex polygons interacting through the manifold pipeline.
