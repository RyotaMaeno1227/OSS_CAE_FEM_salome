# Chrono Migration & Training Hub

This repository now serves a single purpose: track the Project Chrono C-port and
provide learning material for engineers picking up the solver stack. すべての
通知・配信・Slack 連携といった運用機能は 2025-11 で廃止済みです。

## Scope & Layout

- `chrono-C-all/`, `src/`, `include/`, `tests/` – 現在移植中の Chrono C 実装と
  FEM4C 連携コード。`chrono-C-all/tests/bench_coupled_constraint` や
  `tests/test_coupled_constraint` を直接実行して挙動を確認します。
- `docs/` – Coupled/Island ソルバと 3D 抽象化の設計メモ、研修資料、PM KPI。
  Appendix や通知関連 PDF/テンプレートは削除済みです。
- `data/` – 教材・ベンチ用の固定データ。`data/coupled_constraint_presets.yaml`,
  `data/diagnostics/bench_coupled_constraint_multi.(csv|json)`、
  `data/coupled_constraint_endurance.csv` など Chrono の挙動確認に必要な
  最小限のみ残しています。
- `tools/` – Coupled/Island の検証に必須な Python/C ツール
  (`tools/compare_kkt_logs.py`, `tools/run_multi_omega_bench.py`,
  `tools/update_multi_omega_assets.py`, `tools/run_contact_jacobian_check.py` など)。
  通知・アーカイブ系のスクリプトは削除済み。
- `scripts/` – ドキュメント整合性チェックなど教育資料を維持するための
  ユーティリティ。`scripts/check_preset_links.py` が README/Hands-on から
  プリセット表への導線を監視します。

## Active Migration Threads

| Thread | 主目的 | 主要ドキュメント |
|--------|--------|------------------|
| Coupled constraint migration | Chrono main との差分監視、Multi-ω ベンチ、KKT ディスクリプタ移行 | `docs/coupled_island_migration_plan.md`, `docs/coupled_constraint_hands_on.md`, `docs/coupled_constraint_presets_cheatsheet.md` |
| Contact/Island parallelization | 3DOF Jacobian, scheduler stub、oneTBB 評価 | `docs/coupled_contact_api_minimal.md`, `docs/coupled_contact_test_notes.md`, `docs/a_team_handoff.md` |
| Logging & diagnostics | `chrono_log` ハンドラ統合と WARN 収集、週次 KKT 差分 | `docs/chrono_logging_integration.md`, `docs/reports/kkt_spectral_weekly.md`, `docs/chrono_3d_abstraction_note.md` |
| Training & PM tracking | 研修導線、進捗/KPI、レッスンフィードバック | `docs/coupled_constraint_hands_on.md`, `docs/integration/learning_path_map.md`, `docs/pm_status_2024-11-08.md`, `docs/documentation_changelog.md` |

## Educational Materials

- **Hands-on** – `docs/coupled_constraint_hands_on.md` が FEM4C から Chrono C への
  一連の演習を提供。`scripts/check_preset_links.py` で README と Hands-on が
  `docs/coupled_constraint_presets_cheatsheet.md` を参照しているかを常に検証します。
- **Tutorial** – `docs/chrono_coupled_constraint_tutorial.md` /
  `docs/coupled_constraint_tutorial_draft.md` が理論と実装を橋渡し。
- **Contact API note** – `docs/coupled_contact_api_minimal.md` (日) /
  `docs/coupled_contact_api_minimal_en.md` (英) が C API サーフェスを網羅。
- **Learning path map** – `docs/integration/learning_path_map.md` でロードマップと
  推奨コンテンツ順序を確認でき、同ディレクトリの `assets/*.svg` に図版を格納（手動編集でステータスや Run ID を更新）。
- **Preset cheat sheet** – `docs/coupled_constraint_presets_cheatsheet.md` と
  `data/coupled_constraint_presets.yaml` を同期し、Multi-ω 設定や演習条件を共有。
- **Practice sources** – `practice/README.md` と `practice/coupled/ch0x_*` を Hands-on 手順に従って更新し、Run ID/Evidence を `docs/abc_team_chat_handoff.md` のテンプレへ記録。

研修で利用するプリセットは `data/coupled_constraint_presets.yaml` に集約し、
`docs/coupled_constraint_presets_cheatsheet.md` で図表化しています。

## Diagnostics & Data

- `data/coupled_constraint_endurance.csv` – Crono Coupled サンプルログ。
  `tools/plot_coupled_constraint_endurance.py ... --skip-plot --summary-json`
  で統計のみ抽出できます（Webhook や Slack 送信コードは削除されています）。
- `data/diagnostics/bench_coupled_constraint_multi.(csv|json)` – Multi-ω 測定結果。
  更新時は `tools/update_multi_omega_assets.py --refresh-report` を実行し、
  README / Hands-on / `docs/reports/kkt_spectral_weekly.md` を同じタイムスタンプで
  揃えてください。再計測コマンド例:

```bash
./chrono-C-all/tests/bench_coupled_constraint \
  --output data/diagnostics/bench_coupled_constraint_multi.csv \
  --stats-json data/diagnostics/kkt_backend_stats.json \
  --result-json data/diagnostics/bench_coupled_constraint_multi.json \
  --omega 0.85 \
  --omega 1 \
  --omega 1.15
```
Multi-ω preset last updated: 2025-11-15T18:21:10Z
Latest Coupled Presets memo (A↔C sync 2025-11-15): `docs/reports/kkt_spectral_weekly.*` と `data/coupled_constraint_presets.yaml`（`multi_omega_reference`）を Run `local-20251115` で更新済み。Hands-on Chapter 02 の Coupled Presets 節は同じ Run ID / CSV を参照してください。
- `data/diagnostics/chrono_c_kkt_log.csv` / `chrono_main_kkt_log.csv` – KKT 差分比較。
  `tools/compare_kkt_logs.py` が Δκ, Δpivot, WARN 比を Markdown/CSV 化します。

## Out-of-Scope / Removed Items

- GitHub Pages, Slack webhook, artifact mirroring, 旧 Appendix B.x（通知／運用）
  に関するファイルはすべて削除しました。必要であれば各チームのローカル環境で
  任意に構築してください。
- Workflow dispatch で得られる Run ID や通知テンプレートは本リポジトリでは管理しません。
  `docs/a_team_handoff.md` と Migration Plan に必要最小限の Run ID 手順だけを残しています。
- `tools/*` は計算コアの検証用途に限定されています。新しい通知／配布系スクリプトは
  追加しないでください。

## Repository Hygiene

- `scripts/check_preset_links.py` – README/Hans-on から
  `docs/coupled_constraint_presets_cheatsheet.md` へのリンクを確認。
- `tools/update_multi_omega_assets.py --refresh-report` – Multi-ω 関連資産をまとめて更新。
- `docs/documentation_changelog.md` – 研修資料の変更点をまとめ、教育面の透明性を維持。

Troubleshooting コマンド（KKT トレース／Jacobian キャプチャなど）は
`docs/a_team_handoff.md` と `docs/coupled_contact_test_notes.md` に記載されています。
Chrono 移植と教育資料の維持以外の用途では利用しないでください。
