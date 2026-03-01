# FEM4C 設計メモ（モジュール分割）

本資料は、学習者が「どこに何があるか」を把握するための設計メモです。

## 1. 全体フロー
1. 入力を読む（native / parser パッケージ）
2. 節点・要素・材料をグローバル配列へ配置
3. 要素剛性を計算し、全体剛性へアセンブリ
4. 境界条件・荷重を適用
5. 連立方程式を解く（CG）
6. 変位・応力を出力

NastranBalkFile の場合は `fem4c` が parser を実行し、出力パッケージを読み込む。

## 2. 主なディレクトリ
- `src/common/`: 定数、型、グローバル配列、エラー処理
- `src/io/`: 入力読み込み、出力
- `src/elements/`: 要素ごとの剛性計算
- `src/solver/`: 連立方程式ソルバ（CG）
- `src/analysis/`: 解析ドライバ
- `parser/`: NastranBalkFile を parser パッケージに変換

## 3. 入出力インターフェース
- parser 出力パッケージは以下を持つ:
  - `mesh/mesh.dat`
  - `material/material.dat`
  - `Boundary Conditions/boundary.dat`
- boundary では `Fix` / `Force` ブロックを読み込み、`node/surface/ridgeline` のターゲットに展開する。

## 4. CLI 設計
- `./bin/fem4c <input> [output]` が基本。
- `<input>` がファイルなら parser を実行して解析する。
- `<input>` がディレクトリなら parser 出力パッケージを直接解析する。

## 5. 学習用コードの配置
- 追加の演習・検証コードは `practice/` に置く。
- `docs/` は説明文と手順に専念する。

## 6. データ構造の流れ（読み替えガイド）
- `nodes` / `elements` / `materials` がグローバル配列に展開される。
- 入力時点で ID を内部インデックスに変換し、以後は内部 ID を使用する。
- 2D 解析では自由度は各節点 2 DOF（Ux, Uy）。

## 7. 境界条件の設計
- `Fix` は Dirichlet 条件として処理し、該当自由度の行列対角を 1 に置換する。
- `Force` は外力ベクトルに加算する（node/surface/ridgeline を展開）。
- 2D では Z 方向の拘束・荷重は無視される。

## 8. 単位系の流れ
- `material.dat` の E は N/mm^2、密度は kg/mm^3 を前提とする。
- 長さは mm を前提とし、入力の単位系を統一して扱う。

## 9. 拡張時の判断基準
- 3D 対応は「自由度増加」「要素形状」「境界条件の扱い」を同時に拡張する必要がある。
- 非線形対応では「残差」「接線剛性」「収束判定」を追加する。
