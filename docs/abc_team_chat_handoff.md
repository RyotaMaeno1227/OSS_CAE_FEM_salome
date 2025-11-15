# A/B/C チームチャット向け引き継ぎメモ

作成日: 2025-11-14  
目的: Appendix 撤廃後の新チャットでも Chrono 移植と教育資料に集中できるよう、A/B/C 各チームで扱うミッション、リファレンス、Pending を 1 つの資料に統合する。

---

## 1. 共通ルール
- **フォーカス**: Chrono 移植と教育資料のみを議論。通知 Bot・GitHub Pages・Appendix の旧運用は再開しない。
- **Run ID／Evidence**: `docs/a_team_handoff.md` と `docs/coupled_island_migration_plan.md` に同じ Run ID を記載し、Artifacts は `docs/logs/` / `docs/reports/` / `data/diagnostics/` へ格納する。
- **長時間ログ**: `data/coupled_constraint_endurance.csv` を唯一の正とし、`data/endurance_archive/` は空のまま保持する。
- **Presetリンク検証**: README / Hands-on / preset を更新する前後で `python scripts/check_preset_links.py` を必ず実行・共有。
- **チャット記法**: 週次ピン留めに「進捗」「ブロッカー」「必要な入力」「次の Run ID」を列挙し、詳細は本資料へのリンクを貼る。

---

## 2. Aチーム（Coupled／Island／Diagnostics）
- **ミッション**: Chrono-C と chrono-main の Coupled 拘束・Island ソルバ差分解消、KKT ディスクリプタ／Iterative Solver／3DOF 接触を統合。
- **主要ドキュメント**:
  - `docs/a_team_handoff.md`: テスト手順・連絡フロー・Evidence テンプレを集約。
  - `docs/coupled_island_migration_plan.md`: KPI スナップショットと週次バックログ。
  - `docs/chrono_3d_abstraction_note.md`: 3D 抽象化テンプレと進捗バー。
  - `docs/coupled_contact_test_notes.md`: Contact + Coupled 検証のチェックリスト。
- **チャットで扱うトピック**:
  1. 最新 Run ID（例: `tests/test_coupled_constraint --descriptor-mode actions`、`bench_coupled_constraint`）と得られた CSV/MD。
  2. `tools/update_multi_omega_assets.py --refresh-report` の予定と README/Hands-on/preset の同期状況。
  3. oneTBB 設定 (`TBB_INCLUDE_DIR`, `TBB_LIBS`, `bench_island_solver --scheduler tbb`) とフォールバックログ。
  4. Jacobian ログ (`python tools/run_contact_jacobian_check.py --report docs/coupled_contact_test_notes.md`) の更新可否。
- **Pending (2025-11-14)**:
  - 6876543210 系 Run ID を再実行し、Chrono-main との差分を `docs/coupled_island_migration_plan.md` の最新 KPI に反映。
  - Iterative Solver の ω/シャープネス掃引と `data/diagnostics/kkt_backend_stats.json` 更新。
  - 3DOF Jacobian を `tests/test_island_parallel_contacts` へ完全統合し、旧ログを `docs/logs/` から差し替え。

---

## 3. Bチーム（Nightly／Diagnostics Logging）
- **ミッション**: `coupled_endurance.yml` の `workflow_dispatch` を必要なタイミングで実行し、Run ID と生成物を記録。通知系の自動化は廃止のまま維持する。
- **主要ドキュメント**:
  - `docs/pm_status_2024-11-08.md`: Bチームの役割と CSV 保守ルール。
  - `docs/reports/coupled_endurance_failure_history.md`: 歴代失敗ログ。参照のみ。
- **チャットで扱うトピック**:
  1. Run ID と共有フォーマット（例: `Run #19376054987 / Artifact coupled-endurance-19376054987 / data/coupled_constraint_endurance.csv 反映済み`）。
  2. CSV の整合性チェック結果 (`python tools/plot_coupled_constraint_endurance.py --skip-plot --summary-json` のログ)。
  3. 失敗ジョブで得られた条件数 / Rank 欠落を Aチームへ渡すタイミング。
- **Pending (2025-11-14)**:
  - `data/coupled_constraint_endurance.csv` の Run ID 欄を最新の #19376054987 系に更新し、差分を `docs/documentation_changelog.md` へ記載。
  - `data/endurance_archive/` に残った ZIP を削除し、`git status` の結果をチャットで共有。

---

## 4. Cチーム（Tutorials／Docs／Education）
- **ミッション**: Hands-on / Tutorial / Learning Path の維持。Chrono 移植で必要な教育素材を Markdown ベースで整備。
- **主要ドキュメント**:
  - `docs/coupled_constraint_hands_on.md`: Chapter 別演習と進捗表（W2〜W4）。
  - `docs/coupled_constraint_presets_cheatsheet.md`: プリセット表・更新フロー。
  - `docs/documentation_changelog.md`: 改訂履歴（他チーム更新時もここに記録）。
  - `docs/integration/learning_path_map.md`: 研修ルート全体像。
- **チャットで扱うトピック**:
  1. 新規教材レビュー依頼（例: Chapter 03 Contact 演習ログや図版差し替え）。
  2. preset 更新時の diff と `scripts/check_preset_links.py` の結果共有。
  3. README / Hands-on / preset YAML の同時編集計画（Aチームとの同期点を列挙）。
  4. Appendix 撤廃後のリンク切れ把握と軽量化状況。
- **Pending (2025-11-14)**:
  - Hands-on Chapter 02, 03 のスクリーンショットを `docs/integration/learning_path_map.md` の該当箇所に追加。
  - `docs/documentation_changelog.md` へ Appendix 撤廃と本資料追加のエントリを投稿。
  - `docs/git_setup.md` のリンク群（Preset 検証、Run ID）を本資料に合わせて最新化。

---

## 5. 共通チェックリスト
- README/Hands-on/preset を触る場合は `python scripts/check_preset_links.py` の結果を貼る。
- Run ID 生成後は `git status`、主要差分 (`git diff docs/...`) のスクリーンショット／引用を共有。
- Evidence 追加前後で `tools/update_multi_omega_assets.py --refresh-report` の結果に差分がないか確認。

---

## 6. Run ID & Evidence テンプレ
```
- Run: [#<GITHUB_RUN_ID>](https://github.com/<owner>/<repo>/actions/runs/<ID>)
- Artifact: `<name>` (保存先: docs/logs/... or data/diagnostics/...)
- Log/Report: <相当する Markdown/CSV へのパス>
- Notes: {条件数, WARN 比, Jacobian status, etc.}
```
- `python tools/update_descriptor_run_id.py --run-id <ID>` で `docs/logs/kkt_descriptor_poc_e2e.md` と `docs/coupled_island_migration_plan.md` を同期。
- Evidence 追記後は `python scripts/check_preset_links.py` と `git status` の要約をチャットに貼る。

---

## 7. 代表コマンド集
| 用途 | コマンド例 | 補足 |
|------|-----------|------|
| Coupled ディスクリプタ | `./chrono-C-all/tests/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions --pivot-artifact-dir artifacts/descriptor` | Run ID 記録と pivot CSV 取得 |
| Multi-ω 更新 | `python tools/update_multi_omega_assets.py --refresh-report` | README/Hands-on/preset を同時更新 |
| Endurance 要約 | `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot --summary-json data/latest.endurance.json --no-show` | Bチーム向け（通知不可） |
| Jacobian チェック | `python tools/run_contact_jacobian_check.py --log data/diagnostics/contact_jacobian_log.csv --report docs/coupled_contact_test_notes.md` | A/C 両チームで共有 |
| Presetリンク検証 | `python scripts/check_preset_links.py` | README / Hands-on 修正時に実行 |

---

## 8. 次のステップ案
1. 各チームで本資料を確認し、担当者名や最新 Run ID を追記。
2. 新チャットのピン留めに本資料の相対パス (`docs/abc_team_chat_handoff.md`) を貼る。
3. 今後更新した場合は `docs/documentation_changelog.md` へ記録し、該当チームのチャットで共有。

---

## 9. チーム別タスク割り当て（各15件）

### Aチーム（Coupled／Island／Diagnostics）
1. `tests/test_coupled_constraint --descriptor-mode actions` を再実行し、Run ID と pivot CSV を `docs/logs/kkt_descriptor_poc_e2e.md` に追記する。  
2. `python tools/update_multi_omega_assets.py --refresh-report` を走らせ、README / Hands-on / `data/coupled_constraint_presets.yaml` / `docs/reports/kkt_spectral_weekly.md` の整合を確認。  
3. chrono-main から取得した最新 KKT スペクトルログを `tools/compare_kkt_logs.py` で解析し、Δκ・Δpivot を `docs/coupled_island_migration_plan.md` に反映。  
4. `tests/test_island_parallel_contacts` に 3DOF Jacobian ログ出力を統合し、`docs/coupled_contact_test_notes.md` のチェックリストを更新。  
5. oneTBB ビルド (`TBB_INCLUDE_DIR`, `TBB_LIBS`) を構築し、`bench_island_solver --scheduler tbb` の結果を `data/diagnostics/bench_island_scheduler.csv` に追記。  
6. `docs/island_scheduler_poc.md` に TBB fallback の計測値とエスカレーション条件を追記。  
7. `data/diagnostics/kkt_backend_stats.json` を最新 Run で再生成し、CI との差分を確認。  
8. `tests/test_constraint_common_abi` を更新して `ChronoConstraintDiagnostics_C` の ABI をチェックし、結果を `docs/logs/kkt_descriptor_poc_e2e.md` に添付。  
9. `data/diagnostics/sample_diag.json` を `tools/compare_kkt_logs.py --diag-json` で再出力し、テンプレに沿ってメトリクスを更新。  
10. `docs/a_team_handoff.md` の「連絡とエスカレーション」節を最新運用（Slack 任意）に合わせて再確認。  
11. KPI 表 (`docs/coupled_island_migration_plan.md` §5.1) の進捗値を週次 Run ID に合わせて更新。  
12. Rank 欠落時のトラブルシュート手順を `docs/coupled_island_migration_plan.md` のバックログメモへ追記。  
13. `chrono_constraint_common.h` へ追加したフィールドのユニットテストを `chrono-C-all/tests/test_constraint_common_*` に拡張。  
14. `docs/reports/kkt_spectral_weekly.md` の Multi-ω グラフを最新 CSV で再生成し、差分をレビュー。  
15. Cチームと連携して README / Hands-on の Coupled Presets 節に最新計測メモを挿入。

### Bチーム（Nightly／Diagnostics Logging）
1. ✅ 2025-11-14: `workflow_dispatch` で `coupled_endurance.yml` を起動し、`Run #19381234567`（Artifact `coupled-endurance-19381234567`）を `docs/pm_status_2024-11-08.md` / 本ドキュメントに記録。  
2. ✅ `data/coupled_constraint_endurance.csv` へ Step 7200–7209 を追記し、同 Run の condition number (10.89–10.91) とタイムスタンプを反映。  
3. ✅ `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot --summary-json data/latest.endurance.json --no-show` を実行し、Samples 7210 / Duration 25.232s のサマリを `data/latest.endurance.json` に保存。  
4. ✅ `ls data/endurance_archive` → 空ディレクトリを確認。必要ファイルなしのため `git status` の結果のみを共有。  
5. ✅ `docs/reports/coupled_endurance_failure_history.md` はアーカイブ扱いを維持（未変更である旨を `docs/pm_status_2024-11-08.md` に記載）。  
6. ✅ Aチームと列構成を確認し、合意内容を `docs/pm_status_2024-11-08.md` の B ログに追記。  
7. ✅ `docs/documentation_changelog.md` へ今回の CSV/JSON 更新と担当（Bチーム）を登録。  
8. ✅ 監視コマンド `watch -n 60 'tail -n 20 data/coupled_constraint_endurance.csv'` を B ピンへ共有し、本ドキュメントと `docs/pm_status_2024-11-08.md` にリンク。  
9. ✅ 失敗 Run の報告テンプレを `docs/templates/b_team_endurance_templates.md` に整備（条件数 / Rank 欠落ログ込み）。  
10. ✅ `docs/git_setup.md` に Bチーム向け Git 操作チートシートを追記（`git add data/coupled_constraint_endurance.csv data/latest.endurance.json docs/pm_status_2024-11-08.md` 等）。  
11. ✅ 複数 Run ID の優先順位ルール（最新成功 > 最新失敗 > 旧成功）を `docs/pm_status_2024-11-08.md` に文章化し、本セクションから参照。  
12. ✅ `python scripts/check_preset_links.py` の実行結果「Preset links verified for 2 file(s).」をチャットに共有し、ログを `docs/pm_status_2024-11-08.md` に記録。  
13. ✅ `docs/templates/b_team_endurance_templates.md` に `data/latest.endurance.json` 共有テンプレを追加（外部配布時にコピー運用）。  
14. ✅ 本 B セクションを月次レビュー対象とする旨を `docs/pm_status_2024-11-08.md` / `docs/documentation_changelog.md` に記載。  
15. ✅ `workflow_dispatch` 権限・トークン確認手順（Settings > Actions > General のチェックリスト）を `docs/pm_status_2024-11-08.md` へ追記し、リーダー共有ログを残す。

### Cチーム（Tutorials／Docs／Education）
1. `docs/coupled_constraint_hands_on.md` Chapter 02/03 の TODO を解消し、進捗表 (W2〜W4) を更新。  
2. `docs/coupled_constraint_presets_cheatsheet.md` と `data/coupled_constraint_presets.yaml` の値を突合し、差異を修正。  
3. `scripts/check_doc_links.py docs/coupled_constraint_tutorial_draft.md docs/coupled_constraint_hands_on.md docs/coupled_contact_test_notes.md` を実行し、リンク切れを解消。  
4. `docs/integration/learning_path_map.md` に Hands-on のスクリーンショットやロードマップ図を追加。  
5. README の「Educational Materials」節に最新教材リンクが揃っているか見直す。  
6. `docs/documentation_changelog.md` に本タスク割り振りと Appendix 撤廃の更新を追記。  
7. `docs/git_setup.md` のリンク・コマンドを現行フロー（Preset チェック、Run ID 管理）に合わせて更新。  
8. `docs/coupled_contact_api_minimal.md` / `_en.md` の訳語・API 名称の揺れを整える。  
9. Hands-on で参照する `practice/coupled/ch0x_*` ソースをレビューし、README への導線を整備。  
10. `docs/chrono_coupled_constraint_tutorial.md` の図版・式番号を確認し、Chrono main との差異を注記。  
11. `docs/chrono_3d_abstraction_note.md` の学習者向けサマリを抽出し、Learning Path にリンク。  
12. `docs/pm_status_2024-11-08.md` の「C – Tutorials / Docs」欄に最新進捗コメントを追記。  
13. `data/diagnostics/bench_coupled_constraint_multi.*` 更新手順を Hands-on 付録にまとめる。  
14. チャット投稿テンプレ（Markdown）を整備し、本資料と整合させる。  
15. `rg -n "Appendix"` を実行して残骸リンクを洗い出し、不要な参照を削除する PR を準備。

---

## 10. ピン留めメッセージ例
```
[Team A/B/C Weekly]
- Progress: <最新 Run ID と成果>
- Blockers: <依頼したいレビューや不足ログ>
- Inputs Needed: <誰に何を頼むか、締切>
- Next Run: <計画中の Run ID とコマンド>
- Practice Refs: <touch した practice/coupled/ch0x_* / data/*.csv>
Ref: docs/abc_team_chat_handoff.md
```
> メッセージ末尾に `git status` / `python scripts/check_preset_links.py` の抜粋、変更ファイルパス、更新した practice/coupled/ch0x_* のサマリを添付する。

---

## 11. 更新運用メモ
- 編集前に `rg "abc_team_chat_handoff"` で他ドキュメントからの参照を確認し、リンク切れが発生しないようにする。
- 追記したセクション番号と `docs/documentation_changelog.md` のエントリ番号を合わせておくとレビューが容易。
- `git diff docs/abc_team_chat_handoff.md` の結果をスクリーンショット化し、A/B/C すべてのチャットへ一斉連絡する。

本メモは今後のチャット立ち上げ時のデフォルト資料として扱ってください。
