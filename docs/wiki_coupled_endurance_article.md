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
- [パラメータチートシート](../docs/coupled_constraint_presets_cheatsheet.md)
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

## 7. 担当ローテーション / 同期ログ
| 週 (開始日) | 主担当 | バックアップ | 主タスク | 備考 |
|-------------|--------|--------------|----------|------|
| 2024-07-22 | Suzuki | Mori | Coupled Endurance アーティファクトの棚卸し、B.3 チェックリスト更新 | `docs/media/coupled/` の GIF を差し替え済み |
| 2024-07-29 | Mori | Kobayashi | `latest.summary.*` KPI の Wiki 反映、Slack 報告 | `lint_endurance_plan.py` 改定案を Appendix B.5 へ共有 |
| 2024-08-05 | Kobayashi | Suzuki | Nightly ベンチ結果レビュー、Appendix B.4 公開プロセス点検 | `compare_benchmark_results.py` の diff を wiki へ添付 |
| 2024-08-12 | Suzuki | Tanaka | `docs/coupled_endurance_ci_troubleshooting.md` リンク更新、Webhook 試験ログの共有 | ENDURANCE_ALERT_WEBHOOK 検証ログを #chrono-constraints に投稿 |
| 2024-08-19 | Tanaka | Mori | B.3/B.4 チェックリスト棚卸し、workflow_dispatch 監視 | Slack/Webhook で schema/failure-rate digest を要確認（GH token 未設定のため保留） |

最終同期日: **2024-08-19** （更新担当: Tanaka / レビュー: Mori）  
次回レビュー予定: 2024-08-26 週（Appendix B.5 ローテーション表に従う）
