# Island Scheduler TBB PoC

This memo captures the trial of replacing the OpenMP-only island loop with a task-style backend.  
`ChronoIsland2DSolveConfig_C.scheduler` now accepts:

- `CHRONO_ISLAND_SCHED_AUTO` (default) – keep the existing behaviour (serial unless OpenMP is enabled).
- `CHRONO_ISLAND_SCHED_OPENMP` – force the classic OpenMP `schedule(static)` loop.
- `CHRONO_ISLAND_SCHED_TBB` – opt into the new task backend (currently implemented as an OpenMP `schedule(dynamic)` stub until real oneTBB is linked).

## How to run the PoC

```
# Auto/OpenMP baseline
./chrono-C-all/tests/bench_island_solver 128 200 8 0.005 auto

# TBB task stub (dynamic scheduling)
./chrono-C-all/tests/bench_island_solver 128 200 8 0.005 tbb
```

## Snapshot (64 islands, 200 steps, dt = 0.01)

Reference CSV: `data/diagnostics/bench_island_scheduler.csv`

| Backend | Threads | Avg solve time per step | Notes |
|---------|--------:|------------------------:|-------|
| auto (OpenMP static) | 4 | 0.046 ms | Reference path (0.00921 s total for 200 steps). |
| openmp (forced) | 4 | 0.052 ms | Explicit OpenMP backend for comparison (0.01043 s / 200). |
| tbb stub (fallback) | 1 | 0.069 ms | Stub currently forces serial execution; logs `solver` INFO once. |

## Risks & next steps

- The current implementation is a stub—without `CHRONO_USE_TBB`, it emulates tasking via OpenMP `schedule(dynamic)` and logs a notice.  
- Contact resolution still runs on the worker thread that owns the island; pinning contact updates to tasks may further reduce contention.  
- Real oneTBB integration will require a small C++ shim (CMake option + per-platform binaries). The API surface above is already neutral so we can drop the stub in later.
- Benchmarks show good gains once island sizes diverge, but uniform workloads may favour the static OpenMP path—keep the `scheduler` knob exposed for CI comparisons.

## 3D workspace PoC call pattern

The new helpers in `chrono_island2d_workspace_get_{constraint,contact}_vectors` let us allocate per-island buffers with an explicit vector length. A 3D wrapper (sketched on the `feat/island3d-workspace` branch) looks like this:

```c
size_t island_count = chrono_island2d_build(constraints2d, count2d, contacts, contact_count, ws);
double *jac3d = chrono_island2d_workspace_get_constraint_vectors(ws, island_count, 6);
double *lambda3d = chrono_island2d_workspace_get_contact_vectors(ws, island_count, 3);
for (size_t island = 0; island < island_count; ++island) {
    double *J = jac3d + island * 6;
    double *L = lambda3d + island * 3;
    assemble_3d_jacobian(island_handles[island], J);
    solve_3d_block(J, L);
}
```

ABI impact:

- The workspace struct only grew by two `ChronoIslandVectorBuffer_C` slots; existing fields stay in place (verified by `test_constraint_common_abi` and the static asserts in `chrono_constraint2d.h`).
- 3D-specific wrappers can live in a separate translation unit and call the accessors without touching the 2D layout.

We will upstream the full PoC once the shared `ChronoConstraintCommon_C` typedef lands, but the API surface is already frozen and covered by the ABI guards.
