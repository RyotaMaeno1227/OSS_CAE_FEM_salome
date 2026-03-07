# チーム別 Runbook（現行運用）

最終更新: 2026-03-06
対象: PM / Aチーム / Bチーム / Cチーム / Dチーム / Eチーム

## 1. 目的
- FEM4C の 2D 2-link flexible validation roadmap に沿って、5チーム運用を統一する。
- 旧 A/B/C の CI-contract 中心タスクを凍結し、新しい MBD/FEM/coupled 実装へ切り替える。

## 2. 参照優先順位（必須）
1. `docs/long_term_target_definition.md`
2. `docs/04_2d_coupled_scope.md`
3. `docs/05_module_ownership_2d.md`
4. `docs/06_acceptance_matrix_2d.md`
5. `docs/07_input_spec_coupled_2d.md`
6. `docs/08_merge_order_2d.md`
7. `docs/09_compare_schema_2d.md`
8. `FEM4C/fem4c_2link_flexible_detailed_todo.md`
9. `FEM4C/fem4c_codex_team_prompt_pack.md`
10. `docs/abc_team_chat_handoff.md`（Section 0）
11. `docs/fem4c_team_next_queue.md`
12. `docs/team_status.md`
13. `docs/session_continuity_log.md`

## 3. スコープ
### In Scope
- 2D rigid 2-link MBD
- 2D flexible 2-link validation solver
- explicit / Newmark-beta / HHT-alpha
- FEM static snapshot + full mesh 再アセンブル
- parser / examples / acceptance / compare schema

### Out of Scope
- 接触
- 摩擦
- 非線形材料
- 3D MBD
- 一般化された coupled 製品化機能
- 制御連成

### Chrono参照ルール
- 一次参照は `third_party/chrono/chrono-main` のみとする。
- 参考にするのは責務分割、state 構造、integrator 設計、constraint/KKT の考え方に限定する。
- コード転載や依存追加はしない。

## 4. チーム責務
- PM: スコープ、責務、受入、入力仕様、比較 schema、マージ順の固定
- A: body / forces / explicit / kinematics / MBD output
- B: constraint / KKT / dense solver / Newmark / HHT / projection
- C: FEM API 化 / full reassembly / runtime BC / nodeset / snapshot
- D: flexible body wrapper / FE reaction / 1-link -> 2-link flexible 拡張
- E: runner 縮退 / coupled orchestration / parser / examples / compare / end-to-end acceptance

## 5. セッション運用ルール
- PMチャットが「作業してください」のみの場合、追加確認なしで `docs/abc_team_chat_handoff.md` Section 0 と `docs/fem4c_team_next_queue.md` の先頭 `In Progress` / `Todo` から着手する。
- PM/ユーザーのチャットは原則起動トリガーのみとし、短時間ラン是正、超過ラン是正、再開点、禁止コマンド、優先度変更は `docs/fem4c_team_next_queue.md` の `PM運用メモ` を正本とする。
- ユーザーから Codex への通常トリガーは `確認してください` とする。特記が無ければ、Codex は control tower と最新 docs を確認し、受理/差し戻し/次アクションを自走で整理する。
- `確認してください` は全チーム終了後に限定しない。終了済みチームが 1 つでもあれば部分確認を許可し、稼働中チームは `RUNNING` / `READY_TO_WRAP` / `ACTIVE_UNCONFIRMED` の状態に応じて扱う。
- 各チームは `scripts/session_timer.sh start` 実行前に `docs/fem4c_team_next_queue.md` の `PM運用メモ` を確認する。
- `PM運用メモ` に個別注意が追記された場合、次セッションから追加チャット無しで自動適用する。
- blocker 以外の問い合わせは禁止する。
- 開始時に `scripts/session_timer.sh start <team_tag>` を実行する。
- `start` 後 10 分以内に `scripts/session_timer_declare.sh <session_token> <primary_task> <secondary_task> ["plan_note"]` を実行し、`SESSION_TIMER_DECLARE` を `docs/team_status.md` に転記する。
- 中間証跡として `bash scripts/session_timer_guard.sh <token> 10`, `20`, `30`, `60` を記録する。
- 終了時に `scripts/session_timer.sh end <token>` を実行し、出力を `docs/team_status.md` に原文転記する。
- 1セッションは `60 <= elapsed_min <= 90` を基本レンジとし、最低 `elapsed_min >= 60` を必須とする。
- 60分は開発前進に使う。実装系ファイル差分を毎セッション必須とする。
- docs単独更新での完了報告は禁止する。
- 先頭タスク完了後は同一セッションで次タスクへ進む。
- 次タスク候補が無い場合は、同一スコープで `Auto-Next` を `Goal / Scope / Acceptance` 付きで `docs/fem4c_team_next_queue.md` に追記して継続する。
- 同一コマンド反復、長時間ソーク、時間稼ぎ目的の検証は禁止する。
- 検証は今回変更した実装に直結する短時間スモークを原則とする。
- `sleep` 等の人工待機は禁止する。
- D/E チームは `docs/team_status.md` に見出しが無ければ自分で `## Dチーム` / `## Eチーム` を追加してよい。

## 5A. PM監視コマンド
- one-shot 監視:
  - `python scripts/team_control_tower.py`
- 連続監視（スナップショットを `/tmp` に出力）:
  - `bash scripts/watch_team_control_tower.sh 60 /tmp/team_control_tower_snapshot.md`
- 監視結果の見方:
- `ACTIVE_UNCONFIRMED`: `start` はあるが recent guard heartbeat が無く、実稼働を断定できない。継続中なら `guard` を記録させ、停止済みなら `end`/報告を回収する。
- `PLAN_MISSING`: `start` はあるが 10 分以内の `SESSION_TIMER_DECLARE` が無い。継続中なら即 `primary/secondary task` を宣言させ、停止済みなら stale 扱いで同一タスクをやり直す。
- `STALE_NO_GUARD`: `start` だけ残って guard が無い短時間停止ラン。`start` から 12 分以内に最初の guard が無い場合はここへ落とす。queue は進めず、新規 `session_token` で同一タスクをやり直す。
- `STALE_BEFORE_60`: guard はあるが 60分未満で停止した stale session。最後の guard/heartbeat から 12 分以上更新が無いまま `elapsed_min < 60` の場合はここへ落とす。queue は進めず、新規 `session_token` で同一タスクをやり直す。
- `STALE_AFTER_60`: 60分以上進んだ stale session。終了報告を回収できるなら回収し、無理なら同一スコープを再実行する。
- `RUNNING`: 稼働中。`guard60` 未達なら終了報告させない。
- `READY_TO_WRAP`: 稼働中。`guard60` 到達済みで、完了なら終了報告へ進める。
  - `OVERRUN`: 90分超過。ここで区切って終了報告させ、次セッションへ分割する。
  - `READY_NEXT`: 最新報告は受理済み。次回は `作業してください` のみでよい。
  - `NEEDS_REWORK`: 最新報告は不受理。`docs/fem4c_team_next_queue.md` の `PM運用メモ` に従って同一タスクを再開させる。
- 部分確認の扱い:
  - `RUNNING` / `READY_TO_WRAP` / `ACTIVE_UNCONFIRMED` のチームが残っていても、終了済みチームだけ先に確認してよい。
  - PM は終了済みチームへ先にフィードバックし、稼働中チームは次の `確認してください` まで継続させる。
- `ACTIVE_UNCONFIRMED` のチームについて、ユーザーが「停止済み」と確認した場合は stale session とみなし、旧 token の `end` 回収を要求せず queue 先頭タスクを新規 token で再開させる。
- `PLAN_MISSING` のチームについて、ユーザーが停止済みと確認した場合は queue を進めず同一タスクを新規 `session_token` で再開させる。
- `STALE_NO_GUARD` / `STALE_BEFORE_60` はユーザー確認が無くても短時間停止ランとして扱ってよい。A=24分、B/E=5分未満のようなケースはここに分類し、queue を進めない。
- `scripts/run_team_acceptance_gate.sh` は次ランから `SESSION_TIMER_DECLARE` を既定で要求する。旧 latest entry が fail しても、新規ランで declare 付き報告へ置き換える。

## 6. 報告ルール
- 毎セッション終了時に以下を更新する。
  - `docs/team_status.md`
  - `docs/session_continuity_log.md`
- `docs/session_continuity_log.md` では必ず以下4項目を更新する。
  - `Current Plan`
  - `Completed This Session`
  - `Next Actions`
  - `Open Risks/Blockers`
- 完了報告には以下を必ず含める。
  - touched files
  - 実装した関数 / 構造体
  - 実行コマンド
  - pass/fail 根拠
  - timer 出力

## 7. 受入方針
- 一次基準は `docs/fem4c_team_next_queue.md` の `Acceptance` とする。
- M0-M3 は FEM4C 単体で確認できる受入を優先する。
- RecurDyn / AdamsFlex の実データは現時点では未投入のため、compare schema 固定までは必須、数値比較は M4 で必須化する。
- 実データ未取得は M0-M3 の blocker にしない。

## 7A. coupled_compare reason-code 契約
- `FEM4C/scripts/run_coupled_compare_checks.sh` の `result_note` は `pass|make_missing_target|make_failed|FAIL:*` のみを許可する。
- 機械可読の正本は `make -C FEM4C coupled_compare_reason_codes` とし、PM 監査はこの出力を参照してよい。
- repo root からの one-shot 監査は `bash scripts/run_coupled_compare_reason_code_contract_audit.sh [log_path]` を使ってよい。
- repo root wrapper は `contract_audit_target=`, `contract_audit_mode=`, `contract_audit_log_path=`, `contract_audit_result=` を必ず出力する。
- repo root から bundle + wrapper modes をまとめて確認する場合は `bash scripts/run_coupled_compare_reason_code_contract_stack.sh [out_dir]` を使ってよい。
- repo root stack wrapper は `contract_stack_components=`, `contract_stack_out_dir=`, `contract_stack_bundle_log=`, `contract_stack_audit_modes_log=`, `contract_stack_result=` を必ず出力する。
- PM 向けの最上位集約は `bash scripts/run_coupled_compare_reason_code_pm_surface.sh [out_dir]` を使ってよい。
- PM surface wrapper は `pm_surface_components=`, `pm_surface_out_dir=`, `pm_surface_fem4c_log=`, `pm_surface_audit_modes_log=`, `pm_surface_stack_modes_log=`, `pm_surface_result=` を必ず出力する。
- root entrypoint 群の回帰は `bash scripts/test_coupled_compare_reason_code_root_modes.sh` を使ってよい。
- root mode wrapper は `root_modes_components=`, `root_modes_out_dir=`, `root_modes_audit_log=`, `root_modes_stack_log=`, `root_modes_pm_surface_log=`, `root_modes_result=` を必ず出力する。
- repo root の最上位 wrapper は `bash scripts/run_coupled_compare_reason_code_root_surface.sh [out_dir]` を使ってよい。
- root surface wrapper は `root_surface_components=`, `root_surface_out_dir=`, `root_surface_pm_surface_log=`, `root_surface_root_modes_log=`, `root_surface_result=` を必ず出力する。
- 提出ログの transitive 整合確認は `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` を使ってよい。
- validator の required key 群は `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py --print-required-keys` で機械可読に取得できる。
- FEM4C 側の focused bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` を使ってよい。
- repo root の audited entrypoint は `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh [out_dir]` を使ってよい。
- root surface audit wrapper は `root_surface_audit_components=`, `root_surface_audit_out_dir=`, `root_surface_audit_log=`, `root_surface_audit_result=` を必ず出力する。
- root surface contract bundle の repo root 監査は `bash scripts/run_coupled_compare_reason_code_root_surface_contract_audit.sh [log_path]` を使ってよい。
- root surface contract audit wrapper は `root_surface_contract_audit_target=`, `root_surface_contract_audit_mode=`, `root_surface_contract_audit_log_path=`, `root_surface_contract_audit_result=` を必ず出力する。
- focused root-surface contract audit 提出ログの検証は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py <audit_report_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_audit_report_test` を使ってよい。
- 実装と validator の正本は以下 4 点を同時更新する。
  - `FEM4C/scripts/coupled_compare_reason_codes.sh`
  - `FEM4C/scripts/check_coupled_compare_checks_manifest.py`
  - `docs/team_runbook.md`
  - `docs/fem4c_team_next_queue.md`
- 新しい reason code を追加する場合は、wrapper / validator / printer / self-test / runbook / queue を同一セッションで更新する。

## 8. Legacy運用
- 旧 A/B/C CI-contract 運用文書は以下へ退避済み。
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/team_runbook_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/fem4c_team_next_queue_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/abc_team_chat_handoff_legacy_2026-03-06.md`
- 旧運用を再開する場合は PM 明示指示が必要。
