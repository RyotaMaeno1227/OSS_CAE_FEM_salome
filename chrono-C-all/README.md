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
- `chrono_body2d_set_capsule_shape(body, half_length, radius)` / `chrono_body2d_set_edge_shape(body, start, end)`:
  define additional convex primitives.  Capsules participate in GJK/EPA-based collision detection; edges provide
  lightweight, zero-thickness barriers.
- `chrono_collision2d_detect_convex_gjk(body_a, body_b, contact)`: generic collision entry point covering circles,
  polygons, capsules, and edges via a 2D GJK + EPA pipeline.  Convenience wrappers such as
  `chrono_collision2d_detect_capsule_capsule` and `chrono_collision2d_detect_circle_edge` defer to this path.
- `chrono_collision2d_detect_* / resolve_*`: collision routines covering circle–circle, circle–polygon, and
  polygon–polygon (convex) pairs.  Each detection function fills a `ChronoContact2D_C` struct that can store up
  to two contact points for use with `chrono_collision2d_resolve_contact` and the contact manager.
- `chrono_prismatic_constraint2d_*`: slider joint API。`chrono_prismatic_constraint2d_set_limit_spring` でソフトリミット、
  `chrono_prismatic_constraint2d_set_motor_position_target` で位置制御モードに切替えられます。

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
- `tests/test_capsule_edge_collision.c`: capsule/capsule and edge/circle interactions via the GJK/EPA backend.
- `tests/test_island_polygon_longrun.c`: combined constraint + polygon contact scenario executed through the island solver.
- `tests/test_prismatic_constraint.c`: slider joint with stroke limits and motor drive (limit and motor regression).
- `tests/test_spring_constraint.c`: damped spring between an anchor and dynamic body.
- `tests/test_revolute_constraint.c`: pin joint maintaining a pivot under gravity.

## Examples and Visualization

Two self-contained demos can be built with `make examples`:

- `examples/newton_cradle` – four-body Newton's cradle producing `data/newton_cradle.csv`.
- `examples/prismatic_slider` – slider joint with soft limits, velocity/位置モータ切り替えを含むデモ。`data/prismatic_slider.csv` に軸位置・リミット／モータ反力が記録されます。

Run an example and point the MATLAB helpers in `docs/` at the generated CSV to obtain plots and GIF animations.  For instance:

```matlab
% From the repo root or docs/ directory:
newton_cradle_visualization('../data/newton_cradle.csv', 'cradle_frames');
prismatic_slider_visualization('../data/prismatic_slider.csv', 'prismatic_frames');
```

Both scripts emit trajectory/diagnostic plots and an animation built from PNG frames (stored under the output directory).
