# Island Scheduler TBB PoC

This memo captures the trial of replacing the OpenMP-only island loop with a task-style backend.  
`ChronoIsland2DSolveConfig_C.scheduler` now accepts:

- `CHRONO_ISLAND_SCHED_AUTO` (default) – keep the existing behaviour (serial unless OpenMP is enabled).
- `CHRONO_ISLAND_SCHED_OPENMP` – force the classic OpenMP `schedule(static)` loop.
- `CHRONO_ISLAND_SCHED_TBB` – opt into the new task backend implemented in `src/chrono_island2d_tbb.cpp`. When oneTBB headers/libs are not available, the shim logs once and falls back to the OpenMP path.

> Build note: set `TBB_LIBS=-ltbb` (or point to a custom lib directory) when running `make` to link against a system-provided oneTBB. Without the library the backend still compiles, but the runtime will warn that it is reusing the OpenMP loop.

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
| tbb stub (legacy) | 1 | 0.069 ms | Old serial stub left for historical comparison. |
| tbb fallback | 4 | 0.071 ms | 2025-11-15 実行: `bench_island_solver --scheduler tbb`（headers 不足で OpenMP fallback）。`data/diagnostics/bench_island_scheduler.csv` の `tbb_fallback` を更新済み。 |
| tbb (oneTBB enabled) | 4 | — | Pending hardware run (`bench_island_solver --scheduler tbb --csv ...` with `TBB_LIBS=-ltbb`). Update `data/diagnostics/bench_island_scheduler.csv`（または `data/diagnostics/island_scheduler/tbb_<date>.csv` に測定ログを保存）し、`python3 tools/update_descriptor_run_id.py` 再実行で Evidence を揃える。 |

### Escalation rules

- **Fallback persists after providing `TBB_INCLUDE_DIR` / `TBB_LIBS`** – Collect the bench output, `make bench` log, and `ldd chrono-C-all/tests/bench_island_solver` result, then escalate to the parallelization lead (Tanaka) via the Aチームチャット。`docs/a_team_handoff.md` の「島 scheduler（oneTBB）」タスク ID を引用する。
- **Bench crash or NaN timings** – Stop editing CSVs, move the raw log to `data/diagnostics/island_scheduler/tbb_<date>.csv`, and open an issue referencing this memo plus `docs/coupled_island_migration_plan.md` §6.1 の "島ワークスペース 3D 拡張" 行。
- **CI fallback regressions** – If Actions logs show repeated WARN without local repro, file it under `docs/documentation_changelog.md` (tag `island_scheduler`) and notify PM in the weekly chat. Keep `tbb_fallback` row unchanged until root cause is verified.

## Risks & next steps

- The shim calls `oneapi::tbb::parallel_for` (or `tbb::parallel_for` on older distributions). Without headers/libs it degrades gracefully and tells the operator to link TBB.
- Pending 実測は `data/diagnostics/island_scheduler/tbb_<YYYYMMDD>.csv` へ生ログを残し、テーブル更新時に `data/diagnostics/bench_island_scheduler.csv` に反映させる。
- Contact resolution still runs on the worker thread that owns the island; pinning contact updates to tasks may further reduce contention.  
- Real oneTBB integration requires linking against the system lib (or shipping one in `third_party`). The API surface above stays neutral so we can flip the switch per platform.
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

## Island step helper

`chrono_island2d_island_step.h` exposes a single inline that both OpenMP と oneTBB バックエンドが共有している。このヘッダには 2D ループと 3D ラッパ―から再利用する例が Doxygen コメントとして記載されている (`src/chrono_island2d_island_step.h`)。

```c
/**
 * @brief Run the constraint batch solve and contact resolution for a single island.
 */
static inline void chrono_island2d_step_island(ChronoIsland2D_C *island,
                                               double dt,
                                               const ChronoConstraint2DBatchConfig_C *cfg);
```

oneTBB 側 (`chrono_island2d_tbb.cpp`) もこのヘルパを呼び出すだけなので、島内での contact 解像ロジックは完全に共有される。

## Enabling oneTBB locally

1. **ライブラリとヘッダ** – `sudo apt install libtbb-dev` もしくは社内の Code_Aster 環境にある `libtbb.so.*` / `include/tbb/` を参照する。
2. **ビルド時フラグ** – `TBB_INCLUDE_DIR=/path/to/include TBB_LIBS="-L/path/to/lib -ltbb" make bench` のように環境変数を与える (`LD_LIBRARY_PATH` に lib を入れておく)。
3. **計測と CSV 更新** – `./chrono-C-all/tests/bench_island_solver --scheduler tbb --csv data/diagnostics/island_scheduler/tbb_$(date +%Y%m%d).csv` のように raw ログを残しつつ、代表値を `data/diagnostics/bench_island_scheduler.csv` へ転記して `docs/island_scheduler_poc.md` / `docs/coupled_island_migration_plan.md` に追記する。Fallback が出る場合は WARN が 1 回だけ記録されるので、lib が見えているか確認する。
4. **CI での扱い** – Actions では TBB を入れていないため OpenMP fallback になる。実測値を共有する場合は上記手順でローカル測定し、CSV/ドキュメントを同じコミットに含める。
