# チーム別 Runbook（現行運用）

最終更新: 2026-02-06  
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
  - 各チームは `docs/fem4c_team_next_queue.md` の自チーム先頭 `Todo` / `In Progress` から着手する。
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

## 7. アーカイブ方針
- 旧 Chrono 運用・旧 PM 司令文書は以下へ退避済み（参照のみ）:
  - `docs/archive/legacy_chrono/`
- 実装前ドラフトは以下へ退避済み:
  - `docs/archive/drafts/`
- 現行運用では、上記アーカイブ配下をタスク指示の一次参照に使わない。
