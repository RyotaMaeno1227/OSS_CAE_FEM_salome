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
- Aチーム: `@A-team 実行: A16,A17,A5`  
  A16 Makefile のターゲット整理（test/schema/bench 分離）と依存列挙の更新。A17 ベンチ/テスト実行時間の記録と上限警告の方針を追加。A5 外部定義の読み込みパスをコード側で統一し、サンプルの参照先を一本化する。
- Bチーム: `@B-team 実行: B2,B3`  
  C↔C++ 対応表に API 境界（構造体/関数/I/O）を補足し、対応の差分ポイントを整理する。最小入出力サンプルの検証手順に「失敗時の最小再現」節を追加する。
- Cチーム: `@C-team 実行: C13,C5`  
  例題データセットの説明と更新手順を整理し、図版/スクショの命名・保存・参照ルールを確立して README から参照する。
