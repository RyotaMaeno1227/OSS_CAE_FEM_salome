
# FEM4C 2D 2-Link Flexible Roadmap / Detailed ToDo
最終更新: 2026-03-06

## 0. この文書の目的

この文書は、FEM4C を **2D 技術検証ソルバー**として前進させるための実装指示書である。  
対象は以下に固定する。

- **対象モデル**: 2-link planar mechanism
- **柔軟体**: **両リンク flexible**
- **FEM**: **線形静解析 snapshot** を毎 step / 毎 coupling iteration で解く
- **full mesh 再アセンブル**: **必須**
- **MBD**: **explicit と implicit の両方**
- **implicit**: **Newmark-beta と HHT-alpha の両方**
- **比較対象**:
  - rigid 2-link は解析解と比較
  - flexible 2-link は RecurDyn / AdamsFlex と比較
- **言語**: C
- **参照思想**: Project Chrono の `System / Body / Constraint / Timestepper` 責務分割

本ドキュメントの粒度は、**Codex が 60〜90 分で自走できる**単位にそろえる。  
各タスクには、触るファイル、追加する関数、完了条件を明記する。

---

## 1. 今回の方針（更新版）

前回提案した「縮約モデル中心」ではなく、今回の PJ では以下を採用する。

### 1.1 採用する方式
**2D rigid-reference MBD + full-mesh static FEM snapshot coupling**

意味は次の通り。

1. MBD は各 link の **基準剛体運動** を解く
2. 各 flexible link は、その基準座標系にぶら下がる **局所 FE mesh** を持つ
3. 同一 `Δt` 内で、MBD の基準運動から interface 変位を作る
4. その interface 変位を使って **各リンクの full FE mesh を再アセンブルして静解析**
5. FE の interface reaction を MBD に戻す
6. explicit / implicit それぞれの step を更新する
7. 必要なら同一 `Δt` 内で coupling iteration を回す

### 1.2 2D での前提
- FEM は **小ひずみ線形弾性**
- 大きい剛体回転は MBD 側で持つ
- FE は **リンク局所座標系** で解く
- 各リンクの joint 近傍は **node set** で管理する
- joint 周辺 node set の変位は、**[ux, uy, theta] の rigid interpolation** で与える
- FE は **毎回 full mesh 再アセンブル**し、`K` と `f` を解く
- 初版では接触なし
- flexible case の解析解は基本ないので、解析解比較は rigid case のみ

---

## 2. 目標状態（Definition of Done）

以下が全部そろったら、この PJ の 2D マイルストーン達成とする。

### M0: Build recovery
- `make -j` が通る
- `--mode=fem` と `--mode=mbd` が起動する

### M1: rigid 2-link MBD
- gravity 下の rigid 2-link を積分できる
- explicit integrator が動く
- implicit Newmark-beta が動く
- implicit HHT-alpha が動く
- rigid 2-link は解析解または高精度 ODE 参照解と比較できる

### M2: 1-link flexible debug pass
- link1 だけ flexible にして coupling loop が回る
- full mesh 再アセンブル回数がログで確認できる
- 柔軟体を高剛性化すると rigid 解に近づく

### M3: 2-link flexible
- 両リンク flexible で coupled 実行できる
- full mesh 再アセンブルが各 link / 各 iteration で走る
- explicit / Newmark / HHT の 3 系統で同一モデルを実行できる

### M4: 比較
- rigid: 解析解比較
- flexible: RecurDyn / AdamsFlex と
  - joint angle
  - tip position
  - root reaction
  - link tip displacement
  を比較できる

---

## 3. 実装アーキテクチャ

## 3.1 MBD 層
新設する中核モジュール:

- `src/mbd/body2d.*`
- `src/mbd/forces2d.*`
- `src/mbd/system2d.*`
- `src/mbd/assembler2d.*`
- `src/mbd/integrator_explicit2d.*`
- `src/mbd/integrator_newmark2d.*`
- `src/mbd/integrator_hht2d.*`
- `src/mbd/projection2d.*`
- `src/mbd/output2d.*`

責務:
- body 状態 (`x,y,theta,vx,vy,omega,ax,ay,alpha`)
- mass / inertia
- gravity / user force / user torque
- constraint residual / Jacobian
- KKT assemble
- explicit / implicit step
- state history 出力

## 3.2 Flexible body 層
新設する中核モジュール:

- `src/coupled/fem_model_copy.*`
- `src/coupled/flex_body2d.*`
- `src/coupled/flex_nodeset.*`
- `src/coupled/flex_bc2d.*`
- `src/coupled/flex_solver2d.*`
- `src/coupled/flex_reaction2d.*`
- `src/coupled/flex_snapshot2d.*`

責務:
- 1 つの flexible link が持つ FE model の深いコピー
- root / tip node set の保持
- node set に対する rigid interpolation BC
- full mesh 再アセンブル
- static FE solve
- interface reaction 回収
- deformed shape 復元・出力

## 3.3 Coupled 層
新設する中核モジュール:

- `src/coupled/case2d.*`
- `src/coupled/coupling_map2d.*`
- `src/coupled/coupled_step_explicit2d.*`
- `src/coupled/coupled_step_implicit2d.*`
- `src/coupled/coupled_run2d.*`

責務:
- 2つの flexible link と MBD system の束ね
- 同一 `Δt` 内の coupling iteration
- interface displacement の計算
- FE reaction → MBD generalized force 変換
- residual 管理
- step accept / fail 判定

---

## 4. 同一 Δt 内の計算順

## 4.1 explicit step
初版 explicit は **semi-implicit Euler** を採用する。  
理由: 実装が比較的軽く、拘束付き MBD の最初の explicit として扱いやすい。

1. `q_n, v_n` から body pose を更新
2. constraint Jacobian `G(q_n)` を作る
3. 外力 `Q_ext` を assemble
4. 前回の flexible reaction `Q_flex_prev` を一旦使う
5. acceleration-level KKT から `a_n, lambda_n` を解く
6. semi-implicit Euler で `v_{n+1}, q_{n+1}` を更新
7. 新しい `q_{n+1}` から各 link の interface displacement を計算
8. 両リンク FE を **full mesh 再アセンブル**して static solve
9. 新しい flexible reaction `Q_flex_new` を得る
10. `||Q_flex_new - Q_flex_prev||` が大きければ 1〜2 回だけ fixed-point 補正
11. step accept
12. FE snapshot 出力

## 4.2 implicit step (Newmark-beta / HHT-alpha)
1. predictor で `q^(0), v^(0), a^(0)` を作る
2. Newton / modified Newton loop 開始
3. 現在の `q^(k)` から constraint residual / Jacobian を作る
4. 各 link の interface displacement を計算
5. 両リンク FE を **full mesh 再アセンブル**して static solve
6. flexible reaction `Q_flex^(k)` を assemble
7. `R(q,v,a,lambda)=0` を組む
8. effective tangent / KKT を組む
9. 線形方程式を解いて `Δq, Δlambda` を得る
10. `q,v,a,lambda` を更新
11. 残差が収束したら step accept
12. FE snapshot 出力

---

## 5. validation 方針

| 段階 | ケース | 何を見るか |
|---|---|---|
| V1 | rigid 2-link / explicit | 角度・角速度・エネルギー |
| V2 | rigid 2-link / Newmark | 角度・拘束残差 |
| V3 | rigid 2-link / HHT | 数値減衰・拘束残差 |
| V4 | 1-link flexible | link tip displacement |
| V5 | 2-link flexible (高剛性) | rigid 極限へ戻るか |
| V6 | 2-link flexible (実剛性) | RecurDyn/AdamsFlex 比較 |

---

## 6. チーム負荷の割り当て

| チーム | 主担当 | タスク数 |
|---|---|---:|
| PM | スコープ / 受入 / 比較条件固定 | 6 |
| A | Body / Forces / Explicit / Kinematics | 9 |
| B | Constraint / KKT / Newmark / HHT | 9 |
| C | FEM API 化 / full reassembly / nodeset / output | 9 |
| D | Flexible body wrapper / reaction / snapshot / 2-link flex | 9 |
| E | System orchestration / parser / regression / compare | 9 |

**A〜E の開発タスク数は 9 件で統一**する。  
1タスク 60〜90 分なので、各チームの一次負荷はほぼ均一になる。

---

## 7. 開発順序（厳守）

1. **C-01** で build recovery
2. **A-01〜A-03**, **B-01〜B-03**, **E-01** で rigid MBD 骨格
3. **A-04〜A-06**, **B-04〜B-08** で explicit / Newmark / HHT
4. **C-02〜C-06** で FEM model API + full reassembly
5. **D-01〜D-05** で 1-link flexible coupling
6. **D-06〜D-09**, **E-04〜E-09** で 2-link flexible / comparison
7. PM は各 milestone 完了時に acceptance を更新

---

## 8. Detailed ToDo

---
## PM Team

### PM-01 [60-75 min]
**目的**: 2D PJ の必須要件を凍結する  
**新規/更新ファイル**:
- `docs/04_2d_coupled_scope.md`

**実装内容**
1. 文書タイトルを `2D coupled scope for 2-link flexible validation` にする
2. 以下を明記する
   - 両リンク flexible
   - full mesh 再アセンブル必須
   - MBD explicit / Newmark / HHT 必須
   - rigid case は解析解比較
   - flexible case は RecurDyn / AdamsFlex 比較
3. 今回やらないものを明記する
   - 接触
   - 摩擦
   - 非線形材料
   - 3D

**完了条件**
- 上記 5 項目が 1 ページで読める
- `README.md` からリンクが貼られている

---

### PM-02 [60-75 min]
**目的**: モジュール責務を固定する  
**新規/更新ファイル**:
- `docs/05_module_ownership_2d.md`

**実装内容**
1. `mbd/`, `coupled/`, `analysis/`, `solver/` の責務を表で書く
2. `runner.c` から追い出す責務を列挙する
3. `runner.c` に残す責務を列挙する
4. 各チームの担当ファイル一覧を末尾に書く

**完了条件**
- `runner.c` の役割が「入口と分岐」へ縮退する方針になっている

---

### PM-03 [60-90 min]
**目的**: 受入条件の数値指標を固定する  
**新規/更新ファイル**:
- `docs/06_acceptance_matrix_2d.md`

**実装内容**
1. rigid explicit / Newmark / HHT の 3 行を作る
2. flexible 1-link / flexible 2-link の 2 行を追加する
3. 各行に以下を列挙する
   - 入力ファイル
   - 比較対象
   - 許容誤差
   - 出力 CSV 名
4. `constraint residual`, `joint angle`, `tip displacement` を必須列にする

**完了条件**
- 5 ケース以上の受入表が完成している

---

### PM-04 [60-75 min]
**目的**: 入力仕様を固定する  
**新規/更新ファイル**:
- `docs/07_input_spec_coupled_2d.md`

**実装内容**
1. 以下の新規 directive を定義する
   - `MBD_BODY_DYN`
   - `MBD_GRAVITY`
   - `MBD_FORCE`
   - `COUPLED_FLEX_BODY`
   - `COUPLED_FLEX_ROOT_SET`
   - `COUPLED_FLEX_TIP_SET`
2. 各 directive の引数順を 1 行例付きで書く
3. body id と fem file path の対応を明記する

**完了条件**
- Codex が parser 実装時に迷わない粒度になっている

---

### PM-05 [60-75 min]
**目的**: マージ順を固定する  
**新規/更新ファイル**:
- `docs/08_merge_order_2d.md`

**実装内容**
1. `phase-1 build` から `phase-6 validation` まで章立てする
2. 各 phase に task ID を並べる
3. 同時マージしてよい task と、依存で待つ task を分ける

**完了条件**
- 依存順が一本道で見える

---

### PM-06 [60-90 min]
**目的**: 比較データの持ち方を固定する  
**新規/更新ファイル**:
- `docs/09_compare_schema_2d.md`

**実装内容**
1. RecurDyn / AdamsFlex / FEM4C の比較 CSV schema を定義する
2. 列名を固定する
   - `time`
   - `theta1`
   - `theta2`
   - `omega1`
   - `omega2`
   - `tip1_x`
   - `tip1_y`
   - `tip2_x`
   - `tip2_y`
   - `root_reaction_x`
   - `root_reaction_y`
3. 単位も明記する

**完了条件**
- 比較スクリプトがこの schema だけ読めばよい形になっている

---
## Team A (Body / Forces / Explicit / Kinematics)

### A-01 [60-75 min]
**目的**: 剛体 body の実体を作る  
**新規/更新ファイル**:
- `src/mbd/body2d.h`
- `src/mbd/body2d.c`

**実装内容**
1. `mbd_body2d_t` を新設する
2. 保持項目:
   - `id`
   - `mass`
   - `inertia`
   - `q[3]`
   - `v[3]`
   - `a[3]`
   - `force[3]`
   - `is_ground`
3. `mbd_body2d_zero()`
4. `mbd_body2d_init_dyn()`
5. `mbd_body2d_clear_force()`

**完了条件**
- 単体で body 初期化できる
- `runner.c` から切り出して使える

---

### A-02 [60-90 min]
**目的**: 動力学 input を parse できるようにする  
**新規/更新ファイル**:
- `src/io/input.c`
- `src/mbd/body2d.h`

**実装内容**
1. `MBD_BODY_DYN id mass inertia x y theta vx vy omega` を読み込む
2. `MBD_GRAVITY gx gy` を読み込む
3. `MBD_FORCE body_id fx fy mz` を読み込む
4. 未指定時の既定値を入れる
5. 既存 `MBD_BODY` との後方互換を保つ

**完了条件**
- 新旧両 directive で parse が通る
- 異常入力時に明確なエラーメッセージが出る

---

### A-03 [60-75 min]
**目的**: 外力の assemble API を作る  
**新規/更新ファイル**:
- `src/mbd/forces2d.h`
- `src/mbd/forces2d.c`

**実装内容**
1. `mbd_forces2d_apply_gravity()`
2. `mbd_forces2d_apply_user_loads()`
3. `mbd_forces2d_add_generalized_force()`
4. `mbd_forces2d_build_rhs_vector()`
5. `force[3]` を body 配列から RHS へ詰める

**完了条件**
- gravity と body force を同じ経路で足し込める

---

### A-04 [60-75 min]
**目的**: marker / interface の幾何変換を作る  
**新規/更新ファイル**:
- `src/mbd/kinematics2d.h`
- `src/mbd/kinematics2d.c`

**実装内容**
1. local point → world point 変換
2. world point の `∂x/∂q` Jacobian
3. local vector の回転変換
4. `theta` に対する微分
5. unit test 相当の簡単な self-check 関数

**完了条件**
- revolute joint の anchor 計算と flex interface 計算で再利用できる

---

### A-05 [60-90 min]
**目的**: explicit integrator の器を作る  
**新規/更新ファイル**:
- `src/mbd/integrator_explicit2d.h`
- `src/mbd/integrator_explicit2d.c`

**実装内容**
1. 初版 explicit を `semi_implicit_euler` で実装する
2. 関数:
   - `mbd_explicit2d_predict()`
   - `mbd_explicit2d_update_velocity()`
   - `mbd_explicit2d_update_position()`
3. 位置更新は `q_{n+1}=q_n + dt*v_{n+1}` を採用
4. 回転 DOF も同様に更新する

**完了条件**
- unconstrained single body の重力落下が動く

---

### A-06 [60-90 min]
**目的**: explicit path に body state 更新を接続する  
**新規/更新ファイル**:
- `src/mbd/system2d.c`
- `src/mbd/integrator_explicit2d.c`

**実装内容**
1. `mbd_system2d_do_explicit_step()` を作る
2. 流れ:
   - clear force
   - gravity / user load
   - acceleration solve 呼び出し
   - velocity update
   - position update
3. 現時点では flexible reaction はフックだけ置く

**完了条件**
- rigid 2-link explicit の step 関数が呼べる

---

### A-07 [60-75 min]
**目的**: 出力を整える  
**新規/更新ファイル**:
- `src/mbd/output2d.h`
- `src/mbd/output2d.c`

**実装内容**
1. 時系列 CSV writer を作る
2. 1 行に以下を書く
   - step
   - time
   - body_id
   - x
   - y
   - theta
   - vx
   - vy
   - omega
   - ax
   - ay
   - alpha
3. header を固定する

**完了条件**
- rigid case の履歴が CSV に出る

---

### A-08 [60-90 min]
**目的**: flexible reaction を MBD に戻す口を作る  
**新規/更新ファイル**:
- `src/mbd/forces2d.c`
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`

**実装内容**
1. `mbd_system2d_add_flexible_generalized_force(body_id, qflex[3])`
2. 1 step の RHS 組立時に user force に加算する
3. step 開始時に flexible force を clear する

**完了条件**
- coupling 側から 1 body あたり 3DOF の generalized force を戻せる

---

### A-09 [60-75 min]
**目的**: link-local reference frame を保持する  
**新規/更新ファイル**:
- `src/mbd/body2d.h`
- `src/mbd/body2d.c`

**実装内容**
1. 各 body に local frame 原点と初期姿勢を保持する項目を追加
2. `mbd_body2d_set_reference_frame()` を作る
3. flexible body 側が link local frame を参照できる accessor を作る

**完了条件**
- FE 側が current rigid pose を body から取得できる

---
## Team B (Constraint / KKT / Newmark / HHT)

### B-01 [60-75 min]
**目的**: constraint data を system 側へ寄せる  
**新規/更新ファイル**:
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`
- `src/mbd/constraint2d.h`

**実装内容**
1. `mbd_system2d_t` を新設する
2. 保持項目:
   - body array
   - constraint array
   - gravity
   - time control
3. `runner.c` が body/constraint を system へ渡す経路を作る

**完了条件**
- runtime state が `runner.c` ローカル配列に残らない

---

### B-02 [60-90 min]
**目的**: KKT assembler を実装する  
**新規/更新ファイル**:
- `src/mbd/assembler2d.h`
- `src/mbd/assembler2d.c`

**実装内容**
1. `[M G^T; G 0]` の dense KKT を組む
2. body ごとの mass / inertia を対角に詰める
3. constraint Jacobian を body index ごとに詰める
4. RHS ベクトルも同じ順序で作る

**完了条件**
- rigid 2-link の KKT 行列と RHS が数値で出せる

---

### B-03 [60-75 min]
**目的**: 小規模 dense solver を用意する  
**新規/更新ファイル**:
- `src/mbd/linear_solver_dense.h`
- `src/mbd/linear_solver_dense.c`

**実装内容**
1. 部分 pivot 付き Gaussian elimination を実装
2. 入力:
   - dense matrix
   - rhs
3. 出力:
   - solution
4. singular 近傍ならエラーを返す

**完了条件**
- KKT の小規模系を単体で解ける

---

### B-04 [60-90 min]
**目的**: acceleration-level constraint RHS を作る  
**新規/更新ファイル**:
- `src/mbd/constraint2d.c`
- `src/mbd/assembler2d.c`

**実装内容**
1. `Phi(q)` residual を計算
2. `G(q)` を計算
3. 初版は Baumgarte stabilization を入れる
4. `gamma_c = -2*alpha*Phi_dot - beta^2*Phi` 型の RHS を作る
5. パラメータは固定値でよい

**完了条件**
- explicit / implicit 共通で constraint RHS を使える

---

### B-05 [60-90 min]
**目的**: Newmark-beta の器を作る  
**新規/更新ファイル**:
- `src/mbd/integrator_newmark2d.h`
- `src/mbd/integrator_newmark2d.c`

**実装内容**
1. predictor 関数を作る
2. effective acceleration/velocity 更新式を実装する
3. `beta`, `gamma` は引数または system から読む
4. state update 関数を 1 つにまとめる

**完了条件**
- unconstrained single body で Newmark 更新が動く

---

### B-06 [60-90 min]
**目的**: Newmark-beta implicit step を完成させる  
**新規/更新ファイル**:
- `src/mbd/system2d.c`
- `src/mbd/integrator_newmark2d.c`

**実装内容**
1. `mbd_system2d_do_newmark_step()` を作る
2. predictor
3. residual assemble
4. KKT solve
5. corrector
6. residual check
7. 最大反復回数超過時のエラー

**完了条件**
- rigid 2-link の Newmark 計算が 1 run 完了する

---

### B-07 [60-90 min]
**目的**: HHT-alpha の係数計算を実装する  
**新規/更新ファイル**:
- `src/mbd/integrator_hht2d.h`
- `src/mbd/integrator_hht2d.c`

**実装内容**
1. `alpha` を入力で受ける
2. HHT の effective residual 側に `alpha` を反映する
3. `alpha ∈ [-1/3,0]` をチェックする
4. modified Newton 用の関数分離をする

**完了条件**
- HHT 単独モジュールが compile する

---

### B-08 [60-90 min]
**目的**: HHT-alpha step を完成させる  
**新規/更新ファイル**:
- `src/mbd/system2d.c`
- `src/mbd/integrator_hht2d.c`

**実装内容**
1. `mbd_system2d_do_hht_step()` を作る
2. predictor
3. HHT residual assemble
4. modified Newton loop
5. convergence 判定
6. state update

**完了条件**
- rigid 2-link の HHT 計算が 1 run 完了する

---

### B-09 [60-75 min]
**目的**: constraint projection を入れる  
**新規/更新ファイル**:
- `src/mbd/projection2d.h`
- `src/mbd/projection2d.c`
- `src/mbd/system2d.c`

**実装内容**
1. step 後に位置レベル residual を計算
2. 小さい correction KKT を解いて `q` を補正
3. explicit / implicit 共通で呼べるようにする

**完了条件**
- 長時間積分で constraint drift が減る

---
## Team C (FEM API / full reassembly / nodeset / output)

### C-01 [60-75 min]
**目的**: build を復旧する  
**新規/更新ファイル**:
- `src/elements/t6/t6_element.c`
- 必要なら `src/elements/element_base.h`

**実装内容**
1. `t6_register()` の `stiffness` 関数ポインタ型不一致を解消する
2. `make -j` が最後まで通るようにする
3. warning は増やさない

**完了条件**
- `make -j2` 成功

---

### C-02 [60-90 min]
**目的**: globals ベースの FE model を深いコピー可能にする  
**新規/更新ファイル**:
- `src/coupled/fem_model_copy.h`
- `src/coupled/fem_model_copy.c`

**実装内容**
1. `fem_model2d_t` を新設する
2. 保持項目:
   - nodes
   - elements
   - materials
   - boundary conditions
   - loads
   - analysis metadata
3. `fem_model2d_from_globals()`
4. `fem_model2d_free()`

**完了条件**
- 1 つの FE input を globals から独立保持できる

---

### C-03 [60-90 min]
**目的**: model-centric assembly API を作る  
**新規/更新ファイル**:
- `src/coupled/fem_model_copy.h`
- `src/coupled/flex_solver2d.h`
- `src/coupled/flex_solver2d.c`

**実装内容**
1. 既存 `assembly_*` を直接書き換えず、ラッパを作る
2. `flex_solver2d_prepare_model(fem_model2d_t*)`
3. `flex_solver2d_assemble_full_mesh(fem_model2d_t*)`
4. globals に model を一時注入して existing FEM kernel を呼ぶ最小経路でもよい

**完了条件**
- 1 model 単体で full assembly が再利用できる

---

### C-04 [60-90 min]
**目的**: Dirichlet BC を runtime で差し替えられるようにする  
**新規/更新ファイル**:
- `src/coupled/flex_bc2d.h`
- `src/coupled/flex_bc2d.c`

**実装内容**
1. node id / dof / value を保持する runtime BC 配列を作る
2. 既存境界条件に追加上書きできるようにする
3. root / tip nodeset に与える BC をここで表現する

**完了条件**
- step ごとに BC を差し替えて FE solve できる

---

### C-05 [60-90 min]
**目的**: full mesh 再アセンブルを明示化する  
**新規/更新ファイル**:
- `src/coupled/flex_solver2d.c`

**実装内容**
1. `flex_solver2d_reassemble_and_solve()` を作る
2. 処理順:
   - clear previous system arrays
   - assemble full stiffness
   - assemble full force
   - apply runtime BC
   - solve
3. `full_reassembly_count++` を入れる

**完了条件**
- 1 呼び出しで必ず full reassembly が走る

---

### C-06 [60-75 min]
**目的**: full reassembly のログを出す  
**新規/更新ファイル**:
- `src/coupled/flex_solver2d.c`
- `src/io/output.c` または新規 `src/coupled/flex_log2d.c`

**実装内容**
1. link ごとの reassembly count
2. step ごとの solve count
3. coupling iteration index
4. 結果を CSV または log に書く

**完了条件**
- 実行後に「各 link が何回再アセンブルされたか」が見える

---

### C-07 [60-90 min]
**目的**: nodeset データを扱えるようにする  
**新規/更新ファイル**:
- `src/coupled/flex_nodeset.h`
- `src/coupled/flex_nodeset.c`

**実装内容**
1. `node_set_t` を作る
2. root / tip node id 配列を保持する
3. `node_set_contains()`
4. `node_set_center()`
5. `node_set_local_coordinates()` を作る

**完了条件**
- root/tip interface を node set で管理できる

---

### C-08 [60-90 min]
**目的**: inertial equivalent load の受け口を作る  
**新規/更新ファイル**:
- `src/coupled/flex_solver2d.c`
- `src/solver/assembly.c` も必要なら更新

**実装内容**
1. translational acceleration による body force の入力口を作る
2. 角加速度・遠心項は初版では TODO でもよいが、関数シグネチャは切っておく
3. `flex_solver2d_set_inertial_loads()` を作る

**完了条件**
- coupled 側から link ごとの擬似静的 body force を渡せる

---

### C-09 [60-90 min]
**目的**: 変形形状の snapshot 出力を作る  
**新規/更新ファイル**:
- `src/coupled/flex_snapshot2d.h`
- `src/coupled/flex_snapshot2d.c`

**実装内容**
1. local FE displacement を world 座標へ写す
2. deformed node 座標を CSV または VTK へ出す
3. body_id, step, iteration をファイル名に入れる

**完了条件**
- 各 link の変形形状を時系列で見られる

---
## Team D (Flexible body wrapper / reaction / 2-link flex)

### D-01 [60-75 min]
**目的**: flexible body wrapper を作る  
**新規/更新ファイル**:
- `src/coupled/flex_body2d.h`
- `src/coupled/flex_body2d.c`

**実装内容**
1. `flex_body2d_t` を定義する
2. 保持項目:
   - `body_id`
   - `fem_model2d_t model`
   - `node_set_t root_set`
   - `node_set_t tip_set`
   - `u_local`
   - `reaction_root[3]`
   - `reaction_tip[3]`
3. init / free を作る

**完了条件**
- 1 flexible link を 1 struct で保持できる

---

### D-02 [60-90 min]
**目的**: interface の rigid interpolation を実装する  
**新規/更新ファイル**:
- `src/coupled/flex_bc2d.c`
- `src/coupled/flex_body2d.c`

**実装内容**
1. node set 内の各 node について local 座標 `(xk, yk)` を持つ
2. `[ux, uy, theta]` から各 node の `ux_k, uy_k` を計算する
3. root / tip それぞれに適用する関数を作る

**完了条件**
- interface marker 変位を node BC に展開できる

---

### D-03 [60-90 min]
**目的**: 1 flexible link の static solve ラッパを作る  
**新規/更新ファイル**:
- `src/coupled/flex_body2d.c`
- `src/coupled/flex_solver2d.c`

**実装内容**
1. `flex_body2d_solve_snapshot()` を作る
2. 入力:
   - root marker displacement
   - tip marker displacement
   - inertial load
3. 処理:
   - runtime BC 作成
   - full mesh 再アセンブル
   - solve
4. 出力:
   - nodal displacement
   - root/tip reaction

**完了条件**
- 1 link 単体で snapshot solve できる

---

### D-04 [60-90 min]
**目的**: FE reaction を generalized force に変換する  
**新規/更新ファイル**:
- `src/coupled/flex_reaction2d.h`
- `src/coupled/flex_reaction2d.c`

**実装内容**
1. root/tip node reaction から合力・合モーメントを計算する
2. body generalized force `[Fx, Fy, Mz]` に変換する
3. virtual work 一致で符号を整理する
4. root body / tip body のどちらへ返すかを明確化する

**完了条件**
- MBD 側へ body generalized force を返せる

---

### D-05 [60-90 min]
**目的**: 1-link flexible coupling を成立させる  
**新規/更新ファイル**:
- `src/coupled/coupled_step_explicit2d.c`
- `src/coupled/coupled_step_implicit2d.c`

**実装内容**
1. まず link1 だけ flexible に固定
2. MBD body pose → root/tip marker displacement を作る
3. FE snapshot solve
4. generalized reaction を MBD に返す
5. 1 step だけ動かす

**完了条件**
- 1-link flexible の step trace が出る

---

### D-06 [60-90 min]
**目的**: 2-link flexible に拡張する  
**新規/更新ファイル**:
- `src/coupled/case2d.h`
- `src/coupled/case2d.c`
- `src/coupled/coupled_run2d.c`

**実装内容**
1. `flex_body[2]` を保持できる coupled case を作る
2. link1 / link2 を別 mesh で保持できるようにする
3. 両方を solve し、reaction を合算する

**完了条件**
- 両リンク flexible の coupled case が初期化できる

---

### D-07 [60-90 min]
**目的**: coupling residual と iteration 管理を入れる  
**新規/更新ファイル**:
- `src/coupled/coupled_run2d.c`

**実装内容**
1. residual を
   - `||Qflex_new - Qflex_old||`
   - `||u_tip_new - u_tip_old||`
   のいずれかで定義する
2. step 内反復回数上限を設ける
3. 収束 / 非収束ログを出す

**完了条件**
- same-step iteration が明示的に回る

---

### D-08 [60-75 min]
**目的**: snapshot 出力を coupled に接続する  
**新規/更新ファイル**:
- `src/coupled/coupled_run2d.c`
- `src/coupled/flex_snapshot2d.c`

**実装内容**
1. step accept 後だけ snapshot を出す
2. link1 / link2 で別ファイルにする
3. step と time をファイル名へ入れる

**完了条件**
- 両リンクの変形形状が保存される

---

### D-09 [60-75 min]
**目的**: 高剛性 limit test を作る  
**新規/更新ファイル**:
- `examples/coupled_2link_flex_rigid_limit_link1.dat`
- `examples/coupled_2link_flex_rigid_limit_link2.dat`
- 必要なら script

**実装内容**
1. `E` を十分大きくした flexible case を作る
2. rigid 2-link と比較する差分 CSV を出す
3. `theta1, theta2, tip2_x, tip2_y` を比較する

**完了条件**
- flexible → rigid limit が確認できる

---
## Team E (System orchestration / parser / regression / compare)

### E-01 [60-90 min]
**目的**: `runner.c` から system 実行を切り出す  
**新規/更新ファイル**:
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`
- `src/analysis/runner.c`

**実装内容**
1. `mbd_analysis_minimal()` の中身を `mbd_system2d_run()` へ寄せる
2. `runner.c` は
   - parse
   - mode 分岐
   - run 呼び出し
   のみ残す
3. coupled も同様に `coupled_run2d()` へ寄せる準備をする

**完了条件**
- `runner.c` が入口に近い役割だけになる

---

### E-02 [60-90 min]
**目的**: coupled input directive を実装する  
**新規/更新ファイル**:
- `src/io/input.c`
- `src/coupled/case2d.h`
- `src/coupled/case2d.c`

**実装内容**
1. 以下を parse する
   - `COUPLED_FLEX_BODY body_id fem_input_path`
   - `COUPLED_FLEX_ROOT_SET body_id n id1 id2 ...`
   - `COUPLED_FLEX_TIP_SET body_id n id1 id2 ...`
2. 2つの flexible body を保持できる
3. path 文字列の保持と検証を行う

**完了条件**
- coupled case の input が読める

---

### E-03 [60-90 min]
**目的**: rigid 2-link benchmark input を作る  
**新規/更新ファイル**:
- `examples/mbd_2link_rigid_dyn.dat`

**実装内容**
1. `MBD_BODY_DYN`
2. `MBD_GRAVITY`
3. revolute constraints
4. 初期姿勢 / 初期速度
を含む rigid 2-link 入力を作る

**完了条件**
- explicit / Newmark / HHT の3系統で同じ input を回せる

---

### E-04 [60-90 min]
**目的**: explicit coupled run を作る  
**新規/更新ファイル**:
- `src/coupled/coupled_step_explicit2d.h`
- `src/coupled/coupled_step_explicit2d.c`
- `src/coupled/coupled_run2d.c`

**実装内容**
1. explicit step の orchestration を実装する
2. 2-link flexible の順序は
   - explicit MBD step
   - flex1 solve
   - flex2 solve
   - reaction map
   - optional fixed-point correction
3. step history を保存する

**完了条件**
- explicit coupled run が最後まで走る

---

### E-05 [60-90 min]
**目的**: implicit coupled run (Newmark) を作る  
**新規/更新ファイル**:
- `src/coupled/coupled_step_implicit2d.h`
- `src/coupled/coupled_step_implicit2d.c`
- `src/coupled/coupled_run2d.c`

**実装内容**
1. Newmark path の same-step iteration を実装する
2. 反復ごとに flex1 / flex2 を再 solve する
3. residual が収束したら accept する

**完了条件**
- Newmark coupled run が最後まで走る

---

### E-06 [60-90 min]
**目的**: implicit coupled run (HHT) を作る  
**新規/更新ファイル**:
- `src/coupled/coupled_step_implicit2d.c`
- `src/coupled/coupled_run2d.c`

**実装内容**
1. HHT path の same-step iteration を実装する
2. HHT residual と flex reaction を同じ loop に入れる
3. `alpha` をログ出力する

**完了条件**
- HHT coupled run が最後まで走る

---

### E-07 [60-90 min]
**目的**: 2-link flexible input を作る  
**新規/更新ファイル**:
- `examples/coupled_2link_flex_master.dat`
- `examples/flex_link1_q4.dat`
- `examples/flex_link2_q4.dat`

**実装内容**
1. master input に MBD と coupled directive を書く
2. link1 FE input を作る
3. link2 FE input を作る
4. root / tip nodeset の例を書く

**完了条件**
- 両リンク flexible の最小入力一式がそろう

---

### E-08 [60-90 min]
**目的**: 比較スクリプトを作る  
**新規/更新ファイル**:
- `scripts/compare_2link_rigid_analytic.py`
- `scripts/compare_2link_flex_reference.py`

**実装内容**
1. rigid case:
   - FEM4C CSV と解析参照を重ねる
2. flexible case:
   - FEM4C CSV と RecurDyn/AdamsFlex CSV を重ねる
3. RMS 誤差と最大誤差を出す
4. PNG を保存する

**完了条件**
- 比較結果が数値と図で出る

---

### E-09 [60-90 min]
**目的**: end-to-end acceptance script を作る  
**新規/更新ファイル**:
- `scripts/run_2d_coupled_acceptance.sh`

**実装内容**
1. build
2. rigid explicit / Newmark / HHT
3. flexible explicit / Newmark / HHT
4. compare scripts
5. pass/fail summary
を 1 本で回す

**完了条件**
- 1 コマンドで 2D acceptance が実行できる

---

## 9. マイルストーンと task ID 対応

| Milestone | 必須 task |
|---|---|
| M0 build | C-01 |
| M1 rigid MBD | A-01, A-02, A-03, A-04, A-05, A-06, B-01, B-02, B-03, B-04, B-05, B-06, B-07, B-08, B-09, E-01, E-03 |
| M2 1-link flex | C-02, C-03, C-04, C-05, C-06, C-07, D-01, D-02, D-03, D-04, D-05 |
| M3 2-link flex | A-08, A-09, C-08, C-09, D-06, D-07, D-08, D-09, E-02, E-04, E-05, E-06, E-07 |
| M4 compare | PM-01〜PM-06, E-08, E-09 |

---

## 10. 実装上の注意

1. **FEM は局所座標系で解く**
   - global へ直接 large rotation を入れない
2. **full mesh 再アセンブルは省略しない**
   - `flex_solver2d_reassemble_and_solve()` から必ず入る
3. **explicit と implicit で coupled orchestration を分ける**
   - 同じ関数に詰め込まない
4. **1-link flexible は中間 debug 用**
   - 最終 target は両リンク flexible
5. **rigid 解析解比較と flexible 比較を混同しない**
   - rigid: 解析
   - flexible: RecurDyn / AdamsFlex
6. **HHT-alpha は must**
   - Newmark が動いたら終わりではない

---

## 11. 最初の 2 週間で終わらせる順

### Week 1
- PM-01, PM-02
- C-01
- A-01, A-02, A-03
- B-01, B-02, B-03, B-04
- E-01, E-03

### Week 2
- A-04, A-05, A-06, A-07
- B-05, B-06, B-07, B-08, B-09
- PM-03
- rigid acceptance 実行

---

## 12. この PJ での結論

今回の 2D PJ で採るべきコアは次の 1 文で固定する。

> **両リンク flexible、MBD は explicit / Newmark-beta / HHT-alpha を持ち、各 coupling iteration で各 flexible link の full FE mesh を再アセンブルして static snapshot を解き、その reaction を MBD へ返す。**

この 1 文から外れる実装は、今回の roadmap では優先度を下げる。
