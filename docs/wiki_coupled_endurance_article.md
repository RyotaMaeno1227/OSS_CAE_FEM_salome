# 社内 Wiki 記事草案: Coupled Endurance Nightly / CI 運用

本草案は社内ナレッジベース（Confluence / Notion 等）へ掲載することを想定した記事原稿です。`docs/coupled_endurance_ci_troubleshooting.md` と整合を取りながら、運用担当が日次・週次で参照できるよう設計しています。

## 1. 概要
- 対象ジョブ: GitHub Actions `coupled_endurance.yml`（週次 + 手動再実行）
- 目的: Coupled 拘束の長時間安定性を監視し、条件数・警告比率・ランク欠損を閾値管理する
- 主な成果物: `latest.csv`、`latest.summary.{json,md,html}`、PNG/GIF/MP4 可視化、CI アーティファクト

## 2. クイックリンク（Wiki では相対リンクに差し替え）
- [CI トラブルシュート手順書](../docs/coupled_endurance_ci_troubleshooting.md)
- [Coupled チュートリアル（実行ログ・メディア生成）](../docs/chrono_coupled_constraint_tutorial.md)
- [Coupled Hands-on / 学習ガイド](../docs/coupled_constraint_hands_on.md)
- [パラメータプリセット（YAML）](../data/coupled_constraint_presets.yaml)
- [パラメータチートシート（Markdown）](../docs/coupled_constraint_presets_cheatsheet.md)
- GitHub アーティファクト: `https://github.com/<org>/<repo>/actions?query=workflow%3A%22Coupled+Endurance%22`

## 3. 運用フロー（要約）
- 失敗検知 → ログ確認 → ローカル再現 → 報告という 4 ステップを守る。  
- それぞれの細かな操作、Slack テンプレ、画面キャプチャ例は `docs/appendix_optional_ops.md` **B.5 Coupled Endurance Operations** に移動した。  
- Wiki に転載する際は Appendix のチェックリストを参照し、ここでは KPI サマリとリンクを更新するだけでよい。

## 4. 指標定義（計算コア視点）
| KPI | 定義 | しきい値（例） | 取得先 |
|-----|------|----------------|--------|
| `max_condition` | `chrono_coupled_constraint2d_get_diagnostics().condition_number` の最大値 | `1.0e8` | `latest.summary.json` |
| `warn_ratio` | `condition_warning` が立ったサンプル割合 | `0.05` | 同上 |
| `rank_ratio` | ランク欠損フレーム割合 | `0.01` | 同上 |
| `dropping_equations` | 自動ドロップ回数 | `<= 1 / run` | `tools/filter_coupled_endurance_log.py` |

> これらの数値は `docs/coupled_constraint_solver_math.md` の条件数節および `docs/coupled_contact_test_notes.md` の判定基準に紐づいています。

## 5. Appendix 参照（運用系）
| トピック | Appendix 参照先 |
|---------|----------------|
| メディア生成・容量ガイドライン | `docs/appendix_optional_ops.md` §A |
| スクリーンショット要件・レビュー周期 | 同 §B.2 |
| Wiki テンプレート／チェックリスト | 同 §B.1, §B.3, §B.4 |
| 担当ローテーション／変更フロー | 同 §B.5 |

> 詳細な通知フローやスクリーンショット運用はすべて Appendix に移しました。ここでは KPI、最新リンク、Appendix 参照先のみ更新してください。

## 6. 備考
- 具体的な URL・テンプレート・チェックリストは `docs/appendix_optional_ops.md` を参照。  
- 本草案は引き続き数値検証観点で更新し、運用情報は付録へ切り出している。
- プリセットは Markdown 運用（`docs/coupled_constraint_presets_cheatsheet.md`）。外部配布が必要な場合のみ Appendix A.3.2 を参考に個別で PDF 化する。

## 7. 担当ローテーション / 同期ログ
| 週 (開始日) | 主担当 | バックアップ | 主タスク | Markdown 方針 | 備考 |
|-------------|--------|--------------|----------|----------------|------|
| 2025-10-20 | Kobayashi | Tanaka | summary・スクリーンショット更新、Contact+Coupled 判定ログを Appendix B.6 へ反映 | ✅ チートシート＝`.md` を確認 | `tools/filter_coupled_endurance_log.py` contact モードを検証。 |
| 2025-10-27 | Suzuki | Mori | KPI 更新、スクリーンショット差し替え（12h 遅延を報告） | ✅ | 遅延理由（CI 障害）を備忘録に追記。 |
| 2025-11-03 | Tanaka | Kobayashi | Nightly 差分レビュー、Webhook ログ追記、Appendix B.5.1 ローテーション更新 | ✅ Markdown のみ参照 | `docs/integration/learning_path_map.md` を Wiki リンク集へ追加。 |
| 2025-11-10 | Mori | Suzuki | KPI / Appendix B.3 & B.6 棚卸し、プリセットを Markdown 参照に切り替えた旨を Wiki と README に追記 | ✅ 例外: Appendix A.3.2 (外部配布時のみ PDF) | Slack `#chrono-docs` / `#chrono-constraints` へ通知、`scripts/check_doc_links.py` の結果を共有。 |

最終同期日: **2025-11-10** （更新担当: Mori / レビュー: Kobayashi）  
次回レビュー予定: 2025-11-17 週（Appendix B.5 ローテーション表に従う）。B.5.1 の担当ローテ（Wednesday: Kobayashi, Friday: Suzuki）に合わせて schema validation gist と workflow_dispatch 監査を行うこと。  
補足: 2024-08-26 週（担当: Tanaka）で `workflow_dispatch` + schema validation gist 共有（Appendix B.5.2）を実施予定。Run ID が確定次第、当表および Appendix B.5.3 のログを更新する。プリセットは Markdown (`docs/coupled_constraint_presets_cheatsheet.md`) を唯一の参照元とし、PDF が必要な場合のみ Appendix A.3.2 の例外手順を用いる。
