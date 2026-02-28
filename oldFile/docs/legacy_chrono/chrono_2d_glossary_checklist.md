# chrono-2d 用語・表記ガイド＋学習ステップ（1ページ版）

目的: chrono-2d の用語/表記揺れを防ぎ、最小限の学習ステップを 1 ページで確認できるようにする。

## 用語・表記ガイド
| 用語 | 定義/使い方 | 表記ルール |
|------|------------|------------|
| Run ID | 実行を一意に識別するID | `local-chrono2d-YYYYMMDD-XX` を使用 |
| Artifact | 生成物（CSVなど）の保存先 | `chrono-2d/artifacts/kkt_descriptor_actions_local.csv` を基本形とする |
| Log | Run ID を記録する場所 | `docs/chrono_2d_readme.md` に追記 |
| condition_spectral | スペクトル条件数 | `condition_spectral` の表記を固定 |
| min_pivot / max_pivot | KKT pivot の最小/最大値 | `min_pivot` / `max_pivot` を固定 |
| case | プリセット名 | `case` を固定 |
| method | descriptor モード名 | `actions` 固定（chrono-2d） |
| OpenMP | 並列化の前提 | `OpenMP` と表記する（openmp/OMP は避ける） |
| 3D | 非対応の前提 | `3D` と表記する |

## 学習ステップチェックリスト（コマンド＋期待出力）
1. ビルドとCSV生成
   ```bash
   cd chrono-2d
   make test
   ```
   期待: `artifacts/kkt_descriptor_actions_local.csv` が生成される。
2. CSV 先頭の確認
   ```bash
   head -n 3 artifacts/kkt_descriptor_actions_local.csv
   ```
   期待: ヘッダ行に `condition_spectral,min_pivot,max_pivot` が含まれる。
3. 条件数/ピボットの概要確認
   ```bash
   csvstat -H --mean --min --max artifacts/kkt_descriptor_actions_local.csv \
     -c condition_spectral,min_pivot,max_pivot
   ```
   期待: `condition_spectral` が 10 以上なら要見直し、`min_pivot` が 1e-4 未満なら要注意。
4. Run ID と共有ワンライナー
   ```bash
   echo "Run local-chrono2d-YYYYMMDD-XX / Artifact chrono-2d/artifacts/kkt_descriptor_actions_local.csv"
   ```
   期待: `docs/chrono_2d_readme.md` と `docs/abc_team_chat_handoff.md` に同じ Run ID を記録。
5. CSV スキーマ確認
   ```bash
   python tools/check_chrono2d_csv_schema.py --csv artifacts/kkt_descriptor_actions_local.csv
   ```
   期待: `[chrono-2d] OK` と表示される。
6. リンク/整合チェック（更新時のみ）
   ```bash
   python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md
   ```
   期待: `All links validated` が表示される。

## 参照
- `docs/chrono_2d_readme.md`（Run ID テンプレ・CSV説明）
- `docs/team_runbook.md`（Cチームの実行指示と報告ルール）
