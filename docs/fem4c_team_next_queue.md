# FEM4C Team Next Queue

更新日: 2026-03-08（A/B current short stale 整理 / C-49継続 / D-21開始可 / E-14開始可）
用途: チャットで「作業してください」のみが来た場合の、PM/A/B/C/D/E 共通の次タスク起点。

## 0. 路線切替
- 旧 A/B/C の `A-59` / `B-45` / `C-59` 系タスクは凍結する。
- 凍結前の active docs は以下へ退避した。
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/fem4c_team_next_queue_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/abc_team_chat_handoff_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/team_runbook_legacy_2026-03-06.md`
- 現在の正本ロードマップは以下とする。
  - `FEM4C/fem4c_2link_flexible_detailed_todo.md`
  - `FEM4C/fem4c_codex_team_prompt_pack.md`
  - `docs/04_2d_coupled_scope.md`
  - `docs/05_module_ownership_2d.md`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/07_input_spec_coupled_2d.md`
  - `docs/08_merge_order_2d.md`
  - `docs/09_compare_schema_2d.md`

## 1. PM固定決定（2026-03-06）
- 対象モデルは `2-link planar mechanism` に固定する。
- 両リンク flexible を最終 target とする。
- FEM は step / iteration ごとに full mesh 再アセンブルする。
- MBD は explicit / Newmark-beta / HHT-alpha を実装対象とする。
- RecurDyn / AdamsFlex の実データは未投入のため、現時点では compare CSV schema の固定を先行する。
- 実データ未投入は M0-M3 の blocker にしない。M4 で数値比較を必須化する。
- Project Chrono の参照元は `third_party/chrono/chrono-main` のみとする。

## 2. 共通ルール
- 開始時に `scripts/session_timer.sh start <team_tag>` を実行する。
- `start` 後 10 分以内に `scripts/session_timer_declare.sh <session_token> <primary_task> <secondary_task> ["plan_note"]` を実行し、`SESSION_TIMER_DECLARE` を `docs/team_status.md` へ転記する。
- 報告前に `bash scripts/session_timer_guard.sh <session_token> 10`, `20`, `30`, `60` を記録する。
- 終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力を `docs/team_status.md` に転記する。
- 1セッションは 60分以上を必須とし、60-90分を推奨レンジとする。
- 60分は開発前進に使う。実装系ファイル差分を毎セッション必須とする。
- 同一コマンド反復や長時間ソークで時間を消費しない。
- 先頭タスク完了後は、同一セッション内で次タスクへ自動遷移する。
- `docs/team_status.md` と `docs/session_continuity_log.md` を必ず更新する。
- `docs/team_status.md` に `## Dチーム` / `## Eチーム` が無ければ、D/Eチームが最初の報告時に見出しを作成してよい。

## 2A. PM運用メモ（チャット最小化用）
- この節は、PM/ユーザーが各チームへ個別チャットを増やさずに運用是正を伝えるための正本である。
- 各チームは毎セッション開始前にこの節を確認し、追加チャットが無くてもここに書かれた内容を自動適用する。
- ここに書く内容は、短時間ラン是正、超過ラン是正、差し戻し、禁止コマンド、再開点、優先度変更に限定する。
- ここに未記載の一般ルールは `## 2. 共通ルール` と `docs/abc_team_chat_handoff.md` Section 0 に従う。
- ユーザーから Codex への通常依頼キーワードは `確認してください`、ユーザーから各チームへの通常依頼キーワードは `作業してください` とする。
- 5チーム全員の終了を待たず、終了済みチームが出た時点で `確認してください` を送ってよい。Codex は終了済みチームから先に判定し、稼働中チームは継続中として扱う。
- 現在の常設注意:
  1. `elapsed_min < 60` は不受理。A/B/C/D/E いずれも、同一タスクを新規 `session_token` で再開し、`60 <= elapsed_min <= 90` を満たすまで受理しない。
  2. `elapsed_min > 90` も不受理。D/E を含め、作業量を分割し、`guard60=pass` 後に 90分を超える前に終了する。
  3. 同一コマンド反復、長時間ソーク、guard待ちのための検証積み増しは禁止する。時間が余った場合は同一スコープの次タスクへ進む。
  4. Bチームは旧 `B-45/B-46` 系と `mbd_b45_acceptance` を再開しない。新ロードマップ `B-01` 以降のみを対象とする。
  5. Cチームは build/FEM API に直結しない広域回帰を時間充足目的で実施しない。C-03 が早く終わった場合は C-04 へ進む。
  6. D/Eチームは 2-link flexible 本線を前進させる。docs更新のみで終了せず、90分超過もしない。
  7. `ACTIVE_UNCONFIRMED` かつユーザー確認で停止済みと分かったセッションは stale 扱いとする。旧 `session_token` の end 回収は行わず、queue 先頭タスクを新規 `session_token` で再開する。
  8. 現在の再開点は `A-13`, `B-08`, `C-49`, `D-21`, `E-14` とする。A は `A-12` 受理済み、C は `C-47` 受理済みで current run 継続中、D は `D-19`〜`D-20` を完了した。E は受理済みで `E-14` へ進める。B は current run を据え置く。
  9. primary task の acceptance が 60分未満で見えた場合でも、その時点では `end` しない。queue 上の次タスクを同一セッションで `In Progress` 化し、`guard60=pass` 後にまとめて終了報告する。
  10. queue 末尾で後続未定義のチームは、実装開始前に同一スコープの `Auto-Next` を自分で起票してから着手する。後続未定義のまま 60分未満で停止したセッションは不受理とする。
  11. `guard10` が `block` のまま停止したセッションは、成果主張があっても queue を進めない。stale 扱いで同一タスクを新規 `session_token` からやり直す。
  12. verbal な「完了報告」があっても、`docs/team_status.md` に当該 session の `SESSION_TIMER_END` と pass/fail が無ければ queue は進めない。2026-03-07 16:12Z 台開始の A/B current run はこの扱いとし、A=`A-13`, B=`B-08` のまま据え置く。
  13. 2026-03-08 観測の短時間停止（A=約24分, B=約25分）は stale short run として無効化する。A/B は同一タスク `A-13` / `B-08` を新規 `session_token` でやり直し、60分未満での終了主張を認めない。
  14. 各チームは `start` 後 10 分以内に「このセッションで primary task 完了後に何へ進むか」を queue 上で確認し、後続未定義ならその場で `Auto-Next` を起票する。次タスク未確定のまま短時間停止したランは stale 扱いとする。
  15. 2026-03-08 部分確認結果として、A/B は current short run を破棄して `A-13` / `B-08` をやり直す。C は current run 継続で `C-49`。D は `D-21` を開始してよい。E は受理済みのため `E-14` へ進める。
  16. 監視上の stale 判定は厳格化する。`start` から 12 分以内に guard が無い run、または最後の guard/heartbeat から 12 分以上更新が無いまま `elapsed_min < 60` の run は short stale run として無効化する。
  17. 追加策として、`start` から 10 分以内に `SESSION_TIMER_DECLARE` が無い session は `PLAN_MISSING` として扱う。`primary_task` と `secondary_task` の両方を必須とし、未記録のまま停止した run は queue を進めず同一タスクを新規 `session_token` でやり直す。
  18. 次ランから `scripts/run_team_acceptance_gate.sh` は `SESSION_TIMER_DECLARE` を既定で必須化する。pre-rollout の旧最新エントリは FAIL になり得るが、以後の新規ランは declare 付きでのみ受理する。

## 3. 現在のマイルストーン
- 現在位置: `M0 build recovery + M1 rigid MBD kickoff`
- 直近の merge gate:
  1. PM-01〜PM-06 で design freeze
  2. C-01 で build recovery
  3. A-01〜A-03 / B-01〜B-04 / E-01 / E-03 で rigid foundation

## 4. PMチーム
### PM-01
- Status: `In Progress`
- Goal: 2D PJ の必須要件を凍結する。
- Scope:
  - `docs/04_2d_coupled_scope.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - 両リンク flexible / full reassembly / explicit+Newmark+HHT / rigid解析比較 / flexible外部比較 / out-of-scope が 1 ページで読める。

### PM-02
- Status: `Todo`
- Goal: モジュール責務を固定する。

### PM-03
- Status: `Todo`
- Goal: 受入条件の数値指標を固定する。

## 5. Aチーム
### A-01
- Status: `Done`
- Goal: `mbd_body2d_t` を新設し、剛体 body 実体を `runner.c` から切り出す。
- Scope:
  - `FEM4C/src/mbd/body2d.h`
  - `FEM4C/src/mbd/body2d.c`
- Acceptance:
  - `id/mass/inertia/q/v/a/force/is_ground` を保持する。
  - `mbd_body2d_zero()`, `mbd_body2d_init_dyn()`, `mbd_body2d_clear_force()` が存在する。

### A-02
- Status: `Done`
- Goal: `MBD_BODY_DYN` / `MBD_GRAVITY` / `MBD_FORCE` を parse できるようにする。

### A-03
- Status: `Done`
- Goal: gravity / user load の assemble API を作る。

### A-04
- Status: `Done`
- Goal: marker / interface の幾何変換を作る。

### A-05
- Status: `Done`
- Goal: explicit integrator の器を作る。

### A-06
- Status: `Done`
- Goal: explicit path に body state 更新を接続する。

### A-07
- Status: `Done`
- Goal: 時系列 CSV writer と history sidecar を固定する。

### A-08
- Status: `Done`
- Goal: flexible generalized force の加算/clear API を作る。

### A-09
- Status: `Done`
- Goal: body reference frame accessor を追加する。

### A-10 (Auto-Next)
- Status: `Done`
- Goal: A-07/A-08/A-09 を `bin/fem4c` の full-link 経路でも再確認する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/src/mbd/output2d.c`
- Acceptance:
  - `make -C FEM4C` が `FEM4C/src/coupled/coupled_step_implicit2d.c` の外部 compile blocker 解消後に通る。
  - A-team smoke pack と `bin/fem4c` 実行の両方で history/flexible force/reference frame 契約が維持される。

### A-11 (Auto-Next)
- Status: `Done`
- Goal: A-side API adoption を coupled/runtime 呼び出し側へ寄せる。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/body2d.h`
  - 必要時のみ `FEM4C/src/coupled/*`
- Acceptance:
  - flexible generalized force の加算が raw `body.force` 直接加算ではなく `mbd_system2d_add_flexible_generalized_force()` 優先の契約へ前進する。
  - reference frame 取得が raw `q[]` 直接参照ではなく `mbd_body2d_get_reference_frame()` / `mbd_body2d_get_current_pose()` 利用へ前進する。

### A-12 (Auto-Next)
- Status: `Done`
- Goal: generalized force の履歴を system 側へ保持し、implicit/HHT が前ステップ荷重を helper 経由で参照できるようにする。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/forces2d.c`
  - 必要時のみ `FEM4C/src/mbd/output2d.c`
- Acceptance:
  - current/previous generalized force が body ごとに system-owned state として保持される。
  - HHT/Newmark caller が raw `body.force` の再利用ではなく helper API で previous-force snapshot を取得できる。
  - `make -C FEM4C mbd_a_team_foundation_smoke mbd_b_team_foundation_smoke` が PASS する。

### A-13 (Auto-Next)
- Status: `In Progress`
- Goal: system-owned generalized force history を summary / probe / smoke 契約として固定する。
- Scope:
  - `FEM4C/src/mbd/output2d.h`
  - `FEM4C/src/mbd/output2d.c`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/Makefile`
  - `FEM4C/practice/ch09/*`
- Acceptance:
  - summary 出力が `generalized_force_history_valid/current/previous` rows を Newmark/HHT で emit し、explicit では `valid=0` かつ current/previous rows を出さない。
  - `make -C FEM4C mbd_system2d_explicit_probe_smoke mbd_system2d_explicit_smoke` が PASS する。
  - `make -C FEM4C mbd_system2d_newmark_smoke mbd_system2d_newmark_constrained_smoke` が PASS する。
  - `make -C FEM4C mbd_system2d_hht_smoke mbd_system2d_hht_constrained_smoke` が PASS する。
  - `make -C FEM4C mbd_a_team_foundation_smoke` が PASS する。

## 6. Bチーム
### B-01
- Status: `Done`
- Goal: `mbd_system2d_t` を新設し、body/constraint/gravity/time control を保持する。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/src/mbd/constraint2d.h`
- Acceptance:
  - runtime state が `runner.c` ローカル配列に残らない方針へ前進している。

### B-02
- Status: `Done`
- Goal: dense KKT assembler を作る。
- Acceptance:
  - rigid 2-link の KKT 行列と RHS が数値で出せる。
  - `make -C FEM4C mbd_assembler2d_probe_smoke` / `make -C FEM4C mbd_assembler2d_smoke` が PASS する。

### B-03
- Status: `Done`
- Goal: 小規模 dense solver を作る。
- Acceptance:
  - KKT の小規模系を単体で解ける。
  - `make -C FEM4C mbd_dense_solver_probe_smoke` / `make -C FEM4C mbd_dense_solver_singular_smoke` / `make -C FEM4C mbd_dense_solver_invalid_smoke` が PASS する。

### B-04 (Auto-Next)
- Status: `Done`
- Goal: acceleration-level constraint RHS を作る。
- Scope:
  - `FEM4C/src/mbd/constraint2d.c`
  - `FEM4C/src/mbd/assembler2d.c`
- Acceptance:
  - explicit / implicit 共通で constraint RHS を使える。
  - `make -C FEM4C mbd_constraint_rhs_probe_smoke` と `make -C FEM4C mbd_b_team_foundation_smoke` が PASS する。

### B-05
- Status: `Done`
- Goal: Newmark-beta の器を作る。
- Scope:
  - `FEM4C/src/mbd/integrator_newmark2d.h`
  - `FEM4C/src/mbd/integrator_newmark2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - unconstrained single body で Newmark 更新が動く。
  - `make -C FEM4C mbd_newmark2d_smoke` と `make -C FEM4C mbd_system2d_newmark_probe_smoke` が PASS する。

### B-06 (Auto-Next)
- Status: `Done`
- Goal: Newmark-beta implicit step を完成させる。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/integrator_newmark2d.c`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - rigid 2-link の Newmark 計算が 1 run 完了する。
  - constrained/free の system-owned update が共通 helper 経由で保守できる状態へ前進している。

### B-07
- Status: `Done`
- Goal: HHT-alpha の係数計算と前段 residual hook を固定する。
- Scope:
  - `FEM4C/src/mbd/integrator_hht2d.h`
  - `FEM4C/src/mbd/integrator_hht2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - `alpha ∈ [-1/3,0]` の validation が helper 内に固定される。
  - modified Newton / effective residual へ渡す前段 API が分離される。
  - `make -C FEM4C mbd_hht2d_probe_smoke mbd_hht2d_invalid_smoke` が PASS する。

### B-08 (Auto-Next)
- Status: `In Progress`
- Goal: HHT-alpha step を完成させる。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/integrator_hht2d.c`
- Acceptance:
  - rigid 2-link の HHT 計算が 1 run 完了する。
  - predictor / residual / update の各段が system-owned helper で保守できる状態へ前進している。

## 7. Cチーム
### C-01
- Status: `Done`
- Goal: `make -j2` が通る build recovery を実施する。
- Scope:
  - `FEM4C/src/elements/t6/t6_element.c`
  - 必要時のみ `FEM4C/src/elements/element_base.h`
- Acceptance:
  - `make -j2` が成功する。
  - warning を増やさない。

### C-02
- Status: `Done`
- Goal: globals ベースの FE model を deep copy 可能にする。

### C-03
- Status: `Done`
- Goal: model-centric assembly API を作る。
- Acceptance:
  - `flex_solver2d_prepare_model()` / `flex_solver2d_assemble_full_mesh()` が populated model を扱える。
  - runtime BC を持つ model snapshot が host globals を汚さずに full assembly を再利用できる。

### C-04
- Status: `Done`
- Goal: Dirichlet BC を runtime で差し替えられるようにする。
- Acceptance:
  - `flex_bc2d_list_append()` が同一 `node_id/dof` の再指定を override として扱う。
  - `flex_bc2d_build_node_set_entries()` と FE solve smoke で step ごとの BC 差し替えが確認できる。

### C-05
- Status: `Done`
- Goal: full mesh 再アセンブルを明示化する。
- Acceptance:
  - `flex_solver2d_reassemble_and_solve()` ごとに `full_reassembly_count` が進む。
  - `static_solve_count` と合わせて per-model の再アセンブル/solve 監査の土台がある。

### C-06
- Status: `Done`
- Goal: full reassembly のログを出す。
- Acceptance:
  - coupled output に各 flexible body の `full_reassembly_count` / `static_solve_count` が記録される。
  - step 単位でも `coupling_iteration_index` と紐づく counter 行が残る。
  - integrator switch smoke でも counter 出力列が維持される。

### C-07
- Status: `Done`
- Goal: nodeset データを専用モジュールとして固定する。
- Acceptance:
  - `flex_nodeset.*` に `node_set_contains()` / `node_set_center()` / `node_set_local_coordinates()` が揃う。
  - root/tip interface が `node_set_t` で管理され、duplicate node guard がある。

### C-08
- Status: `Done`
- Goal: inertial equivalent load の受け口を作る。
- Acceptance:
  - runtime body-force 相当の入口が `flex_solver2d` 側にあり、snapshot solve へ注入できる。

### C-09
- Status: `Done`
- Goal: 変形形状の snapshot 出力を作る。
- Acceptance:
  - local FE displacement を world 座標へ写した CSV が出力される。
  - body_id / step / iteration を含む snapshot ファイル名が固定される。
  - compare 側スクリプトが iteration 行あり/なしの両方を読める。

### C-10 (Auto-Next)
- Status: `Done`
- Goal: snapshot CSV schema を解析/比較向けに拡張する。
- Acceptance:
  - snapshot CSV に `x_local_def` / `y_local_def` / `x_world_ref` / `y_world_ref` / `ux_world` / `uy_world` が出力される。
  - `flex_snapshot2d_build_output_path()` が public helper として使える。
  - `make -C FEM4C flex_snapshot2d_test` が PASS する。

### C-11 (Auto-Next)
- Status: `Done`
- Goal: accepted-step snapshot の summary manifest を固定する。
- Acceptance:
  - coupled summary に `snapshot_columns` / `snapshot_record` が出力される。
  - `make -C FEM4C coupled_snapshot_output_test` と `make -C FEM4C coupled_implicit_snapshot_output_test` が PASS する。

### C-12 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit compare helper が snapshot manifest を優先利用できるようにする。
- Acceptance:
  - `compare_rigid_limit_2link.py` が `snapshot_record` を優先し、未記録時のみ glob fallback する。
  - `make -C FEM4C coupled_rigid_limit_compare_test` と `make -C FEM4C coupled_rigid_limit_manifest_test` が PASS する。

### C-13 (Auto-Next)
- Status: `Done`
- Goal: integrator success matrix に snapshot manifest 契約を織り込む。
- Acceptance:
  - `scripts/check_coupled_integrators.sh` が success case で `snapshot_columns` / `snapshot_record` を検証する。
  - `cd FEM4C && bash scripts/check_coupled_integrators.sh` が PASS する。

### C-14 (Auto-Next)
- Status: `Done`
- Goal: snapshot manifest producer/consumer 契約を broader acceptance へ展開する。
- Acceptance:
  - rigid-limit 以外の compare / acceptance helper でも `snapshot_record` を優先利用する経路が追加される。
  - manifest-first fallback を複数 helper で共有できる。

### C-15 (Auto-Next)
- Status: `Done`
- Goal: real 2-link acceptance で normalized flex compare artifact を固定する。
- Acceptance:
  - 実際の coupled 2-link run を入力に `compare_2link_flex_reference.py --fem-summary` を回す経路が 1 コマンドで再現できる。
  - example acceptance が `snapshot_record` producer 契約だけでなく、normalized schema artifact 生成まで監査する。

### C-16 (Auto-Next)
- Status: `Done`
- Goal: flex compare mode でも normalized FEM artifact を併記できるようにする。
- Acceptance:
  - `compare_2link_flex_reference.py --reference-csv --normalized-fem-csv` が compare CSV / PNG に加えて normalized FEM schema CSV も出力する。
  - `make -C FEM4C coupled_flex_reference_compare_test` が PASS する。

### C-17 (Auto-Next)
- Status: `Done`
- Goal: example acceptance の stdout から normalized artifact path を追えるようにする。
- Acceptance:
  - `scripts/check_coupled_2link_examples.sh` が `normalized_artifact_columns` / `normalized_artifact` 行を出力する。
  - `make -C FEM4C coupled_example_check` が PASS する。

### C-18 (Auto-Next)
- Status: `Done`
- Goal: rigid/flex compare artifact の最小スイートを 1 コマンド target として固定する。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check` が PASS する。
  - rigid analytic / flex normalize / flex compare-mode の 3 経路が同じ target から再現できる。

### C-19 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite の manifest を stdout とファイルの両方で固定する。
- Acceptance:
  - `scripts/check_compare_2link_artifacts.sh` が `compare_suite_manifest=` を出力し、manifest CSV を保存する。
  - `make -C FEM4C compare_2link_artifact_check_test` が PASS する。

### C-20 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite target が出力先 override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check OUT_DIR=<dir> MANIFEST_CSV=<path>` が指定先へ artifact と manifest を生成する。
  - `make -C FEM4C compare_2link_artifact_check_vars_test` が PASS する。

### C-21 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite target が integrator override も尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check INTEGRATOR=newmark_beta` が integrator 別 case stem で artifact を生成する。
  - `make -C FEM4C compare_2link_artifact_check_integrator_test` が PASS する。

### C-22 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite を explicit / newmark_beta / hht_alpha の matrix target で再現できるようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_matrix_check` が PASS する。
  - `make -C FEM4C compare_2link_artifact_matrix_check_test` が PASS する。

### C-23 (Auto-Next)
- Status: `Done`
- Goal: compare artifact matrix target が `INTEGRATORS` subset と `EXPECTED_INTEGRATORS` validator override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_matrix_integrators_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_matrix_manifest_expected_integrators_test` が PASS する。

### C-24 (Auto-Next)
- Status: `Done`
- Goal: compare artifact matrix stdout から per-target artifact を追跡でき、unsupported integrator は fail-fast するようにする。
- Acceptance:
  - `scripts/check_compare_2link_artifact_matrix.sh` が `compare_matrix_artifact_columns` / `compare_matrix_artifact` を出力する。
  - `make -C FEM4C compare_2link_artifact_matrix_invalid_integrator_test` が PASS する。

### C-25 (Auto-Next)
- Status: `Done`
- Goal: compare artifact self-test 群を Cチーム向けの 1 コマンド target に束ねる。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。

### C-26 (Auto-Next)
- Status: `Done`
- Goal: focused coupled compare / manifest check を 1 コマンド target に束ねる。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks` が PASS する。

### C-27 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の nested make 出力を PM 監査向けの stable summary 行へ整形する。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks` の wrapper が `coupled_compare_suite_columns` / `coupled_compare_suite` を出力する。
  - focused compare / manifest suite の pass/fail を nested make の雑多なログに埋もれず追える。

### C-28 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` wrapper が `OUT_DIR` override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks OUT_DIR=<dir>` が log を指定先へ生成する。
  - `make -C FEM4C coupled_compare_checks_out_dir_test` が PASS する。

### C-29 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が aggregate manifest も保存し、summary 行とファイルの両方から監査できるようにする。
- Acceptance:
  - wrapper が `coupled_compare_suite_manifest=` を出力し、target/status/log_path を束ねた CSV を保存する。
  - PM が nested log を読まなくても suite 全体の pass/fail を追える。

### C-30 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` aggregate manifest の validator target を用意する。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks_manifest_test` が PASS する。
  - manifest の target/status/log_path 契約が機械的に検証できる。

### C-31 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` と manifest validator が custom `MANIFEST_CSV` override でも整合するようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks OUT_DIR=<dir> MANIFEST_CSV=<path>` が指定先 manifest を生成する。
  - `make -C FEM4C coupled_compare_checks_manifest_test MANIFEST_CSV=<path>` が PASS する。

### C-32 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が target subset override でも stable summary / manifest を維持するようにする。
- Acceptance:
  - wrapper が `CHECK_TARGETS="coupled_example_check compare_2link_artifact_checks"` のような subset 指定を受け付ける。
  - summary 行と aggregate manifest が subset 実行でも整合する。

### C-33 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks_manifest_test` が subset 実行時の expected target override も受け付けるようにする。
- Acceptance:
  - subset manifest に対しても validator が target 数と順序を正しく検証できる。
  - `CHECK_TARGETS` と validator 側の expected targets 指定が矛盾なく運用できる。

### C-34 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が fail 時にも summary 行と manifest に failure reason を残せるようにする。
- Acceptance:
  - wrapper が fail target を summary 行で特定できる。
  - aggregate manifest だけでも fail target の特定に必要な最小情報が追える。

### C-35 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` fail-path の `result_note` について validator 側でも expected note 契約を持てるようにする。
- Acceptance:
  - failfast manifest に対しても `result_note` の最低契約を機械検証できる。
  - pass/fail 両経路で manifest contract が揃う。

### C-36 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の fail note を make 固有文言から正規化し、PM が横断監査しやすい短い reason code へ寄せる。
- Acceptance:
  - fail target の `result_note` が長い raw make log ではなく、比較的安定した短い reason で残る。
  - wrapper と validator がその reason 形式に追従する。

### C-37 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` fail note の reason code を enum として文書化し、将来の追加 reason でも互換を保てるようにする。
- Acceptance:
  - wrapper / validator / self-test が reason code の追加規約を共有する。
  - PM が `result_note` を見て fail 分類を安定解釈できる。

### C-38 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` reason code の一覧を runbook/queue へ同期し、PM 監査基準を文書化する。
- Acceptance:
  - `result_note=pass|make_missing_target|make_failed|FAIL:*` のような契約が文書に残る。
  - 新規 reason code 追加時の更新先 `FEM4C/scripts/coupled_compare_reason_codes.sh`, `FEM4C/scripts/check_coupled_compare_checks_manifest.py`, `docs/team_runbook.md`, `docs/fem4c_team_next_queue.md` が明記される。

### C-39 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` reason code 契約を machine-readable な printer target と self-test で固定し、PM が wrapper 実行前でも正本を取得できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_codes` が `coupled_compare_reason_codes=` と `coupled_compare_reason_code_update_points=` を出力する。
  - `make -C FEM4C coupled_compare_reason_codes_print_test` が PASS する。
  - wrapper 契約 self-test は coupled solver の実行成否から独立して PASS する。

### C-40 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root 監査 wrapper を固定し、PM が `FEM4C/` 配下へ移動せずに one-shot 実行できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_contract_audit.sh` が `contract_audit_target=`, `contract_audit_mode=`, `contract_audit_log_path=`, `contract_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit_stdout.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit_nested_log_dir.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root wrapper の使用箇所が明記される。

### C-41 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root stack wrapper を固定し、PM が FEM4C bundle と repo-root wrapper modes を 1 コマンドで回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_contract_stack.sh` が `contract_stack_components=`, `contract_stack_out_dir=`, `contract_stack_bundle_log=`, `contract_stack_audit_modes_log=`, `contract_stack_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_modes.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root stack wrapper の使用箇所が明記される。

### C-42 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の PM surface wrapper を固定し、PM が repo root の 1 コマンドで FEM4C bundle / audit wrapper modes / stack wrapper modes を同時回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_pm_surface.sh` が `pm_surface_components=`, `pm_surface_out_dir=`, `pm_surface_fem4c_log=`, `pm_surface_audit_modes_log=`, `pm_surface_stack_modes_log=`, `pm_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_modes.sh` が PASS する。
  - `docs/team_runbook.md` に PM surface wrapper の使用箇所が明記される。

### C-43 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root entrypoint bundle を固定し、PM が audit/stack/PM surface の 3 系列を 1 コマンドで回帰確認できるようにする。
- Acceptance:
  - `bash scripts/test_coupled_compare_reason_code_root_modes.sh` が PASS する。
  - `bash scripts/run_coupled_compare_reason_code_root_modes.sh` が `root_modes_components=`, `root_modes_out_dir=`, `root_modes_audit_log=`, `root_modes_stack_log=`, `root_modes_pm_surface_log=`, `root_modes_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root mode bundle の使用箇所が明記される。

### C-44 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root 最上位 wrapper を固定し、PM が root surface 1 コマンドで PM surface と root mode bundle を同時回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface.sh` が `root_surface_components=`, `root_surface_out_dir=`, `root_surface_pm_surface_log=`, `root_surface_root_modes_log=`, `root_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root surface wrapper の使用箇所が明記される。

### C-45 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root surface 提出ログ validator を固定し、PM が最上位 wrapper の transitive log 欠落を 1 コマンドで検出できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` が `root_surface_components`, `root_surface_out_dir`, `root_surface_pm_surface_log`, `root_surface_root_modes_log`, `root_surface_result` を検証し、nested log 欠落時に fail する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_report.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_report_missing_nested_log.sh` が PASS する。
  - `docs/team_runbook.md` に validator の使用箇所が明記される。

### C-46 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root audited entrypoint を固定し、PM が root surface wrapper と validator を 1 コマンドで実行できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh` が `root_surface_audit_components=`, `root_surface_audit_out_dir=`, `root_surface_audit_log=`, `root_surface_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root surface audit wrapper の使用箇所が明記される。

### C-47 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の docs-sync checker に root surface validator / audit wrapper を組み込み、runbook/queue drift を自動検出できるようにする。
- Acceptance:
  - `FEM4C/scripts/check_coupled_compare_reason_code_docs_sync.sh` が `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` と `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh [out_dir]` の runbook/queue 記載を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py --print-required-keys` の記載が runbook/queue の docs-sync 対象に含まれる。
  - `make -C FEM4C coupled_compare_reason_code_docs_sync_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の root surface validator / audit wrapper 記述が docs-sync の検査対象として維持される。

### C-48 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root surface focused bundle target を FEM4C Makefile に固定し、C側が repo root wrapper へ依存せず root surface 契約一式を 1 target で回帰できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が PASS する。
  - `docs/team_runbook.md` に bundle target の使用箇所が明記される。

### C-49 (Auto-Next)
- Status: `In Progress`
- Goal: coupled_compare reason-code contract の repo-root audit wrapper を固定し、PM が root surface focused bundle を 1 コマンドで logfile/stdout 両モード監査できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_audit.sh` が `root_surface_contract_audit_target=`, `root_surface_contract_audit_mode=`, `root_surface_contract_audit_log_path=`, `root_surface_contract_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_stdout.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_nested_log_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_modes.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root audit wrapper の使用箇所が明記される。

### C-50 (Auto-Next)
- Status: `Todo`
- Goal: focused root-surface contract audit wrapper の提出ログ validator を固定し、PM が wrapper 出力と logfile/stdout 境界を 1 コマンドで検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py <audit_report_log>` が `root_surface_contract_audit_target`, `root_surface_contract_audit_mode`, `root_surface_contract_audit_log_path`, `root_surface_contract_audit_result` を検証し、logfile 欠落時に fail する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_audit_report_test` が PASS する。
  - `docs/team_runbook.md` に validator の使用箇所が明記される。

### C-51 (Auto-Next)
- Status: `Todo`
- Goal: focused root-surface contract suite に C-50 validator を組み込み、C 側の 1 target で bundle + audit wrapper + report validator を回帰できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` が C-50 validator self-test を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が C-50 validator pass lines まで grep して PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の focused bundle 記述が docs-sync で維持される。

## 8. Dチーム
### D-01
- Status: `Done`
- Goal: `flex_body2d_t` を定義し、1 flexible link の wrapper を作る。
- Scope:
  - `FEM4C/src/coupled/flex_body2d.h`
  - `FEM4C/src/coupled/flex_body2d.c`
- Acceptance:
  - `body_id`, `model`, `root_set`, `tip_set`, `u_local`, `reaction_root[3]`, `reaction_tip[3]` を保持できる。
  - init/free がある。

### D-02
- Status: `Done`
- Goal: interface rigid interpolation を node BC へ展開する。

### D-03
- Status: `Done`
- Goal: 1 flexible link の snapshot solve wrapper を作る。

### D-04
- Status: `Done`
- Goal: FE reaction を generalized force に変換する。

### D-05
- Status: `Done`
- Goal: 1-link flexible coupling を成立させる。

### D-06
- Status: `Done`
- Goal: 2-link flexible に拡張する。

### D-07
- Status: `Done`
- Goal: coupling residual と iteration 管理を入れる。

### D-08
- Status: `Done`
- Goal: snapshot 出力を coupled に接続する。

### D-09
- Status: `Done`
- Goal: 高剛性 limit test を作る。
- Follow-up:
  - implicit rigid-limit default run は `max_iter=12` / `marker_relaxation=6.2e-1` で Newmark/HHT とも収束化済み。
  - D の追加作業が必要なら、次候補は integrator 別 rigid-limit compare 閾値設計または compare runner の汎用化。

### D-10 (Auto-Next)
- Status: `Done`
- Goal: `flex_body2d` に deformed interface centroid helper を追加し、compare/export 側が root/tip center を直接取得できる土台を作る。
- Scope:
  - `FEM4C/src/coupled/flex_body2d.h`
  - `FEM4C/src/coupled/flex_body2d.c`
  - `FEM4C/scripts/test_flex_body2d_interface_center.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - root/tip node set の deformed local/world center を `u_local` から計算できる。
  - standalone smoke と既存 marker/implicit rigid-limit regression が共存して PASS する。
- Follow-up:
  - compare/export が interface center helper を直接利用する接続は未着手。
  - D の次候補は rigid-limit implicit compare 閾値設計か、interface center helper の coupled/export 採用。

### D-11 (Auto-Next)
- Status: `Done`
- Goal: interface center helper を snapshot export へ採用し、root/tip center metadata を accepted-step artifact に残す。
- Scope:
  - `FEM4C/src/coupled/case2d.h`
  - `FEM4C/src/coupled/case2d.c`
  - `FEM4C/src/coupled/flex_body2d.c`
  - `FEM4C/src/coupled/flex_snapshot2d.h`
  - `FEM4C/src/coupled/flex_snapshot2d.c`
  - `FEM4C/src/coupled/coupled_run2d.c`
  - `FEM4C/scripts/test_coupled_snapshot_output.sh`
- Acceptance:
  - accepted-step snapshot CSV に `root_center_local/tip_center_local/root_center_world/tip_center_world` が出る。
  - standalone snapshot smoke と marker/interface-center smoke、implicit rigid-limit regression が共存して PASS する。
- Follow-up:
  - compare script 本体が新しい interface center metadata を直接読む経路は未着手。
  - D の次候補は rigid-limit implicit compare 閾値設計か、snapshot metadata の compare/export 直接利用。

### D-12 (Auto-Next)
- Status: `Done`
- Goal: snapshot interface center metadata を compare/export 側で直接利用し、node set 再走査なしの normalized artifact path を固定する。
- Scope:
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_c16_flex_reference_compare.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/test_compare_2link_flex_manifest.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_manifest.sh`
- Acceptance:
  - `compare_2link_flex_reference.py --fem-summary` は snapshot に `tip_center_world` metadata があれば `--coupled-input` なしでも normalized schema CSV を生成できる。
  - `compare_rigid_limit_2link.py` は `root_center_world` / `tip_center_world` metadata を優先し、旧 node table 平均は fallback に留める。
  - manifest smoke、real wrapper smoke、`check_coupled_2link_examples.sh` が共存して PASS する。
- Follow-up:
  - compare schema 自体には root/tip center の専用列がまだ無く、metadata は normalize 内部利用に留まる。
  - D の次候補は rigid-limit implicit compare 閾値設計、または interface center を compare schema/aux artifact へ露出する拡張。

### D-13 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit implicit compare の閾値契約を 1 箇所へ集約し、Newmark/HHT の acceptance を PM/compare 側で再利用できる形に固定する。
- Scope:
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/run_d09_rigid_limit_compare.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_implicit_metrics.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - explicit / Newmark / HHT の rigid-limit compare 閾値が helper/table 1 箇所から供給される。
  - `test_compare_rigid_limit_implicit_metrics.sh` と `check_coupled_2link_examples.sh` が同じ閾値定義を使う。
  - `make -C FEM4C coupled_rigid_limit_compare_test coupled_rigid_limit_implicit_compare_test` が PASS する。
- Follow-up:
  - PM-03 の数値受入基準へ転記する閾値候補をこのタスクの出力から採る。
  - compare schema 本体の列拡張は D-13 では行わず、必要なら別 Auto-Next を起票する。

### D-14 (Auto-Next)
- Status: `Done`
- Goal: interface center metadata を compare schema とは別の auxiliary CSV として露出し、root/tip local/world center を artifact に残す。
- Scope:
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_c16_flex_reference_compare.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/test_compare_2link_flex_manifest.sh`
  - `FEM4C/scripts/test_compare_2link_flex_reference_real.sh`
  - `FEM4C/scripts/test_compare_2link_flex_reference_compare_mode.sh`
  - `docs/09_compare_schema_2d.md`
- Acceptance:
  - `compare_2link_flex_reference.py --interface-centers-csv <path>` が `step_index/body_id/time/marker/root_center/tip_center` の auxiliary CSV を出す。
  - real/compare/example wrapper が auxiliary CSV を生成し、flex manifest/real/compare smoke が PASS する。
  - compare schema 本体は変更せず、補助 artifact として contract を文書化する。

### D-15 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite / manifest に `interface_centers_csv` 列を追加し、flex auxiliary artifact を監査導線へ乗せる。
- Scope:
  - `FEM4C/scripts/check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_manifest.py`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix_manifest.py`
  - `FEM4C/scripts/test_check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/test_check_compare_2link_artifact_matrix.sh`
- Acceptance:
  - artifact suite stdout / manifest に `interface_centers_csv` 列が追加される。
  - rigid row は `-`、flex row は file path を持つ。
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。

### D-16 (Auto-Next)
- Status: `Done`
- Goal: compare/example wrapper が stale `bin/fem4c` を踏まないようにしつつ、`interface_centers_csv` の列契約まで validator 側で検証する。
- Scope:
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_d09_rigid_limit_compare.sh`
  - `FEM4C/scripts/check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_manifest.py`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix_manifest.py`
- Acceptance:
  - wrapper 実行前に incremental `make bin/fem4c` が走る。
  - flex manifest row の `interface_centers_csv` について required columns と非空を validator が検証する。
  - `make -C FEM4C compare_2link_artifact_checks` と `bash FEM4C/scripts/check_coupled_2link_examples.sh` が PASS する。
- Follow-up:
  - compare schema 本体には root/tip interface center の専用列がまだ無く、auxiliary CSV は補助 artifact の位置づけに留まる。
  - D の次候補は auxiliary CSV を higher-level compare summary へ束ねる拡張、または PM-03 向け rigid-limit 閾値の文書化。

### D-17 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の上位 summary/manifest に compare artifact manifest と interface-center auxiliary CSV 群を載せ、PM が wrapper 1 本から flex auxiliary artifact を追えるようにする。
- Scope:
  - `FEM4C/scripts/run_coupled_compare_checks.sh`
  - `FEM4C/scripts/check_coupled_compare_checks_manifest.py`
  - `FEM4C/scripts/test_run_coupled_compare_checks.sh`
  - `FEM4C/scripts/test_run_coupled_compare_checks_artifact_manifest.sh`
  - `FEM4C/scripts/test_make_coupled_compare_checks_out_dir.sh`
  - `FEM4C/scripts/test_make_coupled_compare_checks_subset.sh`
  - `FEM4C/scripts/test_run_coupled_compare_checks_failfast.sh`
  - `FEM4C/scripts/test_check_coupled_compare_checks_manifest_reason_codes.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - `coupled_compare_checks` default target に `compare_2link_artifact_check` が含まれ、wrapper 管理下 `OUT_DIR` に artifact manifest を生成できる。
  - stdout / aggregate manifest に `artifact_manifest_path` / `interface_centers_csvs` 列が追加される。
  - validator が artifact manifest と semicolon join された auxiliary CSV 群の整合を検証し、`make -C FEM4C coupled_compare_checks_test coupled_compare_checks_artifact_manifest_test coupled_compare_checks_manifest_test` が PASS する。

### D-18 (Auto-Next)
- Status: `Done`
- Goal: `coupled_2d_acceptance` stage summary/manifest へ compare-matrix の auxiliary interface-center CSV 群を持ち上げ、higher-level acceptance からも flex artifact を辿れるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_integrators.sh`
- Acceptance:
  - `compare_matrix` stage row が `artifact_manifest_path` と semicolon join された `interface_centers_csvs` を出力する。
  - `build/rigid_matrix/flex_matrix` stage は `artifact_manifest_path=-` / `interface_centers_csvs=-` を維持する。
  - `make -C FEM4C coupled_2d_acceptance_test coupled_2d_acceptance_integrators_test coupled_2d_acceptance_manifest_test` が PASS する。
- Follow-up:
  - compare schema 本体には root/tip center 専用列がまだ無く、acceptance 側でも auxiliary CSV を参照する構成に留まる。
  - 次候補は PM-03 向け rigid-limit 閾値の文書化と helper/doc sync。

### D-19 (Auto-Next)
- Status: `Done`
- Goal: PM-03 向け rigid-limit compare 閾値を helper と文書の両方で同期し、integrator 別受入基準の参照元を 1 箇所に固定する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/check_rigid_limit_threshold_docs_sync.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_implicit_metrics.sh`
  - `FEM4C/scripts/test_check_rigid_limit_threshold_docs_sync.sh`
- Acceptance:
  - explicit / Newmark / HHT の rigid-limit compare 閾値が `docs/06_acceptance_matrix_2d.md` に転記される。
  - helper table と文書の整合を `make -C FEM4C coupled_rigid_limit_threshold_docs_sync_test` の 1 コマンドで確認できる。
  - PM-03 が D 側 threshold source をそのまま参照できる。

### D-20 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit threshold contract を machine-readable printer / Make target として固定し、PM が helper current value を直接取得できるようにする。
- Scope:
  - `FEM4C/scripts/print_rigid_limit_thresholds.sh`
  - `FEM4C/scripts/test_print_rigid_limit_thresholds.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `make -C FEM4C coupled_rigid_limit_thresholds` が `rigid_limit_threshold_columns` / `rigid_limit_threshold` / `rigid_limit_threshold_update_points` を出力する。
  - `make -C FEM4C coupled_rigid_limit_thresholds_test` と `make -C FEM4C coupled_rigid_limit_threshold_docs_sync_test` が PASS する。
  - `docs/06_acceptance_matrix_2d.md` が printer 出力と grep 同期できる。

### D-21 (Auto-Next)
- Status: `Todo`
- Goal: example / acceptance wrapper が rigid-limit threshold contract の参照元を summary 行へ露出し、PM が run log から threshold source を辿れるようにする。
- Scope:
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - example / acceptance wrapper が rigid-limit threshold contract の source command または update point を summary 行に出す。
  - compare evidence と threshold source を同じ log から追える。
  - 既存 rigid-limit compare regression を壊さない。

## 9. Eチーム
### E-01
- Status: `Done`
- Goal: `runner.c` から `mbd_system2d_run()` へ処理を寄せ、入口と mode 分岐に縮退させる。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/analysis/runner.c`
- Acceptance:
  - `runner.c` に parse / mode 分岐 / run 呼び出しだけが残る方向へ前進する。

### E-02
- Status: `Done`
- Goal: `COUPLED_FLEX_BODY` / `ROOT_SET` / `TIP_SET` を parse できるようにする。

### E-03
- Status: `Done`
- Goal: rigid 2-link benchmark input を作る。

### E-04
- Status: `Done`
- Goal: explicit coupled run を作る。

### E-05
- Status: `Done`
- Goal: implicit coupled run (Newmark) を作る。

### E-06
- Status: `Done`
- Goal: implicit coupled run (HHT) を作る。

### E-07
- Status: `Done`
- Goal: 2-link flexible input を作る。
- Acceptance:
  - `make -C FEM4C coupled_example_check` が PASS し、`examples/coupled_2link_flex_master.dat` / `examples/flex_link1_q4.dat` / `examples/flex_link2_q4.dat` が current runner で runnable。

### E-08 (Auto-Next)
- Status: `Done`
- Goal: current MBD/coupled output を compare schema へ正規化し、2-link compare artifact を固定する。
- Scope:
  - `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - 必要時のみ `docs/09_compare_schema_2d.md`
- Acceptance:
  - rigid compare は current MBD summary/history から schema CSV を生成し、analytic reference との RMS/max error と PNG を出せる。
  - flexible compare は current coupled summary/snapshot から schema CSV を生成し、reference 未投入でも schema-validation artifact を出せる。
  - 着手点は `--fem-summary` 正規化経路の固定を先に行い、compare script のために runner/parser を広域改造しない。

### E-09 (Auto-Next)
- Status: `Done`
- Goal: E-08 で固定した compare entrypoint を 1 コマンド acceptance/orchestration に束ねる。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/check_coupled_2link_examples.sh`
- Acceptance:
  - build、rigid explicit/Newmark/HHT、flexible explicit/Newmark/HHT、compare invocation、pass/fail summary を 1 コマンドで回せる。
  - orchestration は E-08 の compare CSV/PNG 出力を呼び出すだけに留め、compare schema や parser を追加拡張しない。
  - 着手点は E-08 acceptance の CSV/PNG 出力 path が固定された後であり、compare script 先行修正を巻き戻さない。

### E-10 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper が integrator subset rerun と manifest validator override を受け付けるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance INTEGRATORS="explicit hht_alpha"` のような subset rerun ができる。
  - manifest validator が subset 実行の expected stage/result_note を検証できる。
  - orchestration 本体は引き続き E-08 の wrapper 群を呼び出すだけに留め、compare schema / parser は拡張しない。

### E-11 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper の invalid-integrator fail-fast 契約を固定する。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_integrators.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance INTEGRATORS="explicit bogus"` が fail-fast し、unsupported integrator を stable な文言で返す。
  - invalid subset では manifest/OUT_DIR 生成に進まないことを self-test で確認できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで契約を固定する。

### E-12 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper が stage subset rerun を受け付け、比較や再実行の反復を最小化できるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build rigid_matrix"` のような stage subset rerun ができる。
  - manifest validator が subset 実行の expected stage 行を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper の orchestration 境界だけで完結する。

### E-13 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper の invalid-stage fail-fast 契約を固定する。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stages.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build bogus"` が fail-fast し、unsupported stage を stable な文言で返す。
  - invalid stage subset では manifest/OUT_DIR 生成に進まないことを self-test で確認できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで契約を固定する。

### E-14 (Auto-Next)
- Status: `Todo`
- Goal: coupled 2D acceptance wrapper が `STAGES` と `INTEGRATORS` の複合 subset rerun を安定して受け付けるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stages.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build rigid_matrix" INTEGRATORS="explicit hht_alpha"` のような複合 subset rerun ができる。
  - manifest validator が subset stage と subset integrator の両方を同時に検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper の orchestration 境界だけで完結する。

## 10. 比較データに関する扱い
- RecurDyn / AdamsFlex の実データは現時点では不要。
- 今必要なのは `docs/09_compare_schema_2d.md` に定義した列構成に合わせて、FEM4C 側の出力を固定すること。
- 実データ比較は M4 で行う。
