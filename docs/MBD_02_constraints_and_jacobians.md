# MBD 02: Constraints and Jacobians

最終更新: 2026-03-08

## 1. この章のゴール

この章で理解するのは次の内容である。

1. constraint は何を固定する式か
2. residual と Jacobian をなぜ作るのか
3. revolute / distance をどう実装するか
4. KKT へどうつながるか

## 2. constraint の基本形

拘束は、一般には次の形で書く。

- `Phi(q) = 0`

ここで `Phi(q)` は
「今の姿勢が拘束をどれだけ破っているか」
を表す関数である。

この `Phi(q)` が 0 なら拘束を満たしている。

## 3. residual

実装ではまず residual を作る。

- residual = `Phi(q)`

たとえば:

- revolute: 2 つの body 上の対応点が一致しているか
- distance: 2 点間距離が目標値に一致しているか

を見る。

## 4. Jacobian

次に Jacobian を作る。

- `Cq = dPhi/dq`

意味は
「状態 `q` を少し変えたとき、拘束誤差 `Phi` がどう変わるか」
である。

これがあると、

- Newton 型の修正
- KKT 系の assemble
- 反力の計算

へ進める。

## 5. 2D での代表拘束

### 5.1 Revolute

revolute は 2 つの body 上の接続点を一致させる拘束である。

2D では通常、拘束式は 2 本になる。

- x 方向の一致
- y 方向の一致

### 5.2 Distance

distance は 2 点間距離を一定に保つ拘束である。

拘束式は通常 1 本で、
現在距離と目標距離の差を residual にする。

## 6. C 実装で最初に読む file

- `FEM4C/src/mbd/constraint2d.h`
- `FEM4C/src/mbd/constraint2d.c`
- `FEM4C/src/mbd/kinematics2d.h`
- `FEM4C/src/mbd/kinematics2d.c`

次段で読む file:

- `FEM4C/src/mbd/assembler2d.h`
- `FEM4C/src/mbd/assembler2d.c`
- `FEM4C/src/mbd/kkt2d.h`
- `FEM4C/src/mbd/kkt2d.c`

## 7. 実装時の見方

### 7.1 revolute

先に見るべき点:

1. local anchor をどう持つか
2. world 座標へどう変換するか
3. 点差ベクトルを residual にどう置くか
4. 各 body の `x, y, theta` に対する偏微分をどう書くか

### 7.2 distance

先に見るべき点:

1. 2 点間ベクトルをどう作るか
2. length と target length の差をどう定義するか
3. length が 0 に近いときの防御があるか

## 8. KKT とのつながり

拘束付き運動方程式は概念的に次の形になる。

```text
[ M   Cq^T ] [ dq_or_ddq ] = [ rhs_dyn ]
[ Cq   0   ] [ lambda    ]   [ rhs_con ]
```

ここで

- `M`: 質量行列
- `Cq`: constraint Jacobian
- `lambda`: ラグランジュ乗数

である。

重要なのは、constraint 実装だけ見て終わらず、
「この Jacobian が後で KKT のどこに入るか」
を意識することである。

## 9. 学習時の確認ポイント

### 9.1 概念確認

1. residual と Jacobian の違いは何か
2. revolute が 2 本拘束になる理由は何か
3. distance が 1 本拘束になる理由は何か

### 9.2 コード対応

1. local anchor はどこに保存されているか
2. world 座標への変換はどこで行うか
3. constraint Jacobian はどこで assemble されるか

### 9.3 バグ診断

1. Jacobian の `theta` 微分を 1 項落とすと何が起こるか
2. local/world の取り違えで何が壊れるか
3. distance が 0 に近いとき、どの防御が必要か

## 10. 1問1答に使う問い

1. `Phi(q)=0` の `Phi` は何を表していますか。
2. residual と Jacobian の役割の違いを説明してください。
3. 2D revolute 拘束が通常 2 本の式になる理由を説明してください。
4. distance 拘束の residual を言葉で定義してください。
5. `Cq` が KKT 系のどこに入るかを説明してください。
6. Jacobian の `theta` 偏微分を落とすと、どういう不具合が起こりやすいですか。

## 11. 次に読む章

この章の次は本来、

- `MBD_03_kkt_and_lagrange_multipliers.md`
- `MBD_04_explicit_integration.md`

の順で進む。

現時点では、先に `assembler2d.*`, `kkt2d.*`, `linear_solver_dense.*`
を読んで KKT の流れを追うとよい。

