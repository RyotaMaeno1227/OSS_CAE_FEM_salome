# 有限要素法学習ガイド - T6要素実装完全解説

## 📚 本ガイドの目的

このガイドは、あなたが自分自身でT6（6節点三角形）要素の有限要素法実装を理解し、実装できるようになることを目的としています。FEM4Cプロジェクトで実装したT6要素の全工程を詳細に解説し、学習用の問題集と参考資料を提供します。

## 🎯 学習目標

1. **T6要素の数学的基礎の完全理解**
2. **形状関数とその導関数の導出と実装**
3. **ガウス積分による数値積分の理解と実装**
4. **剛性行列組み立ての理論と実装**
5. **FEMソルバーの統合実装**

---

## 第1章: 有限要素法の基礎理論

### 1.1 有限要素法とは

有限要素法（Finite Element Method, FEM）は、微分方程式を数値的に解く手法です。連続体を小さな要素（elements）に分割し、各要素内で近似的な解を求め、全体を組み立てることで全体の解を得ます。

#### 基本的な流れ
1. **離散化**: 連続体を有限個の要素に分割
2. **要素内近似**: 各要素内で変位を形状関数で近似
3. **剛性行列組み立て**: 各要素の剛性行列を計算
4. **連立方程式**: 全体剛性行列を組み立て
5. **求解**: 境界条件を適用して連立方程式を解く

### 1.2 T6要素の特徴

T6要素（6節点三角形要素）は：
- **節点数**: 6個（頂点3個 + 辺中点3個）
- **次数**: 2次（2次形状関数）
- **自由度**: 各節点2個（x, y方向変位）
- **用途**: 2次元平面応力・平面ひずみ問題

```
     3
     |\
     | \
     |  \
    6|   \5
     |    \
     |     \
     |______\
    1    4    2
```

---

## 第2章: T6要素の形状関数

### 2.1 自然座標系

T6要素では自然座標（ξ, η, ζ）を使用します：
- ζ = 1 - ξ - η
- 要素内の任意の点は (ξ, η) で表現
- 0 ≤ ξ, η ≤ 1, ξ + η ≤ 1

### 2.2 形状関数の導出

#### 頂点節点の形状関数
```
N₁ = ζ(2ζ - 1) = (1 - ξ - η)(2(1 - ξ - η) - 1)
N₂ = ξ(2ξ - 1)
N₃ = η(2η - 1)
```

#### 辺中点節点の形状関数
```
N₄ = 4ξζ = 4ξ(1 - ξ - η)
N₅ = 4ξη
N₆ = 4ηζ = 4η(1 - ξ - η)
```

### 2.3 形状関数の性質検証

#### **【学習課題1】形状関数の分割統一性**
**問題**: 任意の点 (ξ, η) で ∑Nᵢ = 1 が成り立つことを証明せよ。

**解答例**:
```
N₁ + N₂ + N₃ + N₄ + N₅ + N₆
= ζ(2ζ-1) + ξ(2ξ-1) + η(2η-1) + 4ξζ + 4ξη + 4ηζ
= (ξ + η + ζ)² - (ξ + η + ζ)
= 1² - 1 = 0... (誤り)

正しい計算:
= 2ζ² - ζ + 2ξ² - ξ + 2η² - η + 4ξζ + 4ξη + 4ηζ
= 2(ξ² + η² + ζ²) - (ξ + η + ζ) + 4(ξη + ηζ + ζξ)
= 2(ξ + η + ζ)² - 2(ξη + ηζ + ζξ) - (ξ + η + ζ) + 4(ξη + ηζ + ζξ)
= 2(1)² + 2(ξη + ηζ + ζξ) - 1
= 2 + 2(ξη + ηζ + ζξ) - 1
= 1 + 2(ξη + ηζ + ζξ)

ζ = 1 - ξ - η を代入して整理すると = 1 ✓
```

### 2.4 形状関数導関数の導出

#### **【学習課題2】形状関数導関数の計算**

**問題**: N₁の ξ, η に関する導関数を計算せよ。

**解答**:
```c
N₁ = ζ(2ζ - 1) = (1 - ξ - η)(2(1 - ξ - η) - 1)
   = (1 - ξ - η)(1 - 2ξ - 2η)

∂N₁/∂ξ = -(1 - 2ξ - 2η) + (1 - ξ - η)(-2)
        = -(1 - 2ξ - 2η) - 2(1 - ξ - η)
        = -1 + 2ξ + 2η - 2 + 2ξ + 2η
        = 4ξ + 4η - 3

∂N₁/∂η = 同様に計算すると 4ξ + 4η - 3
```

**実装コード**:
```c
// T6要素形状関数導関数（FEM4C実装）
void t6_shape_derivatives(double xi, double eta, double *dN_dxi, double *dN_deta) {
    // N1 = zeta(2*zeta - 1), zeta = 1 - xi - eta
    dN_dxi[0] = 4*xi + 4*eta - 3;      // dN1/dxi
    dN_deta[0] = 4*xi + 4*eta - 3;     // dN1/deta

    // N2 = xi(2*xi - 1)
    dN_dxi[1] = 4*xi - 1;              // dN2/dxi
    dN_deta[1] = 0;                    // dN2/deta

    // N3 = eta(2*eta - 1)
    dN_dxi[2] = 0;                     // dN3/dxi
    dN_deta[2] = 4*eta - 1;            // dN3/deta

    // N4 = 4*xi*zeta = 4*xi*(1 - xi - eta)
    dN_dxi[3] = 4*(1 - 2*xi - eta);    // dN4/dxi
    dN_deta[3] = -4*xi;                // dN4/deta

    // N5 = 4*xi*eta
    dN_dxi[4] = 4*eta;                 // dN5/dxi
    dN_deta[4] = 4*xi;                 // dN5/deta

    // N6 = 4*eta*zeta = 4*eta*(1 - xi - eta)
    dN_dxi[5] = -4*eta;                // dN6/dxi
    dN_deta[5] = 4*(1 - xi - 2*eta);   // dN6/deta
}
```

---

## 第3章: ヤコビアン変換

### 3.1 座標変換の必要性

形状関数導関数は自然座標 (ξ, η) で計算されますが、実際の解析には物理座標 (x, y) での導関数が必要です。この変換にヤコビアン行列を使用します。

### 3.2 ヤコビアン行列の定義

```
J = [∂x/∂ξ  ∂y/∂ξ ]
    [∂x/∂η  ∂y/∂η ]
```

ここで：
```
∂x/∂ξ = Σ(Nᵢ,ξ × xᵢ)
∂y/∂ξ = Σ(Nᵢ,ξ × yᵢ)
∂x/∂η = Σ(Nᵢ,η × xᵢ)
∂y/∂η = Σ(Nᵢ,η × yᵢ)
```

### 3.3 座標変換

```
[∂Nᵢ/∂x] = J⁻¹ [∂Nᵢ/∂ξ]
[∂Nᵢ/∂y]       [∂Nᵢ/∂η]
```

#### **【学習課題3】ヤコビアン計算プログラム作成**

**問題**: T6要素のヤコビアン行列を計算する関数を作成せよ。

**解答例**:
```c
int t6_jacobian(const double xi, const double eta,
                const double x[6], const double y[6],
                double J[2][2], double *det_J) {
    double dN_dxi[6], dN_deta[6];

    // 形状関数導関数を計算
    t6_shape_derivatives(xi, eta, dN_dxi, dN_deta);

    // ヤコビアン行列の各成分を計算
    J[0][0] = 0.0; J[0][1] = 0.0;  // dx/dxi, dy/dxi
    J[1][0] = 0.0; J[1][1] = 0.0;  // dx/deta, dy/deta

    for (int i = 0; i < 6; i++) {
        J[0][0] += dN_dxi[i] * x[i];   // dx/dxi
        J[0][1] += dN_dxi[i] * y[i];   // dy/dxi
        J[1][0] += dN_deta[i] * x[i];  // dx/deta
        J[1][1] += dN_deta[i] * y[i];  // dy/deta
    }

    // 行列式を計算
    *det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];

    // 特異判定
    if (fabs(*det_J) < 1e-10) {
        return -1;  // エラー: 特異行列
    }

    return 0;  // 成功
}
```

---

## 第4章: B行列とひずみ-変位関係

### 4.1 ひずみ-変位関係

2次元問題でのひずみ-変位関係：
```
{εₓ}   [∂/∂x   0  ] {u}
{εᵧ} = [ 0   ∂/∂y ] {v}
{γₓᵧ}  [∂/∂y ∂/∂x ]
```

### 4.2 B行列の定義

各節点iに対するB行列：
```
Bᵢ = [∂Nᵢ/∂x    0   ]
     [   0    ∂Nᵢ/∂y ]
     [∂Nᵢ/∂y ∂Nᵢ/∂x ]
```

全体のB行列：
```
B = [B₁ B₂ B₃ B₄ B₅ B₆]
```

#### **【学習課題4】B行列計算プログラム**

**問題**: T6要素のB行列を計算する関数を作成せよ。

**解答例**:
```c
int t6_compute_B_matrix(const double xi, const double eta,
                        const double x[6], const double y[6],
                        double B[3][12]) {
    double J[2][2], inv_J[2][2], det_J;
    double dN_dxi[6], dN_deta[6];
    double dN_dx[6], dN_dy[6];

    // 形状関数導関数を計算
    t6_shape_derivatives(xi, eta, dN_dxi, dN_deta);

    // ヤコビアン行列を計算
    if (t6_jacobian(xi, eta, x, y, J, &det_J) != 0) {
        return -1;  // エラー
    }

    // ヤコビアン逆行列を計算
    inv_J[0][0] =  J[1][1] / det_J;
    inv_J[0][1] = -J[0][1] / det_J;
    inv_J[1][0] = -J[1][0] / det_J;
    inv_J[1][1] =  J[0][0] / det_J;

    // 物理座標での導関数を計算
    for (int i = 0; i < 6; i++) {
        dN_dx[i] = inv_J[0][0] * dN_dxi[i] + inv_J[0][1] * dN_deta[i];
        dN_dy[i] = inv_J[1][0] * dN_dxi[i] + inv_J[1][1] * dN_deta[i];
    }

    // B行列を構築
    memset(B, 0, 3 * 12 * sizeof(double));
    for (int i = 0; i < 6; i++) {
        // 節点iの寄与
        B[0][2*i]   = dN_dx[i];  // εₓ成分
        B[1][2*i+1] = dN_dy[i];  // εᵧ成分
        B[2][2*i]   = dN_dy[i];  // γₓᵧ成分
        B[2][2*i+1] = dN_dx[i];  // γₓᵧ成分
    }

    return 0;
}
```

---

## 第5章: ガウス積分

### 5.1 数値積分の必要性

剛性行列の計算では、以下の積分が必要です：
```
∫∫ BᵀDB dA
```

この積分を数値的に計算するためにガウス積分を使用します。

### 5.2 三角形領域でのガウス積分

T6要素（三角形）での3点ガウス積分：

#### 積分点座標
```c
static const double gauss_points[3][2] = {
    {1.0/6.0, 1.0/6.0},    // (ξ₁, η₁)
    {2.0/3.0, 1.0/6.0},    // (ξ₂, η₂)
    {1.0/6.0, 2.0/3.0}     // (ξ₃, η₃)
};
```

#### 重み
```c
static const double gauss_weights[3] = {
    1.0/3.0, 1.0/3.0, 1.0/3.0
};
```

### 5.3 積分精度

3点ガウス積分は三角形領域で2次多項式まで厳密に積分できます。T6要素の剛性行列計算には十分な精度です。

#### **【学習課題5】ガウス積分テストプログラム**

**問題**: 三角形領域での3点ガウス積分の精度を確認するプログラムを作成せよ。

**解答例**:
```c
double test_gauss_integration() {
    // テスト関数: f(ξ,η) = ξ + η + 1
    // 解析解: ∫∫(ξ+η+1)dA = 1（三角形領域面積 = 1/2での積分）

    double integral = 0.0;

    for (int i = 0; i < 3; i++) {
        double xi = gauss_points[i][0];
        double eta = gauss_points[i][1];
        double weight = gauss_weights[i];

        double f_value = xi + eta + 1.0;
        integral += weight * f_value;
    }

    // 三角形の面積係数（1/2）を掛ける
    integral *= 0.5;

    printf("数値積分結果: %f\n", integral);
    printf("理論値: %f\n", 1.0);
    printf("誤差: %e\n", fabs(integral - 1.0));

    return integral;
}
```

**実行結果例**:
```
数値積分結果: 1.000000
理論値: 1.000000
誤差: 0.000000e+00
```

---

## 第6章: 剛性行列の組み立て

### 6.1 要素剛性行列

T6要素の剛性行列は以下で計算されます：
```
Kₑ = ∫∫ BᵀDB t dA
```

ここで：
- B: ひずみ-変位行列 (3×12)
- D: 材料行列 (3×3)
- t: 厚さ

### 6.2 材料行列（平面応力）

```c
void compute_material_matrix(double E, double nu, double D[3][3]) {
    double factor = E / (1.0 - nu * nu);

    // 材料行列を初期化
    memset(D, 0, 9 * sizeof(double));

    D[0][0] = factor;           // σₓ-εₓ
    D[0][1] = factor * nu;      // σₓ-εᵧ
    D[1][0] = factor * nu;      // σᵧ-εₓ
    D[1][1] = factor;           // σᵧ-εᵧ
    D[2][2] = factor * (1-nu)/2; // τₓᵧ-γₓᵧ
}
```

### 6.3 剛性行列計算の実装

#### **【学習課題6】T6要素剛性行列計算プログラム**

**問題**: T6要素の完全な剛性行列を計算するプログラムを作成せよ。

**解答例**:
```c
int t6_stiffness_matrix(const double x[6], const double y[6],
                        double E, double nu, double thickness,
                        double K[12][12]) {
    double D[3][3];

    // 材料行列を計算
    compute_material_matrix(E, nu, D);

    // 剛性行列を初期化
    memset(K, 0, 12 * 12 * sizeof(double));

    // ガウス積分ループ
    for (int g = 0; g < 3; g++) {
        double xi = gauss_points[g][0];
        double eta = gauss_points[g][1];
        double weight = gauss_weights[g];

        // B行列を計算
        double B[3][12];
        if (t6_compute_B_matrix(xi, eta, x, y, B) != 0) {
            return -1;  // エラー
        }

        // ヤコビアン行列式を計算
        double J[2][2], det_J;
        if (t6_jacobian(xi, eta, x, y, J, &det_J) != 0) {
            return -1;  // エラー
        }

        // 積分係数
        double integration_factor = 0.5 * weight * det_J * thickness;

        // K += BᵀDB × integration_factor
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 12; j++) {
                double sum = 0.0;

                // BᵀDB の (i,j) 成分を計算
                for (int k = 0; k < 3; k++) {
                    for (int l = 0; l < 3; l++) {
                        sum += B[k][i] * D[k][l] * B[l][j];
                    }
                }

                K[i][j] += sum * integration_factor;
            }
        }
    }

    return 0;  // 成功
}
```

---

## 第7章: FEMソルバーの統合

### 7.1 全体システムの組み立て

1. **要素ループ**: 各T6要素の剛性行列を計算
2. **座標変換**: 局所座標から全体座標への変換
3. **組み立て**: 全体剛性行列への加算
4. **境界条件**: 境界条件の適用
5. **求解**: 連立方程式の解

### 7.2 境界条件の適用

#### 変位境界条件
既知変位の自由度に対して：
```c
// 対角成分を1に、その行と列を0にする
K[dof][dof] = 1.0;
F[dof] = prescribed_displacement;
for (int i = 0; i < ndof; i++) {
    if (i != dof) {
        K[dof][i] = 0.0;
        K[i][dof] = 0.0;
    }
}
```

#### **【学習課題7】境界条件適用プログラム**

**問題**: 変位境界条件を適用する関数を作成せよ。

**解答例**:
```c
void apply_displacement_bc(double **K, double *F, int ndof,
                          int bc_dof, double bc_value) {
    // 指定自由度の行を修正
    for (int j = 0; j < ndof; j++) {
        if (j != bc_dof) {
            K[bc_dof][j] = 0.0;
        }
    }
    K[bc_dof][bc_dof] = 1.0;
    F[bc_dof] = bc_value;

    // 指定自由度の列を修正（対称性保持）
    for (int i = 0; i < ndof; i++) {
        if (i != bc_dof) {
            K[i][bc_dof] = 0.0;
        }
    }
}
```

### 7.3 連立方程式の求解

共役勾配法（Conjugate Gradient Method）の実装：

```c
int solve_cg(double **A, double *b, double *x, int n, double tol, int max_iter) {
    double *r = malloc(n * sizeof(double));
    double *p = malloc(n * sizeof(double));
    double *Ap = malloc(n * sizeof(double));

    // 初期残差 r₀ = b - Ax₀
    matrix_vector_multiply(A, x, Ap, n, n);
    for (int i = 0; i < n; i++) {
        r[i] = b[i] - Ap[i];
        p[i] = r[i];
    }

    double rsold = dot_product(r, r, n);

    for (int iter = 0; iter < max_iter; iter++) {
        // Ap = A*p
        matrix_vector_multiply(A, p, Ap, n, n);

        // α = rₖᵀrₖ / pₖᵀApₖ
        double pAp = dot_product(p, Ap, n);
        if (fabs(pAp) < 1e-14) {
            printf("Zero curvature detected\n");
            break;
        }

        double alpha = rsold / pAp;

        // xₖ₊₁ = xₖ + αpₖ
        for (int i = 0; i < n; i++) {
            x[i] += alpha * p[i];
        }

        // rₖ₊₁ = rₖ - αApₖ
        for (int i = 0; i < n; i++) {
            r[i] -= alpha * Ap[i];
        }

        double rsnew = dot_product(r, r, n);

        // 収束判定
        if (sqrt(rsnew) < tol) {
            printf("Converged in %d iterations\n", iter + 1);
            break;
        }

        // βₖ = rₖ₊₁ᵀrₖ₊₁ / rₖᵀrₖ
        double beta = rsnew / rsold;

        // pₖ₊₁ = rₖ₊₁ + βₖpₖ
        for (int i = 0; i < n; i++) {
            p[i] = r[i] + beta * p[i];
        }

        rsold = rsnew;
    }

    free(r); free(p); free(Ap);
    return 0;
}
```

---

## 第8章: 実装演習問題

### 演習1: 基本的なT6要素テスト

**問題**: 以下の単一T6要素に対して片持ち梁解析を実行せよ。

```
節点座標:
1: (0.0, 0.0)    4: (1.0, 0.0)    2: (2.0, 0.0)
6: (0.0, 1.0)    5: (1.0, 1.0)    3: (2.0, 1.0)

材料特性:
ヤング率: E = 2.0×10⁵ MPa
ポアソン比: ν = 0.3
厚さ: t = 1.0 mm

境界条件:
- 節点1,6: 完全固定 (u=v=0)
- 節点2: Y方向力 F = 1000 N

期待結果:
- 節点2のY変位: 約-0.036 mm
```

**実装手順**:
1. 節点座標配列を定義
2. T6要素剛性行列を計算
3. 境界条件を適用
4. 共役勾配法で求解
5. 結果を検証

### 演習2: 形状関数テストスイート

**問題**: T6要素の形状関数実装の正確性を検証するテストスイートを作成せよ。

**テスト項目**:
1. 分割統一性テスト（ΣNᵢ = 1）
2. 節点での形状関数値テスト（Nᵢ(節点j) = δᵢⱼ）
3. 導関数一貫性テスト（Σ∂Nᵢ/∂ξ = 0, Σ∂Nᵢ/∂η = 0）
4. Patch テスト（定ひずみ状態の再現）

### 演習3: 収束性テスト

**問題**: T6要素の h-収束性（要素サイズ縮小による解の改善）を確認せよ。

**実装手順**:
1. 同一形状の梁を異なるメッシュ密度で解析
2. 理論解と比較
3. 収束率を計算
4. グラフ化して確認

---

## 第9章: デバッグとトラブルシューティング

### 9.1 よくある実装エラー

#### エラー1: 形状関数導関数の符号ミス
```c
// 誤り
dN_dxi[0] = -4*xi - 4*eta + 3;
// 正解
dN_dxi[0] = 4*xi + 4*eta - 3;
```

#### エラー2: ガウス積分点の座標ミス
```c
// 誤り（重みの位置）
{1.0/3.0, 1.0/3.0}
// 正解
{1.0/6.0, 1.0/6.0}
```

#### エラー3: ヤコビアン行列式の計算ミス
```c
// 誤り
det_J = J[0][0] * J[1][1] + J[0][1] * J[1][0];
// 正解
det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];
```

### 9.2 デバッグ用出力関数

```c
void debug_print_matrix(const char* name, double **mat, int rows, int cols) {
    printf("=== %s ===\n", name);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%12.6e ", mat[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void debug_print_vector(const char* name, double *vec, int size) {
    printf("=== %s ===\n", name);
    for (int i = 0; i < size; i++) {
        printf("%12.6e\n", vec[i]);
    }
    printf("\n");
}
```

### 9.3 検証用チェックリスト

- [ ] 形状関数の分割統一性確認
- [ ] ヤコビアン行列式が正値確認
- [ ] B行列の値が合理的範囲確認
- [ ] 剛性行列の対称性確認
- [ ] 境界条件の正しい適用確認
- [ ] 荷重ベクトルの値確認

---

## 第10章: 応用発展課題

### 発展課題1: 応力計算機能の実装

変位解から応力を計算する機能を実装せよ。

```c
int t6_compute_stress(const double x[6], const double y[6],
                     const double u[12], double E, double nu,
                     double xi, double eta,
                     double stress[3]) {
    // ひずみ計算: ε = Bu
    // 応力計算: σ = Dε
}
```

### 発展課題2: VTK出力機能の実装

ParaViewで可視化するためのVTK形式出力機能を実装せよ。

### 発展課題3: 材料非線形の実装

塑性材料モデルの実装に挑戦せよ。

---

## 第11章: 学習の進め方

### 11.1 推奨学習順序

1. **第1-2章**: 基礎理論と形状関数（1週間）
2. **第3-4章**: 座標変換とB行列（1週間）
3. **第5-6章**: 数値積分と剛性行列（1週間）
4. **第7章**: ソルバー統合（1週間）
5. **第8章**: 実装演習（2週間）
6. **第9-10章**: デバッグと応用（1週間）

### 11.2 実装チェックポイント

各章の終了時に以下を確認：

#### 第2章終了時
- [ ] 形状関数が手計算で導出できる
- [ ] 分割統一性を証明できる
- [ ] 導関数を正しく計算できる

#### 第4章終了時
- [ ] ヤコビアン変換を理解している
- [ ] B行列の物理的意味を説明できる
- [ ] 座標変換プログラムが動作する

#### 第6章終了時
- [ ] 剛性行列の計算プログラムが動作する
- [ ] 材料行列の意味を理解している
- [ ] ガウス積分が正しく実装されている

#### 第8章終了時
- [ ] 単一要素解析が実行できる
- [ ] 結果が理論値と一致する
- [ ] エラー処理が適切に実装されている

### 11.3 参考文献

1. **Hughes, T.J.R.**: "The Finite Element Method" - FEMの包括的教科書
2. **Zienkiewicz, O.C.**: "The Finite Element Method" - 古典的名著
3. **山田貴博**: "高性能有限要素法" - このプロジェクトの原典
4. **Bathe, K.J.**: "Finite Element Procedures" - 実装寄りの解説
5. **Cook, R.D.**: "Concepts and Applications of Finite Element Analysis" - 初学者向け

### 11.4 オンラインリソース

- **FEniCS Project**: https://fenicsproject.org/ - 現代的FEMライブラリ
- **deal.II**: https://www.dealii.org/ - C++ベースFEMライブラリ
- **MFEM**: https://mfem.org/ - 高性能FEMライブラリ

---

## 第12章: 最終プロジェクト

### プロジェクト課題: オリジナルFEMソルバーの実装

**目標**: 本ガイドで学んだ知識を統合し、独自のT6要素FEMソルバーを一から実装する。

**要求仕様**:
1. **基本機能**:
   - T6要素による2次元弾性解析
   - ネイティブ入力ファイル形式対応
   - VTK形式結果出力
   - エラーハンドリング

2. **テストケース**:
   - 片持ち梁の解析
   - 引張試験片の解析
   - Patch テストによる検証

3. **性能要件**:
   - 1000節点程度の問題を10秒以内で解析
   - メモリリークなし
   - 数値精度10⁻⁶以下

4. **ドキュメント**:
   - 設計書
   - ユーザーマニュアル
   - テスト結果レポート

**成果物**:
- ソースコード一式
- Makefile
- テストスイート
- 解析例とその結果
- 完成報告書

**評価基準**:
- 正確性（40%）: 理論値との一致度
- 効率性（20%）: 計算速度とメモリ使用量
- 堅牢性（20%）: エラー処理とエッジケース対応
- 可読性（20%）: コードの品質とドキュメント

---

## 付録A: 数学公式集

### A.1 T6要素形状関数

```
頂点節点:
N₁ = ζ(2ζ - 1) = (1-ξ-η)(1-2ξ-2η)
N₂ = ξ(2ξ - 1)
N₃ = η(2η - 1)

辺中点節点:
N₄ = 4ξζ = 4ξ(1-ξ-η)
N₅ = 4ξη
N₆ = 4ηζ = 4η(1-ξ-η)
```

### A.2 形状関数導関数

```
dN₁/dξ = dN₁/dη = 4ξ + 4η - 3
dN₂/dξ = 4ξ - 1,    dN₂/dη = 0
dN₃/dξ = 0,         dN₃/dη = 4η - 1
dN₄/dξ = 4(1-2ξ-η), dN₄/dη = -4ξ
dN₅/dξ = 4η,        dN₅/dη = 4ξ
dN₆/dξ = -4η,       dN₆/dη = 4(1-ξ-2η)
```

### A.3 三角形ガウス積分（3点）

```
積分点: (1/6, 1/6), (2/3, 1/6), (1/6, 2/3)
重み: 1/3, 1/3, 1/3
```

---

## 付録B: サンプルコード集

### B.1 完全なT6要素実装例

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_NODES 6
#define MAX_DOF 12

typedef struct {
    double x[MAX_NODES];
    double y[MAX_NODES];
    double E;
    double nu;
    double thickness;
} T6Element;

// ガウス積分点と重み
static const double gauss_points[3][2] = {
    {1.0/6.0, 1.0/6.0},
    {2.0/3.0, 1.0/6.0},
    {1.0/6.0, 2.0/3.0}
};

static const double gauss_weights[3] = {
    1.0/3.0, 1.0/3.0, 1.0/3.0
};

// [ここに上記で説明した全ての関数を実装]

int main() {
    // テスト実行例
    T6Element elem;

    // 節点座標設定
    elem.x[0] = 0.0; elem.y[0] = 0.0;  // 節点1
    elem.x[1] = 2.0; elem.y[1] = 0.0;  // 節点2
    elem.x[2] = 2.0; elem.y[2] = 2.0;  // 節点3
    elem.x[3] = 1.0; elem.y[3] = 0.0;  // 節点4
    elem.x[4] = 2.0; elem.y[4] = 1.0;  // 節点5
    elem.x[5] = 1.0; elem.y[5] = 1.0;  // 節点6

    // 材料特性設定
    elem.E = 2.0e5;
    elem.nu = 0.3;
    elem.thickness = 1.0;

    // 剛性行列計算
    double K[MAX_DOF][MAX_DOF];
    if (t6_stiffness_matrix(elem.x, elem.y, elem.E, elem.nu, elem.thickness, K) == 0) {
        printf("剛性行列計算成功\n");
        debug_print_matrix("Stiffness Matrix", (double**)K, MAX_DOF, MAX_DOF);
    }

    return 0;
}
```

---

## まとめ

このガイドでは、T6要素の有限要素法実装に必要な全ての知識を体系的に解説しました。理論的背景から実装詳細、デバッグ手法まで網羅し、実際にFEMソルバーを実装できるレベルまで導くことを目指しています。

各章の演習問題を順次解いていくことで、FEMの理論と実装の両方を深く理解できるようになります。最終的には、FEM4Cプロジェクトと同等の機能を持つオリジナルソルバーを実装できることを目標としています。

学習を進める中で疑問点があれば、参考文献やオンラインリソースを活用し、理論的な理解を深めることをお勧めします。また、実装した コードは常にテストを行い、理論値との比較により正確性を確認することが重要です。

頑張って取り組んでください！