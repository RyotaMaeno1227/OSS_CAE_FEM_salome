# チーム別 Runbook（現行運用）

最終更新: 2026-03-08
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
14. `AGENTS.md`

- `AGENTS.md` は repo-local の常設運用ルールであり、timer module / acceptance / monitoring の最小原則を固定する。

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

### E-team current acceptance entrypoints
- `make -C FEM4C coupled_2d_acceptance`
  full acceptance orchestration。build / rigid_matrix / flex_matrix / compare_matrix を 1 コマンドで回す。
- `make -C FEM4C coupled_2d_acceptance_contract_checks`
  focused contract bundle。subset / invalid subset / threshold provenance をまとめて確認する。
- subset rerun + manifest override:
  `STAGES="build rigid_matrix"` や `STAGES="compare_matrix"` の subset rerun でも `MANIFEST_CSV=<custom.csv>` を使える。parent manifest は `MANIFEST_CSV` 側へ出し、compare child manifest は `OUT_DIR/compare_matrix/` 配下に残す。
- `make -C FEM4C coupled_2d_acceptance_contract_checks_test`
  focused contract bundle の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_gate`
  one-command gate。full acceptance の後に focused contract bundle を続けて回し、`coupled_acceptance_gate_columns` / `coupled_acceptance_gate,...` の stable summary rows を出す。gate row には `rigid_limit_threshold_source_command` と `rigid_limit_threshold_update_points` も含まれるため、nested manifest を開かずに threshold provenance を追える。full acceptance log と contract bundle log は分離保存する。
- `make -C FEM4C coupled_2d_acceptance_gate_test`
  one-command gate の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_gate_threshold_provenance_test`
  gate row が `rigid_limit_threshold_source_command` と `rigid_limit_threshold_update_points` を stable に出すことを focused self-test で確認する。
- `make -C FEM4C coupled_2d_acceptance_gate_resilience_smoke`
  focused smoke bundle。`coupled_2d_acceptance_gate_test` と `coupled_2d_acceptance_resilience_checks_test` を順に回し、gate wrapper と resilience pack の current PASS surface を 1 コマンドで確認する。Run 1 では non-default 扱いで残す。
- `make -C FEM4C coupled_2d_acceptance_gate_resilience_smoke_test`
  focused gate+resilience smoke bundle の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_docs_sync_surfaces_test`
  docs sync validator の help/inventory surfaces を focused self-test として固定する。
- `make -C FEM4C coupled_2d_acceptance_docs_sync_surfaces_help_test`
  `make help` 上の docs sync surface target 群を focused self-test として固定する。
- `make -C FEM4C coupled_2d_acceptance_docs_sync_surface_smoke`
  focused docs-sync surface smoke bundle。`coupled_2d_acceptance_docs_sync_surfaces_help_test` と `coupled_2d_acceptance_docs_sync_surfaces_test` をまとめて回す。
- `make -C FEM4C coupled_2d_acceptance_docs_sync_surface_smoke_test`
  focused docs-sync surface smoke bundle の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_surface_checks`
  focused surface bundle。`coupled_2d_acceptance_docs_sync_surface_smoke_test`, `coupled_2d_acceptance_docs_sync_test`, `coupled_2d_acceptance_gate_test`, `coupled_2d_acceptance_gate_threshold_provenance_test` をまとめて回す。
- `make -C FEM4C coupled_2d_acceptance_surface_checks_test`
  focused surface bundle の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_lightweight_checks`
  lightweight acceptance pack。`coupled_2d_acceptance_contract_checks_test` と `coupled_2d_acceptance_surface_checks_test` をまとめて回す。
- `make -C FEM4C coupled_2d_acceptance_lightweight_checks_test`
  lightweight acceptance pack の PASS surface を self-test する。
- `make -C FEM4C ensure_fem4c_binary_test`
  compare child wrapper の stale-binary self-heal contract を確認する。`src/*.c|*.h` または `Makefile` が `bin/fem4c` より新しい場合は clean rebuild に入り、fresh binary では不要な clean rebuild を行わない。
- `make -C FEM4C coupled_2d_acceptance_wrapper_smoke`
  focused wrapper smoke pack。`coupled_2d_acceptance_lightweight_checks_test` と `ensure_fem4c_binary_test` をまとめて回す。
- `make -C FEM4C coupled_2d_acceptance_wrapper_smoke_test`
  focused wrapper smoke pack の PASS surface を self-test する。
- `make -C FEM4C coupled_2d_acceptance_compare_stage_integrators_stale_binary_test`
  compare-only subset rerun が stale `bin/fem4c` を self-heal することを top-level contract として確認する。
- `make -C FEM4C coupled_2d_acceptance_resilience_checks`
  focused resilience pack。`coupled_2d_acceptance_wrapper_smoke` と `coupled_2d_acceptance_compare_stage_integrators_stale_binary_test` をまとめて回す。
- `make -C FEM4C coupled_2d_acceptance_resilience_checks_test`
  focused resilience pack の PASS surface を self-test する。
- `make -C FEM4C coupled_rigid_limit_thresholds`
  threshold printer。rigid-limit temporary threshold の current source-of-truth 値を取得する。

### A-team current acceptance entrypoints
- `make -C FEM4C mbd_system2d_history_contract_smoke`
  history-only current command surface。generalized-force history の probe + CLI/system summary contract だけを bundle として確認する。
- `make -C FEM4C mbd_a_team_foundation_smoke`
  full foundation current command surface。history contract 再利用 bundle を含む rigid MBD foundation 全体を確認する。
- `make -C FEM4C mbd_run1_surface_docs_sync_test`
  focused self-test entrypoint。A-team history/foundation current command surface と Run 1 docs surface が `docs/10_review_spec_priority_plan.md` / `docs/team_runbook.md` / `docs/06_acceptance_matrix_2d.md` で同期していることを確認する。
- `bash FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh --print-a-team-surface-summary`
  focused inspection surface。A-team の history/foundation/self-test entrypoint と `review-plan / runbook / acceptance / handoff / queue` を 1 コマンドで機械可読に取得する。

### B-team current acceptance entrypoints
- `make -C FEM4C mbd_system2d_projection_compare_smoke`
  explicit / Newmark / HHT の constrained 2-link で projection 有効時の drift reduction を multi-step で確認する internal MBD contract。
- `make -C FEM4C mbd_system2d_projection_history_output_smoke`
  projection summary/history surface に `position_projection_*` と `position_projection_velocity_*` の rows/columns が残り、最終 step で residual 縮退が確認できることを固定する internal MBD contract。
- `make -C FEM4C mbd_system2d_projection_longrun_contract_smoke`
  `20 step` compare/history と isolated build の 4 本をまとめて回し、long-run drift reduction と snapshot-count stability をまとめて監査する B-team internal contract bundle。
- `make -C FEM4C mbd_output2d_history_field_count_sync_smoke`
  history CSV consumer/probe が `MBD_OUTPUT2D_HISTORY_FIELD_COUNT=31` を共通 source-of-truth として使い、stale literal を持ち込まないことを static に確認する。
- `make -C FEM4C mbd_output2d_rigid_compare_header_single_source_smoke`
  rigid_compare CSV header surface が `mbd_output2d_rigid_compare_header_csv()` の 1 箇所だけに残り、probe consumer が helper 経由で追従することを static に確認する。
- `make -C FEM4C mbd_output2d_rigid_compare_field_count_sync_smoke`
  rigid_compare CSV consumer/probe が `MBD_OUTPUT2D_RIGID_COMPARE_FIELD_COUNT=14` を共通 source-of-truth として使い、stale literal を持ち込まないことを static に確認する。
- `make -C FEM4C mbd_rigid_compare_review_columns_sync_smoke`
  rigid analytic review wrapper / guard script が `get_compare_2link_rigid_review_columns.sh` の 1 箇所を compare columns source-of-truth として使い、`--columns` の stale literal を持ち込まないことを static に確認する。
- `make -C FEM4C compare_2link_artifact_route_fields_getter_test`
  artifact route field getter が shell helper / Python consumer と同じ field 名列を返し、route metadata field-name drift を早期に検知する。
- `make -C FEM4C compare_2link_artifact_route_fields_sync_test`
  rigid/flex artifact route row consumer が `compare_2link_artifact_route_fields.sh` の 1 箇所を source-of-truth として使い、route metadata row の stale literal を持ち込まないことを static に確認する。
- `make -C FEM4C compare_2link_artifact_targets_getter_test`
  compare artifact target getter が shell helper / Python consumer と同じ target order を返し、validator / wrapper の target literal drift を早期に検知する。
- `make -C FEM4C compare_2link_artifact_targets_sync_test`
  compare artifact target order は `compare_2link_artifact_targets.sh` を source-of-truth に使い、manifest validator / core suite / helper consumer が raw target tuple を持ち込まないことを static に確認する。
- `make -C FEM4C compare_2link_artifact_integrators_getter_test`
  compare artifact integrator getter が shell helper / Python consumer と同じ default/subset integrator order を返し、matrix helper drift を早期に検知する。
- `make -C FEM4C compare_2link_artifact_integrators_sync_test`
  compare artifact default / subset integrator order は `compare_2link_artifact_integrators.sh` を source-of-truth に使い、matrix/core wrapper/rigid+flex self-test が raw integrator literal を持ち込まないことを static に確認する。
- `make -C FEM4C compare_2link_artifact_checks`
  compare artifact suite の focused self-test bundle。target/integrator helper、manifest contract、matrix subset、unsupported integrator を fail-fast で reject する契約をまとめて監査する。
- `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_test`
  Run 1 docs-sync validator 自身の help / inventory / count / current-command audit / doc-source / invalid-option surface を固定する focused self-test。`--print-doc-sources` と `--print-a-team-surface-summary` の inspection surface もここで監査する。
- `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_help_test`
  Run 1 docs-sync validator の help target が `make help` から消えていないことを focused に確認する。
- `make -C FEM4C mbd_run1_surface_docs_sync_surface_smoke`
  Run 1 docs-sync validator の main validator / help surface / validator surface bundle を current command surface として 1 コマンドで再確認する。
- `make -C FEM4C mbd_run1_surface_docs_sync_surface_smoke_test`
  `mbd_run1_surface_docs_sync_surface_smoke` bundle target 自体を固定する focused self-test。
- `make -C FEM4C mbd_b_team_foundation_isolated_smoke`
  fresh local `bin/` / `build/` / `parser/` から B-team foundation smoke を再構築し、CLI rebuild 契約と long-run projection contract が shared build state に依存しないことを確認する internal MBD contract。

## 5. セッション運用ルール
- PMチャットが「作業してください」のみの場合、追加確認なしで `docs/abc_team_chat_handoff.md` Section 0 と `docs/fem4c_team_next_queue.md` の先頭 `In Progress` / `Todo` から着手する。
- PM/ユーザーのチャットは原則起動トリガーのみとし、短時間ラン是正、超過ラン是正、再開点、禁止コマンド、優先度変更は `docs/fem4c_team_next_queue.md` の `PM運用メモ` を正本とする。
- ユーザーから Codex への通常トリガーは `確認してください` とする。特記が無ければ、Codex は control tower と最新 docs を確認し、受理/差し戻し/次アクションを自走で整理する。
- `確認してください` は全チーム終了後に限定しない。終了済みチームが 1 つでもあれば部分確認を許可し、稼働中チームは `RUNNING` / `READY_TO_WRAP` / `ACTIVE_UNCONFIRMED` の状態に応じて扱う。
- 各チームは `python3 tools/team_timer/team_timer.py start` 実行前に `docs/fem4c_team_next_queue.md` の `PM運用メモ` を確認する。
- `PM運用メモ` に個別注意が追記された場合、次セッションから追加チャット無しで自動適用する。
- blocker 以外の問い合わせは禁止する。
- 開始時に `python3 tools/team_timer/team_timer.py start <team_tag>` を実行する。
- `start` 後 10 分以内に `python3 tools/team_timer/team_timer.py declare <session_token> <primary_task> <secondary_task> ["plan_note"]` を実行し、`SESSION_TIMER_DECLARE` を `docs/team_status.md` に転記する。
- `start` 後 20 分以内と 40 分以降に `python3 tools/team_timer/team_timer.py progress <session_token> <current_task> <work_kind> ["progress_note"]` を実行し、`SESSION_TIMER_PROGRESS` を `docs/team_status.md` に転記する。
- 中間証跡として `python3 tools/team_timer/team_timer.py guard <token> 10`, `20`, `30`, `60` を記録する。
- 終了時に `python3 tools/team_timer/team_timer.py end <token>` を実行し、出力を `docs/team_status.md` に原文転記する。
- 旧 `scripts/session_timer*.sh` は互換ラッパーであり、今後の正本運用では使用しない。
- 1セッションは `60 <= elapsed_min <= 90` を基本レンジとし、最低 `elapsed_min >= 60` を必須とする。
- 60分は開発前進に使う。実装系ファイル差分を毎セッション必須とし、primary task 完了後も `guard60` まで secondary/Auto-Next へ進める。
- `guard60=pass` 前の非 blocker 中間報告は禁止する。途中共有は `SESSION_TIMER_PROGRESS` と git 差分で行い、PM/ユーザーへの通常報告は `guard60=pass` 後にまとめる。
- `この token のまま継続します` という自己申告だけでは継続とみなさない。次の `guard` または `SESSION_TIMER_PROGRESS` が無く応答が途切れた run は stale 扱いとする。
- 60-90分ラン安定化を最優先とする間は、PM/ユーザーは active run 中の通常進捗問い合わせを行わない。PM は `python3 tools/team_timer/team_control_tower.py` を監視に使い、各チームは `guard60=pass` 後または blocker 時のみチャット応答する。
- active run 中に `確認してください` が来ても、各チームは blocker / destructive conflict / data loss risk 以外では応答しない。
- 各チームは 1 セッションあたり `primary_task 1件 + secondary_task 1件` を上限目安とする。secondary を明示せずに 3件目へ拡張しない。
- docs単独更新での完了報告は禁止する。
- 先頭タスク完了後は同一セッションで次タスクへ進む。
- 次タスク候補が無い場合は、同一スコープで `Auto-Next` を `Goal / Scope / Acceptance` 付きで `docs/fem4c_team_next_queue.md` に追記して継続する。
- 同一コマンド反復、長時間ソーク、時間稼ぎ目的の検証は禁止する。
- 検証は今回変更した実装に直結する短時間スモークを原則とする。
- `sleep` 等の人工待機は禁止する。
- D/E チームは `docs/team_status.md` に見出しが無ければ自分で `## Dチーム` / `## Eチーム` を追加してよい。

## 5A. PM監視コマンド
- one-shot 監視:
  - `python3 tools/team_timer/team_control_tower.py`
- 連続監視（スナップショットを `/tmp` に出力）:
  - `bash scripts/watch_team_control_tower.sh 60 /tmp/team_control_tower_snapshot.md`
- 監視結果の見方:
- `ACTIVE_UNCONFIRMED`: `start` はあるが recent guard heartbeat が無く、実稼働を断定できない。継続中なら `guard` を記録させ、停止済みなら `end`/報告を回収する。
- `PLAN_MISSING`: `start` はあるが 10 分以内の `SESSION_TIMER_DECLARE` が無い。継続中なら即 `primary/secondary task` を宣言させ、停止済みなら stale 扱いで同一タスクをやり直す。
- `PROGRESS_MISSING`: `SESSION_TIMER_DECLARE` はあるが、20 分時点までに `SESSION_TIMER_PROGRESS` が無い。継続中なら progress heartbeat を残し、停止済みなら short stale run と同等に扱う。
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
- `scripts/run_team_acceptance_gate.sh` は次ランから `SESSION_TIMER_DECLARE` と `SESSION_TIMER_PROGRESS` を既定で要求する。旧 latest entry が fail しても、新規ランで declare/progress 付き報告へ置き換える。

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
- repo root wrapper は `contract_audit_target=`, `contract_audit_mode=`, `contract_audit_log_path=`, `contract_audit_cache_log=`, `contract_audit_result=` を必ず出力する。
- contract audit 提出ログ validator は `python3 scripts/check_coupled_compare_reason_code_contract_audit_report.py <audit_report_log>` で `contract_audit_target=`, `contract_audit_mode=`, `contract_audit_log_path=`, `contract_audit_cache_log=`, `contract_audit_result=` を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_contract_audit_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_contract_audit_report_test` を使ってよい。
- `make -C FEM4C coupled_compare_reason_code_contract_checks` は audit cache helper と contract audit report validator を bundle に含める。
- repo root から bundle + wrapper modes をまとめて確認する場合は `bash scripts/run_coupled_compare_reason_code_contract_stack.sh [out_dir]` を使ってよい。
- repo root stack wrapper は `contract_stack_components=`, `contract_stack_out_dir=`, `contract_stack_bundle_log=`, `contract_stack_audit_modes_log=`, `contract_stack_contract_report_log=`, `contract_stack_result=` を必ず出力する。
- PM 向けの最上位集約は `bash scripts/run_coupled_compare_reason_code_pm_surface.sh [out_dir]` を使ってよい。
- PM surface wrapper は `pm_surface_components=`, `pm_surface_out_dir=`, `pm_surface_fem4c_log=`, `pm_surface_audit_modes_log=`, `pm_surface_stack_modes_log=`, `pm_surface_contract_report_log=`, `pm_surface_result=` を必ず出力する。
- root entrypoint 群の回帰は `bash scripts/test_run_coupled_compare_reason_code_root_modes.sh` を使ってよい。
- root mode wrapper は `root_modes_components=`, `root_modes_out_dir=`, `root_modes_audit_log=`, `root_modes_stack_log=`, `root_modes_pm_surface_log=`, `root_modes_pm_surface_contract_log=`, `root_modes_pm_surface_contract_report_log=`, `root_modes_result=` を必ず出力する。
- repo root の最上位 wrapper は `bash scripts/run_coupled_compare_reason_code_root_surface.sh [out_dir]` を使ってよい。
- root surface wrapper は `root_surface_components=`, `root_surface_out_dir=`, `root_surface_pm_surface_log=`, `root_surface_root_modes_log=`, `root_surface_contract_report_log=`, `root_surface_root_modes_contract_report_log=`, `root_surface_result=` を必ず出力する。
- 提出ログの transitive 整合確認は `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` を使ってよい。
- validator の required key 群は `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py --print-required-keys` で機械可読に取得できる。
- FEM4C 側の focused bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` を使ってよい。
- repo root の audited entrypoint は `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh [out_dir]` を使ってよい。
- root surface audit wrapper は `root_surface_audit_components=`, `root_surface_audit_out_dir=`, `root_surface_audit_log=`, `root_surface_audit_contract_report_log=`, `root_surface_audit_result=` を必ず出力する。
- root-surface audit 提出ログ validator は `python3 scripts/check_coupled_compare_reason_code_root_surface_audit_report.py <audit_report_log>` で `root_surface_audit_components=`, `root_surface_audit_out_dir=`, `root_surface_audit_log=`, `root_surface_audit_contract_report_log=`, `root_surface_audit_result=` を検証し、audit log / contract report log の欠落・不一致・親dir外参照を fail-fast する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_audit_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_audit_report_test` を使ってよい。
- audited entrypoint と validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_root_surface_audit_surface.sh [out_dir]` を使ってよい。
- root surface audit surface wrapper は `root_surface_audit_surface_components=`, `root_surface_audit_surface_out_dir=`, `root_surface_audit_surface_report_log=`, `root_surface_audit_surface_validator_log=`, `root_surface_audit_surface_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_audit_surface_test` を使ってよい。
- `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` の bundle log からも `root_surface_audit_surface_*` metadata を追跡できるようにしておく。
- bundle と bundle-log validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface.sh [out_dir]` を使ってよい。
- root surface contract bundle surface wrapper は `root_surface_contract_bundle_surface_components=`, `root_surface_contract_bundle_surface_out_dir=`, `root_surface_contract_bundle_surface_bundle_log=`, `root_surface_contract_bundle_surface_validator_log=`, `root_surface_contract_bundle_surface_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_test` を使ってよい。
- saved surface log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.py <surface_log>` を使ってよい。
- surface-log validator は `root_surface_contract_bundle_surface_components=`, `root_surface_contract_bundle_surface_out_dir=`, `root_surface_contract_bundle_surface_bundle_log=`, `root_surface_contract_bundle_surface_validator_log=`, `root_surface_contract_bundle_surface_result=` と validator handoff を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_report_test` を使ってよい。
- surface wrapper と surface-log validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh [out_dir]` を使ってよい。
- root surface contract bundle surface report wrapper は `root_surface_contract_bundle_surface_report_components=`, `root_surface_contract_bundle_surface_report_out_dir=`, `root_surface_contract_bundle_surface_report_surface_log=`, `root_surface_contract_bundle_surface_report_validator_log=`, `root_surface_contract_bundle_surface_report_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_test` を使ってよい。
- saved report wrapper log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.py <report_wrapper_log>` を使ってよい。
- report-wrapper validator は `root_surface_contract_bundle_surface_report_components=`, `root_surface_contract_bundle_surface_report_out_dir=`, `root_surface_contract_bundle_surface_report_surface_log=`, `root_surface_contract_bundle_surface_report_validator_log=`, `root_surface_contract_bundle_surface_report_result=` と validator handoff を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_test` を使ってよい。
- report wrapper と wrapper-report validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh [out_dir]` を使ってよい。
- root surface contract bundle surface wrapper-report wrapper は `root_surface_contract_bundle_surface_wrapper_report_components=`, `root_surface_contract_bundle_surface_wrapper_report_out_dir=`, `root_surface_contract_bundle_surface_wrapper_report_log=`, `root_surface_contract_bundle_surface_wrapper_report_validator_log=`, `root_surface_contract_bundle_surface_wrapper_report_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_test` を使ってよい。
- saved wrapper-surface log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.py <wrapper_surface_log>` を使ってよい。
- wrapper-surface validator は `root_surface_contract_bundle_surface_wrapper_report_components=`, `root_surface_contract_bundle_surface_wrapper_report_out_dir=`, `root_surface_contract_bundle_surface_wrapper_report_log=`, `root_surface_contract_bundle_surface_wrapper_report_validator_log=`, `root_surface_contract_bundle_surface_wrapper_report_result=` と nested wrapper/validator handoff を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_test` を使ってよい。
- focused bundle log を保存後に再検証する場合は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_report.py <bundle_log>` を使ってよい。
- bundle-log validator は `root_surface_audit_surface_components=`, `root_surface_audit_surface_out_dir=`, `root_surface_audit_surface_report_log=`, `root_surface_audit_surface_validator_log=`, `root_surface_audit_surface_result=` と required pass lines を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_report_test` を使ってよい。
- root surface contract bundle の repo root 監査は `bash scripts/run_coupled_compare_reason_code_root_surface_contract_audit.sh [log_path]` を使ってよい。
- root surface contract audit wrapper は `root_surface_contract_audit_target=`, `root_surface_contract_audit_mode=`, `root_surface_contract_audit_log_path=`, `root_surface_contract_audit_cache_log=`, `root_surface_contract_audit_result=` を必ず出力する。
- focused root-surface contract audit 提出ログ validator は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py <audit_report_log>` で `root_surface_contract_audit_target=`, `root_surface_contract_audit_mode=`, `root_surface_contract_audit_log_path=`, `root_surface_contract_audit_cache_log=`, `root_surface_contract_audit_result=` を検証する。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py --print-required-keys` で機械可読に取得できる。
- `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface.sh [out_dir]` は `COUPLED_COMPARE_SKIP_NESTED_SELFTESTS=1` を受け取り、nested wrapper 経路では self-test を再帰実行せずに契約上の pass lines を保存できる。
- skip モードの focused 回帰は `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_skip_nested_selftests.sh` を使ってよい。
- make 経由では `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_skip_nested_selftests_test` を使ってよい。
- stack 全体の skip 伝播確認には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_test` を使ってよい。
- stack 全体の skip 伝播を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests.sh [out_dir]` を使ってよい。
- skip wrapper は `skip_nested_selftests_components=`, `skip_nested_selftests_out_dir=`, `skip_nested_selftests_pm_surface_log=`, `skip_nested_selftests_root_modes_log=`, `skip_nested_selftests_root_surface_log=`, `skip_nested_selftests_root_surface_contract_bundle_surface_log=`, `skip_nested_selftests_result=` を必ず出力する。
- saved skip wrapper log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_report.py <wrapper_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_report_test` を使ってよい。
- skip wrapper と validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_report.sh [out_dir]` を使ってよい。
- skip wrapper/report surface は `skip_nested_selftests_report_components=`, `skip_nested_selftests_report_out_dir=`, `skip_nested_selftests_report_log=`, `skip_nested_selftests_report_validator_log=`, `skip_nested_selftests_report_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_surface_test` を使ってよい。
- saved skip wrapper/report log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_surface_report.py <surface_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_surface_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_surface_report_test` を使ってよい。
- skip wrapper/report と validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh [out_dir]` を使ってよい。
- skip wrapper/report surface wrapper は `skip_nested_selftests_surface_report_components=`, `skip_nested_selftests_surface_report_out_dir=`, `skip_nested_selftests_surface_report_log=`, `skip_nested_selftests_surface_report_validator_log=`, `skip_nested_selftests_surface_report_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_test` を使ってよい。
- saved skip wrapper/report surface log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.py <wrapper_surface_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_test` を使ってよい。
- skip wrapper-surface report と validator を repo root 1 コマンドで束ねる場合は `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh [out_dir]` を使ってよい。
- skip wrapper-surface report wrapper は `skip_nested_selftests_wrapper_surface_report_components=`, `skip_nested_selftests_wrapper_surface_report_out_dir=`, `skip_nested_selftests_wrapper_surface_report_log=`, `skip_nested_selftests_wrapper_surface_report_validator_log=`, `skip_nested_selftests_wrapper_surface_report_result=` を必ず出力する。
- wrapper の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_test` を使ってよい。
- saved skip wrapper-surface report log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.py <wrapper_surface_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_test` を使ってよい。
- skip-nested-selftests chain 全体の focused bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks` を使ってよい。
- repo-root wrapper から再帰なしで呼ぶ場合は `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_core` を使ってよい。
- bundle の self-test には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_test` を使ってよい。
- help surface の self-test には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_help_test` を使ってよい。
- repo-root 1 コマンド wrapper には `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks.sh [out_dir]` を使ってよい。
- contract bundle wrapper は `skip_nested_selftests_contract_checks_out_dir=`, `skip_nested_selftests_contract_checks_log=`, `skip_nested_selftests_contract_checks_result=` を必ず出力する。
- wrapper の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_bundle_test` を使ってよい。
- saved contract bundle wrapper log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.py <wrapper_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_test` を使ってよい。
- contract bundle report wrapper には `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.sh [out_dir]` を使ってよい。
- contract bundle report wrapper は `skip_nested_selftests_contract_checks_report_components=`, `skip_nested_selftests_contract_checks_report_out_dir=`, `skip_nested_selftests_contract_checks_report_log=`, `skip_nested_selftests_contract_checks_report_validator_log=`, `skip_nested_selftests_contract_checks_report_result=` を必ず出力する。
- wrapper の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_test` を使ってよい。
- saved contract bundle report wrapper log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report.py <wrapper_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_report_test` を使ってよい。
- contract bundle report wrapper-surface には `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.sh [out_dir]` を使ってよい。
- contract bundle report wrapper-surface は `skip_nested_selftests_contract_checks_wrapper_surface_report_components=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_out_dir=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_validator_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_result=` を必ず出力する。
- wrapper の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_test` を使ってよい。
- saved contract bundle report wrapper-surface log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.py <wrapper_surface_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_report_test` を使ってよい。
- contract bundle report wrapper-surface wrapper には `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.sh [out_dir]` を使ってよい。
- contract bundle report wrapper-surface wrapper は `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_components=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_out_dir=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_validator_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_result=` を必ず出力する。
- wrapper の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_test` を使ってよい。
- saved contract bundle report wrapper-surface log を再検証する場合は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.py <wrapper_surface_log>` を使ってよい。
- validator の required key / pass-line 契約は `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.py --print-required-keys` で機械可読に取得できる。
- validator の self-test bundle には `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_report_test` を使ってよい。
- validator の self-test bundle は `make -C FEM4C coupled_compare_reason_code_root_surface_contract_audit_report_test` を使ってよい。
- shared audit cache helper の self-test は `make -C FEM4C coupled_compare_reason_code_audit_cache_test` を使ってよい。
- `make -C FEM4C coupled_compare_reason_code_contract_checks` と `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` は audit cache helper を bundle に含める。
- `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` は root surface / audit / audited surface / contract audit の focused 回帰をまとめて回す。
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
