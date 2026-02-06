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

## コンテクスト継続ルール（必須）
- 注意: チャットのコンテクスト長により途中でトークン切れが発生する可能性がある。
- そのため、PM/各チームは毎回の作業終了時に `docs/session_continuity_log.md` を更新し、以下を必ず記録する。
- 記録項目:
  - `Current Plan`（次に実行する計画）
  - `Completed This Session`（今回実施した内容）
  - `Next Actions`（次アクションを3件以内）
  - `Open Risks/Blockers`（未解決リスク・阻害要因）
- 途中中断時も同様に記録し、次チャット担当は `docs/session_continuity_log.md` を最初に確認してから再開する。

## 直近で実施すべきタスク
- Aチーム: `@A-team 実行: A6,A13,A3`  
  A6 新拘束タイプの実装テンプレを実コードに落とし込む（構造体/ヤコビアン/J組立/最小テストケース/constraints.md 追記まで）。A13 接触モデルに静摩擦緩衝帯と速度減衰パラメータを追加し、A3 で端点ケース（低法線・高速・ゼロ摩擦）を回して回帰を記録する。
- Bチーム: `@B-team 実行: B2,B3,A6支援`  
  Aチーム負荷分散として、A6/A13 で必要な C↔C++ 対応表を API 境界（構造体/関数/I/O）レベルで補完し、最小入出力サンプル（成功系1件・失敗系1件）と期待ログを用意して `docs/abc_team_chat_handoff.md` に引き継ぐ。
- Cチーム: `@C-team 実行: C10,C13,C5`  
  A/B の変更を受けて、`docs/chrono_2d_glossary_checklist.md` の用語統一、`docs/chrono_2d_dataset_guide.md` の更新手順、`docs/chrono_2d_media_rules.md` の命名規約を同期し、`docs/chrono_2d_readme.md` から一貫して辿れる導線に整理する。
