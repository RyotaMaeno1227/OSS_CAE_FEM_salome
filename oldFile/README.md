# oldFile

このディレクトリは、現行開発で使わない旧資料・旧コードを隔離するための保管場所です。

## 目的
- 現行作業（FEM4C + Project Chrono `chrono-main` 参照）での混乱を防ぐ。
- 参照禁止・非推奨の資料を作業ディレクトリから分離する。

## 収容方針
- `oldFile/chrono-C-all/`
  - 旧試行の Chrono C ポート資産（現在は参照禁止）。
- `oldFile/legacy_root/`
  - 旧トップレベル資産一式（`chrono-2d/`, `src/`, `tools/`, `data/`, `config/`, `practice/`, `include/`, `examples/` など）。
- `oldFile/github/workflows/`
  - 旧 workflow（`chrono_2d_*`, `coupled_*`, `nightly_*`）。
- `oldFile/docs/legacy_chrono/`
  - 旧 Chrono/chrono-2d/coupled 系ドキュメント。
- `oldFile/docs/legacy_descriptor/`
  - 旧 descriptor/KKT 補助ドキュメントと週次ログ（現行運用外）。
- `oldFile/docs/archive/`, `oldFile/docs/integration/`, `oldFile/docs/logs/`, `oldFile/docs/templates/`, `oldFile/docs/wiki_samples/`
  - 旧運用・補助資料（現行スコープ外）。
- `oldFile/tmp_notes/`
  - 一時的に生成された `tmp*.md` メモ。

## 例外（現行運用で使用）
- 以下は現行チーム運用で参照するため `docs/` に残す。
  - `docs/fem4c_team_dispatch_2026-02-06.md`
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`

## 運用ルール
- 現行実装・設計の根拠には使わない。
- 必要になった場合のみ、内容をレビューして現行ドキュメントへ再編集して戻す。
- `third_party/chrono/chrono-main` が唯一の Project Chrono 参照元。
