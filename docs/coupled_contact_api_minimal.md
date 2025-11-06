# Coupled & Contact Core API (Minimal Set)

計算コアを駆動するために必要な C API の最小セットを整理する。ここに挙げない補助 API（ログ、通知、メディア生成など）は任意機能として Appendix へ分離する。

## 1. Coupled Constraint

| 関数 | 用途 | 参照 |
|------|------|------|
| `chrono_coupled_constraint2d_init` | アンカー・軸・初期比率から拘束を初期化 | `chrono-C-all/include/chrono_constraint2d.h:540` |
| `chrono_coupled_constraint2d_prepare` | 時刻 `dt` を受け取り、質量・バイアス・行列を再計算 | `chrono-C-all/include/chrono_constraint2d.h:592` |
| `chrono_coupled_constraint2d_apply_warm_start` | 前フレームのインパルスでウォームスタート | `chrono-C-all/include/chrono_constraint2d.h:593` |
| `chrono_coupled_constraint2d_solve_velocity` | 速度レベルの補正を適用 | `chrono-C-all/include/chrono_constraint2d.h:594` |
| `chrono_coupled_constraint2d_solve_position` | 位置レベルの補正を適用 | `chrono-C-all/include/chrono_constraint2d.h:595` |
| `chrono_coupled_constraint2d_get_diagnostics` | ランク・条件数・ピボットを取得し数値安定性を確認 | `chrono-C-all/include/chrono_constraint2d.h:577` |
| `chrono_coupled_constraint2d_set_equation` / `add_equation` | 多式拘束の設定（最大 4 式） | `chrono-C-all/include/chrono_constraint2d.h:570`, `chrono-C-all/include/chrono_constraint2d.h:572` |

> 上記以外の setter（例: `set_condition_warning_log_level`）は運用補助とみなし、Appendix へ移行予定。

## 2. Contact Core

| 関数 | 用途 | 参照 |
|------|------|------|
| `chrono_contact_manifold2d_init/reset/set_bodies` | マニフォールドの初期化・ボディ紐付け | `chrono-C-all/include/chrono_collision2d.h:39`-`chrono-C-all/include/chrono_collision2d.h:45` |
| `chrono_contact_manager2d_begin_step/end_step` | 1 ステップ内でのマニフォールド管理開始／終了 | `chrono-C-all/include/chrono_collision2d.h:69`-`chrono-C-all/include/chrono_collision2d.h:70` |
| `chrono_collision2d_detect_*` | 幾何ペアごとの衝突検出 | `chrono-C-all/include/chrono_collision2d.h:83`-`chrono-C-all/include/chrono_collision2d.h:130` |
| `chrono_collision2d_resolve_*` | 検出結果から接触マニフォールドを生成 | `chrono-C-all/include/chrono_collision2d.h:132`-`chrono-C-all/include/chrono_collision2d.h:159` |
| `chrono_contact_manager2d_update_contact` | Manifold と検出結果を突き合わせる共通ルート | `chrono-C-all/include/chrono_collision2d.h:78`-`chrono-C-all/include/chrono_collision2d.h:81` |

## 3. Island Solver 接続部

| 関数 | 用途 | 参照 |
|------|------|------|
| `chrono_island2d_workspace_init/reset/free` | 島ワークスペースの確保と再利用 | `chrono-C-all/include/chrono_island2d.h:69`-`chrono-C-all/include/chrono_island2d.h:71` |
| `chrono_island2d_build` | 拘束・接触から島を構築 | `chrono-C-all/include/chrono_island2d.h:73`-`chrono-C-all/include/chrono_island2d.h:77` |
| `chrono_island2d_solve` | 各島の拘束解法を実行 | `chrono-C-all/include/chrono_island2d.h:79`-`chrono-C-all/include/chrono_island2d.h:81` |

> `ChronoIsland2DSolveConfig_C` では `constraint_config` と `enable_parallel` のみを扱い、ログや計測フックは任意機能として別管理とする。

## 4. 非コア API の扱い

- ログレベル変更、条件数通知コールバック、メディア生成スクリプト等は Appendix `docs/optional_features_appendix_plan.md` 側へ移動予定。
- 上記に含まれない追加 API を導入する場合は、数値解に直接寄与するかを基準にこのリストへ加えるかを判断する。

