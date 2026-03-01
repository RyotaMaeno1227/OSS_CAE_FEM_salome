# FEM4C 実装ガイド（モジュール別）

このドキュメントは、FEM4C を「自分で実装できるようになる」ために、
各モジュールと主要演算の実装方針・読み順をまとめたものです。

## 0. 使い方（学習ルール）
- 1セクション=1テーマの最小例から始める。
- 例題は「最小入力→計算→検算」の順に完結させる。
- 手を動かす前に「式→変数→関数」の対応表を作る。

## 1. 全体の流れ（最小実装の骨格）
1. 入力を読む（native / Nastran / parserパッケージ）
2. 節点・要素・材料をグローバル配列に配置
3. 要素剛性 → 全体剛性へアセンブリ
4. 境界条件を適用
5. 連立方程式を解く（CG）
6. 変位・応力・反力を出力

入口は `src/fem4c.c` で、`static_analysis()` が全体フローを制御します。
NastranBalkFile を直接 `fem4c` に渡すと、parser → solver を一括実行します。

## 1.1 最小実装チェックリスト
1. `examples/t6_cantilever_beam.dat` を読み込める。
2. `g_num_nodes`, `g_num_elements` がログで確認できる。
3. 1ステップでも良いので CG の反復が回る。
4. `output.dat` が生成され、節点変位が 0 以外になる。

## 1.2 最小検算例（最小メッシュ）
目的: T3 の 1要素で剛性・拘束・荷重が最低限動くか確認する。

入力例（native形式）:
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

確認ポイント:
- `Nodes=3, Elements=1` が表示される。
- `Total applied force magnitude` が `1000` 付近になる。
- 反復が進み、`output.dat` に節点変位が出力される。

## 1.3 最小検算例（parser出力パッケージ）
目的: parser パッケージ入力が読み込めることを確認する。

ディレクトリ構成:
```
run_min/
  mesh/mesh.dat
  material/material.dat
  Boundary Conditions/boundary.dat
```

`mesh/mesh.dat`:
```
Total number of nodes [–]
3
Total number of elements [–]
1
Element type
CTRIA3
nodes
1, 0.0, 0.0, 0.0
2, 1.0, 0.0, 0.0
3, 0.0, 1.0, 0.0
elements
1, 1, 2, 3
```

`material/material.dat`:
```
Young's modulus [N/mm^2]
2.0e5
Poisson's ratio [–]
0.3
density [kg/mm^3]
7.8e-6
```

`Boundary Conditions/boundary.dat`:
```
Total number of Boundary Conditions [–]
2
Fixed(1)
Fix
node 1 12 0.0
Force(1)
Force
node 3 12 2 1000
```

実行:
```bash
./bin/fem4c run_min
```

NastranBalkFile から一括実行する場合:
```bash
./bin/fem4c NastranBalkFile/2Dmesh.dat run_min part_0001
```

確認ポイント:
- `Detected parser output package` が表示される。
- `Total applied force magnitude` が `1000` 付近になる。
- `output.dat` に非ゼロの変位が出る。

ログ例:
```
Detected parser output package in directory: run_min
Problem summary:
  Nodes: 3
  Elements: 1
  Materials: 1
  DOF: 6
Assembling global force vector...
  Total applied force magnitude: 1.000000e+03
```

## 2. common（基盤データとエラー）
対象ファイル:
- `src/common/constants.h`
- `src/common/types.h`
- `src/common/globals.h`
- `src/common/globals.c`
- `src/common/error.h`
- `src/common/error.c`

### 2.1 constants.h
- 解析規模の上限や要素タイプ定数を持ちます。
- まずは `MAX_NODES_PER_ELEMENT`, `MAX_TRACTION_SURFACES` などを読み、
  配列設計の前提を把握します。

### 2.2 types.h
- `node_t`, `element_t`, `material_t` は概念的なモデルです。
- 実装はグローバル配列に集約されるため、ここは理解の補助と考えてOKです。

### 2.3 globals.c/h
- 動的に確保される **実配列** がここに集まります。
- 重要な手順:
  1) `globals_reserve_*()` で容量確保
  2) `globals_initialize_*_entry()` で初期化
  3) `input_validate_map_*()` で ID → index の対応表を更新
- 研究用の最小実装を作るなら、この順番を模倣するのが安全です。

### 2.4 error.c/h
- `error_set()` を使い、関数を戻り値で失敗させる設計です。
- 初心者実装では「即時 printf → return」になりがちなので、
  FEM4C 方式を真似すると後の拡張が楽になります。

### 2.5 検算・デバッグ観点（common）
- `globals_reserve_*` の直後に `g_*_capacity` を確認する。
- `input_validate_map_*` で重複 ID が検出されるか意図的に試す。

### 2.6 IDマッピングの詳細（必読）
FEM4C では「入力ファイルのID」と「内部配列の index」を分離しています。

基本の流れ:
1. `globals_reserve_*()` で容量確保  
2. `globals_initialize_*_entry()` で初期化  
3. `input_validate_map_*()` で **ID → index** の対応表を更新  

利点:
- 入力IDが連番でなくても対応できる。  
- 重複IDの検出が簡単。  

最小確認:
```
node_id = 10 を読み込む
g_node_id_to_index[10] == 0 になることを確認
```

最小入力（native形式）:
```
ID Mapping Test
1 1
10 0.0 0.0
1 10 10 10
2.0e11 0.3
10 1 1 0.0 0.0 0.0
point loads
10 0.0 0.0 0.0
end
```

ログ例:
```
Problem summary:
  Nodes: 1
  Elements: 1
  DOF: 2
```

## 3. io（入力・出力）
対象ファイル:
- `src/io/input.c`, `src/io/input.h`
- `src/io/output.c`, `src/io/output.h`

### 3.1 入力: native 形式
- `input_read_header()` → `input_read_nodes()` → `input_read_elements()` → `input_read_materials()` の順で読み込み。
- `input_read_elements()` はトークン数から T3/Q4/T6 を自動判定します。

### 3.2 入力: Nastran Bulk（サブセット）
- `input_read_nastran_bulk()` が入口。
- `GRID/CTRIA3/CQUAD4/CTRIA6/MAT1/PSHELL/SPC/FORCE` のみ対応。
- `input_nastran_finalize_properties()` で PSHELL の厚みと材料を要素に反映。

### 3.3 入力: parser 出力パッケージ
- `input_read_parser_package()` が入口。
- `mesh/mesh.dat`, `material/material.dat`, `Boundary Conditions/boundary.dat` を読む。
- 2D（x-y）前提で、厚みは unit thickness (1.0) を採用。
- Z方向拘束・荷重は無視されます（注意喚起のみ）。

### 3.4 出力
- `output_write_results()` がメイン。
- `output_export_csv()` で CSV を出力（21列固定）。
- VTK と F06 も自動生成されます。

### 3.5 実装対応表（例）
| 概念 | 変数/関数 | 補足 |
|---|---|---|
| 入力形式判定 | `input_detect_format()` | 先頭行のキーワードで判定 |
| parser出力読み込み | `input_read_parser_package()` | 2D + unit thickness |
| CSV出力 | `output_export_csv()` | 21列固定のフォーマット |

### 3.6 検算・デバッグ観点（io）
- `input_read_*` の直後に `g_num_*` を出力して行数一致を確認。
- `g_node_id_to_index` で逆引きができるか `printf` で確認。

## 4. elements（要素）
対象ファイル:
- `src/elements/element_base.c/h`
- `src/elements/elements.c/h`
- `src/elements/t3/t3_element.c`
- `src/elements/t6/t6_element.c`, `src/elements/t6/t6_stiffness.c`
- `src/elements/q4/q4_element.c`, `src/elements/q4/q4_stiffness.c`

### 4.1 要素登録
- `elements_initialize()` で `t3_register()`, `q4_register()`, `t6_register()` が登録されます。
- 学習段階では「要素タイプ → 形状関数 → 剛性行列」という流れだけ追えばOKです。

### 4.2 T6 要素
- `t6_element_stiffness_matrix()` が剛性計算の中核。
- `t6_shape_functions` と `t6_shape_derivatives` が数式実装の要です。

### 4.3 T3/Q4 要素
- T3 は 1次要素、Q4 は4節点四角形です。
- T6 の理解の前に T3 → Q4 の順で読むと理解しやすいです。

### 4.4 検算・デバッグ観点（elements）
- 形状関数の分割統一性: `sum(N_i) == 1` を数点で確認。
- `det(J)` が 0 に近い場合は節点順序を疑う。

### 4.5 要素剛性の最小検算
- **対称性**: `ke[i][j] == ke[j][i]` を数点で確認。  
- **正定性の感触**: 拘束を加えた後、剛性行列の対角が正になることを確認。  
- **剛体モード**: 拘束が無いと特異になる（解けない）ことを理解しておく。  

簡易チェック（T3 の 1要素）:
```
拘束あり → 解ける
拘束なし → 収束しない or 特異警告
```

最小入力（拘束あり）:
```
T3 One Element
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

ログ例:
```
Checking global stiffness matrix properties...
  Diagonal terms: min = 1.000000e+00, max = ...
  Zero diagonal terms: 0
  Matrix properties check passed
```

## 5. solver（アセンブリと解法）
対象ファイル:
- `src/solver/assembly.c`, `src/solver/assembly.h`
- `src/solver/cg_solver.c`, `src/solver/cg_solver.h`

### 5.1 アセンブリ
- `assembly_global_stiffness_matrix()` が全体剛性を作成。
- 要素ごとの `ke` を `assembly_add_element_stiffness()` で全体へ配置。
- `assembly_apply_boundary_conditions()` で拘束処理。

### 5.2 解法（CG）
- `cg_solver_solve()` が中心。
- 学習目的では「残差の更新と収束条件」を追うだけで十分です。

### 5.3 実装対応表（例）
| 式 | 変数/関数 | 補足 |
|---|---|---|
| `K u = f` | `assembly_global_stiffness_matrix()` | 全体剛性の組立 |
| 収束判定 | `cg_solver_solve()` | 残差ノルムを監視 |

### 5.4 検算・デバッグ観点（solver）
- `g_global_force` の総和が想定値と一致するか確認。
- 反復回数が 0 の場合は荷重/拘束を再チェック。

### 5.5 境界条件の適用ロジック
`assembly_apply_boundary_conditions()` は、固定DOFの行列と荷重を次のように処理します。

概念:
- 拘束DOFの行と列を実質的に無効化し、対角に 1 を入れる。  
- 右辺（力）は拘束変位に合わせて調整される。  

この処理は「ペナルティ法」ではなく、**行列の直接書き換え**です。

最小入力:
```
BC Test
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

ログ例:
```
Applying boundary conditions...
  BC: Node 1 DOF 0 (global 0): diag ... -> 1.000, prescribed=0.000
  BC: Node 1 DOF 1 (global 1): diag ... -> 1.000, prescribed=0.000
  Applied 2 boundary conditions
```

### 5.6 CG収束判定の考え方
CG は残差ノルムが閾値以下になれば収束とみなします。

収束しない典型原因:
- 拘束が足りず剛体モードが残っている。  
- 荷重が全てゼロで、初期解がすでに収束している（反復0）。  
- 行列が特異または条件が悪い。  

最小入力（荷重ゼロ）:
```
Zero Load
3 1
1 0.0 0.0
2 1.0 0.0
3 0.0 1.0
1 1 2 3
2.0e11 0.3
1 1 1 0.0 0.0 0.0
2 1 1 0.0 0.0 0.0
point loads
3 0.0 0.0 0.0
end
```

ログ例:
```
Starting conjugate gradient solver...
  Initial guess already converged
```

## 6. analysis（ワークフロー）
対象ファイル:
- `src/analysis/static.c`, `src/analysis/static.h`

- `static_analysis_preprocessing()` で入力 → 検証 → 要約。
- `static_analysis_solve()` でアセンブリ → 解法。
- `static_analysis_postprocessing()` で出力。

## 7. 実装の最小演習（おすすめ順）
1. **T3 要素の剛性行列**を単独実装し、3節点の小問題で検証。
2. **アセンブリ**を最小メッシュで再現（要素1枚 → Kの形を確認）。
3. **CG ソルバ**を 2x2, 4x4 の小行列で実装練習。
4. **I/O** を native 形式で実装し、`examples/*.dat` を読めるようにする。
5. parser 出力を読む簡易ローダーを自作し、FEM4Cと比較。

## 8. 参考
- T6 の数学的導出: `docs/FEM_LEARNING_GUIDE.md`
- 全体チュートリアル: `docs/tutorial_manual.md`
- 解析入口の簡易ガイド: `USAGE_PARSER.md`

## 9. 初心者がつまずきやすい点（短い補足）
### 9.1 単位系の一貫性
- 入力の単位が混ざると結果が一気に破綻します。  
- parser 出力は単位を正規化している前提ですが、自作入力では **E, 長さ, 荷重** を同じ単位系で揃える。  

### 9.2 拘束不足（剛体モード）
- 拘束が足りないと、剛体モードが残って解けません。  
- 最小例では「2点拘束 + 1点荷重」のように、2Dで少なくとも3自由度を固定する。  

### 9.3 節点順序と要素品質
- 三角形の節点順序が崩れると `det(J)` が負になり、剛性が不安定になります。  
- まずは **反時計回り** の節点順序で入力する習慣をつける。  
