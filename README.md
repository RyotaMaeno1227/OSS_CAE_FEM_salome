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

- When updating a preset, capture the solver behaviour with multiple relaxation factors so reviewers can spot regressions:

  ```bash
  ./chrono-C-all/tests/bench_coupled_constraint \
    --omega 0.85 \
    --omega 1 \
    --omega 1.15 \
    --output data/diagnostics/bench_coupled_constraint_multi.csv \
    --result-json data/diagnostics/bench_coupled_constraint_multi.json
  ```
- Multi-ω preset last updated: 2025-11-08T18:18:55Z
- The resulting CSV feeds `tools/compare_kkt_logs.py` so the Multi-ω table in `docs/reports/kkt_spectral_weekly.md` stays up to date. Keep the new `multi_omega_reference` preset in `data/coupled_constraint_presets.yaml` in sync with the Hands-on Chapter 02 exercises.
- The CSV can still be dropped into design docs or pasted directly into PRs (`tools/plot_coupled_constraint_endurance.py --summary-json` for quick stats). Note: Chapter 02 of `docs/coupled_constraint_hands_on.md` references the same presets, so update both locations in the same commit.
- `python3 tools/update_multi_omega_assets.py --refresh-report` 実行で上記コマンドブロック（README / Hands-on）と preset・Multi-ω CSV/JSON・`data/diagnostics/kkt_backend_stats.json` を自動同期できる。PR では dry-run (`python -m unittest tools.tests.test_update_multi_omega_assets`) も通すこと。

### Multi-ω 差分チェック手順

1. 最新の chrono-C / chrono-main KKT ログを用意する（`data/diagnostics/chrono_c_kkt_log.csv`、`data/diagnostics/chrono_main_kkt_log.csv`）。
2. Multi-ω CSV/JSON を更新する:
   ```bash
   python3 tools/run_multi_omega_bench.py \
     --output-csv data/diagnostics/bench_coupled_constraint_multi.csv \
     --output-json data/diagnostics/bench_coupled_constraint_multi.json
   ```
3. Diagnostics JSON を添えて `tools/compare_kkt_logs.py` を実行する:
   ```bash
   python3 tools/compare_kkt_logs.py \
     --chrono-c data/diagnostics/chrono_c_kkt_log.csv \
     --chrono-main data/diagnostics/chrono_main_kkt_log.csv \
     --multi-omega data/diagnostics/bench_coupled_constraint_multi.csv \
     --multi-omega-json data/diagnostics/bench_coupled_constraint_multi.json \
     --diag-json data/diagnostics/sample_diag.json \
     --csv-output docs/reports/kkt_spectral_weekly.csv
   ```
4. 生成された Markdown/CSV を PR へ添付し、Δκ・Δpivot・WARN 比をレビュアーへ共有する。
  - 更新対象: README.md / `docs/coupled_constraint_hands_on.md` / `data/coupled_constraint_presets.yaml` / `data/diagnostics/bench_coupled_constraint_multi.(csv|json)` / `data/diagnostics/kkt_backend_stats.json` / `docs/reports/kkt_spectral_weekly.md`（`--refresh-report` 指定時）。

## Documentation Link Lint

- `scripts/check_doc_links.py` scans Markdown files for `docs/` links and ensures the targets exist.  
- Run it locally before doc-heavy PRs:
  ```bash
  python scripts/check_doc_links.py \
    docs/coupled_constraint_tutorial_draft.md \
    docs/coupled_constraint_hands_on.md \
    docs/coupled_contact_test_notes.md
  ```
- CI (docs lint) will execute the same script; fix the offending file paths if it reports `missing docs/...`.
- Failure example:
  ```
  Broken links detected:
    - docs/coupled_constraint_tutorial_draft.md: missing docs/nonexistent.md
  ```
  対処手順: (1) パス誤りを修正するかリンク先ファイルを追加、(2) `python scripts/check_doc_links.py ...` を再実行し成功を確認、(3) `docs/documentation_changelog.md` に修正内容を追記。

## Preset Link Smoke Test

- `scripts/check_preset_links.py` は README / Hands-on / Wiki（本編・サンプル）が `docs/coupled_constraint_presets_cheatsheet.md` を参照しているかを検証します。  
- 実行例:
  ```bash
  python scripts/check_preset_links.py
  ```
- CI でも同テストが走ります。失敗時は `.md` リンクが欠落しているファイルを更新し、Appendix B.3/B.5 のローテ表に Markdown 確認結果を記録してください。

## Coupled Endurance Artifact Fetch Helper

`tools/fetch_endurance_artifact.py` には最新失敗 Run を自動検出する `--latest` フラグがあります（`--auto-latest` のエイリアス）。GitHub CLI と `actions:read` 権限があれば以下で Slack/PR 向けのコメント Markdown を取得できます。

```bash
python tools/fetch_endurance_artifact.py \
  --latest \
  --workflow coupled_endurance.yml \
  --run-status failure \
  --job-name archive-and-summarize \
  --repo $GITHUB_REPOSITORY \
  --output-dir data/endurance_archive/repro \
  --summary-out data/endurance_archive/repro/latest.summary.json \
  --comment-file data/endurance_archive/repro/comment.md \
  --console-comment-only
```

- `--comment-file` で生成される Markdown には Run URL、job ログ、Plan/summary の抜粋、再現コマンドが含まれます。
- `--latest` は `gh run list` から直近 50 件を確認し、`--run-status failure` を優先して Run ID を決定します。
- Slack テンプレは Appendix B.5.2 を参照。Run ID は Appendix B.5.3 の監査表へ追記してください。

## Troubleshooting / Debugging Tips

- **KKT backend tracing** – ビルド時に `-DDEBUG_KKT` を付与すると `chrono_kkt_backend_invert_small` が constraint アドレス／行番号付きで pivot 情報を stderr へ吐きます。`tools/compare_kkt_logs.py` の結果と照らして rank 落ちの箇所を絞り込めます。
- **KKT weekly diff export** – `python3 tools/compare_kkt_logs.py --csv-output docs/reports/kkt_spectral_weekly.csv --diag-json data/diagnostics/sample_diag.json` で Markdown + CSV を同時生成し、A/B/C どのチームでも同じフォーマットを添付できます。
- **Jacobian capture** – `tests/test_island_parallel_contacts --jacobian-log-default` で即席 CSV を生成可能。`python3 tools/run_contact_jacobian_check.py --output-dir tmp/jacobians --report docs/coupled_contact_test_notes.md` を使うとログと Markdown が同時に生成され、`--list-presets` で Hands-on 向けプリセット候補も確認できます。
- **Jacobian placeholder 解除** – `docs/coupled_contact_test_notes.md` のチェックリストに従い、`tools/update_descriptor_run_id.py --run-id <ID>` → `python3 tools/run_contact_jacobian_check.py --output-dir artifacts/contact --report docs/coupled_contact_test_notes.md` を実行し、`git diff` で placeholder が実測値に置き換わったことを確認します。
- **Descriptor Run ID 同期** – CI Run 完了後は `python3 tools/update_descriptor_run_id.py --run-id <GITHUB_RUN_ID>` を忘れず実行し、`docs/logs/kkt_descriptor_poc_e2e.md` / `docs/coupled_island_migration_plan.md` / Jacobian ノートで参照を揃えます。`tools/update_descriptor_run_id.py --dry-run --run-id <ID>` で変更を確認してからコミットできます。
- **Multi-ω refresh targets** – `tools/update_multi_omega_assets.py --refresh-report` は README / Hands-on / `data/coupled_constraint_presets.yaml` / `data/diagnostics/bench_coupled_constraint_multi.(csv|json)` / `data/diagnostics/kkt_backend_stats.json` / `docs/reports/kkt_spectral_weekly.md` を一括で更新します。`python -m unittest tools.tests.test_update_multi_omega_assets` で dry-run も実施してください。
