# 2D module ownership and runner shrink plan

最終更新: 2026-03-06

## 1. 方針
- `runner.c` は入口と mode 分岐に縮退する。
- MBD 実行本体は `src/mbd/` に寄せる。
- flexible body と coupled orchestration は `src/coupled/` に寄せる。
- 既存 FEM kernel は `src/solver/` / `src/elements/` / `src/io/` の既存資産を活用し、必要最小限の wrapper を追加する。

## 2. モジュール責務
| 領域 | 責務 | 主なファイル |
|---|---|---|
| `analysis/` | CLI入口、mode分岐、parse済み構造体の run 呼び出し | `src/analysis/runner.c` |
| `mbd/` | body state、force assemble、constraint residual/Jacobian、KKT、explicit/Newmark/HHT、projection、MBD出力 | `src/mbd/body2d.*`, `system2d.*`, `assembler2d.*`, `integrator_*2d.*`, `output2d.*` |
| `coupled/` | flexible link wrapper、nodeset、runtime BC、full reassembly wrapper、snapshot、MBD-FEM 反力授受、coupled step/run | `src/coupled/flex_*`, `case2d.*`, `coupled_step_*2d.*`, `coupled_run2d.*` |
| `io/` | 既存入力、MBD/Coupled directive parse、出力補助 | `src/io/input.c`, `src/io/output.c` |
| `solver/` / `elements/` | 既存 FEM kernel、assembly、linear solve、要素剛性 | `src/solver/*`, `src/elements/*` |
| `scripts/` | acceptance、comparison、補助検証 | `scripts/run_2d_coupled_acceptance.sh`, `scripts/compare_*.py` |

## 3. `runner.c` から追い出す責務
- body/constraint の実体管理
- KKT 行列構築
- explicit/Newmark/HHT の時間積分本体
- flexible body 生成と FE solve orchestration
- coupled same-step iteration
- 時系列 CSV 出力

## 4. `runner.c` に残す責務
- CLI option 解釈
- mode 判定（`fem`, `mbd`, `coupled`）
- parse 結果の妥当性確認
- `mbd_system2d_run()` / `coupled_run2d()` / FEM既存経路の呼び出し
- 終了コードの返却

## 5. チーム担当ファイル
| チーム | 主担当 |
|---|---|
| PM | `docs/04_2d_coupled_scope.md`, `docs/05_module_ownership_2d.md`, `docs/06_acceptance_matrix_2d.md`, `docs/07_input_spec_coupled_2d.md`, `docs/08_merge_order_2d.md`, `docs/09_compare_schema_2d.md` |
| A | `src/mbd/body2d.*`, `forces2d.*`, `kinematics2d.*`, `integrator_explicit2d.*`, `output2d.*` |
| B | `src/mbd/system2d.*`, `constraint2d.*`, `assembler2d.*`, `linear_solver_dense.*`, `integrator_newmark2d.*`, `integrator_hht2d.*`, `projection2d.*` |
| C | `src/coupled/fem_model_copy.*`, `flex_solver2d.*`, `flex_bc2d.*`, `flex_nodeset.*`, `flex_snapshot2d.*` |
| D | `src/coupled/flex_body2d.*`, `flex_reaction2d.*`, `case2d.*` の flexible body 側 |
| E | `src/analysis/runner.c`, `src/coupled/coupled_step_*2d.*`, `src/coupled/coupled_run2d.*`, `examples/*`, `scripts/compare_*.py`, `scripts/run_2d_coupled_acceptance.sh` |

## 6. Chrono参照ルール
- 参照は `third_party/chrono/chrono-main` のみ。
- 真似する対象は責務分割、state保持、integrator構成、constraint/KKT の設計思想。
- そのまま移植しない対象は 3D 前提の API、接触系、複雑な補助モジュール群。
