# FEM4C ファイル構成書

## 1. ディレクトリ構造

```
FEM4C/
├── docs/                     # ドキュメント
│   ├── 01_requirements.md    # 要件定義書
│   ├── 02_file_structure.md  # ファイル構成書
│   ├── 03_design.md         # 設計書
│   └── 04_progress.md       # 進捗確認書
├── src/                     # ソースコード
│   ├── common/              # 共通モジュール
│   │   ├── constants.h      # 定数定義
│   │   ├── types.h          # データ型定義
│   │   ├── globals.h        # グローバル変数定義
│   │   ├── utils.c/h        # ユーティリティ関数
│   │   ├── memory.c/h       # メモリ管理
│   │   └── error.c/h        # エラーハンドリング
│   ├── io/                  # 入出力モジュール
│   │   ├── input.c/h        # データ入力
│   │   ├── nastran.c/h      # Nastranファイル読み込み
│   │   └── output.c/h       # 結果出力
│   ├── mesh/                # メッシュ管理
│   │   ├── mesh.c/h         # メッシュデータ構造
│   │   ├── elements.c/h     # 要素定義
│   │   └── nodes.c/h        # 節点定義
│   ├── material/            # 材料特性
│   │   ├── material.c/h     # 材料定義
│   │   └── constitutive.c/h # 構成則
│   ├── elements/            # 要素ライブラリ
│   │   ├── element_base.c/h # 要素基底クラス
│   │   ├── t6/              # T6要素
│   │   │   ├── t6_element.c/h
│   │   │   ├── t6_stiffness.c/h
│   │   │   └── t6_strain.c/h
│   │   ├── q4/              # Q4要素（将来）
│   │   ├── h8/              # H8要素（将来）
│   │   └── ...              # その他要素
│   ├── solver/              # ソルバーモジュール
│   │   ├── assembly.c/h     # 剛性行列組立
│   │   ├── linear_solver.c/h # 線形方程式求解
│   │   ├── cg_solver.c/h    # 共役勾配法
│   │   └── sparse_matrix.c/h # 疎行列操作
│   ├── analysis/            # 解析制御
│   │   ├── static.c/h       # 静解析
│   │   └── analysis_driver.c/h # 解析ドライバー
│   └── fem4c.c              # メイン関数
├── test/                    # テストケース
│   ├── data/               # テストデータ
│   │   ├── t6_simple.dat   # T6要素テスト
│   │   └── ...
│   ├── unit/               # ユニットテスト
│   └── regression/         # 回帰テスト
├── examples/               # 使用例
├── build/                  # ビルドディレクトリ
├── Makefile               # Makefile
├── CMakeLists.txt         # CMake設定
└── README.md              # プロジェクト説明
```

## 2. ソースファイル詳細

### 2.1 共通モジュール (src/common/)

#### constants.h
```c
// 数値定数、物理定数、収束判定値等
#define MAX_NODES 100000
#define MAX_ELEMENTS 50000
#define MAX_MATERIALS 100
#define TOLERANCE 1.0e-12
```

#### types.h
```c
// データ型定義
typedef struct {
    int id;
    double coords[3];
    double displ[3];
    int bc_flags[3];
} node_t;
```

#### globals.h
```c
// グローバル変数宣言
extern double node_coords[MAX_NODES][3];
extern double node_displ[MAX_NODES][3];
extern int element_nodes[MAX_ELEMENTS][MAX_NODES_PER_ELEM];
```

### 2.2 要素ライブラリ (src/elements/)

#### T6要素実装
- **t6_element.c/h**: T6要素の基本定義
- **t6_stiffness.c/h**: 剛性行列計算
- **t6_strain.c/h**: ひずみ-変位関係

### 2.3 ソルバーモジュール (src/solver/)

#### assembly.c/h
```c
int assemble_global_stiffness(void);
int assemble_force_vector(void);
```

#### linear_solver.c/h
```c
int solve_linear_system(double *K, double *f, double *u, int n);
```

## 3. ヘッダーファイル依存関係

```
fem4c.c
├── analysis/analysis_driver.h
│   ├── solver/assembly.h
│   │   ├── elements/element_base.h
│   │   │   └── elements/t6/t6_element.h
│   │   └── mesh/mesh.h
│   └── solver/linear_solver.h
├── io/input.h
│   └── io/nastran.h
├── io/output.h
└── common/
    ├── constants.h
    ├── types.h
    ├── globals.h
    └── error.h
```

## 4. コンパイル設定

### 4.1 Makefile構成
```makefile
CC = gcc
CFLAGS = -Wall -O3 -fopenmp -std=c99
INCLUDES = -Isrc/common -Isrc/elements -Isrc/solver
SRCDIR = src
OBJDIR = build
```

### 4.2 CMakeLists.txt構成
```cmake
cmake_minimum_required(VERSION 3.10)
project(FEM4C)
set(CMAKE_C_STANDARD 99)
find_package(OpenMP REQUIRED)
```

## 5. テスト構成

### 5.1 ユニットテスト
- 各モジュール単体のテスト
- CUnitフレームワーク使用

### 5.2 回帰テスト
- Fortranソルバーとの結果比較
- 自動化スクリプトによる実行

## 6. ドキュメント構成

### 6.1 設計ドキュメント
- API仕様書
- アルゴリズム説明書
- 実装ガイド

### 6.2 ユーザードキュメント
- インストールガイド
- 使用方法
- 入力ファイル仕様

## 7. ファイル命名規則

### 7.1 ファイル名
- C源ファイル: `module_name.c`
- ヘッダファイル: `module_name.h`
- テストファイル: `test_module_name.c`

### 7.2 関数名
- 公開関数: `module_function_name()`
- 静的関数: `_function_name()`

### 7.3 変数名
- グローバル変数: `g_variable_name`
- ローカル変数: `variable_name`
- 定数: `CONSTANT_NAME`

---
**作成日**: 2025-09-24  
**バージョン**: 1.0  
**ステータス**: 設計完了