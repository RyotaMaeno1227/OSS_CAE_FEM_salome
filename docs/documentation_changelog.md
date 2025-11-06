# Documentation Changelog (Draft)

このページは Coupled/3D 関連ドキュメントの更新履歴を集約するための草案です。正式運用時は週次で最新エントリを追加し、Wiki / 社内ポータルと同期してください。

## 2025-10-21 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/chrono_coupled_constraint_tutorial.md` | メディア生成ケーススタディ追加、英語アウトライン＋用語集の整備 | Cチーム（Mori） | GitHub Pages への埋め込み手順と多言語展開準備を追記 |
| `docs/chrono_3d_abstraction_note.md` | KPI 表に工数/リスク列を追加、進捗バー／簡易ガントテンプレート、月次レポート案を掲載 | アーキ WG（Sato） | 3D 移行可視化の基礎資料 |
| `docs/wiki_coupled_endurance_article.md` | Wiki 投稿テンプレート強化、スクリーンショット要件と運用チェックリストを追加 | DevOps（Suzuki） | `docs/wiki_samples/coupled_endurance_article_sample.md` を参照 |
| `docs/coupled_constraint_presets_cheatsheet.md` | Pandoc 依存の検証ログ、動画化ガイドラインを追記 | Cチーム ドキュメント班（Nakajima） | `docs/media/coupled/` に動画配置予定 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | Wiki 記事のマークアップ済み雛形を新規追加 | DevOps（Suzuki） | Confluence / Markdown 双方で利用可 |

## 2025-11-03 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/chrono_coupled_constraint_tutorial.md` | Stride を用いた `matplotlib` アニメーション例と GitHub Pages 埋め込みクイックリファレンスを追加 | Cチーム（Mori） | Section 9.1/9.2 として追補 |
| `docs/chrono_3d_abstraction_note.md` | KPI/ガントを拘束・接触・並列タスクへ再編し、月次メモを数値指標基準に更新 | アーキ WG（Sato） | Section 10 のテンプレート強化 |
| `docs/wiki_coupled_endurance_article.md` | Wiki 投稿テンプレ利用ガイドと公開後チェック手順を明文化 | DevOps（Suzuki） | Section 6/7 の補足 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | 付録としてテンプレ全文・スクリーンショット一覧・公開フロー確認リストを追加 | DevOps（Suzuki） | 付録 A/B/C を新設 |
| `docs/coupled_island_migration_plan.md` | Coupled/Island 解法の差分と chrono-main への移行ステップを整理 | Cチーム（Mori） | 新規追加 |
| `docs/coupled_constraint_solver_math.md` | Coupled 拘束の行列導出・ピボット戦略を数式付きで解説 | Cチーム（Mori） | 新規追加 |
| `docs/optional_features_appendix_plan.md` | 運用・通知系コンテンツを Appendix へ移す計画を作成 | ドキュメント班（Nakajima） | 新規追加 |
| `docs/chrono_2d_development_plan.md` | テスト一覧を計算コアに限定したカテゴリ表へ更新 | Cチーム（Mori） | Section 3.4 |
| `docs/coupled_contact_api_minimal.md` | Coupled/接触に必要な最小 API セットを明文化 | Cチーム（Mori） | 新規追加 |

## 運用メモ
- 変更日・担当者・Pull Request を必ず記録する。表は最新が上に来るように追記。
- 大きな構成変更（章追加、テンプレート刷新）は別途詳細セクションを作成し、影響範囲を記載する。
- `docs/documentation_changelog.md` 更新後は Slack `#chrono-docs` に通知し、Wiki 側の履歴ページも同期する。

--- 

未反映の変更がある場合は、このファイルのドラフトに先に追記し、レビュー完了後に日付を確定させてください。
