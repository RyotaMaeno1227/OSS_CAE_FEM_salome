# chrono-2d README（学習用 2D ソルバ）

目的: Chrono 2D ソルバ（学習用ミニ実装）のビルドとテスト手順、Run ID 記録ルール、CSV の読み方をまとめる。3D 拡張は行わない前提とし、Chrono C / chrono-main とは別系統として扱う。

## ビルドとテスト
- 依存: OpenMP のみ（oneTBB なし、外部ライブラリなし）。
- 手順:
  ```bash
  cd chrono-2d
  make test            # build + test_coupled_constraint
  # 生成物
  #  - build/obj/*.o
  #  - tests/test_coupled_constraint
  #  - artifacts/kkt_descriptor_actions_local.csv
  ```
- クリーニング: `make clean`

## Hands-on / Tutorial 簡易フロー
1) `make test` を実行して CSV を生成 (`artifacts/kkt_descriptor_actions_local.csv`)  
2) CSV を開き、各行の `condition_spectral`, `min_pivot`, `max_pivot` を確認  
3) 判定: `condition_spectral` が 10 前後、`min_pivot` が 1e-3 以上なら安定。異常値があれば `case` / `time` で該当ステップを特定し、Run ID と併せて共有する。

## Run ID 記録テンプレ（chrono-2d）
```
- Run: local-chrono2d-<yyyymmdd>-<seq>
- Artifact: chrono-2d/artifacts/kkt_descriptor_actions_local.csv
- Log: docs/chrono_2d_readme.md （Run ID を本文に追記）
- Notes: {condition_spectral, min_pivot, max_pivot, case, time}
```
※ `docs/abc_team_chat_handoff.md` の chrono-2d 行にも同じ Run ID を記録し、Chrono C / chrono-main と区別する。

## 条件数・Pivot の読み方（教育向け）
- `condition_bound` / `condition_spectral`: 行和 / スペクトル条件数の近似。1 に近いほどよく、10 以上ならパラメータ見直し候補。
- `min_pivot` / `max_pivot`: KKT ブロックの pivot 範囲。`min_pivot` が 1e-4 未満なら軟化や比率調整を検討。
- `case` 列: プリセット名（例: `tele_yaw_control`）。学習時は「ケースごとに条件数・pivot がどう変わるか」を比較する。

## 拘束タイプ（2D 学習用の概要）
- 距離: 2 点間距離を固定/制御。比率とスプリング剛性をセットで調整。
- 回転: 角度を固定/制御。ヨー補正など。
- 平面: 平面拘束（2D ではライン拘束相当）で位置合わせ。
- プリズマティック: スライダー軸に沿う並進拘束。
- ギヤ: 回転比率を固定する単純ギヤ結合。
- 接触: 簡易接触（テスト用）で pivot/条件数への影響を観察。

## CSV サンプル（先頭行）
```
time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot
0.000000,tele_yaw_control,actions,1.650000e+00,1.650000e+00,1.100000e+00,1.100000e+00
```
- カラム説明:
  - `time`: シミュレーション時間 [s]
  - `case`: プリセット名
  - `method`: `actions` 固定（descriptor モード）
  - `condition_bound` / `condition_spectral`: 行和 / スペクトル条件数
  - `min_pivot` / `max_pivot`: KKT pivot の最小/最大値

## チャット共有テンプレ（chrono-2d）
```
[chrono-2d] make test
- Run: local-chrono2d-20251117-01
- Artifact: chrono-2d/artifacts/kkt_descriptor_actions_local.csv
- Summary: cond_spectral≈1.65, min_pivot=1.10e+00, case=tele_yaw_control
- Diff: (必要なら抜粋を貼付)
```
共有時は `git status` と CSV 抜粋（先頭/異常行）を貼り、Chrono C / chrono-main の Run ID と混在させないようにする。

## 命名・見出しポリシー
- プレフィックスに `Chrono` を付けない（既にディレクトリ名で区別）。
- 見出しは「chrono-2d ...」で開始し、3D には言及しない。
- 生成物は `artifacts/` 配下に置き、CSV 名は `kkt_descriptor_actions_local.csv` を基本形とする。***
