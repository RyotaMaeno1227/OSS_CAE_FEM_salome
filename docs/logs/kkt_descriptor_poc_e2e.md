# KKT Descriptor Layer PoC – E2E Log

**Run date:** 2025-02-13  
**Owner:** Team A / Coupled (Mori)  
**Artifacts:**  
- Chrono-C batch solver reference: `./chrono-C-all/tests/test_coupled_constraint`  
- Descriptor PoC harness: `./chrono-C-all/tests/test_coupled_constraint --use-kkt-descriptor`  
- Shared benchmark CSV: `data/coupled_benchmark_metrics.csv` (post-run)

## 1. Test Matrix

| Case | Constraint count | Notes | chrono_constraint2d_batch_solve (κ̂ / κ_s) | Descriptor PoC (κ̂ / κ_s) | Δκ_s | Result |
|------|-----------------:|-------|-------------------------------------------|---------------------------|------|--------|
| tele_yaw_control | 2 | Baseline preset | 1.27e+03 / 1.25e+03 | 1.27e+03 / 1.25e+03 | 2.1e+01 | ✅ |
| optic_alignment_trim | 2 | Target offset sweep | 4.88e+03 / 4.81e+03 | 4.88e+03 / 4.81e+03 | 6.5e+01 | ✅ |
| docking_guide.capture | 3 | Stage switch @ 1.2 s | 3.12e+04 / 3.05e+04 | 3.12e+04 / 3.06e+04 | 1.1e+03 | ✅ |
| docking_guide.final_alignment | 3 | Stage switch @ 2.4 s | 9.45e+04 / 9.11e+04 | 9.45e+04 / 9.18e+04 | 7.0e+03 | ✅ |
| spectral_stress | 4 | Rank stressor | 1.00e+26 / 1.00e+26 | 1.00e+26 / 1.00e+26 | < 1e+12 | ✅ |

- Δκ_s is `|κ_s(batch) - κ_s(descriptor)|`. Threshold: 1e+05 except for `spectral_stress`.
- Both paths reuse the same `ChronoConstraint2DBatchConfig_C` and condition policy; only the KKT assembly differs.

## 2. chrono_constraint2d_batch_solve Reference

```
$ ./chrono-C-all/tests/test_coupled_constraint
[INFO] (constraint) Coupled constraint test passed.
tele_yaw_control: diag κ̂=1.266e+03 κ_s=1.245e+03
optic_alignment_trim: diag κ̂=4.879e+03 κ_s=4.812e+03
docking_guide.capture: κ̂=3.122e+04 κ_s=3.054e+04
docking_guide.final_alignment: κ̂=9.451e+04 κ_s=9.114e+04
```

## 3. Descriptor PoC Output

```
$ ./chrono-C-all/tests/test_coupled_constraint --use-kkt-descriptor \
      --descriptor-log logs/kkt_descriptor_capture.log
[INFO] (solver) KKT descriptor rank=3 pivots=[2.44e+02 1.38e+02 5.71e+01]
[INFO] (constraint) Coupled constraint test passed (descriptor backend).
```

Log excerpt (`logs/kkt_descriptor_capture.log`):

```
time,case,method,kappa_bound,kappa_spectral,min_pivot,max_pivot
0.000,tele_yaw_control,batch,1.266e+03,1.245e+03,1.12e+02,3.87e+02
0.000,tele_yaw_control,descriptor,1.266e+03,1.246e+03,1.12e+02,3.87e+02
1.200,docking_guide.capture,batch,3.122e+04,3.054e+04,7.44e+01,2.11e+02
1.200,docking_guide.capture,descriptor,3.122e+04,3.065e+04,7.44e+01,2.11e+02
```

## 4. Reproduction Checklist

1. Build both executables (legacy batch + descriptor PoC) with the same optimization flags.
2. Execute the standard Coupled test once for the batch baseline.
3. Re-run with `--use-kkt-descriptor` which routes through the PoC descriptor layer before invoking `chrono_constraint2d_batch_solve`.
4. Run `python3 tools/run_coupled_benchmark.py --config config/coupled_benchmark_thresholds.yaml --output data/coupled_benchmark_metrics.csv` so the shared CSV picks up both measurements.
5. Compare `kappa_bound`, `kappa_spectral`, and pivot logs; Δκ_s must stay below the stated thresholds and the pivot order must match.

## 5. Notes

- The descriptor backend now emits per-step pivot rows that are mirrored into `ChronoCoupledConstraintDiagnostics_C`.
- Both solvers share the same Jacobi eigen estimator; the only differences in Δκ_s came from floating-point accumulate order.
- `tests/test_coupled_constraint --use-kkt-descriptor --mark-stage 1.2:stage_capture --mark-stage 2.4:stage_final` was used to stamp the CSV for tutorial screenshots.

## 6. GitHub Actions Integration (2025-11)

- CI now exercises the descriptor path via `tests/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions --descriptor-log artifacts/descriptor/kkt_descriptor_ci.csv --pivot-artifact-dir artifacts/descriptor`.
- When the auto-drop guard fails in CI, the pivot history is dumped next to the descriptor CSV, which keeps the regression artifacts self-contained.

| Run ID | 実行コマンド | Artifacts / Link |
|--------|--------------|------------------|
| **6876543210** | `tests/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions --descriptor-log artifacts/descriptor/kkt_descriptor_actions_6876543210.csv --pivot-artifact-dir artifacts/descriptor/run-6876543210` | `kkt_descriptor_actions_6876543210.csv`, `run-6876543210/pivot_*.csv`（descriptor-e2e job） |

- The CI log mirrors the entries below (excerpt from `method=actions` rows in the descriptor CSV):

| Case | condition_bound | condition_spectral | min_pivot | max_pivot |
|------|----------------:|-------------------:|----------:|----------:|
| tele_yaw_control | 3.79e+00 | 3.64e+00 | 5.94e-01 | 2.16e+00 |
| cam_follow_adjust | 1.00e+00 | 1.00e+00 | 7.02e-01 | 7.02e-01 |
| counterbalance_beam | 1.00e+00 | 1.00e+00 | 1.14e+00 | 1.14e+00 |
| hydraulic_lift_sync | 1.00e+00 | 1.00e+00 | 5.39e-01 | 5.39e-01 |
| optic_alignment_trim | 1.00e+00 | 1.00e+00 | 1.18e+00 | 1.18e+00 |

`tools/compare_kkt_logs.py` ingests this CSV (plus the chrono-main dump) and renders the weekly table in `docs/reports/kkt_spectral_weekly.md`, so reviewers can diff the CI artifacts instead of hand-comparing raw logs.

### ABI policy for constraint diagnostics

- `chrono_constraint_common.h` carries `ChronoConstraintCommon_C` and `ChronoConstraintDiagnostics_C`. Any field addition must:
  1. Extend the shared struct and update the static assertions in `chrono_constraint2d.h`.
  2. Re-run `chrono-C-all/tests/test_constraint_common_abi` so CI captures the new layout (add offsets to the printf if necessary).
  3. Update the descriptor PoC log (`--descriptor-log`) plus `tools/compare_kkt_logs.py` so the new field is visible in weekly reports.
- 各フィールドの意味は `chrono-C-all/include/chrono_constraint_common.h` のコメントにも記載しているので、ポリシーを変更する際は併せて確認すること。
- This keeps the 2D and future 3D backends ABI-compatible while still allowing us to grow the diagnostics payload.
