# チーム別 Runbook（現行運用）

最終更新: 2026-02-07  
対象: PM-3 / Aチーム / Bチーム / Cチーム

## 1. 目的
- 現在のプロジェクト目標（`docs/long_term_target_definition.md`）に沿って、A/B/C の日次運用を統一する。
- 実装対象を `FEM4C` の現行スプリントへ限定し、旧 Chrono 運用の誤参照を防ぐ。

## 2. 参照優先順位（必須）
1. 長期目標: `docs/long_term_target_definition.md`
2. 現行ディスパッチ: `docs/abc_team_chat_handoff.md`（Section 0 のみ）
3. 次タスクキュー: `docs/fem4c_team_next_queue.md`
4. 進捗報告: `docs/team_status.md`
5. 継続ログ: `docs/session_continuity_log.md`

## 3. スコープ
- In Scope:
  - FEM ソルバー維持・補強（FEM4C）
  - 2D MBD コア移植（Project Chrono コア）
  - 共通 parser 契約の整備
  - 学習用ドキュメント強化
- Out of Scope（当面）:
  - FEM-MBD 連成本実装
  - 3D MBD 本実装
  - 通知運用・Nightly 周辺オペレーション

## 4. チーム運用ルール
- チャット指示が「作業を継続してください」のみの場合:
  - これを「省略指示モード」と定義する。
  - 各チームは追加質問なしで、次の順に自律実行する:
    1. `docs/abc_team_chat_handoff.md` の Section 0 を確認
    2. `docs/fem4c_team_next_queue.md` の自チーム先頭 `Todo` / `In Progress` を着手対象として確定
    3. 着手タスクを `In Progress` に更新して実行開始
  - PM への確認は blocker 発生時のみ許可する（「次に何をやるか」の確認は不要）。
- 自走セッション（短時間・連続運転）:
  - 1回の指示で 15分以上を必須とし、15-30分を推奨レンジとして連続実行する。
  - 進捗連絡は原則セッション末尾に 1 回のみ（小分け報告をしない）。
  - 先頭タスクが早く終わった場合は、同じセッション内で次タスクへ連続着手する。
  - PM 判断が必要な blocker が出ても、15分未満の時点では終了せず、同一セッション内で次の実行可能タスクへ継続する。
  - 15分未満で終了できる例外は、PMが事前承認した緊急停止（環境障害/誤破壊リスク）のみとする。
  - `sleep` 等の待機コマンドで elapsed を稼ぐ行為を禁止する（人工待機禁止）。
- セッション時間の証跡（必須）:
  - 手入力の `start_at/end_at/elapsed_min` は証跡として無効とする。
  - 開始時に `scripts/session_timer.sh start <team_tag>` を実行し、`session_token` を取得する。
  - 終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力（`start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）を `docs/team_status.md` にそのまま貼る。
  - 受入には `elapsed_min >= 15` を必須とし、あわせて実作業証跡（変更ファイル・実行コマンド・pass/fail根拠）を確認する。
  - 15分未満で先頭タスクが完了した場合は、待機せず次タスク着手または blocker 解消作業へ進む。
  - blocker 終了時は「試した対応」「失敗理由」「PMに必要な判断」を 3 点セットで記録する。
- 作業終了時は必ず以下を更新する:
  - `docs/team_status.md`
  - `docs/session_continuity_log.md`（4項目: `Current Plan`, `Completed This Session`, `Next Actions`, `Open Risks/Blockers`）
- コミット時の制約:
  - 担当外ファイルをステージしない。
  - 生成物（`*.dat`, `*.csv`, `*.vtk`, `*.f06`）をコミットしない。
  - `FEM4C` と `chrono-2d` の混在コミットを禁止する。

## 5. A/B/C の責務（現行スプリント）
- Aチーム（実装）:
  - `FEM4C/src/analysis/runner.*` と `FEM4C/src/mbd/*` を中心に、`mbd` 実行経路を段階実装する。
- Bチーム（検証）:
  - `practice/ch09` の拘束検証ハーネスと有限差分照合を維持・拡張する。
- Cチーム（差分整理）:
  - dirty 差分の 3分類（残す/除外/保留）と安全ステージング手順を維持する。

## 6. 受入・検証
- 各タスクの受入条件は `docs/fem4c_team_next_queue.md` の `Acceptance` を一次基準とする。
- 受入に使うコマンド・結果（pass/fail）は `docs/team_status.md` へ記録する。
- 以下のいずれかに該当する報告は差し戻す:
  - `scripts/session_timer.sh` の出力証跡が未記載
  - `elapsed_min < 15`（PM事前承認の緊急停止を除く）
  - 人工待機（`sleep` 等）で elapsed を満たした痕跡がある
  - 実作業証跡（変更ファイル・実行コマンド・pass/fail）が不足している
  - `Done` 0件かつ次タスク `In Progress` なし

## 7. アーカイブ方針
- 旧 Chrono 運用・旧 PM 司令文書は以下へ退避済み（参照のみ）:
  - `docs/archive/legacy_chrono/`
- 実装前ドラフトは以下へ退避済み:
  - `docs/archive/drafts/`
- 現行運用では、上記アーカイブ配下をタスク指示の一次参照に使わない。

## 8. コンテクスト切れ時の新規チャット移行手順（必須）
- 適用条件:
  - チャットが長文化し、PMが継続運用リスクを感じた場合。
  - 途中中断やコンテクスト喪失で、同一チャット継続が不安定な場合。
- PMの移行前作業:
  1. `docs/session_continuity_log.md` の末尾に 4項目（`Current Plan` / `Completed This Session` / `Next Actions` / `Open Risks/Blockers`）を追記する。
  2. `docs/team_status.md` に PM判断（差し戻し/受入/次指示）を追記する。
  3. 必要なルール変更があれば、`docs/abc_team_chat_handoff.md` Section 0 と `docs/fem4c_team_next_queue.md` に先に反映する。
- 新規チャット開始時の初動:
  1. `docs/long_term_target_definition.md` と `docs/abc_team_chat_handoff.md` Section 0 を再確認する。
  2. `docs/fem4c_team_next_queue.md` の先頭 `In Progress` / `Todo` を起点に再開する。
  3. 各チームへの初回送信は `docs/fem4c_team_dispatch_2026-02-06.md` のテンプレを使う。
  4. 2回目以降は「作業を継続してください」の1行運用へ戻す。
- 差し戻しの固定基準:
  - `elapsed_min < 15`、または `scripts/session_timer.sh` 証跡なしは不受理。
  - 人工待機（`sleep` 等）で elapsed を満たした報告は不受理。
  - 不受理時は、同一タスク継続で再実行させる（タスク飛ばし禁止）。
