# Appendix Consolidation Plan (Optional Features)

> **Status:** 2025-11 の方針転換により Appendix そのものを廃止しました。本メモは履歴として残しているだけで、ここに記載された作業はすべて凍結済みです。

計算コアに直接関係しない運用・通知系ドキュメントを別 Appendix に集約するための抜粋メモ（アーカイブ）。

## 対象候補と移設案

| 現行ドキュメント | 範囲 | 現状の主旨 | Appendix 推奨セクション | 備考 |
|------------------|------|------------|-------------------------|------|
| `docs/chrono_coupled_constraint_tutorial.md:321`-`docs/chrono_coupled_constraint_tutorial.md:410` | GitHub Pages 公開・動画埋め込み | メディア共有や CI サイズ監視など運用寄り | Appendix A（`docs/appendix_optional_ops.md`） | 数値検証とは独立。リンクは最小限残し、詳細手順を Appendix へ移動。 |
| ~~`docs/wiki_coupled_endurance_article.md:42`-`docs/wiki_coupled_endurance_article.md:116`~~ | ~~社内 Wiki 投稿テンプレ／スクリーンショット管理~~ | ~~通知／レポート運用~~ | ~~Appendix B（同上）~~ | 通知系ドキュメントは削除済み。今後は Chrono 移植と教育資料のみ管理する。 |
| `docs/wiki_samples/coupled_endurance_article_sample.md:71`-`docs/wiki_samples/coupled_endurance_article_sample.md:138` | Wiki 用コピーテンプレ／公開フロー | オペレーション資料 | Appendix B (添付資料) | Appendix 移行後はサンプルファイルをそのまま引用し、メインにはリンクのみ残す。 |
| `docs/chrono_logging_integration.md:134`-`docs/chrono_logging_integration.md:145` | 複数シンクのログ転送 | ログ通知手順 | Appendix C | ログ API 自体は計算コアに必要だが、通知ワークフロー説明は Appendix で良い。 |
| `docs/coupled_benchmark_setup.md` 全体 | GitHub Actions/Pages 連携 | ベンチ公開と通知 | Appendix D | 解析スクリプト (`tools/*`) へのリンクだけ本編に残す。 |

## 実施ステップ案
> この計画は通知／運用系ドキュメントを Appendix へ集約するためのものでしたが、2025-11 の方針転換により Appendix 自体を削除しました。現在は Project Chrono 移植と教育資料のみを管理対象としています。

### 進行状況メモ
- 2025-11-14: 方針変更により Appendix を含む通知／運用系ドキュメントを削除。Project Chrono 移植と教育資料のみをリポジトリで扱う。

## 未決事項
- Appendix の目次構成（媒体共有／Wiki／CI／通知など）。
- Appendix を GitHub Pages へ公開するか、リポジトリ内部のみとするか。
- サンプル画像・動画ファイルの配置（`docs/media/` にそのまま残すか、`appendix/media/` へ移動するか）。
