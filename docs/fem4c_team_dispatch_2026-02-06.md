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
- mbd/coupled 実行モードの実装側
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
- 差し戻し文面を一括生成する場合: `bash scripts/run_team_audit.sh docs/team_status.md 30`
- 履歴の遵守率確認（短時間終了の傾向分析）: `python scripts/audit_team_history.py --team-status docs/team_status.md --min-elapsed 30`

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
- 先頭タスクが終わったら同セッション内で次タスクへ着手する。
- 先頭タスク完了後の遷移先は `docs/fem4c_team_next_queue.md` の PM固定優先に従う（A→A-17, B→B-14, C→C-19）。
- 進捗報告はセッション末尾に1回のみ（小分け報告しない）。
- session_continuity_log だけ更新した報告は不合格。
- 開始時に `scripts/session_timer.sh start <team_tag>`、終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力を team_status に貼る。
- 手入力の `start_at/end_at/elapsed_min` だけの報告は不合格。
- `sleep` 等の人工待機で elapsed を満たす行為は禁止（不合格）。
- `elapsed_min >= 30` を満たさない終了報告は原則不合格（PM事前承認の緊急停止のみ例外）。

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
- 受入判定は `scripts/session_timer.sh` 出力を含む報告のみ有効とする。
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

## PMレビュー後の次ラウンド指示（2026-02-06, コピペ用）

### Team A

```
@A-team
作業を継続してください（30分以上、推奨30-45分の連続実行。長時間反復ソークは禁止）。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Aチーム先頭 `Todo` / `In Progress` から開始
- A-16 完了後は同セッション内で A-17 を `In Progress` にして継続

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start a_team`
- 終了: `scripts/session_timer.sh end <session_token>`

[必須成果]
- 実装差分ファイル（docsのみは不可）
- A-16 は HHT-α 方式の実装差分を前進させること（反復検証だけで終了しない）。
- 実行コマンド
- 受入判定 pass/fail
- `scripts/session_timer.sh` の出力一式（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）
- `elapsed_min >= 30`（未満は原則差し戻し）

[報告先]
- docs/team_status.md
- docs/session_continuity_log.md（4項目）
```

### Team B

```
@B-team
作業を継続してください（30分以上、推奨30-45分の連続実行。長時間反復ソークは禁止）。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Bチーム先頭 `Todo` / `In Progress` から開始
- B-12 完了後は同セッション内で B-14 を `In Progress` にして継続

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start b_team`
- 終了: `scripts/session_timer.sh end <session_token>`

[注意]
- docs更新のみで終了しないこと（無効報告）

[必須成果]
- 変更ファイル（Makefile/README/probe など実装差分を含む）
- B-12（積分法切替回帰）を前進させること。B-8の耐久反復のみで終了しない。
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
作業を継続してください（30分以上、推奨30-45分の連続実行。長時間反復ソークは禁止）。

[今回の着手タスク]
- docs/fem4c_team_next_queue.md の Cチーム先頭 `Todo` / `In Progress` から開始
- C-18 完了後は同セッション内で C-19 を `In Progress` にして継続

[時間証跡コマンド]
- 開始: `scripts/session_timer.sh start c_team`
- 終了: `scripts/session_timer.sh end <session_token>`

[必須成果]
- 最終判定が入った triage 文書差分
- C-18 は短時間スモーク + `scripts/c_stage_dryrun.sh` の実行結果を必須とする（長時間ループ禁止）。
- 具体的コマンド（必要なら .gitignore 更新）
- pass/fail 判定
- `scripts/c_stage_dryrun.sh` の結果（`dryrun_result`）
- `scripts/session_timer.sh` の出力一式（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）
- `elapsed_min >= 30`（未満は原則差し戻し）

[報告先]
- docs/team_status.md
- docs/session_continuity_log.md（4項目）
```
