# HighperformanceFEM Notes

## Logging Integration Check

- The CI pipeline executes `tests/test_coupled_logging_integration` via `make tests`.
- The test ensures Coupled constraint warnings propagate through custom handlers
  at INFO/WARN/ERROR levels and that multi-sink dispatch behaves as expected.
- When the check fails:
  1. Re-run `./chrono-C-all/tests/test_coupled_logging_integration` locally to inspect the captured counts.
  2. Confirm `chrono_log_set_handler(NULL, NULL)` is invoked during teardown so state does not leak between tests.
  3. Verify the Coupled warning policy still emits logs (`enable_logging=1`, sensible `log_cooldown`).
  4. Inspect new handlers for exceptions or stderr output that may abort execution.
- Additional troubleshooting details are collected in `docs/chrono_logging_integration.md` (§6).

## Coupled Benchmark Site

- Weekly GitHub Pages deployments build `site/` via `tools/build_coupled_benchmark_site.py`. The bundle embeds generation timestamp, commit hash, and a link to `config/coupled_benchmark_thresholds.yaml`.
- An Atom feed (`feed.xml`) is emitted alongside `index.html`; GitHub Pages serves it automatically for consumers that want notifications.
- Structured SVG charts (`svg/condition_trends.svg`) are generated to keep diffs light compared with raster screenshots.
- Local reproduction:

  ```bash
  python3 tools/run_coupled_benchmark.py \
    --config config/coupled_benchmark_thresholds.yaml \
    --csv-validation-jsonl logs/csv_issues.jsonl \
    --csv-validation fail \
    --output data/coupled_benchmark_metrics.csv

  python3 tools/build_coupled_benchmark_site.py \
    --output-dir site \
    --copy-data \
    --threshold-config config/coupled_benchmark_thresholds.yaml \
    data/coupled_benchmark_metrics.csv
  ```

- See `docs/coupled_benchmark_setup.md` for detailed instructions and dependency notes (PyYAML, etc.).

## CI Log Triage (Coupled / Island)

`tools/filter_ci_failures.py` helps isolate failing Coupled/Island tests from the full
`test.log`.

```bash
# Annotate the raw log with [COUPLED]/[ISLAND] tags (in-place)
python3 tools/filter_ci_failures.py test.log --tag-input

# Emit only the relevant sections + keyword hits
python3 tools/filter_ci_failures.py test.log --output test_coupled_island.log
```

- Use `--keywords gear contact` to extend the match set during local debugging.
- `--tag-only` combined with `--tag-output` allows creating a separate tagged log without
  running the filter stage.
- Both files are uploaded as CI artifacts (`chrono-tests`) so you can download the
  trimmed log when diagnosing failures remotely.

## Coupled & Contact API Surface

- The minimal C API required to drive Coupled constraints plus contact/island managers is documented in
  [`docs/coupled_contact_api_minimal.md`](docs/coupled_contact_api_minimal.md) (Japanese) and
  [`docs/coupled_contact_api_minimal_en.md`](docs/coupled_contact_api_minimal_en.md) (English).
- Keep optional logging/visualisation helpers out of the minimal surface—those belong in `docs/appendix_optional_ops.md`.

## Coupled Presets

- `data/coupled_constraint_presets.yaml` tracks the canonical Coupled ratios/softness settings used across tutorials, fixtures, and CI.
- `tests/test_coupled_constraint` consumes the preset IDs (e.g., `tele_yaw_control`, `optic_alignment_trim`) in its sweep table so YAML updates are exercised automatically.
- To preview a preset from the shell:

  ```bash
  python3 - <<'PY'
  import yaml, pprint
  presets = yaml.safe_load(open("data/coupled_constraint_presets.yaml"))
  pprint.pp(next(case for case in presets["use_cases"] if case["id"] == "docking_guide"))
  PY
  ```
