# Coupled & Contact Core API (Minimal Set)

計算コアを駆動するために必要な C API の最小セットを整理する。ここに挙げない補助 API（ログ、通知、メディア生成など）は任意機能として Appendix へ分離する。

## フェーズ別 API

### フェーズ 1: 初期化（Init）

| コンポーネント | 関数 | 主な用途 | 参照 |
|----------------|------|----------|------|
| Coupled 拘束 | `chrono_coupled_constraint2d_init` | アンカー、軸、比率、ばね・ダンパを登録して Equation バッファを初期化 | `chrono-C-all/include/chrono_constraint2d.h:540` |
| Coupled 拘束 | `chrono_coupled_constraint2d_set_equation` / `chrono_coupled_constraint2d_add_equation` | 多式拘束（最大 4 式）の追加・更新 | `chrono-C-all/include/chrono_constraint2d.h:570`, `:572` |
| 接触管理 | `chrono_contact_manifold2d_init` / `reset` / `set_bodies` | Manifold の準備、ボディ紐付け | `chrono-C-all/include/chrono_collision2d.h:39`-`45` |
| 接触管理 | `chrono_contact_manager2d_begin_step` | ステップ開始時のマニフォールド状態クリア | `chrono-C-all/include/chrono_collision2d.h:69` |
| 島ソルバ | `chrono_island2d_workspace_init` / `reset` | 島情報用ワークスペースのメモリ確保・再利用 | `chrono-C-all/include/chrono_island2d.h:69`-`71` |

### フェーズ 2: 解法（Solve）

| コンポーネント | 関数 | 主な用途 | 参照 |
|----------------|------|----------|------|
| Coupled 拘束 | `chrono_coupled_constraint2d_prepare` | `dt` を受け取り、KKT ブロックとバイアスを更新 | `chrono-C-all/include/chrono_constraint2d.h:592` |
| Coupled 拘束 | `chrono_coupled_constraint2d_apply_warm_start` | 前フレームのインパルスでウォームスタート | `chrono-C-all/include/chrono_constraint2d.h:593` |
| Coupled 拘束 | `chrono_coupled_constraint2d_solve_velocity` | 速度レベルの拘束補正と λ 更新 | `chrono-C-all/include/chrono_constraint2d.h:594` |
| Coupled 拘束 | `chrono_coupled_constraint2d_solve_position` | 位置ドリフト補正（Baumgarte） | `chrono-C-all/include/chrono_constraint2d.h:595` |
| 接触管理 | `chrono_collision2d_detect_*` | 幾何ペアごとの衝突検出（円/ポリゴン/カプセル/エッジ） | `chrono-C-all/include/chrono_collision2d.h:83`-`130` |
| 接触管理 | `chrono_collision2d_resolve_*` | 検出結果から Manifold を構築し拘束量を計算 | `chrono-C-all/include/chrono_collision2d.h:132`-`159` |
| 接触管理 | `chrono_contact_manager2d_update_contact` | 検出結果をマニフォールドへ反映 | `chrono-C-all/include/chrono_collision2d.h:78`-`81` |
| 島ソルバ | `chrono_island2d_build` | 拘束・接触から連結成分（島）を生成 | `chrono-C-all/include/chrono_island2d.h:73`-`77` |
| 島ソルバ | `chrono_island2d_solve` | 島単位で `constraint_config` を使い解法を実行 | `chrono-C-all/include/chrono_island2d.h:79`-`81` |
| 島ソルバ | `chrono_island2d_workspace_free` | ワークスペースの解放（シャットダウン時） | `chrono-C-all/include/chrono_island2d.h:70` |

### フェーズ 3: ダイアグノスティクス & モニタリング

| コンポーネント | 関数 | 主な用途 | 参照 |
|----------------|------|----------|------|
| Coupled 拘束 | `chrono_coupled_constraint2d_get_diagnostics` | ランク、条件数、Pivot 最小値/最大値を取得 | `chrono-C-all/include/chrono_constraint2d.h:577` |
| Coupled 拘束 | `chrono_coupled_constraint2d_get_condition_warning_policy` / `set_condition_warning_policy` | 条件数ワーニングの自動ドロップやログ設定 | `chrono-C-all/include/chrono_constraint2d.h:578`, `:589` |
| 接触管理 | `chrono_contact_manager2d_end_step` | ステップ終了時のマニフォールド集計・後処理 | `chrono-C-all/include/chrono_collision2d.h:70` |
| 島ソルバ | `ChronoIsland2DSolveConfig_C` (`enable_parallel`, `constraint_config`) | 並列モードや反復回数のトラッキング設定 | `chrono-C-all/include/chrono_island2d.h:65` |

> `ChronoIsland2DSolveConfig_C` では `constraint_config` と `enable_parallel` のみを扱い、ログや計測フックは任意機能として別管理とする。

## 4. 非コア API の扱い

- ログレベル変更、条件数通知コールバック、メディア生成スクリプト等は `docs/appendix_optional_ops.md` を参照（Appendix C/D）。  
- 上記に含まれない追加 API を導入する場合は、数値解に直接寄与するかを基準にこのリストへ加えるかを判断する。
