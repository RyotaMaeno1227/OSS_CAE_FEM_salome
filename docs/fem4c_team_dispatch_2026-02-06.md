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
- PM受入の機械監査: `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30 --max-elapsed 90`
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
- 上限目安は60分。`elapsed_min > 90` の報告はトークン使い回し/中断混在の疑いとして原則不合格。
- 30分は実装前進に使い、実装系ファイル差分を1件以上必須とする。
- 長時間反復ソーク/耐久ループは禁止（PM明示指示時のみ例外）。
- 検証は短時間スモークに限定し、最大3コマンド程度で受入確認する。
- 検証は「今回変更した実装に直結する受入コマンド」を優先し、全体回帰（例: `python -m unittest discover -s scripts -p 'test_*.py'`, `make -C FEM4C test`）は受入条件または障害切り分けで必要な場合のみ実行する。
- 30分条件を満たすための検証コマンド積み増しは禁止（時間充足目的の回帰実行は不合格）。
- 先頭タスク完了時は同セッションで次タスクへ自動遷移する（PM確認不要）。
- 次タスクが無い場合は `Auto-Next`（最小実装タスク）を `next_queue` に追記して継続する。
- 同一コマンドの反復実行だけで時間を使わない（コード変更なしの連続反復は禁止）。
- 先頭タスク完了後の遷移先は `docs/fem4c_team_next_queue.md` の PM固定優先に従う（A→A-31, B→B-24, C→C-31）。
- 進捗報告はセッション末尾に1回のみ（小分け報告しない）。
- session_continuity_log だけ更新した報告は不合格。
- 開始時に `scripts/session_timer.sh start <team_tag>` を実行し、`session_token` を取得する。
- 報告直前に `bash scripts/session_timer_guard.sh <session_token> 30` を実行し、`guard_result=pass` を確認する。
- 中間証跡として `bash scripts/session_timer_guard.sh <session_token> 10` / `20` を実行し、出力を `team_status` に転記する。
- 終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力を team_status に貼る。
- PMのIDE観測で実稼働が30分未満と判断された場合は報告無効。新規 `session_token` で同タスクを再実行する。
- 手入力の `start_at/end_at/elapsed_min` だけの報告は不合格。
- `sleep` 等の人工待機で elapsed を満たす行為は禁止（不合格）。
- `elapsed_min >= 30` を満たさない終了報告は原則不合格（PM事前承認の緊急停止のみ例外）。
- `elapsed_min > 90` の終了報告は、合理的な継続理由が明記されない限り不合格。
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
- `elapsed_min > 90` の報告も原則差し戻す（合理的な継続理由の明記がある場合のみ例外）。
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

## PMレビュー後の次ラウンド指示（2026-02-21, 最新コピペ用）

以下の3本をそのまま送信してください。  
前回提出は「実稼働時間の整合不一致」として不受理扱いです。今回は再実行ラウンドです。

### Team A

```
@A-team
前回提出は PM 判定で不受理です（実稼働時間の整合が取れないため）。A-38 を再実行してください。

[対象]
- docs/fem4c_team_next_queue.md の A-38（In Progress）

[必須]
- 新規セッションで開始: scripts/session_timer.sh start a_team
- 30分以上の連続実作業（待機・放置・sleep禁止）
- 先頭タスク完了後も同一セッションで A-38 スコープの次実装を継続
- 報告前: bash scripts/session_timer_guard.sh <session_token> 30
- 終了: scripts/session_timer.sh end <session_token>

[検証]
- make -C FEM4C mbd_a24_regression_full_test
- make -C FEM4C mbd_a24_batch_test
- make -C FEM4C mbd_ci_contract_test

[報告]
- docs/team_status.md に新規エントリを追記（変更ファイル/コマンド/pass-fail/タイマー原文）
- docs/session_continuity_log.md の4項目更新
```

### Team B

```
@B-team
前回提出は PM 判定で不受理です（実稼働時間の整合が取れないため）。B-32 を再実行してください。

[対象]
- docs/fem4c_team_next_queue.md の B-32（In Progress）

[必須]
- 新規セッションで開始: scripts/session_timer.sh start b_team
- 30分以上の連続実作業（待機・放置・sleep禁止）
- 報告前: bash scripts/session_timer_guard.sh <session_token> 30
- 終了: scripts/session_timer.sh end <session_token>

[検証]
- make -C FEM4C mbd_b8_knob_matrix_test
- make -C FEM4C mbd_ci_contract_test
- make -C FEM4C mbd_b8_regression_test

[報告]
- docs/team_status.md に新規エントリを追記（変更ファイル/コマンド/pass-fail/タイマー原文）
- docs/session_continuity_log.md の4項目更新
```

### Team C

```
@C-team
前回提出は PM 判定で不受理です（同一コマンド連続実行検知あり）。C-43 を再実行してください。

[対象]
- docs/fem4c_team_next_queue.md の C-43（In Progress）

[必須]
- 新規セッションで開始: scripts/session_timer.sh start c_team
- 30分以上の連続実作業（待機・放置・sleep禁止）
- 同一コマンドの連続実行は禁止（同じコマンドを続けて打たない）
- 報告前: bash scripts/session_timer_guard.sh <session_token> 30
- 終了: scripts/session_timer.sh end <session_token>

[検証]
- python scripts/test_collect_c_team_session_evidence.py
- python scripts/test_recover_c_team_token_missing_session.py
- python scripts/test_run_c_team_collect_preflight_check.py
- C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30

[報告]
- docs/team_status.md に新規エントリを追記（変更ファイル/コマンド/pass-fail/タイマー原文）
- docs/session_continuity_log.md の4項目更新
```

---

## 時間未達の差し戻しテンプレ（2026-03-01 / A-53, B-45, C-58）

以下は、`elapsed_min < 30` で不受理だった直後にそのまま送るテンプレです。

### Team A（A-53 再開）

```
@A-team
前回ランは elapsed 未達のため不受理。A-53を同一タスクで再開してください。

[必須]
- 新規開始: scripts/session_timer.sh start a_team
- 中間: bash scripts/session_timer_guard.sh <token> 10
- 中間: bash scripts/session_timer_guard.sh <token> 20
- 終了前: bash scripts/session_timer_guard.sh <token> 30 （pass必須）
- 終了: scripts/session_timer.sh end <token>

[終了禁止条件]
- guard30 が block の間は終了報告禁止
- docs更新だけで終了禁止（実装差分必須）
- 同一コマンド反復で時間消化禁止

[実装対象]
- A-53: canonical pair marker（owner_pid, lock_wait_sec）を check/test の static + fail-injection で固定

[受入]
- make -C FEM4C mbd_ci_contract_test
- make -C FEM4C mbd_a24_regression_full_test
- make -C FEM4C mbd_a24_batch_test
- すべてPASS + elapsed_min>=30
```

### Team B（B-45 再開）

```
@B-team
前回ランは elapsed 未達のため不受理。B-45を同一タスクで再開してください。

[必須]
- 新規開始: scripts/session_timer.sh start b_team
- 中間: bash scripts/session_timer_guard.sh <token> 10
- 中間: bash scripts/session_timer_guard.sh <token> 20
- 終了前: bash scripts/session_timer_guard.sh <token> 30 （pass必須）
- 終了: scripts/session_timer.sh end <token>

[終了禁止条件]
- guard30 が block の間は終了報告禁止
- docs更新だけで終了禁止（実装差分必須）
- 同一コマンド反復で時間消化禁止

[実装対象]
- B-45: LOCK_WAIT_SEC_MAX 契約の再同期
- 受入4コマンドの前後で sha256sum FEM4C/scripts/test_check_ci_contract.sh を記録

[受入]
- make -C FEM4C mbd_ci_contract_test
- make -C FEM4C mbd_b8_knob_matrix_test
- make -C FEM4C mbd_b8_regression_full_test
- make -C FEM4C mbd_b8_regression_test
- すべてPASS + elapsed_min>=30
```

### Team C（C-58 再開）

```
@C-team
前回ランは elapsed 未達のため不受理。C-58を同一タスクで再開してください。

[必須]
- 新規開始: scripts/session_timer.sh start c_team
- 中間: bash scripts/session_timer_guard.sh <token> 10
- 中間: bash scripts/session_timer_guard.sh <token> 20
- 終了前: bash scripts/session_timer_guard.sh <token> 30 （pass必須）
- 終了: scripts/session_timer.sh end <token>

[終了禁止条件]
- guard30 が block の間は終了報告禁止
- docs更新だけで終了禁止（実装差分必須）
- 同一コマンド反復で時間消化禁止

[実装対象]
- C-58: C_REQUIRE_REVIEW_COMMANDS=1 必須モードで collect/recover/readiness/staging の整合固定

[受入]
- C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
- bash scripts/run_c_team_staging_checks.sh docs/team_status.md
- python scripts/test_run_c_team_staging_checks.py
- python scripts/test_collect_c_team_session_evidence.py
- python scripts/test_recover_c_team_token_missing_session.py
- すべてPASS + elapsed_min>=30
```
