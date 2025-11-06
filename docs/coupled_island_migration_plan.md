# Coupled & Island Solver Migration Plan

Chrono-C の Coupled 拘束と島ソルバを Chrono C++ (`chrono-main`) と同等の数値基盤へ合わせ込むための差分整理と移植計画をまとめる。

## 1. 現行 Chrono-C (2D) の実装メモ
- **Coupled 拘束**はローカル 4x4（最大）行列を構築し、部分ピボット付きガウス消去で逆行列を得る実装。Pivot 選択と条件数評価は `coupled_constraint_invert_matrix` / `coupled_constraint_condition_bound` で行っている (`chrono-C-all/src/chrono_constraint2d.c:258`, `chrono-C-all/src/chrono_constraint2d.c:351`)。
- 条件数が閾値を超えた場合にはワーニングを立て、最小対角ブロックをドロップする自動回復を備える (`chrono-C-all/src/chrono_constraint2d.c:1546`-`chrono-C-all/src/chrono_constraint2d.c:1604`)。
- 島ソルバは Union-Find でボディ連結成分を抽出し、制約／接触ポインタ配列をワークスペースに集約する単純設計 (`chrono-C-all/src/chrono_island2d.c:48`-`chrono-C-all/src/chrono_island2d.c:135`)。並列化は OpenMP による島単位のループ分割 (`chrono-C-all/src/chrono_island2d.c:390` 近辺)。

## 2. chrono-main の該当機能
- Chrono-main は **システムディスクリプタ** (`ChSystemDescriptor`) を通じて拘束と変数を KKT 形式に組み上げ、グローバルスパース行列を構築 (`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:42`-`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:175`)。
- 拘束は `ChConstraintTwoTuples*` 系クラスで抽象化され、ロール／スピン摩擦まで含む多自由度拘束を提供 (`third_party/chrono/chrono-main/src/chrono/solver/ChConstraintTwoTuplesRollingN.h:37`-`third_party/chrono/chrono-main/src/chrono/solver/ChConstraintTwoTuplesRollingN.h:90`)。
- 反復解法は `ChIterativeSolverVI` ファミリで Warm-start、オーバーリラクゼーション、違反履歴などを扱う (`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:24`-`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:62`)。SOR/PMINRES/ADMM など複数ソルバを `ChSolver::Type` で切替可能 (`third_party/chrono/chrono-main/src/chrono/physics/ChSystem.cpp:413`-`third_party/chrono/chrono-main/src/chrono/physics/ChSystem.cpp:464`)。
- 接触管理は法線・接線・ローリング成分をクラスごとに保持し、レポート/アセンブリ時にまとめる (`third_party/chrono/chrono-main/src/chrono/physics/ChContactContainerNSC.cpp:748`-`third_party/chrono/chrono-main/src/chrono/physics/ChContactContainerNSC.cpp:776`)。

## 3. 差分サマリ
| 項目 | Chrono-C (現状) | chrono-main | ギャップ |
|------|----------------|-------------|----------|
| 連立解法 | 拘束ごとに 4x4 逆行列 (直接解法) | グローバル KKT を反復解法 | グローバルスパース構築が未実装 |
| Pivot/条件数 | 固定ピボット閾値＋式ドロップ (`chrono_constraint2d.c:1546`) | ソルバが自動で LCP/VI を処理 | Pivot 選択を柔軟化（列交換＋再スケール）が必要 |
| 接触拘束 | 2 点マニフォールド・Coulomb のみ (`chrono_collision2d.h:29`-`chrono_collision2d.h:152`) | 転がり/スピンまで多次元 (`ChConstraintTwoTuplesRollingN`) | 追加自由度と Jacobian 拡張 |
| 島構築 | Union-Find + OpenMP (`chrono_island2d.c:48`-`chrono_island2d.c:395`) | システムディスクリプタが自動再構成 | マルチ DOF 拘束の分配 & 3D 互換 API |

## 4. 移植ロードマップ
1. **共通ディスクリプタ層の導入**  
   - `ChronoConstraint2DBase_C` を KKT ブロックへ射影する構造体を追加し、`ChSystemDescriptor` 相当の集約器を C API で新設。  
   - 2D では従来の 4x4 と並行運用し、`CHRONO_ENABLE_GLOBAL_SOLVER` フラグで切替。
2. **反復ソルバの写経**  
   - `ChIterativeSolverVI` のオメガ/シャープネス制御と収束基準を C 実装へ移植。  
   - カラム再スケールと収束履歴を `ChronoCoupledConstraint2DDiagnostics_C` に拡張。
3. **接触自由度拡張**  
   - Manifold のデータ構造に回転摩擦を追加し、Rolling/Torsional 係数を扱う (`ChConstraintTwoTuplesRollingN` 互換)。  
   - Coupled 拘束との相互作用をテスト (`tests/test_island_contact_constraint.c` を再適用)。
4. **島ワークスペースの 3D 互換化**  
   - `ChronoIsland2DWorkspace_C` をベクトル長指定型へリファクタ、OpenMP セクションをタスクグラフ化。  
   - 3D 導入時は既存ユニットを共通メモリアロケータ経由で再利用。

## 5. 進行管理
- `docs/chrono_3d_abstraction_note.md` の KPI / ガントは拘束・接触・並列化タスクのみを扱うよう更新済み（同ファイル参照）。
- 次アクション: ディスクリプタ試作ブランチで 2D 版 `ChSolverPSOR` を動作させ、条件数とイテレーション数を比較する。

