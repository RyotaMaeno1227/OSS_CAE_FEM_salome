# Coupled Endurance Nightly / CI 運用（サンプル記事）

{info:title=Coupled Endurance Nightly / CI 運用}
最終更新: 2025-10-21（担当: Suzuki） / Git 同期: [PR #1234](https://github.com/example/highperformanceFEM/pull/1234)
{info}

## 概要
- 対象ジョブ: GitHub Actions `coupled_endurance.yml`
- 目的: Coupled 拘束の長時間安定性を監視し、条件数・警告比率・ランク欠損を閾値管理する
- 成果物: `latest.csv`, `latest.summary.{json,md,html}`, PNG/GIF/MP4 可視化、CI アーティファクト

## クイックリンク
- [CI トラブルシュート手順書](../coupled_endurance_ci_troubleshooting.md)
- [Coupled チュートリアル（メディア生成手順）](../chrono_coupled_constraint_tutorial.md)
- [Hands-on ガイド](../coupled_constraint_hands_on.md)
- [パラメータ YAML](../../data/coupled_constraint_presets.yaml)
- [チートシート（Markdown）](../coupled_constraint_presets_cheatsheet.md)
- [アーティファクト一覧](https://github.com/example/highperformanceFEM/actions?query=workflow%3A%22Coupled+Endurance%22)

## 運用フロー（サマリ）
- 失敗検知 → ログ確認 → ローカル再現 → 報告の 4 ステップを守る。  
- 詳細な手順・Slack テンプレ・画面キャプチャ例は `docs/appendix_optional_ops.md` **B.5 Coupled Endurance Operations** を参照。  
- Wiki 記事では最新 KPI と参照リンクのみ記載し、付録を常に更新する。

## KPI の見方
| KPI | 説明 | 閾値（例） |
|-----|------|------------|
| `max_condition` | Coupled 拘束の最大条件数 | `1.0e8` |
| `warn_ratio` | `condition_warning` フラグの割合 | `<= 0.05` |
| `rank_ratio` | ランク欠損フレーム割合 | `<= 0.01` |
| `dropping_equations` | 自動ドロップ回数 | `<= 1` |

- 判定ロジックは `docs/coupled_constraint_solver_math.md` と `docs/coupled_contact_test_notes.md` を参照。  
- `tools/filter_coupled_endurance_log.py` で force/impulse/diagnostics カラムを抽出し、`tools/plot_coupled_constraint_endurance.py --summary-json` から上記 KPI を取得する。

## Appendix 連携
- メディア生成・スクリーンショット・Wiki テンプレは `docs/appendix_optional_ops.md` **A / B.2 / B.3** を参照。  
- 担当ローテーションと通知テンプレは同ファイル **B.5** に記載。  
- Contact + Coupled 併用テストやログ通知の分岐は **B.6** を参照。  
- 記事の軽量化後は本サンプルをコピーし、必要最低限の KPI とリンクのみ差し替える。

## ローテーション / 同期ログ（例）
| 週 (開始日) | 主担当 | バックアップ | 主タスク | 備考 |
|-------------|--------|--------------|----------|------|
| 2025-10-20 | Kobayashi | Tanaka | summary・スクリーンショット更新、Contact+Coupled 判定ログを Appendix B.6 へ反映 | Slack `#chrono-docs` 通知済み |
| 2025-10-27 | Suzuki | Mori | KPI 更新、スクリーンショット差し替え（12h 遅延を報告） | 遅延理由（CI 障害）を備忘録に追記 |
| 2025-11-03 | Tanaka | Kobayashi | Nightly 差分レビュー、Webhook ログ追記、Appendix B.5.1 更新 | `docs/integration/learning_path_map.md` をリンク集へ追加 |
| 2025-11-10 | Mori | Suzuki | KPI / Appendix Check、プリセットの Markdown 参照を案内 | Slack `#chrono-docs` / `#chrono-constraints` に通知、`scripts/check_doc_links.py` 実行結果を共有 |

> 実際の Wiki では最新 4 週を維持し、Appendix B.3/B.4 の棚卸し結果と一致させてください（往復リンク: Appendix B.5）。

---

> 詳細手順・テンプレート・チェックリストは `docs/appendix_optional_ops.md` の **B. Wiki Workflow Templates & Checklists** に統合済みです。本サンプルは本文構成のみを管理し、運用作業は付録のチェックリストに沿って実施してください。
