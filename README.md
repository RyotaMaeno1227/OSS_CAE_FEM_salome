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
