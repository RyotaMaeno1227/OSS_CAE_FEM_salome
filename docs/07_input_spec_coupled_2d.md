# 2D coupled input specification

最終更新: 2026-03-06

## 1. 目的
2D rigid / flexible 2-link validation のための MBD / coupled directive を固定する。

## 2. MBD directive
### `MBD_BODY_DYN`
```text
MBD_BODY_DYN <id> <mass> <inertia> <x> <y> <theta> <vx> <vy> <omega>
```
例:
```text
MBD_BODY_DYN 1 12.5 0.18 0.0 0.0 1.5707963268 0.0 0.0 0.0
```

### `MBD_GRAVITY`
```text
MBD_GRAVITY <gx> <gy>
```
例:
```text
MBD_GRAVITY 0.0 -9.80665
```

### `MBD_FORCE`
```text
MBD_FORCE <body_id> <fx> <fy> <mz>
```
例:
```text
MBD_FORCE 2 0.0 -15.0 0.0
```

## 3. Coupled directive
### `COUPLED_FLEX_BODY`
```text
COUPLED_FLEX_BODY <body_id> <fem_input_path>
```
例:
```text
COUPLED_FLEX_BODY 1 examples/flex_link1_q4.dat
```

### `COUPLED_FLEX_ROOT_SET`
```text
COUPLED_FLEX_ROOT_SET <body_id> <n> <id1> <id2> ...
```
例:
```text
COUPLED_FLEX_ROOT_SET 1 3 1 2 3
```

### `COUPLED_FLEX_TIP_SET`
```text
COUPLED_FLEX_TIP_SET <body_id> <n> <id1> <id2> ...
```
例:
```text
COUPLED_FLEX_TIP_SET 1 3 8 9 10
```

## 4. 対応関係
- `body_id` は MBD body と flexible link を結び付ける主キーとする。
- `fem_input_path` は各 link 用の FE input を指す。
- `root_set` / `tip_set` は link 局所座標系で joint interface とみなす node set である。

## 4.1 E-07 minimal example set
- `FEM4C/examples/coupled_2link_flex_master.dat`
- `FEM4C/examples/flex_link1_q4.dat`
- `FEM4C/examples/flex_link2_q4.dat`

master input では次のように 2-link flexible の最小構成を参照する。

```text
COUPLED_FLEX_BODY 0 examples/flex_link1_q4.dat
COUPLED_FLEX_ROOT_SET 0 2 1 4
COUPLED_FLEX_TIP_SET 0 2 2 3
COUPLED_FLEX_BODY 1 examples/flex_link2_q4.dat
COUPLED_FLEX_ROOT_SET 1 2 1 4
COUPLED_FLEX_TIP_SET 1 2 2 3
```

注:
- `examples/...` の相対 path は `cd FEM4C` からの実行を前提とする。
- current rigid benchmark (`examples/mbd_2link_rigid_dyn.dat`) と合わせるため、minimal example set の `body_id` は `0`, `1` を使う。

## 5. 後方互換
- 既存 `MBD_BODY` は互換維持する。
- 動力学計算を行う場合は `MBD_BODY_DYN` を優先する。
