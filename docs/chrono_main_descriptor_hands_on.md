# Hands-on: Descriptor E2E（chrono-main）

Chrono main 版の descriptor-e2e を実行し、Run ID と Artifact を記録するためのミニ手順書です。Chrono C 版の Hands-on とは別に管理し、両者の Run ID を並べて比較できるようにします。

## 実行手順
1. ビルド（例: CMake Preset `default`）
   ```bash
   cmake --preset default
   cmake --build --preset default
   ```
2. テスト実行（descriptor モード）
   ```bash
   ctest --preset default --output-on-failure --tests-regex test_coupled_constraint
   ./build/bin/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions \
     --pivot-artifact-dir artifacts/descriptor-<RUN_ID>
   ```
3. 生成物
   - `artifacts/descriptor-<RUN_ID>/pivot_log.csv`
   - `artifacts/descriptor-<RUN_ID>/spectral_log.csv`
   - `artifacts/descriptor-<RUN_ID>/diagnostics.json`

## Run ID 記録テンプレ
```
- Run: [#19582037625](https://github.com/<owner>/<repo>/actions/runs/19582037625)
- Artifact: descriptor-e2e-19582037625 (pivot/spectral/diagnostics)
- Log: docs/logs/kkt_descriptor_poc_e2e_chrono_main.md
- Notes: {condition warnings=0, rank=active_equations, pivot span=...}
```

## Evidence の反映先
- `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md` に追記
- `docs/coupled_island_migration_plan.md` の chrono-main 行に Run ID を転記
- `docs/abc_team_chat_handoff.md` の chrono-main テンプレに Run ID/Artifact を共有

## 週次同期
- `tools/compare_kkt_logs.py --csv-output docs/reports/kkt_spectral_weekly_chrono_main.csv --diag-json artifacts/descriptor-<RUN_ID>/diagnostics.json` を実行し、Chrono C 版との差分を取得。
- 週次チャットでは Chrono C / chrono-main の Run ID ペアとピボット差分スクリーンショットを共有する。***
