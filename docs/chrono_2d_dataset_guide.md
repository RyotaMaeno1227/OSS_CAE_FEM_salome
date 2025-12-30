# chrono-2d Dataset Guide (C13/C5)

このページは chrono-2d の例題データセット説明、更新手順、図版/スクショの命名・保存・参照ルールをまとめたものです。

## 例題データセットの構成
- JSON: `chrono-2d/data/cases_constraints.json`（拘束ケース定義）
- CSV: `chrono-2d/data/cases_contact_extended.csv`, `chrono-2d/data/contact_cases.csv`（接触ケース）
- CSV: `chrono-2d/data/constraint_ranges.csv`（cond 範囲）
- CSV: `chrono-2d/data/cases_combined_constraints.csv`（複合拘束）
- CSV: `chrono-2d/data/approx_tolerances.csv`（近似誤差許容）
- CSV: `chrono-2d/data/parameter_sensitivity_ranges.csv`（感度レンジ）
- Baseline: `chrono-2d/data/bench_baseline.csv`
- 版管理: `chrono-2d/data/dataset_version.txt`（更新日）

## 更新手順（最小版）
1. 対象ファイル（JSON/CSV）を編集し、case 名は snake_case で統一。  
2. `chrono-2d/data/dataset_version.txt` を更新日で上書き。  
3. `make -C chrono-2d test` を実行して PASS を確認。  
4. 変更内容を `docs/team_status.md` に記録し、`docs/documentation_changelog.md` へ追記。  
5. `python scripts/check_doc_links.py <更新md...>` を実行し結果を共有。

## 図版/スクリーンショット ルール（C5）
- 保存先: `docs/integration/assets/` または `docs/media/chrono-2d/`  
- 命名: `chrono-2d-<topic>-<yyyymmdd>.svg|png`  
- 参照: Markdown から相対パスでリンク（README/Hands-on から参照可能にする）  
- スクリーンショットはチャット共有が基本。リポジトリに残す場合は  
  「Run ID + 目的 + 日付」の命名に揃える。  

## README からの参照
- `README.md` の Educational Materials に本ドキュメントをリンクすること。
