# FEM4C 引き継ぎメモ

## 1. 現状概要
- Fortran 版（`../highperformanceFEM/highperformanceFEM`）を参照しながら C 版を再構築した段階。線形静解析・2D 要素（T3 / Q4 / T6）を中心に動作。
- OpenMP 対応 (`-fopenmp`) を前提としたビルド構成になっているため、`make openmp` でのビルドを推奨。
- 入力形式はネイティブフォーマット（書籍付属データ）と Nastran BULK の読み取りをサポート。要素・節点番号は Fortran 版同様に任意の ID を使用可能。

## 2. 最近の変更点（2025-??-??）
| 項目 | 内容 | 参照ファイル |
| --- | --- | --- |
| ID マッピング復元 | 節点・要素・材料 ID をオリジナル番号のまま扱えるよう `g_*_id_to_index` などを追加 | `src/common/globals.[ch]`, `src/io/input.c` |
| 荷重処理強化 | `body`/`tract`/`point` セクションと Nastran SPC/FORCE を Fortran 相当に整備。表面トラクションと体積力を全体荷重へ組み込み | `src/io/input.c`, `src/solver/assembly.c` |
| 拘束の RHS 補正 | 既知変位の寄与を RHS から差し引き、非ゼロ変位境界でも安定化 | `src/solver/assembly.c` |
| 分布荷重積分 | T3/T6/Q4 用のガウス積分を実装し、体積力・境界トラクションを要素単位で集計 | `src/solver/assembly.c` |
| 圧力荷重ベクトル化 | `press` セクションで定義した面に法線方向圧力を積分し、全体力へ加算。入力で面節点（三節点）を列挙 | `src/io/input.c`, `src/solver/assembly.c` |
| スカイライン剛性行列 | 全体剛性を密行列からスカイライン格納へ移行し、CG・境界条件処理も対応 | `src/common/globals.*`, `src/solver/assembly.c`, `src/solver/cg_solver.c`, `src/analysis/static.c`, `src/io/output.c` |

## 3. 想定される次のタスク
1. **ポスト処理の拡張**  
   - 応力の保持・集計、VTK/F06 出力の 1 DOF 面対応など Fortran 版機能の網羅。
2. **テスト整備**  
   - 既存 Fortran 版データセット（`../highperformanceFEM/highperformanceFEM/data`）を用いた再現性テスト、CI スクリプト整備。
3. **圧力荷重の検証ケース追加**  
   - `press` 面を含むサンプル入力と参照解の整備、ユニットテスト化。

## 4. ビルド & 実行メモ
```
make clean
make openmp    # OpenMP を有効化 (推奨)
./bin/fem4c input.dat output.dat
```
- OpenMP 無しでビルドする場合は `make release` でもリンクは通るが、`gcc` によっては `-fopenmp` が必要。
- 入力データの節点番号が欠番・飛び番でも可。荷重・拘束の節点番号も原番号で記述。

## 5. 既知の注意点
- 圧力荷重（`press`）は値に続いて 3 節点ずつ境界面を列挙する必要あり（端点 2 節点 + 中点）。要素境界の節点順序は外向き法線が得られる向きで指定すること。
- 全体剛性はスカイライン格納となったため、新しい要素追加時は DOF 組合せがプロファイルに含まれるか確認すること（`assembly_collect_element_dofs` の拡張が必要）。
- メモリ上限を `MAX_NODE_ID=1,000,000` としているが、配列は固定長のため `MAX_NODES` を超えると即座にエラー。
- OpenMP 並列組立は T3/Q4/T6 以外の要素に未対応。要素追加時は `assembly_apply_body_force_*` 系の拡張も必要。

## 6. 参照すべき資料
- `docs/01_requirements.md` (要件定義)
- `docs/03_design.md` (設計タスク表)
- 原著 Fortran コード: `../highperformanceFEM/highperformanceFEM/src/common/rddat.f90`, `.../assol.f90`

## 7. 連絡事項
- 追加で実装する場合は Fortran 版の挙動を逐一確認し、ID マップを壊さないこと。
- コード整形は `clang-format` (`make format`) で統一。
- Pull/Push などバージョン管理は利用者側で別途設定してください。
