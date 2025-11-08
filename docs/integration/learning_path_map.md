# Coupled Constraint Learning Path Integration (Draft)

このメモは `docs/coupled_constraint_tutorial_draft.md`（説明主体）と `docs/coupled_constraint_hands_on.md`（演習主体）を統合して学習パスを一本化するためのロードマップです。Appendix E（`docs/appendix_optional_ops.md`）の計画に対応し、章単位のマッピングと実装タスクを可視化します。

## 1. 章対応表

| Tutorial セクション | Hands-on Chapter | 統合ポリシー | 実装メモ |
|----------------------|------------------|--------------|----------|
| §1 Theory / 数式フェーズ | Chapter 01 Ratio Sweep | Theory ノートを Tutorial に残し、演習コードとログテンプレは Hands-on へ集約 | 比率スイープ結果（CSV/PNG）は Hands-on で生成し、Tutorial からリンクする。 |
| §2 Implementation / 実装フェーズ | Chapter 02 Softness & Springs | API 表や C スニペットは Tutorial、CSV/可視化は Hands-on | `practice/coupled/ch02_softness.c` の TODO を Tutorial 参照から Hands-on の課題表に移動。 |
| §3 Verification / テストフェーズ | Chapter 04 Endurance & Diagnostics | KPI 表／テスト一覧を Tutorial、実行ログ解析スクリプトを Hands-on | `tools/plot_coupled_constraint_endurance.py` の実行例は Appendix A に任せる。 |
| §4 Hands-on / 実践ミニ課題 | Chapter 03 Contact Integration | Tutorial 側では課題一覧を短くまとめ、詳細手順は Hands-on に一本化 | Contact+Coupled 判定ロジックは Appendix B.6 と連携。 |

## 2. マイルストン

| 週 | 作業内容 | Owner | 完了条件 | ステータス (2025-11-08) |
|----|----------|-------|----------|-----------------------|
| W1 | 章対応表を Tutorial/Hands-on 両方に埋め込み、相互リンクを追記 | Cチーム（Mori） | 両ファイルの冒頭に統合計画リンクが掲載されている。 | ✅ 完了（2025-11-05） |
| W2 | Hands-on の演習コード（`practice/coupled/*`）に TODO マーカーを追加し、Tutorial 側の重複を削除 | Hands-on WG（Kobayashi） | `rg TODO_LEARNING_PATH` の結果が Hands-on にのみ存在する。 | ⏳ 進行中 – `practice/coupled/ch01_ratio_sweep.c` にタグ追加済み、残り 2 ファイル |
| W3 | Appendix B.7 のリンク検証チェックリストを自動化 (`scripts/check_doc_links.py`) | Tooling 班（Suzuki） | CI で Tutorial ↔ Hands-on ↔ Solver Math ↔ Contact Notes のリンク検証が走る。 | 🆕 着手 – スクリプト雛形を本コミットで追加 |
| W4 | 統合レビュー（Hands-on／Tutorial 共通構成）を実施し、`docs/documentation_changelog.md` に統合完了を記録 | Cチーム全体 | 重複節が解消され、学習者ガイドラインが Appendix に移行済み。 | ⏳ 未着手 – W3 の自動化完了後に実施 |

### W2 現状
- `practice/coupled/ch01_ratio_sweep.c` に `// TODO_LEARNING_PATH` を追記し、Hands-on 側でのみ検証する流れを明示済み。  
- 残タスク: `ch02_softness.c`、`ch03_contact.c` のコメント整備、および Tutorial §2.1 の冗長なコード断片の削除。  
- 目標日: 2025-11-12。

### W3 現状
- `scripts/check_doc_links.py` を追加し、`docs/coupled_constraint_tutorial_draft.md` / Hands-on / Solver Math / Contact Notes の参照整合性を検証できるようにした。  
- 次ステップ: CI ワークフローへ統合し、失敗時に Appendix B.7 のチェック項目を自動で埋める。

### W4 事前準備
- W2 と W3 が完了した時点で、Tutorial 側の Hands-on セクション（§4）を短縮し、Hands-on へ誘導するだけの構成に変更する。  
- 統合レビューでは Appendix E と本マップを照合し、`docs/documentation_changelog.md` へ「Learning Path Integration complete」を記載する。

## 3. リスク / TODO
- [ ] Pandoc 出力と Hands-on スクリプトが同じ図版を参照できるよう、`docs/media/coupled/` の命名規約を決める。 (Owner: Suzuki)
- [ ] Hands-on の FEM4C 参照リンクを Tutorial 側 glossary へ昇格させ、章間の行き来を減らす。 (Owner: Mori)
- [ ] Appendix E の「移行完了条件」を `docs/documentation_changelog.md` にも記載し、レビュー時の観点を共有する。 (Owner: Kobayashi)

> このドラフトは Appendix E から参照され、週次の KPI ローテーション（Appendix B.5.1）と同じタイミングで更新します。
