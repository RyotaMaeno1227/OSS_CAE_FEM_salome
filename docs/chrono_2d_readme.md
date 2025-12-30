# chrono-2d README（学習用 2D ソルバ）

目的: Chrono 2D ソルバ（学習用ミニ実装）のビルド・テスト手順、Run ID 記録ルール、CSV の読み方をまとめる。3D 拡張は行わない前提とし、Chrono C / chrono-main とは別系統として扱う。

### 方針（冒頭チェック）
- 3D 非対応を明記（本書・チャット・テンプレに「chrono-2d」を含める）。
- 依存は OpenMP のみ（oneTBB など不要）。
- 命名ポリシー: プレフィックスに余分な「Chrono」を付けない。生成物は `artifacts/` 配下、CSV は `kkt_descriptor_actions_local.csv` を基本形とする。
- 月次で Run ID / CSV サンプルを差し替え、`docs/documentation_changelog.md` に記録する（目安: 毎月1週目）。

## ビルドとテスト
- 手順:
  ```bash
  cd chrono-2d
  make test            # build + test_coupled_constraint
  # 生成物:
  #  - build/obj/*.o
  #  - tests/test_coupled_constraint
  #  - artifacts/kkt_descriptor_actions_local.csv
  ```
- クリーニング: `make clean`

## Hands-on / Tutorial 簡易フロー
1) `make test` で CSV 生成（`artifacts/kkt_descriptor_actions_local.csv`）  
2) `condition_spectral`, `min_pivot`, `max_pivot` を確認  
3) 判定: `condition_spectral` 10 前後、`min_pivot` ≥ 1e-3 なら安定。異常値は `case` / `time` を特定し Run ID と共有。  
4) チャット共有: 下記テンプレに従い Run ID / CSV 抜粋を貼る（例: `echo "Run local-chrono2d-20251118-01 / Artifact chrono-2d/artifacts/kkt_descriptor_actions_local.csv"`）。
5) リンク/整合チェック: `python tools/check_chrono2d_csv_schema.py --csv artifacts/kkt_descriptor_actions_local.csv`（ヘッダ検証）、`python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md` を実行し、結果をチャットに貼る。

### Hands-on ショートカットスクリプト
`chrono-2d/scripts/run_hands_on.sh <RUN_ID>`  
`make test` → CSV 生成 → 先頭行表示 → `artifacts/run_id.log` にメモ。  
例: `./scripts/run_hands_on.sh local-chrono2d-20251118-01`

## Run ID 記録テンプレ（chrono-2d）
```
- Run: local-chrono2d-<yyyymmdd>-<seq>（例: local-chrono2d-20251118-01）
- Artifact: chrono-2d/artifacts/kkt_descriptor_actions_local.csv
- Log: docs/chrono_2d_readme.md （Run ID を本文に追記）
- Notes: {condition_spectral, min_pivot, max_pivot, case, time}
```
※ `docs/abc_team_chat_handoff.md` と `docs/git_setup.md` の chrono-2d テンプレにも同じ Run ID を記録し、Chrono C / chrono-main と区別する。

## 条件数・Pivot の読み方（表で再現可能に）
| 項目 | 意味 | 目安 | コマンド例 |
|------|------|------|-----------|
| `condition_bound` / `condition_spectral` | 行和 / スペクトル条件数 | 1 に近いほど良い、10 以上で要見直し | `csvstat -H --mean --max artifacts/kkt_descriptor_actions_local.csv -c condition_spectral` |
| `min_pivot` | KKT pivot 最小値 | 1e-4 未満で要注意 | `csvstat -H --min artifacts/kkt_descriptor_actions_local.csv -c min_pivot` |
| `max_pivot` | KKT pivot 最大値 | 極端な値は要確認 | `csvstat -H --max artifacts/kkt_descriptor_actions_local.csv -c max_pivot` |
| `case` | プリセット名 | ケース別の傾向比較 | `csvcut -c case,condition_spectral,min_pivot,max_pivot artifacts/kkt_descriptor_actions_local.csv` |

### 条件数・Pivot クイックチェック（即実行）
```bash
cd chrono-2d
# 平均・最小・最大をまとめて確認
csvstat -H --mean --min --max artifacts/kkt_descriptor_actions_local.csv \
  -c condition_spectral,min_pivot,max_pivot
# しきい値に引っかかる行だけ抽出（例: pivot < 1e-3）
csvsql --query "select case,condition_spectral,min_pivot,max_pivot \
from stdin where min_pivot < 1e-3 or condition_spectral > 10" \
  artifacts/kkt_descriptor_actions_local.csv
```
- 目安: `condition_spectral` が 10 超ならケースの拘束設定を見直し、`min_pivot` が 1e-4 未満なら数値不安定の兆候として共有する。

## 拘束タイプ（2D 学習用の概要）
- 距離: 2 点間距離を固定/制御。比率とスプリング剛性をセットで調整。
- 回転: 角度を固定/制御。ヨー補正など。
- 平面: 平面拘束（2D ではライン拘束相当）で位置合わせ。
- プリズマティック: スライダー軸に沿う並進拘束。
- ギヤ: 回転比率を固定する単純ギヤ結合。
- 接触: 簡易接触（テスト用）で pivot/条件数への影響を観察。

## CSV サンプルとスキーマ
```
time,case,method,vn,vt,mu_s,mu_d,stick,condition_bound,condition_spectral,min_pivot,max_pivot
0.000000,tele_yaw_control,actions,0.00,0.00,0.50,0.40,1,1.650000e+00,1.650000e+00,1.100000e+00,1.100000e+00
```
- `time`: シミュレーション時間 [s]
- `case`: プリセット名
- `method`: `actions` 固定（descriptor モード）
- `vn` / `vt`: 法線・接線速度
- `mu_s` / `mu_d`: 静摩擦・動摩擦係数
- `stick`: スティック判定フラグ
- `condition_bound` / `condition_spectral`: 行和 / スペクトル条件数
- `min_pivot` / `max_pivot`: KKT pivot の最小/最大値  
スキーマテンプレ: `docs/chrono_2d_cases_template.csv`（更新時は本書とセットで差し替え）。  
生成スクリプト: `python tools/check_chrono2d_csv_schema.py --emit-sample chrono-2d/artifacts/kkt_descriptor_actions_local.csv` でテンプレを再発行可能。  
例題データセット: `chrono-2d/data/cases_constraints.json`, `chrono-2d/data/cases_contact_extended.csv`（ケース追加時は README とテンプレを同時更新）。

### CSV スキーマ差分確認（C9）
```bash
python tools/check_chrono2d_csv_schema.py --emit-sample /tmp/chrono2d_schema_sample.csv
diff -u docs/chrono_2d_cases_template.csv /tmp/chrono2d_schema_sample.csv
```
- 差分が出た場合はテンプレ/README を同時に更新し、`docs/documentation_changelog.md` に記録する。

## 例題データセットのフォーマット方針（A5）
- JSON: 拘束ケースの定義（anchor/axis など）を `chrono-2d/data/cases_constraints.json` に集約。  
- CSV: 接触・レンジ・複合拘束は CSV で外部定義し、読み取り対象を `chrono-2d/data/` に固定する。  
  - 接触: `chrono-2d/data/cases_contact_extended.csv` / `chrono-2d/data/contact_cases.csv`  
  - 拘束レンジ: `chrono-2d/data/constraint_ranges.csv`  
  - 複合拘束: `chrono-2d/data/cases_combined_constraints.csv`  
- 命名: `case` 名は snake_case を維持し、JSON/CSV 間で同一名を使う。
- 参照パス: 例題は `chrono-2d/data/` のみを参照し、`chrono-2d/artifacts/` の CSV を入力にしない。  
- データ一覧: `bench_baseline.csv`, `cases_constraints.json`, `cases_contact_extended.csv`, `contact_cases.csv`, `constraint_ranges.csv`, `cases_combined_constraints.csv` を基準セットとする。
- コード側は `CHRONO2D_DATA_DIR` / `CHRONO2D_DATA_PATH()` で参照パスを統一する。

## 近似誤差許容と感度レンジ（A7/A14）
- 近似誤差許容（determinism 用）: `chrono-2d/data/approx_tolerances.csv`  
  `case,cond_tol,pivot_tol` で case 別の許容誤差を設定し、テスト側で適用する。  
- 追加ルール: 新規 case を追加する場合は `approx_tolerances.csv` に同名行を追加し、許容値の根拠（参照ログやスイープ結果）をメモに残す。
- パラメータ感度レンジ: `chrono-2d/data/parameter_sensitivity_ranges.csv`  
  `case,cond_min,cond_max,pivot_min,pivot_max` で許容レンジを外出しし、条件数/ピボットの範囲判定に使う。
  複合拘束は `cases_combined_constraints.csv` と同名で運用する。
- データセット版: `chrono-2d/data/dataset_version.txt`  
  データセット更新時に日付ベースで更新し、テスト側で存在確認する（欠落時は fail）。

## 異常系ダンプ/復帰（A10）
- `tests/test_coupled_constraint` は `--dump-json <path>` で最小再現 JSON を出力。  
  失敗理由、`descriptor_log` パス、`approx_tolerances.csv` / `parameter_sensitivity_ranges.csv` の参照、スレッド設定、  
  各 case の cond/pivot/接触パラメータ/J 行を含める。  
- 復帰時は JSON と Run ID をセットで共有し、同一入力で再実行できることを確認する。
  例:
  ```json
  {
    "reason": "composite_planar_prismatic_range",
    "descriptor_log": "artifacts/kkt_descriptor_actions_local.csv",
    "tolerance_csv": "data/approx_tolerances.csv",
    "sensitivity_csv": "data/parameter_sensitivity_ranges.csv",
    "dataset_version": "2025-12-01",
    "dataset_version_path": "data/dataset_version.txt",
    "threads": {"compare": 1, "list": [1, 8]},
    "cases": [{"name": "composite_planar_prismatic", "cond_bound": 1.234e+00, "pivot_min": 1.000e-03}]
  }
  ```

## ログ粒度ポリシー（A15）
- 最小ログ: `--verbose` なしで `make test` を実行し、Run ID と CSV head のみを記録。  
- 詳細ログ: 失敗時のみ `--verbose` と `--dump-json` を付与し、`failure_dump.json` を添付。  
- 報告ルール: Run ID / Artifact / git status / リンクチェック結果を `docs/team_status.md` に記載。
- 実行時間の記録: `scripts/run_timed.py` が所要時間を出力し、上限超過は WARN として表示。  
  既定値: `MAX_TEST_TIME_SEC=30`, `MAX_SCHEMA_TIME_SEC=10`, `MAX_BENCH_TIME_SEC=10`（必要に応じて調整）。

## ケース生成スクリプト（A11）
`chrono-2d/scripts/gen_constraint_cases.py`  
- 生成物の配置: `--output-dir chrono-2d/data/generated` を推奨。  
- 命名ルール:  
  - `cases_constraints_sweep.json`  
  - `cases_contact_sweep.csv`  
- 入力: `chrono-2d/data/cases_constraints.json`, `chrono-2d/data/cases_contact_extended.csv` を読み込む。  
- 生成物レイアウト（固定パス）:
  - `chrono-2d/data/generated/README.md`（生成条件・スケール一覧）
  - `chrono-2d/data/generated/cases_constraints_sweep.json`
  - `chrono-2d/data/generated/cases_contact_sweep.csv`
- 運用導線: 生成物は `chrono-2d/data/generated/` に固定し、Aチームは README に生成条件を追記してから共有する。
- 例:  
  ```bash
  python chrono-2d/scripts/gen_constraint_cases.py \
    --output-dir chrono-2d/data/generated \
    --emit-constraint-sweep --emit-contact-sweep \
    --sweep-scales 0.5,1.0,2.0
  ```

## チャット共有テンプレ（chrono-2d）
```
[chrono-2d] make test
- Run: local-chrono2d-20251118-01
- Artifact: chrono-2d/artifacts/kkt_descriptor_actions_local.csv
- Summary: cond_spectral≈1.65, min_pivot=1.10e+00, case=tele_yaw_control
- Diff: (必要なら抜粋を貼付)
```
共有時は `git status` と CSV 抜粋（先頭/異常行）を貼り、Chrono C / chrono-main の Run ID と混在させない。

## 命名・見出しポリシー
- 見出しは「chrono-2d ...」で開始、3D には言及しない。
- 生成物は `artifacts/` 配下、CSV 名は `kkt_descriptor_actions_local.csv` を基本形とする。
- プレフィックスに余分な「Chrono」を付けない。
- 用語/表記ガイド: `Run ID` / `Artifact` / `Log` を固定表記とし、`OpenMP`/`3D` の表記は統一する。

## 更新フローとチェック
- README/Hands-on/チャットテンプレを更新したら同じコミットで Run ID テンプレも整合。
- リンク・整合チェック: `python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md` を実行し、結果をチャットに貼る。
- Changelog 追記トリガー: Run ID/命名/拘束説明/CSV/リンク更新時は `docs/documentation_changelog.md` に記録。
- 表記揺れチェック: 見出しは sentence case、コードブロックは ```bash```/```csv``` のみ。
- アーカイブ方針: 古い Run ID/CSV を差し替える場合は `artifacts/archive/` に退避し、Changelog に移動日を記載。

### CI/運用導線（C15）
- 実行・報告ルールは `docs/team_runbook.md` を参照し、15分スプリント/長尺バッチの区分に沿って報告する。
- 実行ログは `docs/team_status.md` の C チーム欄に追記し、Run ID/生成物/リンクチェック結果を残す。

## 図版・スクリーンショットルール
- 保存先: `docs/integration/assets/` または `docs/media/chrono-2d/`（新設可）。
- 命名: `chrono-2d-<topic>-<yyyymmdd>.svg|png`。キャプションに Run ID を含める。
- 参照: Markdown から相対パスでリンク。追加時は Changelog に追記。
詳細: `docs/chrono_2d_media_rules.md`

## 例題データセット（概要と更新手順）
### 収録データ
- `chrono-2d/data/cases_constraints.json`: 拘束系の例題（基本ケース）。
- `chrono-2d/data/cases_contact_extended.csv`: 接触系の拡張ケース。
- `chrono-2d/data/cases_combined_constraints.csv`: 複合拘束の例題。

### 更新手順（C13）
1. 変更対象ファイルと目的を明記する（例: `cases_constraints.json` にケース追加）。
2. 変更内容が README/テンプレに影響する場合は同時更新する。
3. Run ID/Artifact 共有が必要な場合は `docs/abc_team_chat_handoff.md` と合わせて記録する。
4. リンク/整合チェック: `python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md`
5. `docs/documentation_changelog.md` に更新内容を追記する。

## 学習ステップチェックリスト（コマンド付き）
1ページ版: `docs/chrono_2d_glossary_checklist.md` を参照（用語/表記ガイド＋チェックリスト）。

1. `cd chrono-2d && make test`
2. `head -n 5 artifacts/kkt_descriptor_actions_local.csv`
3. （任意）`csvstat --mean --min --max artifacts/kkt_descriptor_actions_local.csv -c condition_spectral,min_pivot,max_pivot`
4. Run ID を本書と `docs/abc_team_chat_handoff.md` に記録
5. チャットテンプレで共有（上記フォーマット）
6. `python tools/check_chrono2d_csv_schema.py --csv artifacts/kkt_descriptor_actions_local.csv`
7. （スクリプトがあれば）`python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md`
8. `docs/documentation_changelog.md` に更新履歴を追記

## 新規ドキュメント追加時のルール
- 見出し先頭に「chrono-2d」を付け、3D や chrono-main と混同しない。
- Run ID テンプレと命名ポリシーを本文にリンクし、`docs/abc_team_chat_handoff.md` へ参照を追加。
- 追加直後にリンクチェックを走らせ、結果をチャットに貼る。

### フォーマット統一と簡易Lint（C12）
```bash
python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md docs/team_runbook.md
```
- Markdown は sentence case の見出し＋`bash`/`csv` のコードフェンスを維持。
- Run ID と Artifact は「chrono-2d」を含めて chrono-main / Chrono C と混在させない。
- チェック結果はチャットに貼り、必要なら `docs/documentation_changelog.md` に追記する。
