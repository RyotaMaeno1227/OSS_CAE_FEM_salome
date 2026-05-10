# MBD Learning Map

最終更新: 2026-03-08

## 1. 目的

この文書は、FEM4C の 2D MBD 実装を題材にして、
将来的に Project Chrono を読めるだけの基礎を身につけるための
学習順を固定する。

目標は次の 3 点である。

1. 2D rigid MBD の最小ソルバーを自力で書ける
2. 拘束式、ヤコビアン、KKT、時間積分をコードで説明できる
3. 2D の理解を足場に 3D / Project Chrono へ移行できる

## 2. 学習順

1. 2D rigid body の状態量
2. generalized coordinate / generalized force
3. constraint / Jacobian
4. KKT / Lagrange multipliers
5. explicit integration
6. Newmark-beta
7. HHT-alpha
8. C 実装パターン
9. FEM snapshot coupling
10. Project Chrono concept mapping
11. 2D から 3D への差分

## 3. 初手 3 文書

Run 3 までに最低限そろえる学習 docs は次の 3 本とする。

1. `docs/MBD_00_learning_map.md`
2. `docs/MBD_01_rigid_body_2d_basics.md`
3. `docs/MBD_02_constraints_and_jacobians.md`

この 3 本では次だけを先に固定する。

- 学習順
- 理論 -> code file 対応
- 1問1答の出題範囲
- Project Chrono への橋渡し

## 4. 理論 -> code file 対応

### 入口

- `FEM4C/src/mbd/kernel/body2d.c`
- `FEM4C/src/mbd/kernel/body2d.h`
- `FEM4C/src/mbd/kernel/constraint2d.c`
- `FEM4C/src/mbd/kernel/constraint2d.h`
- `FEM4C/src/mbd/kernel/kinematics2d.c`
- `FEM4C/src/mbd/kernel/kinematics2d.h`

### 次段

- `FEM4C/src/mbd/system/assembler2d.c`
- `FEM4C/src/mbd/system/assembler2d.h`
- `FEM4C/src/mbd/kernel/kkt2d.c`
- `FEM4C/src/mbd/kernel/kkt2d.h`
- `FEM4C/src/numerics/dense/linear_solver_dense.c`
- `FEM4C/src/numerics/dense/linear_solver_dense.h`

### 時間積分

- `FEM4C/src/mbd/kernel/integrator_explicit2d.c`
- `FEM4C/src/mbd/kernel/integrator_newmark2d.c`
- `FEM4C/src/mbd/kernel/integrator_hht2d.c`

### 最後に読むもの

- `FEM4C/src/mbd/system/system2d.c`
- `FEM4C/src/coupled/flex_body2d.c`
- `FEM4C/src/coupled/coupled_step_explicit2d.c`
- `FEM4C/src/coupled/coupled_step_implicit2d.c`

`system2d.c` は入口に置かない。最初は重すぎるためである。

## 5. 1問1答の出題形式

学習用の 1問1答は次の 6 種に分ける。

1. 概念確認型
2. 式穴埋め型
3. コード対応型
4. trace 型
5. バグ診断型
6. 設計判断型

## 6. 学習 1 サイクル

1. 理論を 1 章読む
2. 対応する code file を 1-2 本読む
3. 1問1答を実施する
4. 小さな修正か logging 追加を自分でやる
5. 再度 1問1答を実施する

## 7. Project Chrono への橋渡し

対応づける主概念は次のとおり。

- `mbd_system2d_t` -> `ChSystem`
- `mbd_body2d_t` -> `ChBody`
- `mbd_constraint2d_t` -> `ChLink` / constraint object
- KKT / descriptor assemble -> Chrono descriptor
- integrator -> timestepper
- generalized force / reaction -> loads / constraint reactions

重要なのは API 名ではなく責務境界である。

## 8. 2D から 3D へ拡張するときの視点

最初に追加で学ぶ差分は次の 3 つに絞る。

1. DOF が 3 -> 6 に増える
2. 回転表現が単一角から行列 / quaternion へ変わる
3. 接触・摩擦で拘束と力の扱いが複雑になる
