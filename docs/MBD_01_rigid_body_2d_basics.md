# MBD 01: Rigid Body 2D Basics

最終更新: 2026-03-08

## 1. この章のゴール

この章で理解するのは次の内容である。

1. 2D 剛体 1 個の状態量
2. generalized coordinate と generalized force
3. C でどう struct に落とすか
4. どの file を最初に読むべきか

## 2. 最小状態量

2D rigid body の最小状態は次で表す。

- `q = [x, y, theta]`
- `v = [vx, vy, omega]`
- `a = [ax, ay, alpha]`

ここで

- `x, y` は重心位置
- `theta` は平面内回転角
- `vx, vy` は並進速度
- `omega` は角速度

である。

## 3. generalized coordinate / generalized force

2D では自由度が 3 つなので、状態ベクトルも力ベクトルも長さ 3 で扱える。

- generalized coordinate: `q = [x, y, theta]`
- generalized force: `Q = [Fx, Fy, Tz]`

ここで重要なのは、
「回転も translation と同じベクトルの 1 成分として扱う」
という点である。

## 4. 最小の運動方程式

拘束なしなら、概念的には次を解けばよい。

- `M a = Q`

2D rigid body 1 個の質量行列は、最小形では次のように見てよい。

- `diag(m, m, I)`

ここで

- `m`: 質量
- `I`: 面外軸まわり慣性モーメント

である。

## 5. C 実装でどう持つか

入口として先に読む file は次。

- `FEM4C/src/mbd/body2d.h`
- `FEM4C/src/mbd/body2d.c`
- `FEM4C/src/mbd/forces2d.h`
- `FEM4C/src/mbd/forces2d.c`
- `FEM4C/src/mbd/kinematics2d.h`
- `FEM4C/src/mbd/kinematics2d.c`

読むときの観点は次の 4 つ。

1. body struct に何が入っているか
2. `q`, `v`, `a`, `force` をどう初期化しているか
3. ground body をどう区別しているか
4. 座標変換や局所点の扱いをどこに置いているか

## 6. 先に理解すべき実装パターン

### 6.1 状態は配列で持つ

2D では自由度が固定なので、`double q[3]` のような固定長配列が読みやすい。

### 6.2 力は毎 step で clear -> accumulate する

`force` は使い回すより、毎 step で clear してから加算する方が追いやすい。

### 6.3 geometry と state を分ける

body が持つべきものは大きく 2 つに分かれる。

- 不変量: 質量、慣性、ground 属性
- 変化量: `q`, `v`, `a`, `force`

## 7. 学習時の確認ポイント

### 7.1 概念確認

1. なぜ 2D rigid body の自由度は 3 なのか
2. `theta` を generalized coordinate に含める理由は何か
3. generalized force の 3 成分は何を意味するか

### 7.2 コード対応

1. `q`, `v`, `a`, `force` をどの struct が持つか
2. gravity や external force をどこで合成するか
3. kinematics helper はどこに置くべきか

### 7.3 バグ診断

1. `theta` の更新だけ抜けたら何が壊れるか
2. `force` を clear し忘れるとどうなるか
3. ground body に acceleration update を入れると何が壊れるか

## 8. 1問1答に使う問い

1. `q = [x, y, theta]` の各成分の物理意味を説明してください。
2. 2D で generalized force が `[Fx, Fy, Tz]` になる理由を説明してください。
3. 2D rigid body 1 個の最小質量行列を式で書いてください。
4. `body2d` に持たせるべき不変量と変化量を分けて答えてください。
5. ground body を通常 body と同じ更新経路に入れると何が起きますか。

## 9. 次に読む章

次は `docs/MBD_02_constraints_and_jacobians.md` を読む。
次の章では、body 単体ではなく body 間をつなぐ constraint を扱う。

