# chrono-C-all 2D Constraint Stabilization

This directory contains a minimal C implementation of the 2D rigid body and distance
constraint utilities used for experimentation within `chrono-C-all`.  The
implementation focuses on providing a stabilized distance constraint solver that
combines Baumgarte velocity biasing, warm starting, and a soft constraint term to
control compliance.

## Key APIs

- `chrono_body2d_set_circle_shape(body, radius)`: assign a circular shape for a body.  Inverse
  mass / inertia must be set manually via `chrono_body2d_set_mass`.
- `chrono_body2d_set_polygon_shape(body, vertices, count)`: register a convex polygon (local coordinates).
- `chrono_body2d_set_polygon_shape_with_density(body, vertices, count, density)`: convenience helper that
  stores the polygon shape _and_ computes mass / inertia for a uniform density polygon.  Setting `density <= 0`
  leaves the body static.
- `chrono_body2d_set_capsule_shape(body, half_length, radius)` / `chrono_body2d_set_edge_shape(body, start, end)`:
  define additional convex primitives.  Capsules participate in GJK/EPA-based collision detection; edges provide
  lightweight, zero-thickness barriers.
- `chrono_collision2d_detect_convex_gjk(body_a, body_b, contact)`: generic collision entry point covering circles,
  polygons, capsules, and edges via a 2D GJK + EPA pipeline.  Convenience wrappers such as
  `chrono_collision2d_detect_capsule_capsule` and `chrono_collision2d_detect_circle_edge` defer to this path.
- `chrono_collision2d_detect_* / resolve_*`: collision routines covering circle–circle, circle–polygon, and
  polygon–polygon (convex) pairs.  Each detection function fills a `ChronoContact2D_C` struct that can store up
  to two contact points for use with `chrono_collision2d_resolve_contact` and the contact manager.
- `chrono_distance_constraint2d_set_spring(constraint, stiffness, damping)`: 距離拘束をソフト化するフック。バネ定数・減衰を設定すると `last_spring_force` に最新の引張力を出力し、`tests/test_distance_constraint_soft` で動的な収束挙動を回帰できます。`chrono_distance_constraint2d_set_softness_linear` / `_angular` で平行移動と回転のコンプライアンスを個別に調整できます。
- `chrono_distance_angle_constraint2d_*`: 距離と相対角を同時に拘束する複合ジョイント。線形／角度ソフトネスやスプリングの設定、`last_distance_force` / `last_angle_force` でログ出力が可能です。
- `chrono_coupled_constraint2d_*`: 距離と角度の線形結合 `a * (d - d0) + b * (θ - θ0) = c` を維持する拘束。最大 4 本の線形式を同時に保持でき、式ごとにソフトネス・バネ/ダンパを個別設定可能です。`chrono_coupled_constraint2d_add_equation` / `set_equation` で係数やターゲットを追加し、`chrono_coupled_constraint2d_get_diagnostics` で条件数やランク欠損を取得できます。`chrono_coupled_constraint2d_set_condition_warning_policy` により、条件数閾値超過時のログ出力クールダウンや自動方程式無効化（最小対角の式をドロップ）を有効化できます。`last_distance_force_eq[]` / `last_angle_force_eq[]` には式別の最新反力を記録します。
- `chrono_prismatic_constraint2d_*`: slider joint API。`chrono_prismatic_constraint2d_set_limit_spring` でソフトリミット、
  `chrono_prismatic_constraint2d_set_motor_position_target` で位置制御モードに切替えられます。
- `chrono_revolute_constraint2d_enable_motor` / `chrono_revolute_constraint2d_set_motor_position_target`: ピンジョイントに
  速度・位置モータを追加し、`last_motor_torque` で直近の駆動トルクを取得可能です。
- `chrono_gear_constraint2d_*`: 2 つのボディの角速度を比率付きで拘束するギア拘束。`ratio` と `phase` を設定して
  角度・角速度の線形関係を維持します。
- `chrono_planar_constraint2d_*`: 平面ジョイント（2軸スライダ）。`enable_limit` / `set_limit_spring` で各軸のソフトリミット、
  `set_motor_position_target` で X/Y 軸それぞれの位置モータ制御が可能です。

## Building the tests

The distance constraint regression `tests/test_distance_constraint_stabilization.c` can be built with a standard C compiler:

```bash
gcc -std=c99 -Iinclude src/chrono_body2d.c src/chrono_constraint2d.c \
    tests/test_distance_constraint_stabilization.c -lm \
    -o tests/test_distance_constraint_stabilization
./tests/test_distance_constraint_stabilization
```

The test connects a dynamic body to a static anchor and verifies that the solver
converges to the target rest length within a tolerance of 1 mm while reporting
intermediate constraint distances.
Success criteria: the output includes `Constraint stabilized within tolerance`
and the process exits with code 0.
Repro notes: run from the repo root, capture stdout if needed (`./tests/test_distance_constraint_stabilization > /tmp/constraint_log.txt`).
No artifacts are generated; only stdout is produced.

Additional regression tests are available via `make test` (see the top-level `Makefile`).  Notable examples:

- `tests/test_polygon_collision.c`: circle vs polygon and polygon vs polygon collision regression.
- `tests/test_polygon_mass_properties.c`: verifies mass / inertia output from `chrono_body2d_set_polygon_shape_with_density`.
- `tests/test_polygon_slope_friction.c`: block sliding on an inclined plane with friction.
- `tests/test_polygon_spin_collision.c`: counter-rotating convex polygons interacting through the manifold pipeline.
- `tests/test_capsule_edge_collision.c`: capsule/capsule and edge/circle interactions via the GJK/EPA backend.
- `tests/test_island_polygon_longrun.c`: combined constraint + polygon contact scenario executed through the island solver.
- `tests/test_prismatic_constraint.c`: slider joint with stroke limits and motor drive (limit and motor regression).
- `tests/test_prismatic_constraint_endurance.c`: 長時間のモータ切替えとリミット衝突を通じて PID チューニングとソフトリミットの安定性を検証します。
- `tests/test_distance_angle_constraint.c`: 距離と角度を同時に拘束する複合ジョイントの回帰テスト。
- `tests/test_distance_angle_endurance.c`: 距離＋角度拘束の耐久シナリオでパラメータ推奨値を確認し、最新のログ出力を検証します。
- `tests/test_distance_constraint_multi.c`: 距離拘束を複数本同時に解くケースで角速度連成とソフトネス設定が安定するか検証します。
- `tests/test_coupled_constraint.c`: 距離・角度比を持つカップリング拘束が期待どおり収束するか検証します。複数式の追加、ダイアグノスティクス、およびログ出力が想定どおり動作するか確認します。
- `tests/test_coupled_constraint_endurance.c`: 複合拘束の耐久・ステージ切替シナリオを長時間実行し、CSV (`data/coupled_constraint_endurance.csv`) に出力した力・トルク推移と診断フラグをチェックします。
- `tests/test_spring_constraint.c`: damped spring between an anchor and dynamic body.
- `tests/test_revolute_constraint.c`: pin joint maintaining a pivot under gravity.
- `tests/test_planar_constraint_longrun.c` / `tests/test_planar_constraint_endurance.c`: 2 軸スライダのモータ／リミット挙動を長時間シナリオで回帰し、位置・角度・エネルギーの安定性を確認します。

## Examples and Visualization

Two self-contained demos can be built with `make examples`:

- `examples/newton_cradle` – four-body Newton's cradle producing `data/newton_cradle.csv`.
- `examples/prismatic_slider` – slider joint with soft limits, velocity/位置モータ切り替えを含むデモ。`data/prismatic_slider.csv` に軸位置・リミット／モータ反力が記録されます。
- `examples/planar_constraint_demo` – 2 軸プラナー拘束のデモ。モータ目標の切替えとリミット衝突を CSV (`data/planar_constraint.csv`) へ記録し、`docs/planar_constraint_visualization.m` で可視化できます。
- `tests/test_planar_constraint.c` では 2 軸スライダの位置モータとリミット挙動を確認できます。
- ギア／リボルートのモータ挙動は `tests/test_gear_constraint.c` や `tests/test_revolute_constraint.c` のシナリオを参考にしてください。
- `tools/plot_coupled_constraint_endurance.py` – Python (matplotlib) ベースの可視化スクリプト。`data/coupled_constraint_endurance.csv` を読み込み、距離・角度・式別反力・条件数・診断フラグを 3 枚のグラフにまとめます。`python tools/plot_coupled_constraint_endurance.py --show` や `--output coupled.png` で利用できます。

Run an example and point the MATLAB helpers in `docs/` at the generated CSV to obtain plots and GIF animations.  For instance:

```matlab
% From the repo root or docs/ directory:
newton_cradle_visualization('../data/newton_cradle.csv', 'cradle_frames');
prismatic_slider_visualization('../data/prismatic_slider.csv', 'prismatic_frames');
planar_constraint_visualization('../data/planar_constraint.csv', 'planar_frames');
```

Both scripts emit trajectory/diagnostic plots and an animation built from PNG frames (stored under the output directory).

## Planar ジョイントの推奨パラメータと安定性チェック

- 位置モータを駆動する際は、先に `chrono_planar_constraint2d_enable_motor(axis, 1, 0.0, max_force)` で最大駆動力（推奨 15–18 N）を登録し、その後 `chrono_planar_constraint2d_set_motor_position_target(axis, target, 3.5, 1.2)` を呼び出すと PID 位置制御モードに切り替わります（周波数 = 3.5 Hz、減衰率 = 1.2）。
- Baumgarte 係数は `chrono_planar_constraint2d_set_baumgarte(constraint, 0.15)` を目安にすると、位置誤差収束と数値安定性のバランスが取りやすくなります。必要に応じて `set_slop(1e-4)`、`set_max_correction(0.08)` を併用してください。
- Y 軸リミットをソフトに拘束する場合は `chrono_planar_constraint2d_enable_limit(axis, 1, lower, upper)` に続けて `chrono_planar_constraint2d_set_limit_spring(axis, 55.0, 8.0)` を設定すると、急峻な衝突でもエネルギー発散を抑制できます。
- `tests/test_planar_constraint_longrun` は 6000 ステップ（dt = 0.01）を通してモータ目標の切り替え・姿勢フィード・リミット衝突を検証し、位置誤差（≦ 0.018 m）と運動エネルギー（≦ 6.9 J）が許容範囲に収まっていることを確認します。長時間安定性を調整したい場合はこの回帰テストをベースラインとして活用してください。
### Distance constraint tuning example

```c
ChronoDistanceConstraint2D_C distance;
chrono_distance_constraint2d_init(&distance, anchor, body, local_a, local_b, rest);
// Baumgarte bias handles positional drift; slop avoids jitter around rest length
chrono_distance_constraint2d_set_baumgarte(&distance, 0.35);
chrono_distance_constraint2d_set_slop(&distance, 5e-4);
chrono_distance_constraint2d_set_max_correction(&distance, 0.07);

// Linear softness adds a small compliance along the constraint axis (meters/Newton)
// Angular softness damps relative rotation effects (radians/Newton-meter)
chrono_distance_constraint2d_set_softness_linear(&distance, 0.01);
chrono_distance_constraint2d_set_softness_angular(&distance, 0.25);

// Optional spring/damper term pulls the bodies back toward rest_length
chrono_distance_constraint2d_set_spring(&distance, 30.0, 4.0);

// Record impulses for debugging/logging
double last_force = distance.last_spring_force;
```

`tests/test_distance_constraint_soft` と `tests/test_distance_constraint_multi` を使うと、線形／角度ソフトネスやスプリング係数のチューニング結果を素早く確認できます。

### Coupled constraint tuning example

```c
ChronoCoupledConstraint2D_C coupled;
chrono_coupled_constraint2d_init(&coupled,
                                 anchor,
                                 body,
                                 local_anchor,
                                 local_anchor,
                                 (double[2]){1.0, 0.0},
                                 rest_distance,
                                 rest_angle,
                                 1.0,
                                 0.5,
                                 0.0);

// Bias term for position correction and tolerance window to avoid chattering
chrono_coupled_constraint2d_set_baumgarte(&coupled, 0.35);
chrono_coupled_constraint2d_set_slop(&coupled, 5e-4);
chrono_coupled_constraint2d_set_max_correction(&coupled, 0.08);

// Treat distance and angle compliance independently
chrono_coupled_constraint2d_set_softness_distance(&coupled, 0.015);
chrono_coupled_constraint2d_set_softness_angle(&coupled, 0.03);

// Optional springs help track rapidly changing targets; damping in radians/s / N*m/s
chrono_coupled_constraint2d_set_distance_spring(&coupled, 40.0, 3.5);
chrono_coupled_constraint2d_set_angle_spring(&coupled, 18.0, 0.8);

// Inspect solver impulses/forces after chrono_constraint2d_batch_solve(...)
double distance_force = coupled.last_distance_force;  // Newton
double angle_torque = coupled.last_angle_force;       // N*m
```

距離リード側をやや硬く（ソフトネスは 0.01-0.02）、角度リード側は角速度揺らぎを抑えるため 0.02-0.04 を目安にすると安定しやすくなります。ターゲットをステージごとに切り替える場合は、距離スプリングの剛性を 30-45 N/m、角度側を 15-25 N*m/rad 程度に設定し、`tests/test_coupled_constraint` を回帰ベースラインとして Slop / Baumgarte / damping を調整してください。最新の `last_distance_force` / `last_angle_force` ログを CSV 化すると、`docs/planar_constraint_visualization.m` と同様のプロットに組み込めます。

### Coupled diagnostics workflow

1. **ログ収集**  
   - テストまたはシミュレーション内で `chrono_coupled_constraint2d_get_diagnostics(constraint)` の戻り値を取得し、`condition_number`, `rank`, `flags` を CSV へ追記します。耐久テスト（`tests/test_coupled_constraint_endurance.c`）は `../data/coupled_constraint_endurance.csv` を出力するサンプルです。  
   - 条件数警告を抑制し過ぎないよう、`ChronoCoupledConditionWarningPolicy_C` の `log_cooldown` を 0.25-0.5 秒程度に設定し、WARN ログを `chrono_log_warn`（今後統合予定）経由で収集します。

2. **可視化**  
   ```bash
   python tools/plot_coupled_constraint_endurance.py \
       data/coupled_constraint_endurance.csv \
       --output artifacts/coupled_endurance.png
   ```
   - グラフにはステップ毎の距離／角度誤差、式別反力（`last_distance_force_eq[i]` / `last_angle_force_eq[i]`）、条件数、診断フラグが描画されます。  
   - ステージ切替を追跡したい場合は CSV に `phase_id` 列を追加し、スクリプトの凡例で判別します。

3. **ボトルネック特定**  
   - 条件数が `1e6` を超える区間を抽出し、同タイムスタンプの WARN ログと付き合わせます。  
   - 反力ピークが 2 本目以降の式（`last_*_force_eq[1+]`）に集中する場合、追加式の `ratio_*` や `softness_*` を見直します。  
   - 自動ドロップが発生している場合は `diagnostics.rank` と `constraint.equation_active[]` が一致しているか確認し、必要に応じて `max_drop` を引き上げるか式の定義を調整します。

> 代表的なパラメータ例と ASCII 図は `docs/chrono_coupled_constraint_tutorial.md` を参照してください。
