# Documentation Changelog (Draft)

このページは Coupled/3D 関連ドキュメントの更新履歴を集約するための草案です。正式運用時は週次で最新エントリを追加し、Wiki / 社内ポータルと同期してください。

## 2025-11-14 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `README.md` | リポジトリの目的を Project Chrono 移植＋教育資料に限定し、通知／ベンチ公開／Appendix 参照を整理 | Aチーム（Mori） | Coupled Benchmark Site / Link Lint など運用節を削除し、新レイアウトを提示 |
| `docs/a_team_handoff.md` | Appendix B.5 系導線を撤去し、週次レビュー／Evidence テンプレを本編に統合 | Aチーム（Mori） | Slack 共有は任意周知のみと明記 |
| `docs/coupled_island_migration_plan.md`, `docs/chrono_3d_abstraction_note.md` | KPI テーブルの更新手順から Appendix 参照を除去し、週次レビューでの直接更新方針を明記 | Aチーム（Mori） | pm_status との同期手順も追記 |
| `docs/coupled_constraint_presets_cheatsheet.md` | 更新チェックリストを Appendix 依存から独立させ、Slack 通知は任意作業と説明 | Docs 班（Nakajima） | `scripts/check_preset_links.py` 実行を継続 |
| `docs/coupled_constraint_hands_on.md` | 学習パス表の Appendix 参照を撤去し、リンク検証フローを現行仕様へ更新 | Docs 班 |
| `docs/chrono_2d_development_plan.md` | Appendix への移行計画をアーカイブ扱いにし、ユーティリティ系はスコープ外と明記 | Aチーム |
| `docs/pm_status_2024-11-08.md` | 通知／Endurance Archive の扱いを更新し、空ディレクトリを残す方針を記録 | PM |
| `docs/abc_team_chat_handoff.md` | Bチーム 15 件タスクの実施ログ、Run #19381234567 の反映状況を明記 | Bチーム（Diagnostics） | セクション 9 に完了マークを追加 |
| `docs/pm_status_2024-11-08.md` | Bセクションへ Nightly 更新ログ（Run ID、CSV 追記、テンプレ整備、権限確認）を追加 | Bチーム（Diagnostics） | Run 優先順位ルール・監視コマンド・workflow_dispatch 手順を追記 |
| `docs/git_setup.md` | Nightly 向け Git 差分確認チートシートを新設 | Bチーム（Diagnostics） | `git add data/coupled_constraint_endurance.csv ...` など定型手順を記載 |
| `docs/templates/b_team_endurance_templates.md` | Endurance 失敗共有／summary 配布テンプレを新規作成 | Bチーム（Diagnostics） | チャット投稿用。Run 個別情報はリポジトリに残さない |
| `docs/pm_status_2024-11-08.md` | 新規 Run #19381254567 を追記し、監視ワンライナー／κ・Rank 3行テンプレ、週次計画欄を追加 | Bチーム（Diagnostics） | B ログ 2025-11-15 セクションに記録 |
| `docs/pm_status_2024-11-08.md` | 新規 Run #19381264567 を追記し、監視ワンライナー実行例と週次計画を更新 | Bチーム（Diagnostics） | B ログ 2025-11-16 セクションに記録 |
| `docs/abc_team_chat_handoff.md` | Bチームタスク表と Run 優先順位ルール例を更新（#19381264567, Step 7210–7239） | Bチーム（Diagnostics） | セクション 9 に反映 |
| `docs/templates/b_team_endurance_templates.md` | 複数 Run 報告例追加（成功/失敗混在）、κ/Rank 3行テンプレ補足、監視・列チェック・Rank 抽出ワンライナーを拡充 | Bチーム（Diagnostics） | 週次運用手順を明文化 |
| `docs/git_setup.md` | Endurance 更新後の最小確認ブロックを確認実行、追記維持 | Bチーム（Diagnostics） | tail/plot/preset check のセットを提示 |
| `docs/pm_status_2024-11-08.md` | 新規 Run #19381244567 を追記し、監視ワンライナー／フォーマット共有運用を明文化 | Bチーム（Diagnostics） | Nightly B ログを 2025-11-14 セクションに追加 |
| `docs/abc_team_chat_handoff.md` | Bチームタスク表を最新 Run (#19381244567) に更新 | Bチーム（Diagnostics） | Step 7210–7219 の反映を明記 |
| `docs/templates/b_team_endurance_templates.md` | 監視ワンライナー・κ/Rank サマリ・外部配布定型文を追記 | Bチーム（Diagnostics） | Aチーム共有用のサマリ定型を明文化 |

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
| `.github/workflows/ci.yaml` | `scripts/check_doc_links.py` を docs lint ステップに追加 | Tooling | Tutorial/Hands-on のリンク検証を CI で強制 |

## 2025-11-14 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/appendix_optional_ops.md`, `docs/coupled_endurance_ci_troubleshooting.md`, `docs/wiki_coupled_endurance_article*.md`, `docs/media/coupled/README.md`, `docs/logs/notification_audit.md` | Project Chrono 移植と教育資料に scope を絞るため、通知／運用系ドキュメントを削除 | PM | 長時間耐久運用は各チーム環境へ移譲 |
| `tools/filter_coupled_endurance_log.py`, `tools/report_archive_failure_rate.py`, `tools/compose_endurance_notification.py`, `tools/fetch_endurance_artifact.py` | 同上、通知系スクリプトを削除 | PM | 不要機能の撤去 |
| README / Hands-on / チュートリアル各種 | Appendix 参照を削除し、Chrono 移植＋教育コンテンツのみに整理 | Cチーム | Markdown 方針＋Chrono 重点に統一 |
| `docs/appendix_optional_ops.md` | Markdown 方針（A/B/C/D/E）、ローテ表の Markdown チェック欄、通知テンプレ整備を追記 | Cチーム | Appendix 全体で PDF 排除を明文化 |
| `docs/wiki_coupled_endurance_article.md` / `docs/wiki_samples/coupled_endurance_article_sample.md` | ローテ表に Markdown 方針列を追加し、例外条件を記載 | Cチーム | Wiki/サンプル共に `.md` 参照を保証 |
| `README.md`, `docs/media/coupled/README.md` | `scripts/check_doc_links.py` の運用例と Slack テンプレの参照先を追記 | Cチーム | lint/告知フローを統一 |
| `docs/logs/notification_audit.md` | Webhook/メール通知の記録テンプレを新規追加 | Cチーム | Appendix C.4 とリンク
| `scripts/check_preset_links.py` & `.github/workflows/ci.yaml` | Markdown プリセットリンク検証スクリプトを追加し、CI に組み込み | Tooling | README/Hands-on/Wiki が `.md` を参照しているか自動チェック |
| `docs/coupled_constraint_hands_on.md`, `practice/coupled/ch0x_*`, `practice/README.md` | Chapter 02/03 TODO を解消し、Practice ソースと Appendix C（Multi-ω 更新手順）を追加 | Cチーム | Run ID / Evidence は `docs/abc_team_chat_handoff.md` と同期 |
| `docs/coupled_constraint_presets_cheatsheet.md`, `data/coupled_constraint_presets.yaml` | ユースケース表に hydraulic/optic/multi_omega を追加し、YAML と値を突合 | Cチーム | `python scripts/check_preset_links.py` を実行済み |
| `docs/integration/learning_path_map.md`, `docs/integration/assets/learning_path_overview.svg`, `.../hands_on_ch02_progress.svg` | 可視化セクションと SVG 図版を追加し、`docs/chrono_3d_abstraction_note.md` からリンク | Cチーム | Hands-on/README で参照 |
| `README.md`, `docs/git_setup.md` | Educational Materials へのリンク整備、Run ID／preset チェックの手順を追加 | Cチーム | C チームの週次チェック項目へ反映 |
| `docs/abc_team_chat_handoff.md`, `docs/pm_status_2024-11-08.md`, `docs/wiki_samples/schema_validation_gist.svg` | タスク表 15 件／チャットテンプレ更新、C チーム欄の進捗メモ・Run ID 参照先を刷新 | Cチーム | 新チャットのピン留め前提 |
| `docs/coupled_contact_api_minimal*.md`, `docs/chrono_coupled_constraint_tutorial.md`, `docs/chrono_3d_abstraction_note.md` | 日英 API ドキュメントの用語整理、Chrono main との式番号対応表・学習者向けサマリを追記 | Cチーム | Learning Path / Tutorial から参照 |

## 2025-11-17 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/coupled_constraint_hands_on.md`, `practice/coupled/ch0x_*`, `practice/README.md` | Hands-on Chapter 02/03 のサンプルに Chrono API 呼び出しを組み込み、出力 CSV/ログ（`data/diagnostics/ch02_softness_sample.csv`, `ch03_contact_sample.log`）を配置 | Cチーム | Run ID 例: `local-20251117-ch02`, `local-20251117-ch03` |
| `docs/coupled_constraint_presets_cheatsheet.md`, `data/coupled_constraint_presets.yaml` | プリセット表と YAML の値を再突合し、hydraulic/optic/multi-ω 行を更新 | Cチーム | `python scripts/check_preset_links.py` 実行済み |
| `docs/integration/learning_path_map.md`, `docs/integration/assets/hands_on_ch02_progress.svg` | 学習パスの可視化を更新し、Run ID 例と図版更新手順を追記 | Cチーム | SVG を手動編集しステータスを反映 |
| `README.md`, `docs/git_setup.md` | Educational Materials と preset チェック／Run ID 連携の手順を整合 | Cチーム | Hands-on との導線を明示 |
| `docs/abc_team_chat_handoff.md` | C チームタスクに Owner/期限を追記し、チャット配布用に整理 | Cチーム | 新チャットのピン留め前提 |
| `docs/chrono_coupled_constraint_tutorial.md`, `docs/coupled_contact_api_minimal*.md`, `docs/chrono_3d_abstraction_note.md` | Chrono main との図版・式番号対応表と学習者向けサマリを更新、Appendix 表記を整理 | Cチーム | 用語揺れ/リンク切れを修正 |
| `tools/tests/test_update_multi_omega_assets.py` | 旧 Appendix 記述を削除し、テストケースをノート表記へ変更 | Cチーム | Appendix 廃止方針に合わせて整備 |
| `docs/chrono_main_descriptor_hands_on.md`, `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md`, `docs/chrono_main_ci_plan.md`, `third_party/chrono/chrono-main/practice/*`, `tools/update_descriptor_run_id.py` | chrono-main 向け Hands-on/ログ/CI案/Practice 雛形を追加し、Run ID 更新スクリプトに `--variant chrono-main` を導入 | Cチーム | 実行用バイナリは未作成。`ch01_descriptor_e2e.sh <RUN_ID>` 実測後に Evidence を追記する想定 |
| `third_party/chrono/chrono-main/README.md`, `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md`, `docs/coupled_island_migration_plan.md`, `docs/abc_team_chat_handoff.md` | chrono-main 向け descriptor-e2e Run ID テンプレ、ログ、README の CI 最小手順を追加 | Cチーム | Run ID 例: 19582037625、CI ジョブ名 `descriptor-e2e-chrono-main` |
| `docs/chrono_2d_readme.md`, `docs/abc_team_chat_handoff.md`, `docs/git_setup.md` | chrono-2d 用 README と Run ID テンプレを追加し、OpenMP のみ依存・3D 非対応を明記 | Cチーム | Run ID 例: `local-chrono2d-20251117-01` |

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
