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
| `docs/chrono_coupled_constraint_tutorial.md` | メディア公開手順を付録へ移し、本編は数値チューニングに専念 | Cチーム（Mori） | Section 9.1 リダイレクト |
| `docs/chrono_3d_abstraction_note.md` | KPI/ガントを拘束・接触・並列タスクへ再編し、月次メモを数値指標基準に更新 | アーキ WG（Sato） | Section 10 のテンプレート強化 |
| `docs/wiki_coupled_endurance_article.md` | 運用テンプレ／チェックリストを付録へ移動し、本文をサマリに集約 | DevOps（Suzuki） | Section 6/7 をリダイレクト |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | 付録を削除し、付録ファイル参照のみに整理 | DevOps（Suzuki） | Appendix 参照導線 |
| `docs/coupled_island_migration_plan.md` | バックログ表をガント／進捗バー付きテンプレへ更新 | Cチーム（Mori） | Section 6 |
| `docs/coupled_constraint_solver_math.md` | Coupled 拘束の行列導出・ピボット戦略を数式付きで解説 | Cチーム（Mori） | 新規追加 |
| `docs/coupled_constraint_tutorial_draft.md` | Coupled 拘束の学習ドラフト（数式→実装→テスト）を新設 | Cチーム（Mori） | 新規追加 |
| `docs/appendix_optional_ops.md` | オペレーション（メディア／Wiki／ログ／ベンチ）を集約した付録を作成 | ドキュメント班（Nakajima） | 新規追加 |
| `docs/optional_features_appendix_plan.md` | 運用・通知系コンテンツを Appendix へ移す計画を更新 | ドキュメント班（Nakajima） | Appendix 実装に合わせ調整 |
| `docs/chrono_2d_development_plan.md` | テスト一覧を計算コアに限定したカテゴリ表へ更新 | Cチーム（Mori） | Section 3.4 |
| `docs/coupled_contact_api_minimal.md` | Init/Solve/Diagnostics のフェーズ別 API 表へ再構成（英語併記） | Cチーム（Mori） | 構成変更 |
| `docs/chrono_logging_integration.md` | 運用ヒントを付録へ移動し、本編を API 解説に集中 | DevOps（Suzuki） | Section 5 |
| `docs/coupled_benchmark_setup.md` | 付録への移行に伴い本編をリダイレクト化 | DevOps（Suzuki） | Appendix D 参照 |
| `docs/chrono_3d_abstraction_note.md` | KPI/ガントを最新進捗値（拘束・接触・並列）へ更新 | アーキ WG（Sato） | Section 10 |
| `docs/coupled_constraint_tutorial_draft.md` | 英語節＋図表＋サンプルコードを加え、完成版チュートリアルへ更新 | Cチーム（Mori） | Revamped |
| `docs/coupled_constraint_hands_on.md` | FEM4C 形式のハンズオン手順を新規追加 | Cチーム（Mori） | 新規追加 |
| `docs/coupled_contact_test_notes.md` | Coupled＋Contact 併用テストの意図と判定指標を整理 | Cチーム（Mori） | 新規追加 |
| `docs/wiki_coupled_endurance_article.md` | KPI 定義節を追加し、計算コア指標を強調 | DevOps（Suzuki） | Section 4 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | KPI テーブルと Appendix 参照を追加 | DevOps（Suzuki） | Template update |

## 2025-11-08 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/wiki_coupled_endurance_article.md` | KPI 以外の運用情報を Appendix 経由に集約し、メイン本文をサマリ化 | Cチーム | Appendix B.5 を参照する運用に更新 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | サンプル記事を軽量化し、Appendix B.5/B.6/B.7 への導線のみ残す構成に刷新 | Cチーム | Wiki 反映時は本文だけ差し替える |
| `docs/appendix_optional_ops.md` | B.5 に KPI ローテ／連絡票を追加し、B.6 (Contact+Coupled), B.7 (リンク検証), A.3 (PDF), D (CLI), E (学習統合案) を追記 | Cチーム | Wiki / Slack / Benchmark / 学習パスの運用窓口 |
| `docs/chrono_coupled_constraint_tutorial.md` | メディア／通知手順を撤去し、Appendix A/C 参照のみを残す | Cチーム | 計算コア節へ集中 |
| `docs/coupled_constraint_tutorial_draft.md` | 日英リンクを検証し、Appendix B.7 チェックリストへの誘導を追加 | Cチーム | Hands-on / Solver Math / Contact Notes 参照済み |
| `docs/pm_status_2024-11-08.md` | `docs/coupled_island_migration_plan.md` と同期する KPI 表を追加 | Cチーム | Appendix B.5.1 の担当ローテ管理下 |
| `docs/chrono_3d_abstraction_note.md` | KPI バッジを 80/70/45 に更新し、pm_status / Migration plan と同期する旨を明記 | Cチーム | Section 10 |
| `docs/coupled_island_migration_plan.md` | KPI スナップショット表に最新進捗（82%/72%/48%）を追記し、pm_status との同期手順を追記 | Cチーム | KKT ディスクリプタ PoC 連動 |
| `docs/coupled_constraint_presets_cheatsheet.md` | Appendix A.3 ベースで Markdown 参照へ集約（PDF 依存を撤廃） | Cチーム | Slack `#chrono-docs`/`#chrono-constraints` で周知 |
| `docs/wiki_coupled_endurance_article.md` | Appendix 連携リンク・KPI 棚卸し表・最新 4 週の同期ログを更新 | Cチーム | Appendix B.3/B.4 と同期 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | Appendix 連携版テンプレに合わせてクイックリンクと操作説明を刷新 | Cチーム | サンプルは本文のみを保持 |
| `docs/appendix_optional_ops.md` | A.3（PDF）, B.3/B.4 棚卸し、B.5.1 KPI ログ、B.6/B.7 チェックリスト、D.2/D.3 CLI、E 章リンクを更新 | Cチーム | Wiki / KPI / Benchmark / 学習パスの統合運用 |
| `docs/integration/learning_path_map.md` | Hands-on ↔ Tutorial の章対応とマイルストンを記したドラフトを新規追加 | Cチーム | Appendix E から参照 |
| `docs/coupled_constraint_hands_on.md` | Appendix E / Integration map への案内を冒頭へ追加 | Cチーム | 学習パスでナビゲーションを統一 |
| `docs/coupled_constraint_presets_cheatsheet.md` | Markdown 配布を正式化し、Appendix A.3 に外部 PDF 生成手順（任意）を追記 | Cチーム | Slack `#chrono-docs` / `#chrono-constraints` で共有 |
| `docs/wiki_coupled_endurance_article.md` / `docs/wiki_samples/coupled_endurance_article_sample.md` | 最新 4 週のローテログと PDF リンク、Appendix 連携の棚卸し結果を反映 | Cチーム | Appendix B.3/B.4 と同期 |
| `docs/pm_status_2024-11-08.md`, `docs/coupled_island_migration_plan.md`, `docs/chrono_3d_abstraction_note.md` | KPI を 83 / 73 / 50 に更新し、Appendix B.5.1 へ記録 | Cチーム | 2025-11-10 ローテ |
| `docs/appendix_optional_ops.md` | PDF 最終チェックリスト、Contact+Coupled KPI 通知、Benchmark CLI 出力例、Link Check 手順を追加 | Cチーム | Appendix A/B/D/E を拡充 |
| `docs/coupled_contact_test_notes.md` | Appendix B.6 / Slack KPI テンプレへの導線を追記 | Cチーム | Contact Ops メモを同期 |
| `scripts/check_doc_links.py` | Tutorial/Hands-on/Notes のリンク検証スクリプトを追加 | Tooling | Appendix E.1 から呼び出し |
| `docs/integration/learning_path_map.md` | W2/W3/W4 ステータスと詳細メモを追記 | Cチーム | Appendix E との整合 |
| `docs/coupled_constraint_tutorial_draft.md`, `docs/coupled_constraint_hands_on.md` | 統合ステータス（W2 進行中 / W3 着手）と Learning Path Snapshot 表を追記 | Cチーム | 学習パス進捗を共有 |
| `docs/media/coupled/README.md` | プレースホルダ PDF の注意書きと Slack テンプレを追加 | Cチーム | Appendix A.3.2 と連動 |
| `README.md` | `scripts/check_doc_links.py` の使い方を追加 | Cチーム | ドキュメント lint を周知 |
| `docs/appendix_optional_ops.md` | 外部 PDF 受け渡し、命名規則、Webhook/メール通知、Google カレンダー案、図版ガイドを追加 | Cチーム | Appendix A/C/B.5.1 を拡張 |
| `docs/wiki_coupled_endurance_article.md` | 備考に Pandoc 差し替え待ちと担当・予定日を記載、ローテ表を最新化 | Cチーム | Appendix B.3/B.4 と同期 |
| `docs/wiki_samples/coupled_endurance_article_sample.md` | ローテ表を本編と同書式に更新し、Appendix B.5 への往復リンクを追記 | Cチーム | サンプルとの整合を確保 |
| `docs/chrono_coupled_constraint_tutorial.md` | Appendix 参照リンクを再確認し、運用系記述が無い状態を維持 | Cチーム | Appendix A/C へのリダイレクトのみ残存 |
| `.github/workflows/ci.yaml` | `scripts/check_doc_links.py` を docs lint ステップに追加 | Tooling | Tutorial/Hands-on のリンク検証を CI で強制 |

Slack summary (2025-11-10, #chrono-docs / #chrono-constraints):
- Preset PDF remains provisional (Pandoc unavailable); checklist + README note added.  
- KPI snapshot synced to 83 / 73 / 50 across pm_status / migration plan / 3D abstraction.  
- Learning path map updated with W2–W4 status; new `scripts/check_doc_links.py` introduced for Appendix E automation.  
- Wiki rotation tables + appendix checklists refreshed with the latest four-week history and contact KPI reporting guidance.

## 運用メモ
- 変更日・担当者・Pull Request を必ず記録する。表は最新が上に来るように追記。
- 大きな構成変更（章追加、テンプレート刷新）は別途詳細セクションを作成し、影響範囲を記載する。
- `docs/documentation_changelog.md` 更新後は Slack `#chrono-docs` に通知し、Wiki 側の履歴ページも同期する。

--- 

未反映の変更がある場合は、このファイルのドラフトに先に追記し、レビュー完了後に日付を確定させてください。
