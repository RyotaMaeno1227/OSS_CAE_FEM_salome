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
※ `docs/abc_team_chat_handoff.md` の chrono-2d テンプレにも同じ Run ID を記録し、Chrono C / chrono-main と区別する。

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

## 学習ステップチェックリスト（コマンド付き）
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
