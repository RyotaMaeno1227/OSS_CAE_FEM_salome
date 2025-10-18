# FEM4C Implementation Status & History (2025-10-12)

## 現在の実装サマリ
- **解析フロー**: `src/fem4c.c` → `src/analysis/static.c` で初期化／前処理／解法／後処理／終了の 4 フェーズを制御。`static_analysis_*` 系でフェーズ分離済み。
- **メッシュ・グローバル管理**: `src/common/globals.c` を全面動的配列化済み。ノード／要素／材料 ID マップも動的確保し、Fortran 版の任意 ID を維持できる。
- **要素対応**: 平面要素 T6 / T3 / Q4 をサポート。T6 は高次三角形要素として完全実装し、B マトリクス・応力計算を `src/elements/t6/t6_stiffness.c` で提供。T3/Q4 も応力計算関数を装備。
- **入力 (IO)**:
  - ネイティブフォーマット (`*.dat`) と Nastran BULK (`*.dat` 長書式) の両方をサポート。
  - Nastran GRID* / CTRIA6 / MAT1 / PSHELL / SPC / FORCE など主要カードを取り込み。
  - 材料・PSHELL の動的マッピングを `input_nastran_finalize_properties` で実装（材料複製・ID マップ更新）。
  - 荷重（集中・分布・圧力）の組み込みを Fortran 版同等に再現。
- **ソルバ**:
  - 全体剛性組立ては Skyline 格納に再構築（`assembly_build_stiffness_profile`）。OpenMP 有効時は `assembly_parallel_stiffness_matrix` で並列組立。
  - 解法は共役勾配法 (CG) + スカイライン行列積 (`src/solver/cg_solver.c`)。`g_solver_info` に収束履歴を記録。
  - 拘束処理はスカイライン直接編集で対角 1 化＋RHS 補正。
- **出力**:
  - 標準 `output.dat`、VTK (`output.vtk`)、Nastran F06 (`output.f06`) を出力。F06 は `output_write_nastran_f06_file`。
  - CSV (`output.csv` / `beam_output.csv`) には節点変位（x,y,z,ux,uy,uz,|u|）と要素応力（σx, σy, τxy, Von Mises, 主応力）を 21 列構成で出力。ID／節点番号とも入力ファイルの番号を保持。
  - `visualize_results.m` で MATLAB 可視化（変形前後／応力／ベクトル）に対応。CSV のヘッダ／列順を前提に自動パース。
- **サンプル解析**:
  - `examples/t6_cantilever_beam.dat` を最新環境で成功（`run.log` 参照）。CG は 273 iter で収束、CSV・VTK・F06 生成を確認。
  - Nastran 2D mesh (`NastranBalkFile/2Dmesh.dat`) もインポート～解析まで確認済み。

## 主要モジュールと参照ファイル
| 領域 | 役割 | 主なファイル |
|------|------|--------------|
| エントリポイント | CLI 引数処理と解析開始 | `src/fem4c.c` |
| 解析制御 | 初期化～後処理のフェーズ管理 | `src/analysis/static.c` |
| グローバル状態 | メッシュ・材料・ID マップの動的管理 | `src/common/globals.c`, `src/common/globals.h` |
| 入力 | ネイティブ/Nastran パースと荷重適用 | `src/io/input.c`, `src/io/input.h` |
| 要素実装 | 要素ごとの剛性・応力計算 | `src/elements/t6/t6_stiffness.c`, `src/elements/t3/t3_element.c`, `src/elements/q4/q4_element.c` |
| 組立・荷重 | スカイラインプロファイル構築と RHS 集計 | `src/solver/assembly.c` |
| ソルバ | 共役勾配法・残差管理 | `src/solver/cg_solver.c` |
| 出力 | DAT/CSV/VTK/F06 出力、反力プレースホルダ | `src/io/output.c`, `visualize_results.m` |

## 主要変更履歴
| 日付 | 変更 | 概要 |
|------|------|------|
| 2025-10-10 | グローバル配列動的化 | `globals.c` を全面改修し reserve/reset が動的確保に対応。 |
| 2025-10-10 | Nastran パーサ強化 | GRID* 長書式、CTRIA6、MAT1/PSHELL 連携、要素材料 ID マップ整備。 |
| 2025-10-11 | CG 行列演算の境界チェック追加 | `cg_matrix_vector_multiply` に Skyline オフセット検査を実装しセグフォ防止。 |
| 2025-10-11 | CSV 出力導入 | `output_export_csv` と `static_write_results` を改修し節点変位＆要素応力を CSV 化。 |
| 2025-10-12 | MATLAB 可視化更新 | `visualize_results.m` を手動パース／自動ファイル選択／節点 ID マップ付きに改修。 |
| 2025-10-13 | CSV フォーマット修正 | ノード／要素行をヘッダ 21 列に整列。要素 ID の保持と空列の配置を是正し、MATLAB 可視化と一致。 |

## 既知の課題・TODO
- **デバッグログ**: `solver/assembly.c` や T6 の B マトリクス出力など、デバッグ目的の `printf`/`fprintf` が大量に残存 → リリース時は OFF にする仕組みが必要。
- **応力格納**: 要素応力は CSV/VTK 出力時に都度計算しており、グローバル配列へ保持していない。後続処理向けに永続化検討。
- **CG 収束**: 大規模モデル（梁512要素）で 1000 iter 超。前処理や収束判定改善（Tolerance / Preconditioner）を検討。
- **境界条件出力**: 反力計算はプレースホルダ（`output_calculate_reactions` 未実装）。
- **テスト整備**: 自動テスト／CI が未整備。現状は手動実行ログのみ。
- **ドキュメント整合性**: 旧ドキュメント（例: `docs/05_handover_notes.md`）には固定長配列前提の記述が残る。必要に応じて更新。

## 検証・サンプル実行状況
- **直近 run.log**: `examples/t6_cantilever_beam.dat` を入力、273 iter・最終残差 `8.4e-09` で収束。ノード191／要素68／DOF382。CSV・VTK・F06 を検証済み。
- **再解析ログ**: 2025-10-13 時点で `examples/t6_cantilever_beam.dat`（T6 梁）と `NastranBalkFile/2Dmesh.dat`（CTRIA6 メッシュ）を再実行。CSV は双方とも 21 列構成で整列し、MATLAB/ParaView で可視化確認済み。
- **サンプルデータ**:
  - `examples/` 以下に T3/Q4/T6 各カンチレバーの入出力ひな型。
  - `NastranBalkFile/2Dmesh.dat` に 2D CTRIA6 メッシュ（GRID* 長書式含む）。
- **可視化**: ParaView で VTK を Warp by Vector、MATLAB で `visualize_results.m` を実行して変位・応力を確認。

## 参照ドキュメント
- 設計・履歴: `docs/04_progress.md`, `docs/05_handover_notes.md`
- 詳細仕様: `FEM4C_Reference_Manual.md`, `PHASE2_IMPLEMENTATION_REPORT.md`
- Fortran 版比較: `../highperformanceFEM/highperformanceFEM`

## 使い方メモ
### 解析実行
```bash
cd ~/highperformanceFEM/FEM4C
make openmp               # 必要なら
./bin/fem4c <input.dat> <output.dat>
```
例: `./bin/fem4c examples/t6_cantilever_beam.dat beam_output.dat`

### 可視化
- **ParaView**: `beam_output.vtk` を Open → Warp By Vector (Displacement)。
- **MATLAB**:
  ```matlab
  clear csvFile;         % 必要なら
  csvFile = 'beam_output.csv';
  visualize_results
  ```

## 今後戻ってきたときのチェックリスト
1. `docs/06_fem4c_implementation_history.md` を開いて直近状況を把握。
2. `run.log` や `beam_output.*` など最新解析結果を確認。必要なら `examples/*` を再実行。
3. 「既知の課題」または `docs/05_handover_notes.md` の残課題から次に着手する項目を選定。
4. 変更後は `make openmp`（もしくは `make release`）→ サンプル解析でリグレッションを実施し、CSV/VTK/F06 の正当性をチェック。
5. ドキュメント（特に 05/06）と実装の乖離が出た場合は更新ログを追記。
