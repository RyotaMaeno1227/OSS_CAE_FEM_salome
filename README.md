# FEM4C Development Hub

現在の開発対象は次の2点です。

- FEM4C（構造解析ソルバー）
- FEM4C `--mode=mbd`（Project Chrono `chrono-main` を参照した 2D MBD 実装）

## 現在の参照方針
- Project Chrono の参照元は `third_party/chrono/chrono-main` のみ。
- `chrono-C-all` は現行開発では使用しません。

## 主要ディレクトリ
- `FEM4C/` : 実装本体（FEM/MBD）
- `docs/` : 現行運用ドキュメント（runbook/queue/status 等）
- `third_party/chrono/chrono-main` : Chrono 参照元
- `oldFile/` : 旧資料・旧コードの隔離先

## 主要ドキュメント
- `docs/long_term_target_definition.md`
- `docs/team_runbook.md`
- `docs/abc_team_chat_handoff.md`
- `docs/fem4c_team_next_queue.md`
- `FEM4C/docs/solver_reorg_mbd_migration_plan.md`

## 補足
過去の Chrono C 試行コードや旧ドキュメントは `oldFile/` に隔離しています。
現行作業では参照しないでください。
