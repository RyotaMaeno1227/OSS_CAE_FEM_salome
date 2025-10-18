# FEM4C 設計書・詳細タスク表

## 1. 設計概要

### 1.1 アーキテクチャ設計
```
[入力] → [メッシュ管理] → [要素処理] → [ソルバー] → [出力]
   ↓         ↓            ↓           ↓        ↓
 Nastran   節点・要素    T6要素      線形求解   結果出力
 ファイル   データ管理    剛性行列     CG法      変位・応力
```

### 1.2 データフロー
1. 入力ファイル読み込み → メッシュデータ構築
2. 材料特性設定 → 要素剛性行列計算
3. 全体剛性行列組立 → 境界条件適用
4. 線形方程式求解 → 変位計算
5. 応力計算 → 結果出力

## 2. モジュール設計

### 2.1 共通モジュール設計

#### constants.h 設計
```c
#ifndef CONSTANTS_H
#define CONSTANTS_H

// システム制限
#define MAX_NODES           100000
#define MAX_ELEMENTS        50000
#define MAX_MATERIALS       100
#define MAX_FILENAME_LEN    256
#define MAX_TITLE_LEN       80

// 数値定数
#define TOLERANCE           1.0e-12
#define MAX_ITERATIONS      10000
#define PI                  3.14159265358979323846

// 要素タイプ定数
#define ELEMENT_T6          6
#define ELEMENT_Q4          4
#define ELEMENT_H8          8

// 材料タイプ定数
#define MATERIAL_ISOTROPIC  1
#define MATERIAL_ORTHOTROPIC 2

#endif
```

#### types.h 設計
```c
#ifndef TYPES_H
#define TYPES_H

// 基本データ型
typedef struct {
    int id;
    double coords[3];
    double displ[3];
    double force[3];
    int bc_flags[3];
} node_t;

typedef struct {
    int id;
    int type;
    int nodes[10];  // 最大10節点要素対応
    int material_id;
} element_t;

typedef struct {
    int id;
    int type;
    double young_modulus;
    double poisson_ratio;
    double thickness;
} material_t;

// エラーコード
typedef enum {
    FEM_SUCCESS = 0,
    FEM_ERROR_FILE_NOT_FOUND,
    FEM_ERROR_MEMORY_ALLOCATION,
    FEM_ERROR_INVALID_INPUT,
    FEM_ERROR_CONVERGENCE_FAILED,
    FEM_ERROR_SINGULAR_MATRIX
} fem_error_t;

#endif
```

### 2.2 T6要素設計

#### t6_element.h インターフェース
```c
#ifndef T6_ELEMENT_H
#define T6_ELEMENT_H

#include "types.h"

// T6要素特有の定数
#define T6_NODES_PER_ELEMENT  6
#define T6_DOF_PER_NODE       2
#define T6_TOTAL_DOF          12
#define T6_GAUSS_POINTS       3

// 形状関数・微分計算
fem_error_t t6_shape_functions(double xi, double eta, double N[6]);
fem_error_t t6_shape_derivatives(double xi, double eta, double dN[6][2]);

// 剛性行列計算
fem_error_t t6_element_stiffness(int element_id, 
                                 double ke[T6_TOTAL_DOF][T6_TOTAL_DOF]);

// ひずみ-変位関係
fem_error_t t6_strain_displacement(int element_id, double B[3][T6_TOTAL_DOF]);

// 応力計算
fem_error_t t6_element_stress(int element_id, double stress[3]);

#endif
```

### 2.3 ソルバー設計

#### linear_solver.h インターフェース
```c
#ifndef LINEAR_SOLVER_H
#define LINEAR_SOLVER_H

// 共役勾配法
fem_error_t solve_cg(double *A, double *b, double *x, int n, 
                     double tolerance, int max_iter, int *iterations);

// 前処理共役勾配法
fem_error_t solve_pcg(double *A, double *b, double *x, int n,
                      double tolerance, int max_iter, int *iterations);

// 直接法（小規模問題用）
fem_error_t solve_direct(double *A, double *b, double *x, int n);

#endif
```

## 3. 詳細タスク表

### Phase 1: T6要素基本フレームワーク（2週間）

| Task ID | タスク名 | 担当 | 工数 | 前提条件 | 成果物 |
|---------|----------|------|------|----------|---------|
| **1.1 基盤整備** |
| 1.1.1 | ディレクトリ構造作成 | Dev | 0.5日 | - | ディレクトリ一式 |
| 1.1.2 | 共通ヘッダーファイル作成 | Dev | 1日 | 1.1.1 | constants.h, types.h |
| 1.1.3 | グローバル変数定義 | Dev | 0.5日 | 1.1.2 | globals.h, globals.c |
| 1.1.4 | エラーハンドリング実装 | Dev | 1日 | 1.1.2 | error.h, error.c |
| 1.1.5 | Makefile作成 | Dev | 0.5日 | 1.1.1-4 | Makefile |
| **1.2 入出力モジュール** |
| 1.2.1 | 基本入力関数実装 | Dev | 1日 | 1.1完了 | input.h, input.c |
| 1.2.2 | 簡易データ読み込み | Dev | 1日 | 1.2.1 | T6テストデータ対応 |
| 1.2.3 | 基本出力関数実装 | Dev | 0.5日 | 1.2.1 | output.h, output.c |
| **1.3 T6要素実装** |
| 1.3.1 | T6形状関数実装 | Dev | 1日 | 1.1完了 | t6_element.c基本部分 |
| 1.3.2 | T6要素剛性行列計算 | Dev | 2日 | 1.3.1 | t6_stiffness.c |
| 1.3.3 | ガウス積分実装 | Dev | 1日 | 1.3.1 | ガウス点・重み計算 |
| 1.3.4 | T6単体テスト | Dev | 1日 | 1.3.1-3 | テストケース |
| **1.4 基本ソルバー** |
| 1.4.1 | 剛性行列組立実装 | Dev | 1.5日 | 1.3完了 | assembly.c |
| 1.4.2 | 共役勾配法実装 | Dev | 2日 | 1.4.1 | cg_solver.c |
| 1.4.3 | 境界条件処理 | Dev | 1日 | 1.4.1 | 拘束・荷重処理 |
| **1.5 統合テスト** |
| 1.5.1 | 単一要素テスト | Dev | 1日 | 1.4完了 | T6要素1個の解析 |
| 1.5.2 | 複数要素テスト | Dev | 1日 | 1.5.1 | 簡単なメッシュ |
| 1.5.3 | Fortranとの結果比較 | Dev | 1日 | 1.5.2 | 検証レポート |

### Phase 2: 2D要素拡張（3週間）

| Task ID | タスク名 | 担当 | 工数 | 前提条件 | 成果物 |
|---------|----------|------|------|----------|---------|
| **2.1 要素基盤クラス** |
| 2.1.1 | 要素共通インターフェース設計 | Dev | 1日 | Phase1完了 | element_base.h |
| 2.1.2 | 要素判別機能実装 | Dev | 1日 | 2.1.1 | elements.c |
| **2.2 Q4要素実装** |
| 2.2.1 | Q4形状関数実装 | Dev | 1日 | 2.1完了 | q4_element.c |
| 2.2.2 | Q4剛性行列計算 | Dev | 2日 | 2.2.1 | q4_stiffness.c |
| 2.2.3 | Q4単体テスト | Dev | 1日 | 2.2.2 | テストケース |
| **2.3 T3要素実装** |
| 2.3.1 | T3形状関数実装 | Dev | 0.5日 | 2.1完了 | t3_element.c |
| 2.3.2 | T3剛性行列計算 | Dev | 1日 | 2.3.1 | t3_stiffness.c |
| **2.4 その他2D要素** |
| 2.4.1 | Q9要素実装 | Dev | 2日 | 2.2完了 | q9関連ファイル |
| 2.4.2 | S8要素実装 | Dev | 2日 | 2.2完了 | s8関連ファイル |
| **2.5 材料特性拡張** |
| 2.5.1 | 平面ひずみ対応 | Dev | 1日 | Phase1完了 | material.c拡張 |
| 2.5.2 | 厚さ考慮実装 | Dev | 1日 | 2.5.1 | プレート要素対応 |
| **2.6 統合テスト** |
| 2.6.1 | 混合要素メッシュテスト | Dev | 2日 | 2.4完了 | 複合メッシュ検証 |
| 2.6.2 | 2D要素性能テスト | Dev | 2日 | 2.6.1 | 性能ベンチマーク |

### Phase 3: 3D要素対応（3週間）

| Task ID | タスク名 | 担当 | 工数 | 前提条件 | 成果物 |
|---------|----------|------|------|----------|---------|
| **3.1 3D基盤整備** |
| 3.1.1 | 3次元データ構造拡張 | Dev | 1日 | Phase2完了 | 3D対応globals.h |
| 3.1.2 | 3次元ガウス積分 | Dev | 1日 | 3.1.1 | 3Dガウス点計算 |
| **3.2 H8要素実装** |
| 3.2.1 | H8形状関数実装 | Dev | 1.5日 | 3.1完了 | h8_element.c |
| 3.2.2 | H8剛性行列計算 | Dev | 2.5日 | 3.2.1 | h8_stiffness.c |
| 3.2.3 | H8単体テスト | Dev | 1日 | 3.2.2 | テストケース |
| **3.3 T4要素実装** |
| 3.3.1 | T4形状関数実装 | Dev | 1日 | 3.1完了 | t4_element.c |
| 3.3.2 | T4剛性行列計算 | Dev | 1.5日 | 3.3.1 | t4_stiffness.c |
| **3.4 T10要素実装** |
| 3.4.1 | T10形状関数実装 | Dev | 2日 | 3.3完了 | t10_element.c |
| 3.4.2 | T10剛性行列計算 | Dev | 3日 | 3.4.1 | t10_stiffness.c |
| **3.5 3D統合テスト** |
| 3.5.1 | 3D要素単体テスト | Dev | 2日 | 3.4完了 | 各要素検証 |
| 3.5.2 | 3D混合メッシュテスト | Dev | 2日 | 3.5.1 | 3D複合メッシュ |
| 3.5.3 | 3D性能テスト | Dev | 2日 | 3.5.2 | 3D性能検証 |

### Phase 4: Nastran対応・最終化（2週間）

| Task ID | タスク名 | 担当 | 工数 | 前提条件 | 成果物 |
|---------|----------|------|------|----------|---------|
| **4.1 Nastranファイル対応** |
| 4.1.1 | Nastranパーサー実装 | Dev | 3日 | Phase3完了 | nastran.c |
| 4.1.2 | 要素自動判別実装 | Dev | 1日 | 4.1.1 | 要素タイプ判別 |
| 4.1.3 | Nastran入力テスト | Dev | 1日 | 4.1.2 | 入力検証 |
| **4.2 OpenMP並列化** |
| 4.2.1 | 要素剛性計算並列化 | Dev | 2日 | Phase3完了 | OpenMP対応 |
| 4.2.2 | 行列組立並列化 | Dev | 2日 | 4.2.1 | assembly並列化 |
| 4.2.3 | 並列性能測定 | Dev | 1日 | 4.2.2 | 性能レポート |
| **4.3 最終検証** |
| 4.3.1 | 全要素回帰テスト | Dev | 2日 | 4.2完了 | 回帰テスト一式 |
| 4.3.2 | ドキュメント整備 | Dev | 1日 | 4.3.1 | ユーザーマニュアル |

## 4. リスク管理

### 4.1 技術リスク
| リスク | 影響度 | 発生確率 | 対策 |
|--------|--------|----------|------|
| T6要素計算精度不一致 | 高 | 中 | Fortranコードとの詳細比較 |
| メモリ不足 | 中 | 低 | 固定配列サイズ調整 |
| OpenMP性能劣化 | 中 | 中 | 並列化戦略見直し |
| Nastran互換性問題 | 高 | 中 | 段階的実装・テスト強化 |

### 4.2 スケジュールリスク
| リスク | 影響度 | 発生確率 | 対策 |
|--------|--------|----------|------|
| Phase1遅延 | 高 | 中 | バッファ1週間確保 |
| 3D要素実装困難 | 中 | 低 | 2D要素完成を優先 |
| 統合テスト時間不足 | 中 | 中 | 各Phaseでテスト強化 |

## 5. 品質保証

### 5.1 テスト戦略
- **ユニットテスト**: 各関数レベル
- **統合テスト**: モジュール間連携
- **回帰テスト**: Fortranとの結果比較
- **性能テスト**: OpenMP効果測定

### 5.2 コードレビュー
- Phase完了時に設計レビュー実施
- 重要関数のコードレビュー
- ドキュメント整合性チェック

---
**作成日**: 2025-09-24  
**バージョン**: 1.0  
**総タスク数**: 59タスク  
**総工数**: 約70人日（10週間）