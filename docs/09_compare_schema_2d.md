# 2D compare schema

最終更新: 2026-03-06

## 1. 方針
- RecurDyn / AdamsFlex / FEM4C の比較は CSV でそろえる。
- 実データが未取得でも schema は先に固定する。
- 比較スクリプトはこの schema のみを前提にする。

## 2. 共通CSV列
| 列名 | 単位 | 説明 |
|---|---|---|
| `time` | s | 時刻 |
| `theta1` | rad | link1 の角度 |
| `theta2` | rad | link2 の角度 |
| `omega1` | rad/s | link1 の角速度 |
| `omega2` | rad/s | link2 の角速度 |
| `tip1_x` | m | link1 tip x |
| `tip1_y` | m | link1 tip y |
| `tip2_x` | m | link2 tip x |
| `tip2_y` | m | link2 tip y |
| `root_reaction_x` | N | root reaction x |
| `root_reaction_y` | N | root reaction y |
| `constraint_residual` | - | 拘束残差ノルム |
| `full_reassembly_count_link1` | count | link1 の full reassembly 累積回数 |
| `full_reassembly_count_link2` | count | link2 の full reassembly 累積回数 |

## 3. ファイル命名
- FEM4C:
  - `results/rigid_2link_explicit.csv`
  - `results/rigid_2link_newmark.csv`
  - `results/rigid_2link_hht.csv`
  - `results/flex_2link_explicit.csv`
  - `results/flex_2link_newmark.csv`
  - `results/flex_2link_hht.csv`
- 参照値:
  - `reference/recurdyn/flex_2link_reference.csv`
  - `reference/adamsflex/flex_2link_reference.csv`

## 4. 実データ未取得時の扱い
- `reference/recurdyn/` と `reference/adamsflex/` は空でもよい。
- 実データ未取得時は compare script は
  - schema validation
  - 列欠落検知
  - 時刻単調増加チェック
 までを行う。
- 実数値比較（RMS/max error）は参照CSV投入後に有効化する。
