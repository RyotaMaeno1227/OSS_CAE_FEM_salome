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
- Aチーム: `@A-team 実行: A7,A9,A10,A11,A18`  
  A7 近似誤差許容のケース別定義を作成しテストへ組込み。A9 手計算ミニケースを追加して pivot/cond を厳密比較。A10 異常系ダンプ/復帰機構の拡張案をまとめる。A11 ケース生成スクリプトを強化しパラメータスイープを追加。A18 複合拘束ケースの追加候補と評価観点を具体化。
- Bチーム: `@B-team 実行: B1,B2,B3`  
  移植対象ファイルの棚卸しと C↔C++ 対応表を更新し、Aチーム向けの最小入出力サンプルを整備。Aチームが即テストできるよう README か handoff に導線を追加し、記録は `docs/team_status.md` にまとめる。
- Cチーム: `@C-team 実行: C6,C9,C10,C12`  
  リンク/整合チェック手順の定着、CSV スキーマ説明の差分確認、命名ポリシー短文化、フォーマット/Lint の実施結果を記録する。
