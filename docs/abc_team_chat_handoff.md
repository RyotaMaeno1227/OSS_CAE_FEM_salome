# A/B/C/D/E チームチャット向け引き継ぎメモ

作成日: 2026-03-06
現行運用: FEM4C 2D 2-link flexible roadmap

---

## 0. 現行ディスパッチ（2026-03-06）
この節だけを見れば再開できるようにする。PMチャットが「作業してください」のみの場合も、この節と `docs/fem4c_team_next_queue.md` を正本として動く。

### 共通参照順
1. `docs/long_term_target_definition.md`
2. `docs/04_2d_coupled_scope.md`
3. `docs/05_module_ownership_2d.md`
4. `docs/06_acceptance_matrix_2d.md`
5. `docs/07_input_spec_coupled_2d.md`
6. `docs/08_merge_order_2d.md`
7. `docs/09_compare_schema_2d.md`
8. `FEM4C/fem4c_2link_flexible_detailed_todo.md`
9. `FEM4C/fem4c_codex_team_prompt_pack.md`
10. `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md`
11. `docs/10_review_spec_priority_plan.md`
12. `docs/fem4c_team_next_queue.md`
13. `docs/team_status.md`
14. `docs/session_continuity_log.md`
15. `AGENTS.md`

### 共通ルール
- 旧 `A-59 / B-45 / C-59` 系は凍結済み。旧資料は `oldFile/docs/archive/roadmap_reset_2026-03-06/` を参照のみ。
- 参照する Project Chrono は `third_party/chrono/chrono-main` のみ。
- 1セッションは 60-90分を基本とし、最低 60分。実装系ファイル差分を必須とする。
- 同一コマンド反復や時間稼ぎ目的の重い回帰は禁止。
- `python3 tools/team_timer/team_timer.py start <team_tag>` / `python3 tools/team_timer/team_timer.py guard <token> 10|20|30|60` / `python3 tools/team_timer/team_timer.py end <token>` を必ず使う。
- `start` 後 10 分以内に `python3 tools/team_timer/team_timer.py declare <token> <primary_task> <secondary_task> ["plan_note"]` を実行し、`SESSION_TIMER_DECLARE` を `docs/team_status.md` に残す。
- `start` 後 20 分以内と 40 分以降に `python3 tools/team_timer/team_timer.py progress <token> <current_task> <work_kind> ["progress_note"]` を実行し、`SESSION_TIMER_PROGRESS` を `docs/team_status.md` に残す。
- 旧 `scripts/session_timer*.sh` は互換ラッパーであり、正本運用では使わない。
- `guard60=pass` 前に PM/ユーザーへ送ってよいのは blocker 報告だけとする。通常の進捗共有は `SESSION_TIMER_PROGRESS` とローカル差分で行い、中間報告チャットは送らない。
- `この token のまま継続します` と書いても、実際の次 guard/progress が残らなければ停止扱いになる。チャット応答が終わった run は stale とみなし、次回は同一タスクを新規 `session_token` で再開する。
- 60-90分ラン安定化を最優先とする間は、PM/ユーザーは active run 中の通常進捗問い合わせを行わない。確認は `python3 tools/team_timer/team_control_tower.py` を使い、各チームは `guard60=pass` 後または blocker 時だけ応答する。
- active run 中に `確認してください` が来ても、各チームは blocker / destructive conflict / data loss risk 以外ではチャット応答しない。
- 各チームは 1 セッションで `primary_task 1件 + secondary_task 1件` を上限目安とし、secondary 未定義のまま 3件目へ広げない。
- PMからの追加連絡、短時間ラン是正、超過ラン是正、再開点、禁止コマンドは、原則チャットではなく `docs/fem4c_team_next_queue.md` の `PM運用メモ` を正本とする。
- review-spec 採用期間中は `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md` と `docs/10_review_spec_priority_plan.md` を queue より上位の優先文書として扱う。
- 各チームはセッション開始前に `docs/fem4c_team_next_queue.md` の `PM運用メモ` を必ず確認する。
- 監視上 `STALE_NO_GUARD` / `STALE_BEFORE_60` と判定されたセッションは短時間停止ランとして無効とし、queue を進めず同一タスクを新規 `session_token` でやり直す。
- `start` から 10 分を超えても `SESSION_TIMER_DECLARE` が無い session は `PLAN_MISSING` として扱う。停止済みなら stale 扱い、継続中なら即 `primary/secondary` を宣言する。
- `start` から 12 分以内に guard が無い run、または最後の guard/heartbeat から 12 分以上更新が無いまま `elapsed_min < 60` の run は short stale run とみなす。
- PM は `python3 tools/team_timer/team_control_tower.py` で one-shot 監視し、必要なら `bash scripts/watch_team_control_tower.sh 60 /tmp/team_control_tower_snapshot.md` で連続監視する。
- ユーザーから Codex への基本トリガーは `確認してください` とする。特記が無ければ、control tower / `docs/team_status.md` / `docs/session_continuity_log.md` / `docs/fem4c_team_next_queue.md` を確認し、受理判定と次アクション整理を行う。
- 5チーム全員の終了を待つ必要はない。3-4チームが終了し、残りが `RUNNING` / `READY_TO_WRAP` / `ACTIVE_UNCONFIRMED` の段階でも、ユーザーは `確認してください` を送ってよい。
- `確認してください` を受けた PM は、終了済みチームから先に受理/差し戻しを処理し、稼働中チームは継続中として扱う。
- `ACTIVE_UNCONFIRMED` のチームについてユーザーが停止済みと確認した場合は、旧 session を stale 扱いとして破棄し、queue 先頭タスクを新規 `session_token` で再開させる。
- 終了時は `docs/team_status.md` と `docs/session_continuity_log.md` を更新する。
- `D/E` チームは `docs/team_status.md` に見出しが無ければ自分で `## Dチーム` / `## Eチーム` を追加してよい。
- RecurDyn / AdamsFlex の実データはまだ無い。今は compare schema 固定が優先で、実データ未投入は blocker にしない。

### 省略指示モード既定文
- PMチャットが `作業してください` のみなら、各チームは追加確認なしで以下を適用する。
- review-spec 採用期間中は、older open task より `docs/10_review_spec_priority_plan.md` の Run 1 -> Run 2 -> Run 3 を優先する。
- 個別の是正事項や注意事項は、追加チャットが無くても `docs/fem4c_team_next_queue.md` の `PM運用メモ` を自動適用する。
- A:
  - `A-01` 以降の先頭 `In Progress` / `Todo` から着手する。
  - `A-01` 系の legacy 継続ではなく、`body2d` / `forces2d` / explicit 側の roadmap を前進させる。
- B:
  - 旧 `B-45/B-46` は凍結済み。新ロードマップの `B-01` 以降から着手する。
  - `system2d` / KKT / Newmark / HHT 側を前進させる。
- C:
  - 旧 `C-59/C-60` は凍結済み。新ロードマップの `C-01` 以降から着手する。
  - build recovery と FEM API / full reassembly 側を前進させる。
- D:
  - `D-01` 以降の先頭 `In Progress` / `Todo` から着手する。
  - flexible body wrapper / reaction / 2-link flex 側を前進させる。
- E:
  - `E-01` 以降の先頭 `In Progress` / `Todo` から着手する。
  - `runner.c` 縮退 / parser / orchestration / compare 側を前進させる。
- 共通終了条件:
  - `guard10/20/30/60` を記録する。
  - `guard60=pass` 後のみ終了報告する。
- 先頭タスクが早く終わった場合は、同一セッションで次タスクへ自動遷移する。
- `guard60` は token を開いたまま待つ意味ではない。60分時点まで secondary/Auto-Next へ進めて継続作業する。
  - docs単独更新、同一コマンド反復、時間稼ぎ目的の重い回帰は禁止する。

### PMチーム
- ミッション: スコープ、責務、受入、入力仕様、比較 schema、マージ順を固定する。
- 先頭タスク: `PM-01`
- 主文書:
  - `docs/04_2d_coupled_scope.md`
  - `docs/05_module_ownership_2d.md`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/07_input_spec_coupled_2d.md`
  - `docs/08_merge_order_2d.md`
  - `docs/09_compare_schema_2d.md`
- 報告先:
  - `docs/team_status.md` の `## PMチーム`
  - `docs/session_continuity_log.md`

### Aチーム
- ミッション: Body / Forces / Explicit / Kinematics を実装する。
- current restart point:
  - `docs/fem4c_team_next_queue.md` の current A-task を正本とする。
- current acceptance entrypoints:
  - `make -C FEM4C mbd_system2d_history_contract_smoke`
    history-only current command surface。generalized-force history の probe + CLI/system summary contract だけを bundle として確認する。
  - `make -C FEM4C mbd_a_team_foundation_smoke`
    full foundation current command surface。history contract 再利用 bundle を含む rigid MBD foundation 全体を確認する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test`
    focused self-test entrypoint。Run 1 review spec / runbook / acceptance matrix / handoff の A-team surface が上の 2 コマンドと矛盾していないことを確認する。
  - `bash FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh --print-a-team-surface-summary`
    focused inspection surface。A-team の history/foundation/self-test entrypoint と `review-plan / runbook / acceptance / handoff / queue` を 1 コマンドで機械可読に取得する。
- 主ファイル:
  - `FEM4C/src/mbd/body2d.*`
  - `FEM4C/src/mbd/forces2d.*`
  - `FEM4C/src/mbd/kinematics2d.*`
  - `FEM4C/src/mbd/integrator_explicit2d.*`
  - `FEM4C/src/mbd/output2d.*`
- 報告先:
  - `docs/team_status.md` の `## Aチーム`
  - `docs/session_continuity_log.md`

### Bチーム
- ミッション: Constraint / KKT / Newmark / HHT を実装する。
- 先頭タスク: `B-01`
- 主ファイル:
  - `FEM4C/src/mbd/system2d.*`
  - `FEM4C/src/mbd/assembler2d.*`
  - `FEM4C/src/mbd/constraint2d.*`
  - `FEM4C/src/mbd/linear_solver_dense.*`
  - `FEM4C/src/mbd/integrator_newmark2d.*`
  - `FEM4C/src/mbd/integrator_hht2d.*`
  - `FEM4C/src/mbd/projection2d.*`
- 報告先:
  - `docs/team_status.md` の `## Bチーム`
  - `docs/session_continuity_log.md`

### Cチーム
- ミッション: FEM API / full reassembly / nodeset / snapshot を実装する。
- 先頭タスク: `C-01`
- 主ファイル:
  - `FEM4C/src/coupled/fem_model_copy.*`
  - `FEM4C/src/coupled/flex_solver2d.*`
  - `FEM4C/src/coupled/flex_bc2d.*`
  - `FEM4C/src/coupled/flex_nodeset.*`
  - `FEM4C/src/coupled/flex_snapshot2d.*`
  - `FEM4C/src/elements/t6/t6_element.c`
- 報告先:
  - `docs/team_status.md` の `## Cチーム`
  - `docs/session_continuity_log.md`

### Dチーム
- ミッション: flexible body wrapper / reaction / 2-link flex を実装する。
- 先頭タスク: `D-01`
- 主ファイル:
  - `FEM4C/src/coupled/flex_body2d.*`
  - `FEM4C/src/coupled/flex_reaction2d.*`
  - `FEM4C/src/coupled/case2d.*`
- 報告先:
  - `docs/team_status.md` の `## Dチーム`
  - `docs/session_continuity_log.md`

### Eチーム
- ミッション: system orchestration / parser / regression / compare を実装する。
- 先頭タスク: `E-01`
- 主ファイル:
  - `FEM4C/src/analysis/runner.c`
  - `FEM4C/src/mbd/system2d.*`
  - `FEM4C/src/coupled/coupled_step_*2d.*`
  - `FEM4C/src/coupled/coupled_run2d.*`
  - `FEM4C/examples/*`
  - `scripts/compare_*.py`
- 報告先:
  - `docs/team_status.md` の `## Eチーム`
  - `docs/session_continuity_log.md`
