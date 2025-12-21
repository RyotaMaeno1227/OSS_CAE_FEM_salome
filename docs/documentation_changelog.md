# Documentation Changelog (Draft)

このページは Coupled/3D 関連ドキュメントの更新履歴を集約するための草案です。正式運用時は週次で最新エントリを追加し、Wiki / 社内ポータルと同期してください。

## 2025-12-01 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/team_runbook.md` | 自動実行キュー（A/B/C）を追加し、長尺バッチの連続実行セットと報告ルールを明文化 | PM | 周回ごとに Run ID/Artifact/Log/`git status`/リンクチェック結果をセットで共有。 |
| `docs/team_status.md` | 自動実行キューへの対応計画を追記し、C チームの 1 周目（C4/C12/C20 ドキュメント更新）を記録 | A/Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` の結果を共有。 |
| `docs/chrono_2d_readme.md` | 条件数/ピボット即時チェックのワンライナーとフォーマット/Lint 手順を追記（C4/C12） | Cチーム | `python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md docs/team_runbook.md` 実行済み。 |
| `docs/abc_team_chat_handoff.md` | C チーム 15分スプリント用に Run ID 貼付・cond/pivot 即時チェック・CSV スキーマ確認・リンク/Lint コマンド・命名ポリシー・報告手順を追加（C3/C4/C6/C9/C12/C15） | Cチーム | スプリントはドキュメント更新のみで Run ID/生成物なし。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/team_status.md` | 上記スプリント実施ログ（C3/C4/C6/C9/C12/C15）と PM コメント反映（多タスク束ね・3分では終わらない前提）を追記 | Cチーム | Run ID なし（ドキュメントのみ）。`git status` 対象は docs/team_runbook.md などを含む。 |
| `docs/team_runbook.md` | 15 分スプリント報告ルールに「複数タスク束ねで3分では終わらない前提、積み増しは PM 相談」を追記し、B スプリント指示を拡充（チャット文面サンプル/保持30日チェック/安定・実験別の共通化案） | PM | `python scripts/check_doc_links.py docs/team_runbook.md docs/team_status.md docs/documentation_changelog.md ...` 実行済み。 |
| `docs/team_status.md` | 自動実行キュー（B3,B6,B8,B15,B16,B17,B18）の実施準備と報告ルールを追記 | Bチーム | Run ID は長尺バッチ各周で記録予定。リンクチェックはスクリプト有無で判断。 |
| `docs/team_status.md`, `docs/team_runbook.md` | 外部CI実行不可環境向けに B3/B6/B15 の報告枠・最小Artifacts構成・未実施注記を追加 | Bチーム | CI/cron 未実行。`python scripts/check_doc_links.py docs/team_status.md docs/team_runbook.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリントでの B3/B6/B8/B15/B16 報告枠（外部CI不可）を追加 | Bチーム | Run ID 未取得（外部CI不可）。`python scripts/check_doc_links.py docs/team_status.md docs/team_runbook.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_runbook.md`, `docs/team_status.md` | B 15分スプリント指示を拡張（チャットテンプレ例、保持30日チェックリスト、YAML共通化手順案） | Bチーム | 外部CI未実行。リンクチェック実行済み。 |
| `docs/team_status.md` | 15分スプリント指示に基づき B3/B6/B8/B10/B15/B16/B17/B18 の報告枠・チェックリストを整理（外部CI未実行） | Bチーム | Run ID 未取得。`python scripts/check_doc_links.py docs/team_status.md docs/team_runbook.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリント（B3/B6/B8/B15/B16）の実行枠を追記し、未実施理由と最小Artifacts構成の確認を記録 | Bチーム | 外部CI不可のため Run ID 未取得。リンクチェック実行済み。 |
| `docs/team_status.md` | 15分スプリント（B3/B6/B8/B15/B16）の報告枠点検を追記（外部CI不可） | Bチーム | Run ID 未取得。リンクチェック実行済み。 |
| `docs/team_status.md` | Run ID/Artifact/Log 追記は外部CI不可のため保留し、PMへ共有依頼する旨を追記 | Bチーム | Run ID 未取得。 |
| `docs/team_status.md` | 15分スプリント（B3/B6/B8/B15/B16）をドキュメント整備で完了した旨を追記 | Bチーム | 外部CI不可のため Run ID 未取得。リンクチェック実行済み。 |
| `docs/team_status.md` | 次の実行指示（B3/B6/B15）を外部CI不可前提で確認し、報告枠の未実施注記を追加 | Bチーム | Run ID 未取得。リンクチェック実行済み。 |
| `docs/abc_team_chat_handoff.md`, `docs/team_status.md` | B1/B2/B3 対応として移植棚卸し・C↔C++ 対応表・最小入出力サンプルを追加 | Bチーム | Run ID なし（ドキュメントのみ）。リンクチェック実行済み。 |
| `docs/abc_team_chat_handoff.md`, `chrono-C-all/README.md` | B1/B2/B3 の最小サンプルに成功条件を追記し、Aチーム向け検証手順を補強 | Bチーム | Run ID なし（ドキュメントのみ）。 |
| `docs/chrono_2d_readme.md`, `docs/abc_team_chat_handoff.md`, `docs/team_status.md` | A5/A7/A11/B1 の支援として外部定義パス整理、許容誤差の追記ルール、ケース生成例を追加 | Bチーム | Run ID なし（ドキュメントのみ）。 |
| `docs/chrono_2d_readme.md`, `docs/abc_team_chat_handoff.md`, `docs/team_status.md` | A5/A7/A11/B1 の更新として生成物レイアウト/運用導線と C↔C++ 対応状況を追記 | Bチーム | Run ID なし（ドキュメントのみ）。 |
| `docs/abc_team_chat_handoff.md`, `chrono-C-all/README.md`, `docs/team_status.md` | B1/B2 の対応として対応表の未対応理由/次対応先と最小サンプル再現性メモを追記 | Bチーム | Run ID なし（ドキュメントのみ）。 |
| `docs/team_status.md` | A5/A8/A12 の precheck を追記し、A12 は compare_bench_csv.py による head/summary 報告とする PM 指示を明記 | Aチーム | Run ID: local-chrono2d-20251201-02（precheck、生成物なし）。リンクチェックは docs 未更新時は省略。 |
| `docs/team_status.md` | 15分スプリント結果（A5/A8/A12/A14/A17）を追記し、compare_bench_csv warn-only 実行と閾値確認を記録 | Aチーム | Run ID: local-chrono2d-20251201-03（warn-only、生成物なし）。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリント 2回目（A5/A8/A12/A14/A17）の確認ログを追加し、警告フラグ(-Wall/-Wextra等)とベンチ基準の現状を整理 | Aチーム | Run ID: local-chrono2d-20251201-04（確認のみ、生成物なし）。リンクチェックは追記後に実施予定。 |
| `docs/team_status.md` | 15分スプリント 2回目の再実行で警告ビルド（-Wshadow/-Wconversion）とベンチ drift 検出を記録 | Aチーム | Run ID: local-chrono2d-20251201-04（warn-only、Artifacts 削除）。警告3件と drift (threads=1, 0.57–0.60us vs 0.21us) を明記。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリント 3回目で baseline 同士比較（driftなし）を記録 | Aチーム | Run ID: local-chrono2d-20251201-05（確認のみ、生成物なし）。`python tools/compare_bench_csv.py --previous chrono-2d/data/bench_baseline.csv chrono-2d/data/bench_baseline.csv` の結果を反映。 |
| `docs/team_status.md` | 15分スプリント 3回目（A5/A8/A12/A14/A17）の確認ログを追記し、baseline 同士の compare_bench_csv 実行と CFLAGS (-Wall/-Wextra/-pedantic/-fopenmp) 現状を記録 | Aチーム | Run ID: local-chrono2d-20251201-05（確認のみ、生成物なし）。リンクチェックは追記後に実施予定。 |
| `docs/team_status.md` | 自動実行キュー（A5-A12）の warn-only→fail 2 周を実施し、ベンチ drift 検出と警告ビルド結果を記録 | Aチーム | Run ID: local-chrono2d-20251201-06（warn-only）、local-chrono2d-20251201-07（fail）。生成物は削除。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | A8 警告対応（未使用変数/未使用関数/未使用戻り値）を反映し、-Wshadow/-Wconversion ビルドで警告ゼロを確認 | Aチーム | Run ID: local-chrono2d-20251201-08（警告修正、生成物なし）。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | A5 外部定義移行のタスク票（優先度/対象データ/次ステップ）を追記 | Aチーム | Run ID: local-chrono2d-20251201-09（タスク票のみ、生成物なし）。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリント（A5/A8/A12/A14/A17）を一括実施し、warn-only ベンチ drift と閾値確認、ログ粒度案を記録 | Aチーム | Run ID: local-chrono2d-20251201-10（warn-only、生成物削除）。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `chrono-2d/tests/test_coupled_constraint.c`, `chrono-2d/tests/test_minicase.c`, `chrono-2d/data/approx_tolerances.csv`, `chrono-2d/scripts/gen_constraint_cases.py`, `chrono-2d/src/solver.c`, `docs/team_status.md` | A7/A9/A10/A11/A18 を実装（ケース別許容誤差、mini-case、dump-json 拡張、ケース生成スイープ、複合拘束追加） | Aチーム | Run ID: local-chrono2d-20251201-11。`make -C chrono-2d test` PASS。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/chrono_2d_readme.md`, `chrono-2d/docs/constraints.md`, `chrono-2d/data/parameter_sensitivity_ranges.csv`, `chrono-2d/tests/test_coupled_constraint.c`, `docs/team_status.md` | A5/A7/A10/A11/A14 の方針・仕様・感度レンジを明文化し、テスト側の感度判定を追加 | Aチーム | Run ID: local-chrono2d-20251201-12。`make -C chrono-2d test` PASS。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `chrono-2d/data/parameter_sensitivity_ranges.csv`, `chrono-2d/data/cases_combined_constraints.csv`, `chrono-2d/tests/test_coupled_constraint.c`, `chrono-2d/docs/constraints.md`, `docs/chrono_2d_readme.md`, `docs/team_status.md` | A10/A14/A18 の更新（dump-json 参照項目拡張、感度レンジ見直し、複合拘束の評価観点と候補追加） | Aチーム | Run ID: local-chrono2d-20251201-13。`make -C chrono-2d test` PASS。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `chrono-2d/src/solver.c`, `chrono-2d/data/cases_combined_constraints.csv`, `chrono-2d/data/parameter_sensitivity_ranges.csv`, `chrono-2d/tests/test_coupled_constraint.c`, `chrono-2d/docs/constraints.md`, `docs/chrono_2d_readme.md`, `docs/team_status.md` | A10/A14/A18 追加対応（dump-json 例、感度レンジ根拠メモ、複合拘束候補の追加） | Aチーム | Run ID: local-chrono2d-20251201-14。`make -C chrono-2d test` PASS。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `chrono-2d/tests/test_coupled_constraint.c`, `chrono-2d/data/dataset_version.txt`, `chrono-2d/docs/constraints.md`, `docs/chrono_2d_readme.md`, `docs/team_status.md` | A15/A19/A20 の更新（ログ粒度ポリシー、データセット版管理、拘束仕様の整合） | Aチーム | Run ID: local-chrono2d-20251201-15。`make -C chrono-2d test` PASS。`python scripts/check_doc_links.py docs/team_status.md docs/documentation_changelog.md` 実行済み。 |
| `docs/team_status.md` | 15分スプリント（C3/C4/C6/C9/C12/C15）について PM コメントを確認し、追加作業不要で十分な負荷と判断した旨を追記 | Cチーム | Run ID なし（確認のみ）。`python scripts/check_doc_links.py docs/team_runbook.md docs/team_status.md docs/documentation_changelog.md ...` 実行済み。 |
| `docs/chrono_2d_readme.md` | Changelog トリガー記載を短文化し、15分スプリントの簡潔化方針を反映 | Cチーム | `docs/chrono_2d_cases_template.csv` を確認（差分なし）。 |
| `docs/team_status.md` | 15分スプリント（C3/C4/C6/C9/C12/C15）での上記変更とリンクチェック結果を追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/abc_team_chat_handoff.md` | Cチームのチャットトピックに CSV スキーマ確認と CI/運用導線を追記 | Cチーム | `python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/chrono_2d_readme.md` | CI/運用導線（team_runbook/team_status の参照）を追記 | Cチーム | 15分スプリント対応の導線を明文化。 |
| `docs/team_status.md` | 15分スプリント（C3/C4/C6/C9/C12/C15）の追加実施ログを追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/team_status.md` | PM 発出済みの C4/C12/C20 を消化済みとして整理（追加作業なし） | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/chrono_2d_readme.md` | Run ID 同期先（git_setup）と用語/表記ガイドを追記 | Cチーム | OpenMP/3D 方針の表記統一を明記。 |
| `docs/team_status.md` | C8/C11/C18 の対応ログを追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/team_status.md` | 直近タスク（C4/C6/C9/C12）の実施ログを追記し、リンク/スキーマ確認の再実行を記録 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/team_status.md` | タスク更新分の C4/C6/C9/C12 を再実行し、確認ログを追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/chrono_2d_readme.md` | cond/pivot の目安補足と CSV スキーマ差分確認手順を追記 | Cチーム | C4/C9 を明文化。 |
| `docs/team_status.md` | C4/C6/C9/C12 の実作業ログを追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |
| `docs/chrono_2d_glossary_checklist.md` | 用語・表記ガイドと学習ステップを 1 ページに統合 | Cチーム | C10/C7 の要件を反映。 |
| `docs/chrono_2d_readme.md` | 1ページ版チェックリストへの導線を追加 | Cチーム | `docs/chrono_2d_glossary_checklist.md` を参照。 |
| `docs/team_status.md` | C10/C7 の実施ログを追記 | Cチーム | Run ID なし（ドキュメントのみ）。`python scripts/check_doc_links.py ...` 実行済み。 |

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
| `docs/abc_team_chat_handoff.md`, `docs/chrono_2d_readme.md` | Run ID ワンライナーとリンクチェック運用を追記し、chrono-2d 月次更新手順を強化 | Cチーム | Run ID 例: local-chrono2d-20251118-01 |

## 2025-11-18 更新

| ドキュメント | 主な変更内容 | 担当 | 備考 |
|--------------|--------------|------|------|
| `docs/chrono_2d_readme.md` | 月次更新フローを明記し、サンプルCSVを vn/vt/µs/µd/stick 列付きに更新。リンク/整合チェックと Run ID 共有ワンライナーを追記 | Cチーム | スキーマテンプレ: `docs/chrono_2d_cases_template.csv`、Run 例: local-chrono2d-20251118-01 |
| `docs/abc_team_chat_handoff.md` | chrono-2d/chrono-main/Chrono C の Run ID 貼付ワンライナーと異常時連絡テンプレを追記 | Cチーム | チャット共有用フォーマットを統一 |
| `docs/chrono_main_descriptor_hands_on.md`, `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md`, `docs/chrono_main_ci_plan.md`, `third_party/chrono/chrono-main/practice/*`, `tools/update_descriptor_run_id.py` | chrono-main 向け Hands-on/ログ/CI案/Practice 雛形を追加し、Run ID 更新スクリプトに `--variant chrono-main` を導入 | Cチーム | 実行用バイナリは未作成。`ch01_descriptor_e2e.sh <RUN_ID>` 実測後に Evidence を追記する想定 |
| `third_party/chrono/chrono-main/README.md`, `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md`, `docs/coupled_island_migration_plan.md`, `docs/abc_team_chat_handoff.md` | chrono-main 向け descriptor-e2e Run ID テンプレ、ログ、README の CI 最小手順を追加 | Cチーム | Run ID 例: 19582037625、CI ジョブ名 `descriptor-e2e-chrono-main` |
| `docs/chrono_2d_readme.md`, `docs/abc_team_chat_handoff.md`, `docs/git_setup.md` | chrono-2d 用 README と Run ID テンプレを追加し、OpenMP のみ依存・3D 非対応を明記 | Cチーム | Run ID 例: `local-chrono2d-20251117-01` |
| `docs/chrono_2d_cases_template.csv`, `chrono-2d/scripts/run_hands_on.sh`, `docs/chrono_2d_readme.md` | chrono-2d の CSV スキーマテンプレと Hands-on ショートカット運用ガイドを追加し、条件数/ピボット解説を表形式に再編 | Cチーム | 月次で Run ID / CSV を差し替え、リンクチェックと Changelog 記録を必須化 |

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
