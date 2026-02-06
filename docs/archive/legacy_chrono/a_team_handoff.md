# Aチーム引き継ぎメモ（Coupled／Island Solver）

最終更新: 2025-11-08  
対象: Aチーム（Coupled 拘束／Contact 3DOF／島ソルバ／KKT Diagnostics）

**Quicklinks:** [現況](#1-現況サマリ) | [成果物](#2-直近の成果物) | [Pending](#pending--environment-tasks) | [Backlog](#3-未完了タスク--担当) | [テスト手順](#4-推奨テスト運用コマンド) | [Run ID フロー](#run-id-更新フローツール-update_descriptor_run_idpy) | [週次レポート出力例](#toolscompare_kkt_logs.py--出力サンプル--添付手順) | [Evidence](#7-evidence-quicklinks)
---

## 1. 現況サマリ
- **進捗指標**: `docs/archive/legacy_chrono/pm_status_2024-11-08.md` と `docs/archive/legacy_chrono/coupled_island_migration_plan.md` の KPI では、Coupled 拘束移行 80%、Island ソルバ統合 70%、3D 抽象化 45%（残タスク 20〜25%）。  
- **KKT ディスクリプタ PoC**: `docs/logs/kkt_descriptor_poc_e2e.md` に batch ソルバとの Δκ_s / pivot 差分が記録済み。`tests/test_coupled_constraint --use-kkt-descriptor` で再現可能。  
- **共有ダイアグノスティクス**: `chrono_constraint_common.h` の `ChronoConstraintDiagnostics_C` に WARN/INFO と pivot ログを集約し、`tools/compare_kkt_logs.py`＋`docs/reports/kkt_spectral_weekly.md` で chrono-main との週次比較を実施。  
- **Island ワークスペース**: `chrono_island2d_workspace_get_{constraint,contact}_vectors` で任意 DOF のバッファを提供、scheduler backend（auto/openmp/tbb）は stub 実装で serial fallback する状態 (`docs/island_scheduler_poc.md`)。

---

## 2. 直近の成果物
1. **Multi-ω Coupled ベンチ** – `chrono-C-all/tests/bench_coupled_constraint.c` が `--omega` を複数受け取り、`data/diagnostics/bench_coupled_constraint_multi.csv` へ書き出し。README「Coupled Presets」節と `docs/coupled_constraint_tutorial_draft.md` / `docs/coupled_constraint_hands_on.md` で参照。  
2. **KKT/Spectral 週次レポート** – `data/diagnostics/chrono_c_kkt_log.csv`・`chrono_main_kkt_log.csv` を `tools/compare_kkt_logs.py` が解析し、Δκ/Δpivot/WARN 比を `docs/reports/kkt_spectral_weekly.md` に反映。  
3. **3DOF Jacobian API** – `chrono_collision2d.c/h` に Rolling/Torsional 行を追加し、`tests/test_contact_jacobian_3dof.c` で検証。マイグレーション計画 §6 の「接触ヤコビアン 3DOF 化」に紐付く。  
4. **Small Matrix Helper** – `chrono_small_matrix.h/c` と `bench_small_matrix.c` を追加し、KKT backend の行列演算を共通化（`docs/small_matrix_helper.md` に使用ガイド）。  
5. **PM/PoC ドキュメント** – `docs/archive/legacy_chrono/pm_status_2024-11-08.md`、`docs/archive/legacy_chrono/coupled_island_migration_plan.md`、`docs/island_scheduler_poc.md` が最新の PoC 前提とタスク分解を保持。
6. **Descriptor CI モード** – `tests/test_coupled_constraint --descriptor-mode actions --pivot-artifact-dir ...` を Actions へ追加し、`docs/logs/kkt_descriptor_poc_e2e.md` と `docs/reports/kkt_spectral_weekly.md` の差分を直接リンク可能にした。Multi-ω CSV と `tools/compare_kkt_logs.py` のレポート拡張も同じフローで再生成する。
7. **Multi-ω 自動更新ツール** – `python3 tools/update_multi_omega_assets.py --refresh-report` が `bench_coupled_constraint` の再計測、README／Hands-on／`data/coupled_constraint_presets.yaml`／`data/diagnostics/kkt_backend_stats.json` を同一タイムスタンプで更新し、週報 (`docs/reports/kkt_spectral_weekly.md`) まで反映するようになった。
8. **Descriptor Run ID 更新スクリプト** – `tools/update_descriptor_run_id.py --run-id <GITHUB_RUN_ID>` で `docs/logs/kkt_descriptor_poc_e2e.md` と `docs/archive/legacy_chrono/coupled_island_migration_plan.md` の Run ID を同時更新できる。`descriptor-e2e` ジョブの完了後に実行し、ドキュメントと KPIs を揃える。

### Pending / Environment Tasks

- **oneTBB 実測** – `bench_island_solver --scheduler tbb` の実測値は oneTBB 導入環境が整い次第計測。現状は `tbb_fallback` 行でフォールバック時間を記録。
- **Descriptor log artifacts** – 次回 CI Run の `github.run_id` を取得したら `python3 tools/update_descriptor_run_id.py --run-id <ID>` を実行し、`docs/logs/kkt_descriptor_poc_e2e.md` と Migration Plan を同時更新。
- **Jacobian evidence** – `tests/test_island_parallel_contacts --jacobian-log artifacts/contact/contact_jacobian_log.csv` をローカルで実行し、`--jacobian-report docs/coupled_contact_test_notes.md` で placeholder を差し替える。CI が通ったタイミングでエビデンスを貼り替える。

### Run ID 更新フロー（`tools/update_descriptor_run_id.py`）

1. `descriptor-e2e` ジョブ完了後に Actions の Run ID（例: `6876543210`）を控える。
2. リポジトリルートで `python3 tools/update_descriptor_run_id.py --run-id 6876543210` を実行。
3. スクリプトが `docs/logs/kkt_descriptor_poc_e2e.md` と `docs/archive/legacy_chrono/coupled_island_migration_plan.md` を同時更新するので、`git diff` で Run ID が揃っているか確認。
4. 必要に応じて `tests/test_island_parallel_contacts --jacobian-report docs/coupled_contact_test_notes.md --jacobian-log ...` を追走し、Jacobian セクションも同じ Run ID を参照させる。
   - 変更内容を確認したい場合は `python3 tools/update_descriptor_run_id.py --dry-run --run-id <ID>` で差分を印刷してから本番コマンドを実行する。

### `tools/compare_kkt_logs.py` – 出力サンプル & 添付手順

`python3 tools/compare_kkt_logs.py --csv-output docs/reports/kkt_spectral_weekly.csv --diag-json data/diagnostics/chrono_c_diagnostics_sample.json` を実行すると、Markdown（`docs/reports/kkt_spectral_weekly.md`）と CSV の両方が更新されます。レビュー時は以下のような差分を PR 説明へ貼りつつ、生成された CSV を artifacts に添付してください。

```
| Scenario | eq_count | κ̂ (Chrono-C) | κ̂ (chrono-main) | Δκ̂ | κ_s (Chrono-C) | κ_s (chrono-main) | Δκ_s | min pivot Δ | max pivot Δ | pivot₀ Δ | Log levels (C/main) | Status |
|----------|---------:|--------------:|-----------------:|-----:|---------------:|------------------:|------:|-------------:|-------------:|-----------:|---------------------|--------|
| tele_yaw_control | 2 | 1.266e+03 | 1.266e+03 | 1.0e-03 | 1.245e+03 | 1.246e+03 | 1.0e+00 | 2.0e-02 | 3.0e-02 | 0.0e+00 | warning/warning | ✅ |
```

- `docs/reports/kkt_spectral_weekly.csv` はそのまま Excel / Google Sheet へ流用可能。
- `--diag-json data/diagnostics/chrono_c_diagnostics_sample.json` を指定すると `ChronoConstraintDiagnostics_C` の各ケース（min/max pivot, κ_s）を Markdown に追加し、`data/diagnostics/kkt_backend_stats.json` と突き合わせられる。

---

## 3. 未完了タスク & 担当
| タスク | 内容 | 参照 | 優先度 |
|--------|------|------|--------|
| KKT ディスクリプタ E2E 完全化 | PoC ログを `chrono_constraint2d_batch_solve` 本流へ統合し、CI (`--use-kkt-descriptor --descriptor-log`) で常時比較。 | docs/archive/legacy_chrono/coupled_island_migration_plan.md §6.1, docs/logs/kkt_descriptor_poc_e2e.md | High |
| Iterative Solver チューニング | `ChronoConstraintIterativeParams_C` を使い `bench_coupled_constraint --omega` の結果を `docs/reports/kkt_spectral_weekly.md` に毎週反映。 | same as above | High |
| 3DOF Jacobian 統合 | `tests/test_contact_jacobian_3dof` のロジックを `test_island_parallel_contacts` へ統合し、Coupled+Contact の島回帰を更新。 | docs/coupled_contact_test_notes.md §3 | High |
| 島 scheduler（oneTBB） | `chrono_island2d.c` の backend stub を oneTBB 実装へ差し替え、`bench_island_solver --scheduler tbb` で性能計測、`data/diagnostics/bench_island_scheduler.csv` 更新。 | docs/island_scheduler_poc.md | High |
| Diagnostics 共有化 | `ChronoConstraintDiagnostics_C` のフィールド追加時に ABI テスト (`tests/test_constraint_common_abi.c`) を走らせ、`tools/compare_kkt_logs.py` に新列を反映。 | docs/logs/kkt_descriptor_poc_e2e.md（ABI policy） | Medium |
| 週次自動化 | `kkt_spectral_weekly.md` と `bench_coupled_constraint_multi.csv` を一括更新するスクリプト化。 | docs/reports/kkt_spectral_weekly.md | Medium |
| Contact+Coupled ベンチ拡張 | 3DOF Jacobian対応の `tests/test_coupled_constraint` 派生を追加し、`docs/archive/legacy_chrono/coupled_island_migration_plan.md` の進捗に紐付ける。 | docs/coupled_contact_test_notes.md | Medium |
| Scheduler ログ拡張 | backend 選択と fallback を `ChronoIslandSchedulerBackend_C` に記録し、週次レポートで OpenMP/TBB 差異を比較。 | chrono_island2d.c, docs/island_scheduler_poc.md | Medium |
| Shared KPI 更新運用 | `docs/archive/legacy_chrono/pm_status_2024-11-08.md` / Migration Plan / `docs/archive/legacy_chrono/chrono_3d_abstraction_note.md` を同じ Run ID で更新。 | チーム共有メモ | Medium |
| Evidence 整理 | `docs/reports/kkt_spectral_weekly.md` と `docs/island_scheduler_poc.md` に最新 Run ID, commit, コマンドを追記し、Slack 共有のテンプレ化。 |  | Medium |

---

## 4. 推奨テスト／運用コマンド
- `./chrono-C-all/tests/test_coupled_constraint --use-kkt-descriptor --descriptor-log out/descriptor.csv`  
- `./chrono-C-all/tests/bench_coupled_constraint --omega 0.85 --omega 0.92 --csv data/diagnostics/bench_coupled_constraint_multi.csv`  
- `./chrono-C-all/tests/test_contact_jacobian_3dof`（※統合後は `test_island_parallel_contacts`）  
- `./chrono-C-all/tests/bench_island_solver --scheduler auto --csv data/diagnostics/bench_island_scheduler.csv`  
- `python3 tools/compare_kkt_logs.py data/diagnostics/chrono_c_kkt_log.csv data/diagnostics/chrono_main_kkt_log.csv --report docs/reports/kkt_spectral_weekly.md --csv-output docs/reports/kkt_spectral_weekly.csv`
- `python3 tools/run_contact_jacobian_check.py --log data/diagnostics/contact_jacobian_log.csv --report docs/coupled_contact_test_notes.md`（Jacobian エビデンスを素早く更新）

上記テストは CI の `ci.yaml` でも自動実行されるが、パラメータ変更時は必ずローカルで先に回す。ログ／CSV は `data/diagnostics/` 配下に追記し、PR 説明に含める。

---

## 5. 連絡とエスカレーション
- 週次レビュー: 旧 Appendix B.5 の枠は廃止したため、`docs/archive/legacy_chrono/coupled_island_migration_plan.md` の KPI 表をそのままレビュー議事録として更新する。通知チャネルは任意（Slack `#chrono-constraints` もしくは新チャットのピン）。Slack が使えない場合は PR / Issue コメントに `docs/logs/kkt_descriptor_poc_e2e.md` 等のリンクを貼る。  
- ブロッカー例: oneTBB ビルド失敗、Δκ_s が閾値逸脱、3DOF Jacobian が Contact 回帰に統合できない場合。エスカレーション先は PM / Architect WG。  
- Evidence 保管先: `docs/logs/`（PoC）、`docs/reports/`（週次）、`data/diagnostics/`（CSV）。PR では必ずこれらへの差分リンクを添付する。

## 6. oneTBB 導入ガイド
1. **ライブラリの入手** – Linux では `sudo apt-get install libtbb-dev` などで `tbb` とヘッダを導入する。既存の Code_Aster 環境を使う場合は `/usr/lib` や `../Code_Aster/.../lib/libtbb*.so` を参照してもよい。
2. **ビルド設定** – 環境変数で `TBB_INCLUDE_DIR=/path/to/include`、`TBB_LIBS="-L/path/to/lib -ltbb"` を渡し、`make bench` で `chrono_island2d_tbb.cpp` が `tbb::parallel_for` を拾えるようにする。実行時は `LD_LIBRARY_PATH` に同じ lib ディレクトリを追加する。
3. **性能ログ取得** – `./chrono-C-all/tests/bench_island_solver --scheduler tbb --csv data/diagnostics/bench_island_scheduler.csv` を走らせ、`docs/island_scheduler_poc.md` と `docs/archive/legacy_chrono/coupled_island_migration_plan.md` の表を更新する。フォールバック時はログに WARN が 1 回だけ出る。
   - WARN (`oneTBB headers not found at build time; using fallback path`) が出た場合は `ldd chrono-C-all/tests/bench_island_solver` で `libtbb.so` の解決状況を確認し、`TBB_LIBS` が正しく渡っているかをチェック。
4. **ドキュメント更新** – oneTBB を有効化する手順を PR 説明に含め、`docs/archive/legacy_chrono/a_team_handoff.md` の本節と `docs/island_scheduler_poc.md` の「Risks & next steps」を同タイミングで修正する。

## 7. Evidence Quicklinks

| Doc / Artifact | Quick Link | Run ID / 更新テンプレ |
|----------------|------------|-----------------------|
| Descriptor & Weekly report | `docs/logs/kkt_descriptor_poc_e2e.md`, `docs/reports/kkt_spectral_weekly.md`, `docs/reports/kkt_spectral_weekly.csv` | `| <RUN_ID> | <ISO8601> | <Owner> | <preset/omega> | <cmd> | <artifacts> | <memo> |` |
| Diagnostics stats | `data/diagnostics/kkt_backend_stats.json`, `data/diagnostics/sample_diag.json`（`tools/compare_kkt_logs.py --diag-json` 入力） | 更新時は `python3 tools/compare_kkt_logs.py --csv-output ... --diag-json ...` を PR 説明へ貼る |
| Contact Jacobian | `data/diagnostics/contact_jacobian_log.csv`, `docs/coupled_contact_test_notes.md` | `python3 tools/run_contact_jacobian_check.py --output-dir artifacts/contact --report docs/coupled_contact_test_notes.md` |
| Island/TBB | `docs/island_scheduler_poc.md`, `data/diagnostics/bench_island_scheduler.csv`（＋ `data/diagnostics/island_scheduler/tbb_<date>.csv`） | TBB 実測時に Run ID (bench) を `docs/island_scheduler_poc.md` のメモ欄へ追記 |

Weekly Run ID を記録するときは上記テンプレをそのままテーブルへ追加し、`tools/update_descriptor_run_id.py --dry-run --run-id <ID>` で差分を確認してからコミットしてください。
- Evidence Markdown テンプレは本節で示した 3 行形式（Run / Artifact / Log）をそのまま使用する。

#### Evidence 記入例
```
- Run: [#6876543210](https://github.com/acme/highperformanceFEM/actions/runs/6876543210)
- Artifact: [`coupled-endurance-6876543210`](https://github.com/acme/highperformanceFEM/actions/runs/6876543210/artifacts/123456)
- Log: [`docs/logs/kkt_descriptor_poc_e2e.md`](../docs/logs/kkt_descriptor_poc_e2e.md)
```
JSON や CSV を添付する場合はファイル名（例: `data/diagnostics/chrono_c_diagnostics_sample.json`）を文末に追記する。

---

このメモは Aチーム向けの引き継ぎ資料として作成しました。最新の実装状況は `docs/archive/legacy_chrono/coupled_island_migration_plan.md` と `docs/archive/legacy_chrono/pm_status_2024-11-08.md` を合わせて確認してください。
