# チーム別 Runbook

本書は Chrono の C 移植を進めるための方針と、直近の実行タスクのみをまとめます。

## 方針とマイルストーン
- 目的: Chrono の C 移植を最優先で進め、検証の再現性と記録を最小限の運用で担保する。
- マイルストーン: Aチーム=移植本体と検証、Bチーム=移植前捌き（棚卸し/差分整理/最小サンプル）、Cチーム=ドキュメント導線整備。
- Run ID 書式: `local-chrono2d-YYYYMMDD-XX` / `#<ID> (chrono-main)` / `#<ID> (Chrono C)` を厳守。
- 報告: `docs/team_status.md` の自チーム欄のみ更新（Run ID/Artifact/Log/`git status`/リンクチェック結果）。
- 生成物: `artifacts/*.csv` やテストバイナリはコミットしない。
- リンクチェック: `python scripts/check_doc_links.py <対象md...>` を実行し結果を共有。
- Changelog: ドキュメント更新があれば `docs/documentation_changelog.md` に記録。

## 直近で実施すべきタスク
- Aチーム: `@A-team 実行: A10,A14,A18`  
  A10 dump-json の出力例を README に 1 件追加し、最小再現の具体例を残す。A14 感度レンジの初期値レビュー（高/中/低レンジの根拠を1行メモ）。A18 複合拘束の追加候補をもう1組用意し、判定条件を `chrono-2d/docs/constraints.md` に追記。
- Bチーム: `@B-team 実行: A5,A7,A11,B1`  
  `chrono-2d/data/generated` の生成物レイアウト（固定パス/命名）を README に 1 ページで整理し、Aチームの運用導線を明確化。C↔C++ 対応表に「対応済み/未対応」列を追加して見える化。
- Cチーム: `@C-team 実行: C10,C7`  
  用語・表記ガイドの本文化（用語表＋統一ルール）と、学習ステップのチェックリスト化（コマンド＋期待出力）を 1 ページにまとめる。
