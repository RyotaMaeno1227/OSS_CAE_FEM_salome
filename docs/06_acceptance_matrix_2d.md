# 2D acceptance matrix

最終更新: 2026-03-06

## 1. 受入方針
- M0-M3 は FEM4C 単体で確認できる項目を先に固める。
- M4 で外部比較を必須化する。
- 外部比較データ未取得は、M0-M3 の blocker ではない。

## 2. 受入表
| Case ID | フェーズ | 入力ファイル | 積分器 | 比較対象 | 必須出力CSV | 必須指標 | 許容 | 備考 |
|---|---|---|---|---|---|---|---|---|
| R1 | rigid | `examples/mbd_2link_rigid_dyn.dat` | explicit | 解析解/高精度ODE | `rigid_2link_explicit.csv` | `constraint_residual`, `theta1`, `theta2` | PM-03で固定 | M1 |
| R2 | rigid | `examples/mbd_2link_rigid_dyn.dat` | Newmark-beta | 解析解/高精度ODE | `rigid_2link_newmark.csv` | `constraint_residual`, `theta1`, `theta2` | PM-03で固定 | M1 |
| R3 | rigid | `examples/mbd_2link_rigid_dyn.dat` | HHT-alpha | 解析解/高精度ODE | `rigid_2link_hht.csv` | `constraint_residual`, `theta1`, `theta2` | PM-03で固定 | M1 |
| F1 | flexible 1-link | `examples/coupled_1link_flex_master.dat` | explicit | rigid極限/内部比較 | `flex_1link_explicit.csv` | `constraint_residual`, `tip_displacement`, `root_reaction` | PM-03で固定 | M2 |
| F2 | flexible 2-link | `examples/coupled_2link_flex_master.dat` | explicit | RecurDyn/AdamsFlex | `flex_2link_explicit.csv` | `constraint_residual`, `theta1`, `theta2`, `tip_displacement` | PM-03で固定 | M3/M4 |
| F3 | flexible 2-link | `examples/coupled_2link_flex_master.dat` | Newmark-beta | RecurDyn/AdamsFlex | `flex_2link_newmark.csv` | `constraint_residual`, `theta1`, `theta2`, `tip_displacement` | PM-03で固定 | M3/M4 |
| F4 | flexible 2-link | `examples/coupled_2link_flex_master.dat` | HHT-alpha | RecurDyn/AdamsFlex | `flex_2link_hht.csv` | `constraint_residual`, `theta1`, `theta2`, `tip_displacement` | PM-03で固定 | M3/M4 |

## 3. 必須列
- `time`
- `constraint_residual`
- `theta1`
- `theta2`
- `tip1_x`
- `tip1_y`
- `tip2_x`
- `tip2_y`
- `tip_displacement`
- `root_reaction_x`
- `root_reaction_y`

## 4. 実データ未取得時の扱い
- `R1-R3` は今すぐ受入対象。
- `F1` は内部比較（高剛性 limit / 反復収束 / full reassembly回数）で先に進める。
- `F2-F4` は CSV schema と出力経路を先に固定し、RecurDyn / AdamsFlex の実データ取得後に数値比較を有効化する。
- したがって、外部参照値が無い現時点での必須事項は「比較可能な列構成を固定すること」である。
