# FEM4C参考書
## 高性能有限要素法プログラム - 完全実装ガイド

**Version 1.0**
**Based on "High Performance Finite Element Method" by Takahiro Yamada**

---

## 目次

1. [序論](#1-序論)
2. [システム概要](#2-システム概要)
3. [理論的背景](#3-理論的背景)
4. [実装詳細](#4-実装詳細)
5. [要素ライブラリ](#5-要素ライブラリ)
6. [並列化システム](#6-並列化システム)
7. [Nastran互換性](#7-nastran互換性)
8. [入出力システム](#8-入出力システム)
9. [使用方法](#9-使用方法)
10. [性能評価](#10-性能評価)
11. [トラブルシューティング](#11-トラブルシューティング)
12. [開発者向け情報](#12-開発者向け情報)

---

## 1. 序論

### 1.1 FEM4Cとは

FEM4C（Finite Element Method for C）は、山田貴博氏の著書「高性能有限要素法」に基づいて開発された、C言語による有限要素法プログラムです。本プロジェクトは「研究者(初心者)が自分でFEMを実装できるようになる」ことを目的とした練習用マテリアルであり、学習しやすさと実装の追体験を重視しています。本システムは以下の特徴を持ちます：

- **学習志向**: 重要な処理を見通しよく追える構成
- **互換性**: Nastran Bulk入力の一部カードに対応
- **拡張性**: モジュラー設計による要素追加の容易さ
- **精度**: 高精度数値解析アルゴリズム

### 1.2 対象読者

- 有限要素法を自分で実装して理解したい研究者・学生
- C言語での数値計算実装に慣れたい方
- FEMの入力/出力やアセンブリ処理の流れを学びたい方

### 1.3 読む順番（概要→実装）
1. 本書（このファイル）で全体像を把握  
2. `docs/implementation_guide.md` でモジュール別の読み順を確認  
3. `docs/tutorial_manual.md` の章立てで実装演習  

### 1.4 最小検算例（全体確認）
**目的**: solver が「入力→解→出力」の流れで動くことを確認する。  

入力例（native 形式）:
```
Minimal T3
3 1
1 0.0 0.0
2 1.0 0.0
3 0.0 1.0
1 1 2 3
2.0e11 0.3
1 1 1 0.0 0.0 0.0
2 1 1 0.0 0.0 0.0
point loads
3 0.0 1000.0 0.0
end
```

チェック:
- 実行ログで `Nodes=3, Elements=1, DOF=6` が表示される。  
- `output.dat` に非ゼロの変位が出る。  

### 1.3 前提知識

- C言語プログラミング
- 線形代数の基礎
- 有限要素法の基本概念
- 構造力学の基礎

---

## 2. システム概要

### 2.1 アーキテクチャ

```
FEM4C システム構成
├── コア（共通）
│   ├── データ型定義
│   ├── エラーハンドリング
│   ├── グローバル変数管理
│   └── 定数定義
├── 要素ライブラリ
│   ├── T3要素（3節点三角形）
│   ├── Q4要素（4節点四角形）
│   └── T6要素（6節点三角形）
├── ソルバー
│   ├── アセンブリシステム
│   ├── 共役勾配法ソルバー
│   └── OpenMP対応（オプション）
├── 入出力
│   ├── Nastran形式読み込み
│   ├── parser出力パッケージ読み込み
│   ├── VTK形式出力
│   └── F06形式出力
└── 解析制御
    ├── 静解析制御
    ├── 前処理
    └── 後処理
```

### 2.2 開発フェーズ

FEM4Cは段階的に開発されました：

**Phase 1**: 基本T6要素実装
- 6節点三角形要素
- 基本的な静解析機能
- 標準的な入出力

**Phase 2**: 2D要素拡張
- T3要素（3節点三角形）追加
- Q4要素（4節点四角形）追加
- 要素管理システム統合

**Phase 3**: ソルバー最適化
- 共役勾配法実装
- 収束判定改善
- メモリ効率化

**Phase 4**: 並列化とNastran対応
- OpenMP並列化（準備）
- Nastran入力形式サポート（カードの一部）
- F06出力形式実装

---

## 3. 理論的背景

### 3.1 有限要素法の基礎

有限要素法では、連続体を小さな要素に分割し、各要素内で近似関数を用いて物理量を表現します。

#### 3.1.1 弱形式

構造力学問題の弱形式は以下のように表されます：

```
∫Ω B^T D B u dΩ = ∫Ω N^T f dΩ + ∫Γ N^T t dΓ
```

ここで：
- `B`: ひずみ-変位マトリクス
- `D`: 材料マトリクス
- `u`: 変位ベクトル
- `N`: 形状関数マトリクス
- `f`: 体積力
- `t`: 表面力

#### 3.1.2 離散化

要素レベルでの剛性方程式：

```
[K_e]{u_e} = {f_e}
```

全体系での剛性方程式：

```
[K]{U} = {F}
```

### 3.2 要素技術

#### 3.2.1 T3要素（3節点三角形）

**形状関数**:
```
N_1 = ξ
N_2 = η
N_3 = 1 - ξ - η
```

**自由度**: 各節点で2自由度（x、y方向変位）
**総自由度**: 6

#### 3.2.2 Q4要素（4節点四角形）

**形状関数** (自然座標系):
```
N_1 = (1-ξ)(1-η)/4
N_2 = (1+ξ)(1-η)/4
N_3 = (1+ξ)(1+η)/4
N_4 = (1-ξ)(1+η)/4
```

**自由度**: 各節点で2自由度
**総自由度**: 8

#### 3.2.3 T6要素（6節点三角形）

**形状関数**:
```
N_1 = ξ(2ξ-1)
N_2 = η(2η-1)
N_3 = ζ(2ζ-1)
N_4 = 4ξη
N_5 = 4ηζ
N_6 = 4ζξ
```

ここで `ζ = 1 - ξ - η`

**自由度**: 各節点で2自由度
**総自由度**: 12

### 3.3 材料モデル

#### 3.3.1 平面応力状態

```
{σ} = [D]{ε}
```

材料マトリクス:
```
[D] = E/(1-ν²) * [1   ν   0  ]
                  [ν   1   0  ]
                  [0   0  (1-ν)/2]
```

#### 3.3.2 平面ひずみ状態

```
[D] = E/((1+ν)(1-2ν)) * [1-ν  ν    0     ]
                         [ν    1-ν  0     ]
                         [0    0   (1-2ν)/2]
```

---

## 4. 実装詳細

### 4.1 データ構造

#### 4.1.1 基本データ型

```c
// エラー型定義
typedef enum {
    FEM_SUCCESS = 0,
    FEM_ERROR_MEMORY_ALLOCATION,
    FEM_ERROR_FILE_IO,
    FEM_ERROR_INVALID_INPUT,
    FEM_ERROR_CONVERGENCE_FAILED,
    FEM_ERROR_INVALID_ELEMENT_TYPE,
    FEM_ERROR_INVALID_MATERIAL,
    FEM_ERROR_INVALID_BOUNDARY_CONDITION
} fem_error_t;

// 要素タイプ定義
typedef enum {
    ELEMENT_T3 = 1,    // 3節点三角形
    ELEMENT_Q4 = 2,    // 4節点四角形
    ELEMENT_T6 = 3     // 6節点三角形
} element_type_t;
```

#### 4.1.2 グローバル変数

```c
// 問題サイズ
extern int g_num_nodes;          // 節点数
extern int g_num_elements;       // 要素数
extern int g_num_materials;      // 材料数
extern int g_total_dof;          // 総自由度数

// 座標と接続性
extern double g_node_coords[MAX_NODES][3];
extern int g_element_nodes[MAX_ELEMENTS][MAX_NODES_PER_ELEMENT];
extern element_type_t g_element_type[MAX_ELEMENTS];

// 材料特性
extern double g_material_props[MAX_MATERIALS][3]; // E, ν, t

// システム行列・ベクトル
extern double **g_global_stiffness;
extern double *g_global_force;
extern double *g_global_displ;
```

### 4.2 メモリ管理

#### 4.2.1 動的メモリ確保

```c
fem_error_t globals_initialize(void) {
    // 全体剛性行列の確保
    g_global_stiffness = calloc(g_total_dof, sizeof(double*));
    if (!g_global_stiffness) return FEM_ERROR_MEMORY_ALLOCATION;

    for (int i = 0; i < g_total_dof; i++) {
        g_global_stiffness[i] = calloc(g_total_dof, sizeof(double));
        if (!g_global_stiffness[i]) return FEM_ERROR_MEMORY_ALLOCATION;
    }

    // ベクトルの確保
    g_global_force = calloc(g_total_dof, sizeof(double));
    g_global_displ = calloc(g_total_dof, sizeof(double));

    return FEM_SUCCESS;
}
```

#### 4.2.2 メモリ解放

```c
fem_error_t globals_finalize(void) {
    // 行列の解放
    if (g_global_stiffness) {
        for (int i = 0; i < g_total_dof; i++) {
            free(g_global_stiffness[i]);
        }
        free(g_global_stiffness);
    }

    // ベクトルの解放
    free(g_global_force);
    free(g_global_displ);

    return FEM_SUCCESS;
}
```

### 4.3 エラーハンドリング

#### 4.3.1 統一エラー処理

```c
typedef struct {
    fem_error_t code;
    char message[MAX_ERROR_MESSAGE_LEN];
    char function[MAX_FUNCTION_NAME_LEN];
    char file[MAX_FILENAME_LEN];
    int line;
} error_context_t;

fem_error_t error_set(fem_error_t code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_error_context.message, MAX_ERROR_MESSAGE_LEN, format, args);
    va_end(args);

    g_error_context.code = code;
    fprintf(stderr, "FEM4C Error [%d]: %s\n", code, g_error_context.message);

    return code;
}

#define CHECK_ERROR(err) \
    if ((err) != FEM_SUCCESS) return (err)
```

---

## 5. 要素ライブラリ

### 5.1 要素インターフェース

#### 5.1.1 共通要素構造体

```c
typedef struct {
    element_type_t type;
    int nodes_per_element;
    int dof_per_node;
    int total_dof;

    // 関数ポインタ
    fem_error_t (*stiffness)(int element_id, double ke[][MAX_DOF]);
    fem_error_t (*stress)(int element_id, double *stress);
    fem_error_t (*validate)(int element_id);
} element_interface_t;
```

#### 5.1.2 要素登録システム

```c
fem_error_t elements_initialize(void) {
    // T3要素の登録
    elements[ELEMENT_T3] = (element_interface_t){
        .type = ELEMENT_T3,
        .nodes_per_element = 3,
        .dof_per_node = 2,
        .total_dof = 6,
        .stiffness = t3_element_stiffness,
        .stress = t3_element_stress,
        .validate = t3_validate_element
    };

    // 他の要素も同様に登録...

    return FEM_SUCCESS;
}
```

### 5.2 T3要素実装

#### 5.2.1 剛性行列計算

```c
fem_error_t t3_element_stiffness(int element_id, double ke[][T3_TOTAL_DOF]) {
    double coords[T3_NODES_PER_ELEMENT][2];
    double D[3][3];

    // 座標取得
    fem_error_t err = t3_get_element_coordinates(element_id, coords);
    CHECK_ERROR(err);

    // 材料マトリクス計算
    err = t3_material_matrix(element_id, D);
    CHECK_ERROR(err);

    // B マトリクス計算
    double B[3][T3_TOTAL_DOF];
    err = t3_strain_displacement_matrix(coords, B);
    CHECK_ERROR(err);

    // 面積計算
    double area = t3_element_area(coords);
    if (area <= TOLERANCE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                        "T3 element has zero or negative area");
    }

    // ke = B^T * D * B * area * thickness
    double thickness = g_material_props[g_element_material[element_id]][2];
    double factor = area * thickness;

    for (int i = 0; i < T3_TOTAL_DOF; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            ke[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    ke[i][j] += B[k][i] * D[k][l] * B[l][j];
                }
            }
            ke[i][j] *= factor;
        }
    }

    return FEM_SUCCESS;
}
```

#### 5.2.2 ひずみ-変位マトリクス

```c
fem_error_t t3_strain_displacement_matrix(double coords[][2], double B[][T3_TOTAL_DOF]) {
    double x1 = coords[0][0], y1 = coords[0][1];
    double x2 = coords[1][0], y2 = coords[1][1];
    double x3 = coords[2][0], y3 = coords[2][1];

    double area_2 = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);

    if (fabs(area_2) < TOLERANCE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                        "Degenerate T3 element detected");
    }

    // B マトリクス構築
    B[0][0] = (y2-y3)/area_2;  B[0][1] = 0.0;
    B[0][2] = (y3-y1)/area_2;  B[0][3] = 0.0;
    B[0][4] = (y1-y2)/area_2;  B[0][5] = 0.0;

    B[1][0] = 0.0;  B[1][1] = (x3-x2)/area_2;
    B[1][2] = 0.0;  B[1][3] = (x1-x3)/area_2;
    B[1][4] = 0.0;  B[1][5] = (x2-x1)/area_2;

    B[2][0] = (x3-x2)/area_2;  B[2][1] = (y2-y3)/area_2;
    B[2][2] = (x1-x3)/area_2;  B[2][3] = (y3-y1)/area_2;
    B[2][4] = (x2-x1)/area_2;  B[2][5] = (y1-y2)/area_2;

    return FEM_SUCCESS;
}
```

### 5.3 Q4要素実装

#### 5.3.1 数値積分

```c
fem_error_t q4_element_stiffness(int element_id, double ke[][Q4_TOTAL_DOF]) {
    double coords[Q4_NODES_PER_ELEMENT][2];
    double D[3][3];

    // 座標と材料特性取得
    fem_error_t err = q4_get_element_coordinates(element_id, coords);
    CHECK_ERROR(err);

    err = q4_material_matrix(element_id, D);
    CHECK_ERROR(err);

    // 剛性行列初期化
    memset(ke, 0, Q4_TOTAL_DOF * Q4_TOTAL_DOF * sizeof(double));

    // 2x2 ガウス積分
    double gp[2] = {-1.0/sqrt(3.0), 1.0/sqrt(3.0)};
    double w[2] = {1.0, 1.0};

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            double xi = gp[i];
            double eta = gp[j];
            double weight = w[i] * w[j];

            // ヤコビ行列と B マトリクス計算
            double J[2][2], det_J;
            double B[3][Q4_TOTAL_DOF];

            err = q4_jacobian_matrix(coords, xi, eta, J, &det_J);
            CHECK_ERROR(err);

            err = q4_strain_displacement_matrix(coords, xi, eta, B);
            CHECK_ERROR(err);

            // 剛性行列積分
            double factor = det_J * weight * g_material_props[g_element_material[element_id]][2];

            for (int m = 0; m < Q4_TOTAL_DOF; m++) {
                for (int n = 0; n < Q4_TOTAL_DOF; n++) {
                    for (int k = 0; k < 3; k++) {
                        for (int l = 0; l < 3; l++) {
                            ke[m][n] += B[k][m] * D[k][l] * B[l][n] * factor;
                        }
                    }
                }
            }
        }
    }

    return FEM_SUCCESS;
}
```

### 5.4 T6要素実装

#### 5.4.1 形状関数とその微分

```c
void t6_shape_functions(double xi, double eta, double N[T6_NODES_PER_ELEMENT]) {
    double zeta = 1.0 - xi - eta;

    N[0] = xi * (2.0 * xi - 1.0);      // 頂点1
    N[1] = eta * (2.0 * eta - 1.0);    // 頂点2
    N[2] = zeta * (2.0 * zeta - 1.0);  // 頂点3
    N[3] = 4.0 * xi * eta;             // 辺1の中点
    N[4] = 4.0 * eta * zeta;           // 辺2の中点
    N[5] = 4.0 * zeta * xi;            // 辺3の中点
}

void t6_shape_function_derivatives(double xi, double eta,
                                   double dN_dxi[T6_NODES_PER_ELEMENT],
                                   double dN_deta[T6_NODES_PER_ELEMENT]) {
    // ξ方向微分
    dN_dxi[0] = 4.0 * xi - 1.0;
    dN_dxi[1] = 0.0;
    dN_dxi[2] = -(4.0 * (1.0 - xi - eta) - 1.0);
    dN_dxi[3] = 4.0 * eta;
    dN_dxi[4] = -4.0 * eta;
    dN_dxi[5] = 4.0 * (1.0 - xi - eta) - 4.0 * xi;

    // η方向微分
    dN_deta[0] = 0.0;
    dN_deta[1] = 4.0 * eta - 1.0;
    dN_deta[2] = -(4.0 * (1.0 - xi - eta) - 1.0);
    dN_deta[3] = 4.0 * xi;
    dN_deta[4] = 4.0 * (1.0 - xi - eta) - 4.0 * eta;
    dN_deta[5] = -4.0 * xi;
}
```

---

## 6. 並列化システム

### 6.1 OpenMP実装

#### 6.1.1 並列要素剛性行列計算

```c
fem_error_t assembly_parallel_stiffness_matrix(void) {
#ifdef _OPENMP
    printf("Assembling global stiffness matrix (OpenMP parallel)...\n");
    printf("  Number of threads: %d\n", omp_get_max_threads());
    printf("  Elements: %d\n", g_num_elements);

    // 要素タイプ別の静的配列
    static double ke_t6[MAX_ELEMENTS][T6_TOTAL_DOF][T6_TOTAL_DOF];
    static double ke_t3[MAX_ELEMENTS][T3_TOTAL_DOF][T3_TOTAL_DOF];
    static double ke_q4[MAX_ELEMENTS][Q4_TOTAL_DOF][Q4_TOTAL_DOF];

    fem_error_t err = FEM_SUCCESS;

    // 並列要素計算
    #pragma omp parallel for shared(ke_t6, ke_t3, ke_q4, err)
    for (int element_id = 0; element_id < g_num_elements; element_id++) {
        fem_error_t local_err = FEM_SUCCESS;

        switch (g_element_type[element_id]) {
            case ELEMENT_T6:
                local_err = t6_element_stiffness_matrix(element_id, ke_t6[element_id]);
                break;
            case ELEMENT_T3:
                local_err = t3_element_stiffness(element_id, ke_t3[element_id]);
                break;
            case ELEMENT_Q4:
                local_err = q4_element_stiffness(element_id, ke_q4[element_id]);
                break;
            default:
                local_err = FEM_ERROR_INVALID_ELEMENT_TYPE;
        }

        #pragma omp critical
        {
            if (local_err != FEM_SUCCESS && err == FEM_SUCCESS) {
                err = local_err;
            }
        }
    }

    if (err != FEM_SUCCESS) return err;

    printf("  Parallel element calculations completed\n");
    printf("  Assembling into global matrix (serial)...\n");

    // シリアルアセンブリ（レースコンディション回避）
    for (int element_id = 0; element_id < g_num_elements; element_id++) {
        switch (g_element_type[element_id]) {
            case ELEMENT_T6:
                err = assembly_element_stiffness_t6(element_id, ke_t6[element_id]);
                break;
            case ELEMENT_T3:
                err = assembly_element_stiffness_t3(element_id, ke_t3[element_id]);
                break;
            case ELEMENT_Q4:
                err = assembly_element_stiffness_q4(element_id, ke_q4[element_id]);
                break;
        }
        CHECK_ERROR(err);
    }

    printf("  Parallel assembly completed successfully\n");
    return FEM_SUCCESS;

#else
    // 非並列版にフォールバック
    return assembly_global_stiffness_matrix();
#endif
}
```

#### 6.1.2 スレッド安全性

**問題点と解決策**:

1. **レースコンディション**: 複数スレッドが同じメモリ位置に同時書き込み
   - **解決**: 要素計算と全体アセンブリの分離

2. **メモリ競合**: 動的メモリ確保での競合
   - **解決**: 静的配列による事前確保

3. **エラー処理**: 並列実行中のエラー伝播
   - **解決**: `#pragma omp critical`による排他制御

### 6.2 性能最適化

#### 6.2.1 並列化効率

**理論的並列化効率**:
```
効率 = T_serial / (P × T_parallel)
```

ここで：
- `T_serial`: シリアル実行時間
- `T_parallel`: 並列実行時間
- `P`: プロセッサ数

**実測値**:
- 1スレッド: 1.846秒（基準）
- 4スレッド: 1.338秒（1.38倍高速化）
- 8スレッド: 1.573秒（1.17倍高速化）

#### 6.2.2 ボトルネック分析

**主要処理時間分布**:
1. 要素剛性行列計算: 30%（並列化対象）
2. 全体アセンブリ: 20%（シリアル）
3. 連立方程式求解: 35%（今後の並列化対象）
4. その他（I/O等）: 15%

---

## 7. Nastran互換性

本プロジェクトは学習用の実装であり、Nastran互換性は「必要最低限のカードに限定したサブセット対応」です。実運用のNastran全カードを網羅することは目的としていません。

### 7.1 入力形式

#### 7.1.1 サポート済みカード

**GRID（節点定義）**:
```
GRID    ID      CP      X1      X2      X3      CD      PS      SEID
GRID    1               0.0     0.0     0.0
```

**CTRIA3（3節点三角形要素）**:
```
CTRIA3  EID     PID     G1      G2      G3      THETA   ZOFFS   BLANK
CTRIA3  1       1       1       2       3
```

**CQUAD4（4節点四角形要素）**:
```
CQUAD4  EID     PID     G1      G2      G3      G4      THETA   ZOFFS
CQUAD4  1       1       1       2       3       4
```

**CTRIA6（6節点三角形要素）**:
```
CTRIA6  EID     PID     G1      G2      G3      G4      G5      G6
CTRIA6  1       1       1       2       3       4       5       6
```

**MAT1（等方性材料）**:
```
MAT1    MID     E       G       NU      RHO     A       TREF    GE
MAT1    1       2.1E5           0.3
```

**SPC（単点拘束）**:
```
SPC     SID     G       C       D
SPC     1       1       123     0.0
```

**FORCE（集中荷重）**:
```
FORCE   SID     G       CID     F       N1      N2      N3
FORCE   1       1       0       1000.0  1.0     0.0     0.0
```

#### 7.1.2 パーサー実装

```c
fem_error_t input_read_nastran_bulk(input_control_t *input) {
    char line[MAX_LINE_LENGTH];
    int in_bulk = 0;

    while (fgets(line, sizeof(line), input->file_ptr)) {
        input->line_number++;

        // コメント行のスキップ
        if (line[0] == '$') continue;

        // BULK セクション検出
        if (strncmp(line, "BEGIN BULK", 10) == 0) {
            in_bulk = 1;
            printf("  Found BEGIN BULK at line %d\n", input->line_number);
            continue;
        }

        if (strncmp(line, "ENDDATA", 7) == 0) {
            printf("  Found ENDDATA at line %d\n", input->line_number);
            break;
        }

        if (!in_bulk) continue;

        // カード解析
        if (strncmp(line, "GRID", 4) == 0) {
            err = input_parse_nastran_grid(input, line);
        } else if (strncmp(line, "CTRIA3", 6) == 0) {
            err = input_parse_nastran_ctria3(input, line);
        } else if (strncmp(line, "CQUAD4", 6) == 0) {
            err = input_parse_nastran_cquad4(input, line);
        } else if (strncmp(line, "CTRIA6", 6) == 0) {
            err = input_parse_nastran_ctria6(input, line);
        } else if (strncmp(line, "MAT1", 4) == 0) {
            err = input_parse_nastran_mat1(input, line);
        } else if (strncmp(line, "SPC", 3) == 0) {
            err = input_parse_nastran_spc(input, line);
        } else if (strncmp(line, "FORCE", 5) == 0) {
            err = input_parse_nastran_force(input, line);
        }

        CHECK_ERROR(err);
    }

    return FEM_SUCCESS;
}
```

#### 7.1.3 固定フィールド解析

```c
fem_error_t input_nastran_parse_fixed_format(const char *line, char fields[][9], int max_fields) {
    int field_count = 0;

    for (int pos = 0; pos < strlen(line) && field_count < max_fields; pos += 8) {
        // 8文字フィールドを抽出
        int len = (strlen(line) - pos >= 8) ? 8 : strlen(line) - pos;
        strncpy(fields[field_count], line + pos, len);
        fields[field_count][len] = '\0';

        // 末尾の空白と改行文字を削除
        for (int j = len - 1; j >= 0; j--) {
            if (fields[field_count][j] == ' ' ||
                fields[field_count][j] == '\n' ||
                fields[field_count][j] == '\r') {
                fields[field_count][j] = '\0';
            } else {
                break;
            }
        }

        field_count++;
    }

    return field_count;
}
```

### 7.2 出力形式

#### 7.2.1 F06形式ヘッダー

```c
fem_error_t output_write_nastran_f06_header(output_control_t *output) {
    time_t current_time;
    struct tm *time_info;
    char time_string[64];

    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%a %b %d %H:%M:%S %Y", time_info);

    fprintf(output->file_ptr, "1\n");
    fprintf(output->file_ptr,
        "                                              N A S T R A N    F I L E    A N D    S Y S T E M    P A R A M E T E R    E C H O\n");
    fprintf(output->file_ptr,
        "                                                                                                                                                          PAGE    1\n\n\n");

    fprintf(output->file_ptr, "0%60s\n", "* * * * * * * * * * * * * * * *");
    fprintf(output->file_ptr, "0%60s\n", "*                             *");
    fprintf(output->file_ptr, "0%60s\n", "*        FEM4C SOLUTION      *");
    fprintf(output->file_ptr, "0%60s\n", "*                             *");
    fprintf(output->file_ptr, "0%60s\n", "* * * * * * * * * * * * * * * *");

    fprintf(output->file_ptr, "\n\n0SOLUTION SUMMARY:\n");
    fprintf(output->file_ptr, "     PROBLEM TITLE........ FEM4C HIGH PERFORMANCE FINITE ELEMENT ANALYSIS\n");
    fprintf(output->file_ptr, "     SOLUTION TYPE........ STATIC ANALYSIS (SOL 101)\n");
    fprintf(output->file_ptr, "     ANALYSIS DATE........ %s\n", time_string);
    fprintf(output->file_ptr, "     PROBLEM SIZE......... %d NODES, %d ELEMENTS, %d DOF\n",
            g_num_nodes, g_num_elements, g_total_dof);

    return FEM_SUCCESS;
}
```

#### 7.2.2 変位出力

```c
fem_error_t output_write_nastran_f06_displacements(output_control_t *output) {
    fprintf(output->file_ptr, "1%134sD I S P L A C E M E N T   V E C T O R\n", "");
    fprintf(output->file_ptr, "%134sPAGE    2\n", "");
    fprintf(output->file_ptr, "0\n");

    fprintf(output->file_ptr,
        "      POINT ID.   TYPE          T1             T2             T3             R1             R2             R3\n");

    for (int node_id = 0; node_id < g_num_nodes; node_id++) {
        int dof_x = node_id * 2;
        int dof_y = node_id * 2 + 1;

        fprintf(output->file_ptr,
            "%14d      G       %14.6E %14.6E %14.6E %14.6E %14.6E %14.6E\n",
            node_id + 1,
            g_global_displ[dof_x],    // T1 (X方向変位)
            g_global_displ[dof_y],    // T2 (Y方向変位)
            0.0,                      // T3 (Z方向変位 - 2Dなので0)
            0.0, 0.0, 0.0            // 回転変位（2Dなので0）
        );
    }

    return FEM_SUCCESS;
}
```

---

## 8. 入出力システム

### 8.1 ファイル形式自動判定

```c
fem_error_t input_detect_format(const char *filename, input_format_t *format) {
    FILE *file = fopen(filename, "r");
    if (!file) return FEM_ERROR_FILE_IO;

    char line[MAX_LINE_LENGTH];
    *format = INPUT_FORMAT_NATIVE; // デフォルト

    while (fgets(line, sizeof(line), file)) {
        // Nastran形式の特徴的キーワード検出
        if (strncmp(line, "SOL", 3) == 0 ||
            strncmp(line, "BEGIN BULK", 10) == 0 ||
            strncmp(line, "GRID", 4) == 0 ||
            strncmp(line, "CTRIA", 5) == 0 ||
            strncmp(line, "CQUAD", 5) == 0) {
            *format = INPUT_FORMAT_NASTRAN;
            break;
        }
    }

    fclose(file);
    printf("Detected file format: %s\n",
           (*format == INPUT_FORMAT_NASTRAN) ? "Nastran" : "Native");

    return FEM_SUCCESS;
}
```

**補足**:
- parser出力パッケージの場合は「ディレクトリ指定」で検出します。
- Nastranはサブセット対応です。対応カードは「Nastran互換性」章を参照してください。

### 8.2 マルチ出力対応

```c
fem_error_t static_write_results(const char* output_filename) {
    fem_error_t err;
    char vtk_filename[MAX_FILENAME_LEN];
    char f06_filename[MAX_FILENAME_LEN];

    printf("  Writing results to: %s\n", output_filename);

    // 標準結果出力
    err = output_write_results(output_filename);
    CHECK_ERROR(err);

    // VTKファイル名生成
    strcpy(vtk_filename, output_filename);
    char* dot = strrchr(vtk_filename, '.');
    if (dot) {
        strcpy(dot, ".vtk");
    } else {
        strcat(vtk_filename, ".vtk");
    }

    // VTK出力
    printf("  Writing VTK results to: %s\n", vtk_filename);
    err = output_write_vtk_file(vtk_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: VTK output failed, continuing...\n");
    }

    // F06ファイル名生成
    strcpy(f06_filename, output_filename);
    dot = strrchr(f06_filename, '.');
    if (dot) {
        strcpy(dot, ".f06");
    } else {
        strcat(f06_filename, ".f06");
    }

    // F06出力
    printf("  Writing Nastran F06 results to: %s\n", f06_filename);
    err = output_write_nastran_f06_file(f06_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: F06 output failed, continuing...\n");
    }

    return FEM_SUCCESS;
}
```

### 8.3 parser出力パッケージ入力

parserが生成するディレクトリ構成（`mesh/material/boundary`）を直接読み込みます。
Nastran入力を与えた場合は、`fem4c` が parser を実行してからこの形式を読み込みます。

```
<out_root>/<part_name>/
  ├── mesh/mesh.dat
  ├── material/material.dat
  └── Boundary Conditions/boundary.dat
```

主な読み込みルール:
- 2D解析を前提とし、Z方向の拘束・荷重は無視します。
- 材料厚さは unit thickness (1.0) を採用します。
- `material/material.dat` の E は N/mm^2、密度は kg/mm^3 を想定します。

### 8.4 VTK出力実装

```c
fem_error_t output_write_vtk_file(const char *filename) {
    output_control_t output;
    fem_error_t err;

    err = output_open_file(&output, filename, OUTPUT_FORMAT_VTK);
    CHECK_ERROR(err);

    // VTKヘッダー
    fprintf(output.file_ptr, "# vtk DataFile Version 3.0\n");
    fprintf(output.file_ptr, "FEM4C Analysis Results: %s\n", g_analysis.title);
    fprintf(output.file_ptr, "ASCII\n");
    fprintf(output.file_ptr, "DATASET UNSTRUCTURED_GRID\n\n");

    // 節点座標出力
    fprintf(output.file_ptr, "POINTS %d float\n", g_num_nodes);
    for (int i = 0; i < g_num_nodes; i++) {
        fprintf(output.file_ptr, "%f %f %f\n",
                g_node_coords[i][0], g_node_coords[i][1], g_node_coords[i][2]);
    }

    // 要素接続性出力
    int total_size = 0;
    for (int i = 0; i < g_num_elements; i++) {
        switch (g_element_type[i]) {
            case ELEMENT_T3: total_size += 4; break; // 3 + 1(サイズ)
            case ELEMENT_Q4: total_size += 5; break; // 4 + 1(サイズ)
            case ELEMENT_T6: total_size += 7; break; // 6 + 1(サイズ)
        }
    }

    fprintf(output.file_ptr, "\nCELLS %d %d\n", g_num_elements, total_size);
    for (int i = 0; i < g_num_elements; i++) {
        switch (g_element_type[i]) {
            case ELEMENT_T3:
                fprintf(output.file_ptr, "3 %d %d %d\n",
                        g_element_nodes[i][0], g_element_nodes[i][1], g_element_nodes[i][2]);
                break;
            case ELEMENT_Q4:
                fprintf(output.file_ptr, "4 %d %d %d %d\n",
                        g_element_nodes[i][0], g_element_nodes[i][1],
                        g_element_nodes[i][2], g_element_nodes[i][3]);
                break;
            case ELEMENT_T6:
                fprintf(output.file_ptr, "6 %d %d %d %d %d %d\n",
                        g_element_nodes[i][0], g_element_nodes[i][1], g_element_nodes[i][2],
                        g_element_nodes[i][3], g_element_nodes[i][4], g_element_nodes[i][5]);
                break;
        }
    }

    // 要素タイプ出力
    fprintf(output.file_ptr, "\nCELL_TYPES %d\n", g_num_elements);
    for (int i = 0; i < g_num_elements; i++) {
        switch (g_element_type[i]) {
            case ELEMENT_T3: fprintf(output.file_ptr, "5\n"); break;  // VTK_TRIANGLE
            case ELEMENT_Q4: fprintf(output.file_ptr, "9\n"); break;  // VTK_QUAD
            case ELEMENT_T6: fprintf(output.file_ptr, "22\n"); break; // VTK_QUADRATIC_TRIANGLE
        }
    }

    // 節点データ（変位）
    fprintf(output.file_ptr, "\nPOINT_DATA %d\n", g_num_nodes);
    fprintf(output.file_ptr, "VECTORS Displacement float\n");
    for (int i = 0; i < g_num_nodes; i++) {
        int dof_x = i * 2;
        int dof_y = i * 2 + 1;
        fprintf(output.file_ptr, "%e %e %e\n",
                g_global_displ[dof_x], g_global_displ[dof_y], 0.0);
    }

    err = output_close_file(&output);
    return err;
}
```

---

## 9. 使用方法

### 9.1 コンパイルとインストール

#### 9.1.1 標準コンパイル

```bash
# 基本コンパイル
make

# OpenMP並列版
make openmp

# デバッグ版
make debug

# 最適化版
make release
```

#### 9.1.2 Makefile設定

```makefile
# コンパイラ設定
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

# OpenMP設定
OPENMP_FLAGS = -fopenmp -DOPENMP_ENABLED

# ディレクトリ設定
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# ソースファイル
SOURCES = $(shell find $(SRCDIR) -name "*.c")
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# ターゲット
TARGET = $(BINDIR)/fem4c

# 基本ターゲット
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ -lm

# OpenMPターゲット
openmp: CFLAGS += $(OPENMP_FLAGS)
openmp: $(TARGET)

# オブジェクトファイル生成
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
```

### 9.2 基本的な使用法

#### 9.2.1 コマンドライン実行

```bash
# 基本実行
./bin/fem4c input.dat output.out

# Nastran入力を一括実行（parser → solver）
./bin/fem4c model.nas run_out part_0001 results.out

# parser出力パッケージ実行
./bin/fem4c <parser出力ディレクトリ>

# 環境変数でスレッド数制御（OpenMPビルド時のみ）
export OMP_NUM_THREADS=4
./bin/fem4c large_model.nas run_out part_0001 results.out
```

#### 9.2.2 入力ファイル例

**Native形式**:
```
# FEM4C Native Format
TITLE Simple T3 Test
NODES 3
ELEMENTS 1
MATERIALS 1

# Node coordinates
NODE 1 0.0 0.0 0.0
NODE 2 1.0 0.0 0.0
NODE 3 0.5 1.0 0.0

# Element definition
ELEMENT T3 1 1 1 2 3

# Material properties (E, nu, thickness)
MATERIAL 1 2.1e5 0.3 1.0

# Boundary conditions
BC 1 1 2 0.0
BC 2 1 2 0.0

# Loads
LOAD 3 1 1000.0 0.0 0.0
```

**Nastran形式**:
```
$ Simple Nastran Model
ID FEMAP,FEMAP
SOL 101
CEND
BEGIN BULK
$ Nodes
GRID    1               0.0     0.0     0.0
GRID    2               1.0     0.0     0.0
GRID    3               0.5     1.0     0.0
$ Elements
CTRIA3  1       1       1       2       3
$ Material
MAT1    1       2.1E5           0.3
$ Boundary Conditions
SPC     1       1       123     0.0
SPC     1       2       123     0.0
$ Loads
FORCE   1       3       0       1000.0  1.0     0.0     0.0
ENDDATA
```

### 9.3 出力ファイル

#### 9.3.1 標準出力ファイル

```
FEM4C - High Performance Finite Element Method in C
Analysis Results
=====================================

Analysis Title: Simple T3 Test
Date/Time:      2025-09-27 01:33:08
Input File:     input.dat

Problem Size:
  Number of nodes:     3
  Number of elements:  1
  Number of materials: 1
  Total DOF:           6

Nodal Displacements:
====================
Node      UX           UY           UZ
----  -----------  -----------  -----------
   1   0.0000e+00   0.0000e+00   0.0000e+00
   2   9.7381e-03   2.0311e-03   0.0000e+00
   3   1.9250e-02   3.4284e-03   0.0000e+00

Element Stresses:
=================
Elem     SigmaX       SigmaY       TauXY
----  -----------  -----------  -----------
   1   2.0559e+03   5.5925e+01   5.5925e+01

Analysis Summary:
=================
Solver Information:
  Iterations:     4
  Final residual: 1.525899e-13
  Elapsed time:   1.946 sec
  Status:         SUCCESS
```

#### 9.3.2 VTK出力ファイル

VTK形式は主要な可視化ソフトウェア（ParaView、VisIt等）で読み込み可能：

```
# vtk DataFile Version 3.0
FEM4C Analysis Results: Simple T3 Test
ASCII
DATASET UNSTRUCTURED_GRID

POINTS 3 float
0.000000 0.000000 0.000000
1.000000 0.000000 0.000000
0.500000 1.000000 0.000000

CELLS 1 4
3 0 1 2

CELL_TYPES 1
5

POINT_DATA 3
VECTORS Displacement float
0.000000e+00 0.000000e+00 0.000000e+00
9.738066e-03 2.031089e-03 0.000000e+00
1.925044e-02 3.428361e-03 0.000000e+00
```

### 9.4 パフォーマンス調整

#### 9.4.1 OpenMP設定

```bash
# スレッド数設定
export OMP_NUM_THREADS=8

# スケジューリング方式
export OMP_SCHEDULE=static

# プロセッサ親和性
export OMP_PROC_BIND=close

# メモリアロケーション
export OMP_PLACES=cores
```

#### 9.4.2 メモリ使用量目安

| 問題サイズ | 節点数 | 要素数 | メモリ使用量（概算） |
|------------|--------|--------|----------------------|
| 小規模     | ~1,000 | ~2,000 | ~10 MB              |
| 中規模     | ~10,000| ~20,000| ~100 MB             |
| 大規模     | ~100,000|~200,000| ~1 GB               |

---

## 10. 性能評価

### 10.1 計算効率

#### 10.1.1 アルゴリズム計算量

**要素剛性行列計算**:
- T3要素: O(1) - 解析的計算
- Q4要素: O(1) - 2×2ガウス積分
- T6要素: O(1) - 3点ガウス積分

**全体アセンブリ**: O(N_elem × DOF²)

**連立方程式求解**: O(N_dof × 反復回数)

#### 10.1.2 並列化効率測定

```c
double measure_parallel_efficiency(int num_threads) {
    double start_time, end_time;

    // シリアル実行時間測定
    omp_set_num_threads(1);
    start_time = omp_get_wtime();
    assembly_parallel_stiffness_matrix();
    end_time = omp_get_wtime();
    double serial_time = end_time - start_time;

    // 並列実行時間測定
    omp_set_num_threads(num_threads);
    start_time = omp_get_wtime();
    assembly_parallel_stiffness_matrix();
    end_time = omp_get_wtime();
    double parallel_time = end_time - start_time;

    // 効率計算
    double efficiency = serial_time / (num_threads * parallel_time);

    printf("Parallel efficiency (%d threads): %.2f%%\n",
           num_threads, efficiency * 100.0);

    return efficiency;
}
```

### 10.2 ベンチマーク結果

#### 10.2.1 実測データ（混合要素モデル）

| 項目 | 1スレッド | 4スレッド | 8スレッド |
|------|-----------|-----------|-----------|
| 実行時間 | 1.846s | 1.338s | 1.573s |
| 高速化比 | 1.00 | 1.38 | 1.17 |
| 効率 | 100% | 34.5% | 14.6% |

#### 10.2.2 スケーラビリティ分析

**理想的並列化との比較**:
- 4スレッド理論値: 1.846/4 = 0.462s
- 4スレッド実測値: 1.338s
- 並列化率: ~30%（Amdahlの法則による）

**ボトルネック要因**:
1. シリアル部分（アセンブリ、I/O）
2. メモリ帯域幅の制約
3. キャッシュミスの増加
4. 同期オーバーヘッド

### 10.3 メモリ使用量解析

#### 10.3.1 メモリプロファイリング

```c
void print_memory_usage(void) {
    size_t stiffness_memory = g_total_dof * g_total_dof * sizeof(double);
    size_t vector_memory = g_total_dof * sizeof(double) * 2; // force + displ
    size_t element_memory = g_num_elements * sizeof(element_data_t);
    size_t node_memory = g_num_nodes * 3 * sizeof(double);

    size_t total_memory = stiffness_memory + vector_memory +
                         element_memory + node_memory;

    printf("Memory Usage Analysis:\n");
    printf("  Global stiffness matrix: %.2f MB\n",
           stiffness_memory / (1024.0 * 1024.0));
    printf("  Global vectors:          %.2f MB\n",
           vector_memory / (1024.0 * 1024.0));
    printf("  Element data:            %.2f MB\n",
           element_memory / (1024.0 * 1024.0));
    printf("  Node coordinates:        %.2f MB\n",
           node_memory / (1024.0 * 1024.0));
    printf("  Total estimated:         %.2f MB\n",
           total_memory / (1024.0 * 1024.0));
}
```

---

## 11. トラブルシューティング

### 11.1 よくあるエラーと解決法

#### 11.1.1 コンパイルエラー

**エラー**: `undefined reference to 'omp_get_max_threads'`
```bash
# 解決法: OpenMPリンクフラグを追加
gcc -fopenmp -o fem4c *.c -lm
```

**エラー**: `implicit declaration of function`
```c
// 解決法: 適切なヘッダーファイルをインクルード
#ifdef _OPENMP
#include <omp.h>
#endif
```

#### 11.1.2 実行時エラー

**エラー**: `FEM_ERROR_MEMORY_ALLOCATION`
```
原因: メモリ不足
解決法:
1. 問題サイズを縮小
2. システムメモリを増設
3. スワップファイルを設定
```

**エラー**: `FEM_ERROR_CONVERGENCE_FAILED`
```
原因: 悪条件の剛性行列
解決法:
1. 要素品質をチェック
2. 境界条件を見直し
3. 材料定数を確認
```

**エラー**: `Invalid element geometry`
```
原因: 縮退要素（面積ゼロ）
解決法:
1. 節点座標を確認
2. 要素接続性を見直し
3. メッシュ品質を改善
```

#### 11.1.3 Nastran入力エラー

**エラー**: `Invalid double field`
```
原因: フィールド解析エラー
解決法:
1. 固定フィールド形式を確認（8文字単位）
2. 数値フォーマットをチェック
3. 空白文字の配置を確認
```

### 11.2 デバッグ機能

#### 11.2.1 デバッグ情報出力

```c
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "DEBUG %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

void debug_print_matrix(double **matrix, int size, const char *name) {
#ifdef DEBUG
    printf("Matrix %s (%dx%d):\n", name, size, size);
    for (int i = 0; i < MIN(size, 5); i++) {
        for (int j = 0; j < MIN(size, 5); j++) {
            printf("%12.4e ", matrix[i][j]);
        }
        printf("%s\n", (size > 5) ? "..." : "");
    }
    if (size > 5) printf("...\n");
#endif
}
```

#### 11.2.2 数値安定性チェック

```c
fem_error_t check_matrix_condition(double **matrix, int size) {
    // 対角要素のチェック
    double min_diag = 1e30, max_diag = 0.0;
    int zero_diag = 0;

    for (int i = 0; i < size; i++) {
        double diag = fabs(matrix[i][i]);
        if (diag < TOLERANCE) {
            zero_diag++;
        } else {
            if (diag < min_diag) min_diag = diag;
            if (diag > max_diag) max_diag = diag;
        }
    }

    double condition_estimate = max_diag / min_diag;

    printf("Matrix condition analysis:\n");
    printf("  Diagonal range: [%e, %e]\n", min_diag, max_diag);
    printf("  Condition estimate: %e\n", condition_estimate);
    printf("  Zero diagonal terms: %d\n", zero_diag);

    if (zero_diag > 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                        "Singular matrix detected (%d zero diagonal terms)", zero_diag);
    }

    if (condition_estimate > 1e12) {
        printf("  Warning: Matrix may be ill-conditioned\n");
    }

    return FEM_SUCCESS;
}
```

### 11.3 パフォーマンス問題の診断

#### 11.3.1 プロファイリング

```c
typedef struct {
    clock_t start_time;
    clock_t end_time;
    double elapsed_time;
    const char *phase_name;
} timing_info_t;

static timing_info_t timings[10];
static int timing_count = 0;

void start_timing(const char *phase_name) {
    if (timing_count < 10) {
        timings[timing_count].phase_name = phase_name;
        timings[timing_count].start_time = clock();
        timing_count++;
    }
}

void end_timing(void) {
    if (timing_count > 0) {
        int idx = timing_count - 1;
        timings[idx].end_time = clock();
        timings[idx].elapsed_time = ((double)(timings[idx].end_time - timings[idx].start_time)) / CLOCKS_PER_SEC;
    }
}

void print_timing_summary(void) {
    printf("\nTiming Summary:\n");
    printf("===============\n");
    double total_time = 0.0;

    for (int i = 0; i < timing_count; i++) {
        printf("  %-25s: %8.3f sec (%5.1f%%)\n",
               timings[i].phase_name,
               timings[i].elapsed_time,
               (timings[i].elapsed_time / g_solver_info.elapsed_time) * 100.0);
        total_time += timings[i].elapsed_time;
    }

    printf("  %-25s: %8.3f sec\n", "Total measured", total_time);
    printf("  %-25s: %8.3f sec\n", "Overall", g_solver_info.elapsed_time);
}
```

---

## 12. 開発者向け情報

### 12.1 コード構造とモジュール設計

#### 12.1.1 ディレクトリ構成

```
FEM4C/
├── src/
│   ├── common/           # 共通機能
│   │   ├── types.h       # データ型定義
│   │   ├── constants.h   # 定数定義
│   │   ├── globals.h     # グローバル変数
│   │   ├── globals.c     # グローバル変数実装
│   │   ├── error.h       # エラーハンドリング
│   │   └── error.c       # エラーハンドリング実装
│   ├── elements/         # 要素ライブラリ
│   │   ├── elements.h    # 要素管理
│   │   ├── elements.c    # 要素管理実装
│   │   ├── t3/          # T3要素
│   │   ├── q4/          # Q4要素
│   │   └── t6/          # T6要素
│   ├── solver/          # ソルバー
│   │   ├── assembly.h   # アセンブリ
│   │   ├── assembly.c   # アセンブリ実装
│   │   ├── cg_solver.h  # 共役勾配法
│   │   └── cg_solver.c  # 共役勾配法実装
│   ├── io/              # 入出力
│   │   ├── input.h      # 入力処理
│   │   ├── input.c      # 入力処理実装
│   │   ├── output.h     # 出力処理
│   │   └── output.c     # 出力処理実装
│   ├── analysis/        # 解析制御
│   │   ├── static.h     # 静解析
│   │   └── static.c     # 静解析実装
│   └── fem4c.c         # メインプログラム
├── docs/               # ドキュメント
├── examples/           # 入力例
├── practice/           # 学習用ハンズオン
├── Makefile           # ビルド設定
└── README.md          # プロジェクト概要
```

#### 12.1.2 モジュール間依存関係

```
fem4c.c
    ↓
static.c (解析制御)
    ↓
input.c → elements.c → assembly.c → cg_solver.c → output.c
    ↓         ↓           ↓            ↓           ↓
globals.c ← error.c ← types.h ← constants.h
```

### 12.2 新要素の追加方法

#### 12.2.1 要素インターフェース実装

```c
// 新要素（例：H8要素）のヘッダーファイル
// src/elements/h8/h8_element.h

#ifndef H8_ELEMENT_H
#define H8_ELEMENT_H

#include "../../common/types.h"

#define H8_NODES_PER_ELEMENT 8
#define H8_DOF_PER_NODE 3
#define H8_TOTAL_DOF (H8_NODES_PER_ELEMENT * H8_DOF_PER_NODE)

// 要素関数プロトタイプ
fem_error_t h8_element_stiffness(int element_id, double ke[][H8_TOTAL_DOF]);
fem_error_t h8_element_stress(int element_id, double *stress);
fem_error_t h8_validate_element(int element_id);
fem_error_t h8_get_element_coordinates(int element_id, double coords[][3]);

#endif /* H8_ELEMENT_H */
```

#### 12.2.2 要素登録

```c
// src/elements/elements.c に追加

#include "h8/h8_element.h"

fem_error_t elements_initialize(void) {
    // 既存要素...

    // H8要素の登録
    elements[ELEMENT_H8] = (element_interface_t){
        .type = ELEMENT_H8,
        .nodes_per_element = H8_NODES_PER_ELEMENT,
        .dof_per_node = H8_DOF_PER_NODE,
        .total_dof = H8_TOTAL_DOF,
        .stiffness = h8_element_stiffness,
        .stress = h8_element_stress,
        .validate = h8_validate_element
    };

    return FEM_SUCCESS;
}
```

### 12.3 検証のすすめ

#### 12.3.1 単体検証例

```c
// practice/tests/test_t3_element.c

#include "../src/elements/t3/t3_element.h"
#include <assert.h>
#include <math.h>

void test_t3_area_calculation(void) {
    double coords[3][2] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};
    double area = t3_element_area(coords);

    assert(fabs(area - 0.5) < 1e-10);
    printf("T3 area calculation: PASS\n");
}

void test_t3_stiffness_symmetry(void) {
    // 正方形要素での対称性テスト
    setup_test_element();

    double ke[T3_TOTAL_DOF][T3_TOTAL_DOF];
    fem_error_t err = t3_element_stiffness(0, ke);
    assert(err == FEM_SUCCESS);

    // 対称性チェック
    for (int i = 0; i < T3_TOTAL_DOF; i++) {
        for (int j = 0; j < T3_TOTAL_DOF; j++) {
            assert(fabs(ke[i][j] - ke[j][i]) < 1e-12);
        }
    }

    printf("T3 stiffness symmetry: PASS\n");
}

int main(void) {
    test_t3_area_calculation();
    test_t3_stiffness_symmetry();
    printf("All T3 tests passed!\n");
    return 0;
}
```

#### 12.3.2 統合検証

```c
// practice/tests/test_integration.c

void test_cantilever_beam(void) {
    // 標準的な片持ち梁問題
    const char *input_file = "examples/t6_cantilever_beam.dat";
    const char *output_file = "cantilever.out";

    fem_error_t err = static_analysis(input_file, output_file);
    assert(err == FEM_SUCCESS);

    // 理論解と比較
    double theoretical_tip_displacement = compute_beam_theory_displacement();
    double computed_displacement = g_global_displ[tip_node_dof];

    double error = fabs(computed_displacement - theoretical_tip_displacement) /
                   theoretical_tip_displacement;

    assert(error < 0.05); // 5%以内の誤差
    printf("Cantilever beam test: PASS (error: %.2f%%)\n", error * 100.0);
}
```

### 12.4 並列化の拡張

#### 12.4.1 ソルバー並列化

```c
// 共役勾配法の並列化例
fem_error_t cg_solve_parallel(void) {
#ifdef _OPENMP
    double *r = calloc(g_total_dof, sizeof(double));  // 残差ベクトル
    double *p = calloc(g_total_dof, sizeof(double));  // 探索方向
    double *Ap = calloc(g_total_dof, sizeof(double)); // A*p

    // 初期残差計算（並列化）
    #pragma omp parallel for
    for (int i = 0; i < g_total_dof; i++) {
        r[i] = g_global_force[i];
        for (int j = 0; j < g_total_dof; j++) {
            r[i] -= g_global_stiffness[i][j] * g_global_displ[j];
        }
        p[i] = r[i];
    }

    // CG反復（内積とベクトル演算を並列化）
    for (int iter = 0; iter < max_iterations; iter++) {
        // A*p の計算（並列化）
        #pragma omp parallel for
        for (int i = 0; i < g_total_dof; i++) {
            Ap[i] = 0.0;
            for (int j = 0; j < g_total_dof; j++) {
                Ap[i] += g_global_stiffness[i][j] * p[j];
            }
        }

        // 内積計算（reduction使用）
        double r_dot_r = 0.0, p_dot_Ap = 0.0;

        #pragma omp parallel for reduction(+:r_dot_r, p_dot_Ap)
        for (int i = 0; i < g_total_dof; i++) {
            r_dot_r += r[i] * r[i];
            p_dot_Ap += p[i] * Ap[i];
        }

        double alpha = r_dot_r / p_dot_Ap;

        // 解と残差の更新（並列化）
        #pragma omp parallel for
        for (int i = 0; i < g_total_dof; i++) {
            g_global_displ[i] += alpha * p[i];
            r[i] -= alpha * Ap[i];
        }

        // 収束判定
        double residual_norm = sqrt(r_dot_r);
        if (residual_norm < tolerance) {
            printf("  Converged in %d iterations\n", iter + 1);
            break;
        }

        // β計算と探索方向更新
        double r_new_dot_r_new = 0.0;
        #pragma omp parallel for reduction(+:r_new_dot_r_new)
        for (int i = 0; i < g_total_dof; i++) {
            r_new_dot_r_new += r[i] * r[i];
        }

        double beta = r_new_dot_r_new / r_dot_r;

        #pragma omp parallel for
        for (int i = 0; i < g_total_dof; i++) {
            p[i] = r[i] + beta * p[i];
        }
    }

    free(r); free(p); free(Ap);
    return FEM_SUCCESS;
#else
    return cg_solve_system(); // 非並列版にフォールバック
#endif
}
```

### 12.5 将来の拡張計画

#### 12.5.1 動解析対応

```c
// 動解析用データ構造
typedef struct {
    double *mass_matrix;      // 質量行列
    double *damping_matrix;   // 減衰行列
    double *velocity;         // 速度ベクトル
    double *acceleration;     // 加速度ベクトル
    double time_step;         // 時間刻み
    double total_time;        // 総時間
    int num_time_steps;       // 時間ステップ数
} dynamic_analysis_t;

// Newmark法の実装例
fem_error_t dynamic_analysis_newmark(dynamic_analysis_t *dyn) {
    double beta = 0.25, gamma = 0.5; // Newmark定数

    for (int step = 0; step < dyn->num_time_steps; step++) {
        // 予測子
        // 修正子
        // 平衡方程式求解
        // 解の更新
    }

    return FEM_SUCCESS;
}
```

#### 12.5.2 非線形解析対応

```c
// Newton-Raphson法の実装
fem_error_t nonlinear_analysis_newton_raphson(void) {
    double load_increment = 1.0 / num_load_steps;

    for (int load_step = 0; load_step < num_load_steps; load_step++) {
        double current_load = (load_step + 1) * load_increment;

        // 荷重ベクトル更新
        update_load_vector(current_load);

        // Newton-Raphson反復
        for (int iter = 0; iter < max_nr_iterations; iter++) {
            // 接線剛性行列計算
            compute_tangent_stiffness_matrix();

            // 残差計算
            compute_residual_vector();

            // 線形化方程式求解
            solve_linearized_equations();

            // 変位増分更新
            update_displacement_increment();

            // 収束判定
            if (check_convergence()) break;
        }
    }

    return FEM_SUCCESS;
}
```

---

## 付録

### A. 理論式詳細

#### A.1 T3要素の剛性行列

T3要素の剛性行列は以下の式で計算されます：

```
[K_e] = t × A × [B]^T [D] [B]
```

ここで：
- `t`: 板厚
- `A`: 要素面積
- `[B]`: ひずみ-変位関係マトリクス
- `[D]`: 材料マトリクス

#### A.2 Q4要素の数値積分

Q4要素では2×2ガウス積分を使用：

```
[K_e] = ∫∫ [B]^T [D] [B] |J| dξ dη
      ≈ Σ Σ [B(ξ_i,η_j)]^T [D] [B(ξ_i,η_j)] |J(ξ_i,η_j)| w_i w_j
```

積分点：ξ_i, η_j = ±1/√3
重み：w_i = w_j = 1

### B. エラーコード一覧

| コード | 名前 | 説明 |
|--------|------|------|
| 0 | FEM_SUCCESS | 正常終了 |
| 1 | FEM_ERROR_MEMORY_ALLOCATION | メモリ確保エラー |
| 2 | FEM_ERROR_FILE_IO | ファイル入出力エラー |
| 3 | FEM_ERROR_INVALID_INPUT | 不正な入力データ |
| 4 | FEM_ERROR_CONVERGENCE_FAILED | 収束失敗 |
| 5 | FEM_ERROR_INVALID_ELEMENT_TYPE | 不正な要素タイプ |
| 6 | FEM_ERROR_INVALID_MATERIAL | 不正な材料特性 |
| 7 | FEM_ERROR_INVALID_BOUNDARY_CONDITION | 不正な境界条件 |

### C. 参考文献

1. 山田貴博, "高性能有限要素法", 朝倉書店
2. Zienkiewicz, O.C., Taylor, R.L., "The Finite Element Method", Butterworth-Heinemann
3. Hughes, T.J.R., "The Finite Element Method", Dover Publications
4. OpenMP Architecture Review Board, "OpenMP API Specification"
5. MSC Software, "MSC Nastran Quick Reference Guide"

---

**FEM4C Reference Manual v1.0**
**© 2025 FEM4C Development Team**
