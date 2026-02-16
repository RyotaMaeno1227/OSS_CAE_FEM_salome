# FEM4C Team Dispatch Messages (2026-02-06)

このファイルは PM-3 から各チームへ送る連絡文テンプレです。  
そのまま各チームチャットへ貼り付けて使ってください。

---

## Team A 向け連絡文（実装）

```
@A-team
PM-3 依頼です。今回スプリントは FEM4C Phase 2 を優先してください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- mbd 独立ソルバー実行モードの実装側
- 対象は handoff の Aチーム欄どおり（runner/fem4c 周辺）

[進捗報告先]
1) docs/team_status.md に「Aチーム」欄で実行内容・コマンド・結果を追記
2) docs/session_continuity_log.md にセッション終了時の4項目
   - Current Plan
   - Completed This Session
   - Next Actions
   - Open Risks/Blockers

[受入チェック]
- handoff の Aチーム受入基準を満たしたら、team_status に完了判定を明記してください。
```

---

## Team B 向け連絡文（検証）

```
@B-team
PM-3 依頼です。今回スプリントは FEM4C MBD拘束APIの数値検証を優先してください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- distance/revolute の残差・ヤコビアン検証ハーネス
- 有限差分照合と閾値設定

[進捗報告先]
1) docs/team_status.md に「Bチーム」欄で実行コマンド、閾値、pass/fail を追記
2) docs/session_continuity_log.md にセッション終了時の4項目を追記

[受入チェック]
- handoff の Bチーム受入基準を満たしたら、再現コマンド1行を team_status に必ず記載してください。
```

---

## Team C 向け連絡文（差分整理）

```
@C-team
PM-3 依頼です。今回スプリントは FEM4C の巨大 dirty 差分の整理を最優先で進めてください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- 差分3分類（実装として残す / 生成物・不要物 / 意図不明）
- FEM4C/test/* 削除群の暫定判定
- 安全な git add 手順案の作成

[進捗報告先]
1) docs/team_status.md に「Cチーム」欄で分類結果と判断根拠を追記
2) docs/session_continuity_log.md にセッション終了時の4項目を追記

[成果物]
- docs/abc_team_chat_handoff.md の Cチーム受入基準を満たす整理レポートを docs/ 配下へ追加してください。
- staging 検証は `scripts/c_stage_dryrun.sh` を優先使用し、`dryrun_result` を team_status に記録してください。
```

---

## PM メモ

- 全チーム共通で、まず `docs/abc_team_chat_handoff.md` の Section 0 を読む。
- 進捗は `docs/team_status.md`、セッション引継ぎは `docs/session_continuity_log.md`。
- 混在コミット回避のため、担当範囲外ファイルはステージしない。
- PM受入の機械監査: `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30`
- 外部CI未接続時は、日次受入をローカル3コマンドで完結する:
  - `make -C FEM4C test`
  - `make -C FEM4C mbd_ci_contract`
  - `make -C FEM4C mbd_ci_contract_test`
- GitHub Actions 実Run確認は PM/ユーザーが節目で数回のみ実施し、毎セッション必須にしない。
- C-team staging運用の機械監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass`
- C-team見出し位置まで含めた厳格監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section`
- C-team見出し位置 + coupled凍結ポリシー監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze`
- C-team見出し位置 + coupled凍結 + timer完了監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer`
- C-team staging監査 + 関連テスト一括: `bash scripts/run_c_team_staging_checks.sh docs/team_status.md`
- 差し戻し文面を一括生成する場合: `bash scripts/run_team_audit.sh docs/team_status.md 30 pass_section_freeze`
- 履歴の遵守率確認（短時間終了の傾向分析）: `python scripts/audit_team_history.py --team-status docs/team_status.md --min-elapsed 30`
- 先頭タスク付きチャット文を自動生成する場合: `python scripts/render_team_dispatch_from_queue.py --team all`

---

## 30分開発モード（次回以降の推奨文面）

以下をそのまま送ると、短時間終了を避けて自走しやすくなります。

### A/B/C 共通文面

```
作業を継続してください。今回は30分開発モードです。

[参照先]
- docs/abc_team_chat_handoff.md Section 0
- docs/fem4c_team_next_queue.md
- docs/team_runbook.md

[実行ルール]
- 30分以上を必須とし、30-45分を推奨レンジとして連続実行する。
- 30分は実装前進に使い、実装系ファイル差分を1件以上必須とする。
- 長時間反復ソーク/耐久ループは禁止（PM明示指示時のみ例外）。
- 検証は短時間スモークに限定し、最大3コマンド程度で受入確認する。
- 先頭タスク完了時は同セッションで次タスクへ自動遷移する（PM確認不要）。
- 次タスクが無い場合は `Auto-Next`（最小実装タスク）を `next_queue` に追記して継続する。
- 同一コマンドの反復実行だけで時間を使わない（コード変更なしの連続反復は禁止）。
- 先頭タスク完了後の遷移先は `docs/fem4c_team_next_queue.md` の PM固定優先に従う（A→A-31, B→B-24, C→C-31）。
- 進捗報告はセッション末尾に1回のみ（小分け報告しない）。
- session_continuity_log だけ更新した報告は不合格。
- 開始時に `scripts/session_timer.sh start <team_tag>` を実行し、`session_token` を取得する。
- 報告直前に `bash scripts/session_timer_guard.sh <session_token> 30` を実行し、`guard_result=pass` を確認する。
- 終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力を team_status に貼る。
- 手入力の `start_at/end_at/elapsed_min` だけの報告は不合格。
- `sleep` 等の人工待機で elapsed を満たす行為は禁止（不合格）。
- `elapsed_min >= 30` を満たさない終了報告は原則不合格（PM事前承認の緊急停止のみ例外）。
- `elapsed_min < 30` の途中報告は禁止（30分到達まで同セッションで継続）。
- 外部CI未接続時でも、ローカル3コマンド（`test` / `mbd_ci_contract` / `mbd_ci_contract_test`）は必須で実行する。

[終了条件]
- `elapsed_min >= 30` を満たす。
- Doneタスクを1件以上作る。
- 実装系ファイル差分を1件以上含む。
- 変更ファイル・実行コマンド・pass/fail根拠を team_status に記録する。
- 次タスクを In Progress にするか、blocker を明記して終了する。
```

## 最小チャット運用（固定文面）

次回以降は、以下の 1 行だけ送れば運用可能です。

```
作業を継続してください
```

解釈ルール:
- この 1 行は「省略指示モード」の開始を意味する。
- 各チームは `docs/abc_team_chat_handoff.md` Section 0 と `docs/fem4c_team_next_queue.md` を自動参照し、先頭未完了タスクへ着手する。
- PM への追加確認は blocker 発生時のみ許可する。
- 受入判定は `scripts/session_timer.sh` 出力と `scripts/session_timer_guard.sh` の `guard_result=pass` を含む報告のみ有効とする。
- `elapsed_min >= 30` を満たさない報告は原則差し戻す。
- 人工待機（`sleep` 等）を含む報告は無効とする。

## 新規チャット移行時のPM初回送信テンプレ（コピペ用）

```
新規チャットへ移行します。まず以下を確認してから再開してください。

[必読]
1) docs/long_term_target_definition.md
2) docs/abc_team_chat_handoff.md の Section 0
3) docs/fem4c_team_next_queue.md
4) docs/team_runbook.md の「8. コンテクスト切れ時の新規チャット移行手順」

[運用]
- 以降は省略指示モードです。「作業を継続してください」で進めます。
- elapsed_min < 30、または session_timer 証跡なしは不受理です。
- 不受理時は同一タスクを継続し、再提出してください。
```

---

## PMレビュー後の次ラウンド指示（2026-02-15, 最新コピペ用）

以下の3本をそのまま送信してください。  
次回以降は原則「作業を継続してください」の1行運用に戻して構いません（省略指示モード）。

### Team A

```
@A-team
作業を継続してください（30分以上、推奨30-45分の連続実行）。

[今回のゴール]
- `docs/fem4c_team_next_queue.md` の Aチーム先頭 `In Progress`（A-31）を `Done` に近づける。
- 先頭完了後は同セッションで次タスクを `In Progress` にして継続する。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Aチーム先頭 `Todo` / `In Progress` から開始
- A先頭タスクの完了条件を満たす差分を優先（`--mode=mbd` 対象のコード差分必須）

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start a_team`
- 報告可否判定: `bash scripts/session_timer_guard.sh <session_token> 30`
- 終了: `scripts/session_timer.sh end <session_token>`

[Aチーム専用: 動的自走ルール（必須）]
- `elapsed_min < 30` の途中報告は禁止。30分到達まで同セッションで実装を継続する。
- A-31 が早く完了した場合は、同セッションで次タスク（`Todo` / `In Progress`）へ自動遷移する。
- 次タスク候補が無い場合は `Auto-Next` を `docs/fem4c_team_next_queue.md` に追記して継続する。
- 報告直前に自己監査を実行し、FAILなら継続する:
  - `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30 --teams A`
- `session_timer_guard` が `guard_result=block` の間はチャット報告せず、同一セッションで次実装へ進む。

[禁止事項]
- 長時間反復ソーク/耐久ループで時間を消費しない。
- `sleep` 等の人工待機をしない。
- 反復検証のみで終了しない。

[必須成果]
- 実装差分ファイル（docsのみは不可）
- A先頭タスクの `Acceptance` を満たすこと。
- 検証は短時間スモーク（最大3コマンド）で行うこと。
- 実行コマンド
- 受入判定 pass/fail
- `scripts/session_timer.sh` の出力一式（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）
- `scripts/session_timer_guard.sh` の出力（`guard_result=pass`）
- `elapsed_min >= 30`（未満は原則差し戻し）

[報告先]
- docs/team_status.md
- docs/session_continuity_log.md（4項目）
```

### Team B

```
@B-team
作業を継続してください（30分以上、推奨30-45分の連続実行）。

[今回のゴール]
- B-24（B-8自己テスト一時ファイル衝突の静的契約固定）を実装で前進させる。
- 先頭完了後は同セッションで次タスクを `In Progress` にして継続する。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Bチーム先頭 `Todo` / `In Progress` から開始
- B-24 の完了条件を満たす差分を優先（自己テスト用一時スクリプト名の衝突回避 + 静的契約同期）
- 先頭完了後は同セッションで次タスクへ自動遷移（候補が無ければ `Auto-Next` を追記）

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start b_team`
- 終了: `scripts/session_timer.sh end <session_token>`

[禁止事項]
- 長時間反復ソーク/耐久ループで時間を消費しない。
- `sleep` 等の人工待機をしない。
- B-8系の耐久反復だけで終了しない。

[注意]
- docs更新のみで終了しないこと（無効報告）

[必須成果]
- 変更ファイル（Makefile/README/probe など実装差分を含む）
- B-24（B-8自己テスト一時ファイル衝突の静的契約固定）を前進させること。
- 検証は短時間スモーク（最大3コマンド）で行うこと。
- 1行再現コマンド
- pass/fail と閾値
- `scripts/session_timer.sh` の出力一式（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）
- `elapsed_min >= 30`（未満は原則差し戻し）

[報告先]
- docs/team_status.md
- docs/session_continuity_log.md（4項目）
```

### Team C

```
@C-team
作業を継続してください（30分以上、推奨30-45分の連続実行）。

[今回のゴール]
- C-35（strict latest 失敗理由の提出ログ固定）を前進させる。
- 先頭完了後は同セッションで次タスクを `In Progress` にして継続する。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Cチーム先頭 `Todo` / `In Progress` から開始
- C-35 の完了条件を満たす差分を優先（`collect_preflight_check_reason=*` の提出ログ固定）
- 先頭完了後は同セッションで次タスクへ自動遷移（候補が無ければ `Auto-Next` を追記）

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start c_team`
- 終了: `scripts/session_timer.sh end <session_token>`

[禁止事項]
- 長時間反復ソーク/耐久ループで時間を消費しない。
- `sleep` 等の人工待機をしない。

[必須成果]
- 最終判定が入った triage 文書差分
- C-35 は strict latest fail-fast 時の理由キー（`collect_preflight_check_reason=*`）を提出ログへ残し、判定理由を追跡可能にすること。
- 検証は短時間スモーク（最大3コマンド）で行うこと。
- 具体的コマンド（必要なら .gitignore 更新）
- pass/fail 判定
- `scripts/c_stage_dryrun.sh` の結果（`dryrun_result`）
- `safe_stage_command=git add <path-list>` の記録（strict-safe 判定用）
- strict latest 提出テンプレ（必須）:
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --team-status docs/team_status.md --append-to-team-status --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1`
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <session_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1`
- `scripts/session_timer.sh` の出力一式（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）
- `elapsed_min >= 30`（未満は原則差し戻し）

[報告先]
- docs/team_status.md
- docs/session_continuity_log.md（4項目）
```
