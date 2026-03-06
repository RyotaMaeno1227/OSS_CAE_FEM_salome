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
10. `docs/fem4c_team_next_queue.md`
11. `docs/team_status.md`
12. `docs/session_continuity_log.md`

### 共通ルール
- 旧 `A-59 / B-45 / C-59` 系は凍結済み。旧資料は `oldFile/docs/archive/roadmap_reset_2026-03-06/` を参照のみ。
- 参照する Project Chrono は `third_party/chrono/chrono-main` のみ。
- 1セッションは 60-90分を基本とし、最低 60分。実装系ファイル差分を必須とする。
- 同一コマンド反復や時間稼ぎ目的の重い回帰は禁止。
- `scripts/session_timer.sh start <team_tag>` / `bash scripts/session_timer_guard.sh <token> 10|20|30|60` / `scripts/session_timer.sh end <token>` を必ず使う。
- PMからの追加連絡、短時間ラン是正、超過ラン是正、再開点、禁止コマンドは、原則チャットではなく `docs/fem4c_team_next_queue.md` の `PM運用メモ` を正本とする。
- 各チームはセッション開始前に `docs/fem4c_team_next_queue.md` の `PM運用メモ` を必ず確認する。
- PM は `python scripts/team_control_tower.py` で one-shot 監視し、必要なら `bash scripts/watch_team_control_tower.sh 60 /tmp/team_control_tower_snapshot.md` で連続監視する。
- ユーザーから Codex への基本トリガーは `確認してください` とする。特記が無ければ、control tower / `docs/team_status.md` / `docs/session_continuity_log.md` / `docs/fem4c_team_next_queue.md` を確認し、受理判定と次アクション整理を行う。
- 終了時は `docs/team_status.md` と `docs/session_continuity_log.md` を更新する。
- `D/E` チームは `docs/team_status.md` に見出しが無ければ自分で `## Dチーム` / `## Eチーム` を追加してよい。
- RecurDyn / AdamsFlex の実データはまだ無い。今は compare schema 固定が優先で、実データ未投入は blocker にしない。

### 省略指示モード既定文
- PMチャットが `作業してください` のみなら、各チームは追加確認なしで以下を適用する。
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
- 先頭タスク: `A-01`
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
