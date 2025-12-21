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
- Aチーム: `@A-team 実行: A5,A6,A8,A12,A18`  
  A5 外部定義の移行タスク票を確定し、A6 新拘束タイプ追加テンプレの下書きを作成。A8 警告修正の残件があれば対応し、A12 ベンチ warn-only→fail を再実行して drift 差分を記録。A18 複合拘束ケースの追加候補と評価観点を具体化。
- Bチーム: `@B-team 実行: B1,B2,B3`  
  移植対象ファイルの棚卸しと C↔C++ 対応表を作成し、Aチームが使える入出力サンプル（最小ケース）を整備。記録は `docs/team_status.md` にまとめる。
- Cチーム: `@C-team 実行: C4,C6,C9,C12`  
  条件数/ピボット解説の更新、リンク/整合チェックの実行、CSV スキーマ説明の差分確認、フォーマット/Lint の実施結果を記録。
