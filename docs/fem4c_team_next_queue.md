# FEM4C Team Next Queue

更新日: 2026-03-07（C-03継続 / D-08継続 / E-07着手反映）
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
- 現在の常設注意:
  1. `elapsed_min < 60` は不受理。A/B/C/D/E いずれも、同一タスクを新規 `session_token` で再開し、`60 <= elapsed_min <= 90` を満たすまで受理しない。
  2. `elapsed_min > 90` も不受理。D/E を含め、作業量を分割し、`guard60=pass` 後に 90分を超える前に終了する。
  3. 同一コマンド反復、長時間ソーク、guard待ちのための検証積み増しは禁止する。時間が余った場合は同一スコープの次タスクへ進む。
  4. Bチームは旧 `B-45/B-46` 系と `mbd_b45_acceptance` を再開しない。新ロードマップ `B-01` 以降のみを対象とする。
  5. Cチームは build/FEM API に直結しない広域回帰を時間充足目的で実施しない。C-03 が早く終わった場合は C-04 へ進む。
  6. D/Eチームは 2-link flexible 本線を前進させる。docs更新のみで終了せず、90分超過もしない。

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
- Status: `In Progress`
- Goal: `MBD_BODY_DYN` / `MBD_GRAVITY` / `MBD_FORCE` を parse できるようにする。

### A-03
- Status: `Todo`
- Goal: gravity / user load の assemble API を作る。

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
- Status: `In Progress`
- Goal: dense KKT assembler を作る。

### B-03
- Status: `Todo`
- Goal: 小規模 dense solver を作る。

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
- Status: `In Progress`
- Goal: full mesh 再アセンブルを明示化する。
- Acceptance:
  - `flex_solver2d_reassemble_and_solve()` ごとに `full_reassembly_count` が進む。
  - `static_solve_count` と合わせて per-model の再アセンブル/solve 監査の土台がある。

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
- Status: `In Progress`
- Goal: snapshot 出力を coupled に接続する。

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
- Status: `In Progress`
- Goal: 2-link flexible input を作る。

## 10. 比較データに関する扱い
- RecurDyn / AdamsFlex の実データは現時点では不要。
- 今必要なのは `docs/09_compare_schema_2d.md` に定義した列構成に合わせて、FEM4C 側の出力を固定すること。
- 実データ比較は M4 で行う。
