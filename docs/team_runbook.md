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
- 各チームは `scripts/session_timer.sh start` 実行前に `docs/fem4c_team_next_queue.md` の `PM運用メモ` を確認する。
- `PM運用メモ` に個別注意が追記された場合、次セッションから追加チャット無しで自動適用する。
- blocker 以外の問い合わせは禁止する。
- 開始時に `scripts/session_timer.sh start <team_tag>` を実行する。
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
  - `RUNNING`: 稼働中。`guard60` 未達なら終了報告させない。
  - `READY_TO_WRAP`: 稼働中。`guard60` 到達済みで、完了なら終了報告へ進める。
  - `OVERRUN`: 90分超過。ここで区切って終了報告させ、次セッションへ分割する。
  - `READY_NEXT`: 最新報告は受理済み。次回は `作業してください` のみでよい。
  - `NEEDS_REWORK`: 最新報告は不受理。`docs/fem4c_team_next_queue.md` の `PM運用メモ` に従って同一タスクを再開させる。

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

## 8. Legacy運用
- 旧 A/B/C CI-contract 運用文書は以下へ退避済み。
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/team_runbook_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/fem4c_team_next_queue_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/abc_team_chat_handoff_legacy_2026-03-06.md`
- 旧運用を再開する場合は PM 明示指示が必要。
