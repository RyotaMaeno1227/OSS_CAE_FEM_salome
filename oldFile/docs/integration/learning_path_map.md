# Coupled Constraint Learning Path Integration (Draft)

このメモは `docs/coupled_constraint_tutorial_draft.md`（説明主体）と `docs/coupled_constraint_hands_on.md`（演習主体）を統合して学習パスを一本化するためのロードマップです。旧 Appendix E は撤去済みのため、本メモを唯一の参照として章単位のマッピングと実装タスクを可視化します。

> 3D 拡張を検討する場合は `docs/archive/legacy_chrono/chrono_3d_abstraction_note.md` の「学習パス向けサマリ」を先に読み、段階的に参照するようにしてください。

## 1. 章対応表

| Tutorial セクション | Hands-on Chapter | 統合ポリシー | 実装メモ |
|----------------------|------------------|--------------|----------|
| §1 Theory / 数式フェーズ | Chapter 01 Ratio Sweep | Theory ノートを Tutorial に残し、演習コードとログテンプレは Hands-on へ集約 | 比率スイープ結果（CSV/PNG）は Hands-on で生成し、Tutorial からリンクする。 |
| §2 Implementation / 実装フェーズ | Chapter 02 Softness & Springs | API 表や C スニペットは Tutorial、CSV/可視化は Hands-on | `practice/coupled/ch02_softness.c` の TODO を Tutorial 参照から Hands-on の課題表に移動。 |
| §3 Verification / テストフェーズ | Chapter 04 Endurance & Diagnostics | KPI 表／テスト一覧を Tutorial、実行ログ解析スクリプトを Hands-on | `tools/plot_coupled_constraint_endurance.py` の実行例は Appendix A に任せる。 |
| §4 Hands-on / 実践ミニ課題 | Chapter 03 Contact Integration | Tutorial 側では課題一覧を短くまとめ、詳細手順は Hands-on に一本化 | Contact+Coupled 判定ロジックは Appendix B.6 と連携。 |

## 2. マイルストン

| 週 | 作業内容 | Owner | 依存ファイル / タスク | 完了条件 | ステータス (2025-11-17) |
|----|----------|-------|-----------------------|----------|-----------------------|
| W1 | 章対応表を Tutorial/Hands-on 両方に埋め込み、相互リンクを追記 | Cチーム（Mori） | `docs/coupled_constraint_tutorial_draft.md` / `docs/coupled_constraint_hands_on.md` | 両ファイルの冒頭に統合計画リンクが掲載されている。 | ✅ 完了（2025-11-05） |
| W2 | Hands-on の演習コード（`practice/coupled/*`）に TODO マーカーを追加し、Tutorial 側の重複を削除 | Hands-on WG（Kobayashi） | `practice/coupled/ch*.c`（設置済み）, `docs/coupled_constraint_tutorial_draft.md` §2 | `rg TODO_LEARNING_PATH` の結果が Hands-on にのみ存在し、`data/diagnostics/ch02_softness_sample.csv` / `ch03_contact_sample.log` を参照。 | ✅ 完了（2025-11-17, Run ID 例: `local-20251117-ch02` / `local-20251117-ch03`） |
| W3 | Appendix B.7 のリンク検証チェックリストを自動化 (`scripts/check_doc_links.py` + `scripts/check_preset_links.py`) | Tooling 班（Suzuki） | 両スクリプト, `.github/workflows/ci.yaml`, Appendix B.7/E.2 | CI で Tutorial ↔ Hands-on ↔ Solver Math ↔ Contact Notes ↔ Markdown プリセットのリンク検証が走る。 | ⏳ 進行中 – スクリプト実行ログあり、CI 連携を監視中 |
| W4 | 統合レビュー（Hands-on／Tutorial 共通構成）を実施し、`docs/documentation_changelog.md` に統合完了を記録 | Cチーム全体 | changelog, Wiki sample | 重複節が解消され、学習者ガイドラインが本マップへ集約。 | ⏳ 未着手 – W2/W3 完了後に実施 |

### W2 現状
- `practice/coupled/ch01_ratio_sweep.c` に Chrono API 呼び出しを組み込み、Hands-on 側でのみ検証する流れを明示済み。  
- `ch02_softness.c` / `ch03_contact.c` のサンプルを配置し、出力先を `data/diagnostics/ch02_softness_sample.csv` / `ch03_contact_sample.log` に固定。  
- 目標日: 2025-11-17（Run ID 例: `local-20251117-ch02`, `local-20251117-ch03`）。

### W3 現状
- `scripts/check_doc_links.py` を追加し、`docs/coupled_constraint_tutorial_draft.md` / Hands-on / Solver Math / Contact Notes の参照整合性を検証できるようにした。  
- 次ステップ: CI ワークフローへ統合し、失敗時に Appendix B.7 のチェック項目を自動で埋める。

### W4 事前準備
- W2 と W3 が完了した時点で、Tutorial 側の Hands-on セクション（§4）を短縮し、Hands-on へ誘導するだけの構成に変更する。  
- 統合レビューでは Appendix E と本マップを照合し、`docs/documentation_changelog.md` へ「Learning Path Integration complete」を記載する。

## 3. 可視化スナップショット

学習パスと Hands-on の進捗を共有しやすくするため、SVG 形式のサマリ図とスクリーンショットを追加しました。`docs/integration/assets/` 配下でバージョン管理しているため、ドキュメントから直接参照できます。

![Learning Path Overview](assets/learning_path_overview.svg)

- 図の生成元: `docs/integration/assets/learning_path_overview.svg`（手動編集可能な SVG）。Chapter/Appendix の流れと Run ID/Evidence の格納先を示しています。

![Hands-on Chapter 02 & 03 Progress](assets/hands_on_ch02_progress.svg)

- 図の生成元: `docs/integration/assets/hands_on_ch02_progress.svg`。Chapter 02/03 の TODO 解消状況と紐付くアーティファクト（CSV、README 節）をラベル化しています。
- Hands-on 更新時は `tools/update_multi_omega_assets.py --refresh-report` を実行し、図中のステータス表示も合わせて編集してください。

#### 図版の更新手順（簡易）
- 編集対象: `docs/integration/assets/learning_path_overview.svg`, `docs/integration/assets/hands_on_ch02_progress.svg`
- 手順: テキストエディタで直接編集し、進捗バーやステータスラベルを最新 Run ID に合わせて手動更新する（例: `local-20251117-ch02`）。変更後は `git diff` を確認し、`scripts/check_doc_links.py` を再実行。
- chrono-main も並行で更新する場合は、Run ID のペア（Chrono C / chrono-main）を図版横にコメントし、`docs/logs/kkt_descriptor_poc_e2e_chrono_main.md` へリンクする。

## 4. リスク / TODO
- [ ] Pandoc 出力と Hands-on スクリプトが同じ図版を参照できるよう、`docs/media/coupled/` の命名規約を決める。 (Owner: Suzuki)
- [ ] Hands-on の FEM4C 参照リンクを Tutorial 側 glossary へ昇格させ、章間の行き来を減らす。 (Owner: Mori)
- [ ] Appendix E の「移行完了条件」を `docs/documentation_changelog.md` にも記載し、レビュー時の観点を共有する。 (Owner: Kobayashi)

> このドラフトは Appendix E から参照され、週次の KPI ローテーション（Appendix B.5.1）と同じタイミングで更新します。
