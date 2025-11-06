# Appendix Consolidation Plan (Optional Features)

計算コアに直接関係しない運用・通知系ドキュメントを別 Appendix に集約するための抜粋メモ。

## 対象候補と移設案

| 現行ドキュメント | 範囲 | 現状の主旨 | Appendix 推奨セクション | 備考 |
|------------------|------|------------|-------------------------|------|
| `docs/chrono_coupled_constraint_tutorial.md:321`-`docs/chrono_coupled_constraint_tutorial.md:410` | GitHub Pages 公開・動画埋め込み | メディア共有や CI サイズ監視など運用寄り | Appendix A: Media Publishing & Sharing | 数値検証とは独立。リンクは最小限残し、詳細手順を Appendix へ移動。 |
| `docs/wiki_coupled_endurance_article.md:42`-`docs/wiki_coupled_endurance_article.md:116` | 社内 Wiki 投稿テンプレ／スクリーンショット管理 | 通知／レポート運用 | Appendix B: Wiki Workflow Templates | 本編ではフロー概要のみを保持、テンプレ全文とチェックリストを Appendix へ。 |
| `docs/wiki_samples/coupled_endurance_article_sample.md:71`-`docs/wiki_samples/coupled_endurance_article_sample.md:138` | Wiki 用コピーテンプレ／公開フロー | オペレーション資料 | Appendix B (添付資料) | Appendix 移行後はサンプルファイルをそのまま引用し、メインにはリンクのみ残す。 |
| `docs/chrono_logging_integration.md:134`-`docs/chrono_logging_integration.md:145` | 複数シンクのログ転送 | ログ通知手順 | Appendix C: Diagnostics & Notification | ログ API 自体は計算コアに必要だが、通知ワークフロー説明は Appendix で良い。 |
| `docs/coupled_benchmark_setup.md` 全体 | GitHub Actions/Pages 連携 | ベンチ公開と通知 | Appendix D: CI/Benchmark Ops | 解析スクリプト (`tools/*`) へのリンクだけ本編に残す。 |

## 実施ステップ案
1. 各ドキュメントの該当セクションを切り出し、`docs/appendix_optional_ops.md`（新設予定）へ統合。
2. 元ドキュメントには章見出しを残し、「詳細は Appendix」を追記。コード／数値手順は従来通り残す。
3. Appendix 側では更新履歴と担当ローテーションをまとめ、オペレーションの参照先を一元化。

## 未決事項
- Appendix の目次構成（媒体共有／Wiki／CI／通知など）。
- Appendix を GitHub Pages へ公開するか、リポジトリ内部のみとするか。
- サンプル画像・動画ファイルの配置（`docs/media/` にそのまま残すか、`appendix/media/` へ移動するか）。

