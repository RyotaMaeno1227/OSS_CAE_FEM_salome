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

| Backend | Threads | Avg solve time per step | Notes |
|---------|--------:|------------------------:|-------|
| auto (OpenMP static) | 4 | 0.472 ms | Reference path; fastest when workloads are uniform. |
| tbb stub (dynamic) | 4 | 0.455 ms | ~3.6% faster, smoother tail when island sizes differ. |
| tbb stub (dynamic) | 1 | 1.812 ms | Falls back to serial when OpenMP is absent; info log emitted once. |

## Risks & next steps

- The current implementation is a stub—without `CHRONO_USE_TBB`, it emulates tasking via OpenMP `schedule(dynamic)` and logs a notice.  
- Contact resolution still runs on the worker thread that owns the island; pinning contact updates to tasks may further reduce contention.  
- Real oneTBB integration will require a small C++ shim (CMake option + per-platform binaries). The API surface above is already neutral so we can drop the stub in later.
- Benchmarks show good gains once island sizes diverge, but uniform workloads may favour the static OpenMP path—keep the `scheduler` knob exposed for CI comparisons.
