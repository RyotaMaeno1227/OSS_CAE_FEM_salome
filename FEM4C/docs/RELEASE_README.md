# FEM4C Release Package Overview

このドキュメントはリリース版のディレクトリ構成と各ファイルの役割をまとめたものです。  
ビルドや解析を行う際の参考にしてください。

## ルートディレクトリ

- `Makefile`  
  - `make`, `make openmp` などでソルバーをビルドするための定義。
- `bin/`  
  - ビルド済みバイナリを格納。`make openmp` の後に `bin/fem4c` が生成される。
- `src/`  
  - C 言語による FEM4C 本体のソースコード。`src/common`, `src/elements`, `src/solver` などモジュール毎に分割。
- `docs/`  
  - プロジェクト文書。要件・設計メモ、手順書や本ファイル（構成説明）を含む。
- `practice/`  
  - 教科書の章ごとに対応した練習コードとテストドライバ。学習者がローカルで改造しながら確認できるサンプルをまとめている。
- `examples/`  
  - 代表的な入力データ。`t3_cantilever_beam.dat`, `q4_cantilever_beam.dat`, `t6_cantilever_beam.dat` など解析用のサンプルが含まれる。
- `test/`  
  - 自動テストや再現性確認用の入力・スクリプトを収めたフォルダ。リリース後の検証・回帰テストに利用できる。
- レポート類 (`FEM4C_Reference_Manual.md`, `PHASE2_IMPLEMENTATION_REPORT.md`, `T6_PROGRESS_REPORT.md`, `README.md`)  
  - 実装経緯や手順、追加情報をまとめた Markdown ドキュメント。

## 代表的なサブディレクトリ

- `src/common/`  
  - 定数、グローバル変数、エラーハンドリングなど全体で共有するユーティリティ。
- `src/elements/`  
  - 要素ごとの実装。`t6`, `t3`, `q4` といった各サブディレクトリに要素固有の形状関数や剛性組み立てがある。
- `src/solver/`  
  - 全体剛性行列の組み立て (`assembly.c`) と線形ソルバー (`cg_solver.c`) など解析コア。
- `src/io/`  
  - 入出力の読み書き、VTK・テキスト結果の生成。
- `docs/`  
  - 参考資料 (`FEM4C_Reference_Manual.md` 等) や、本ファイルのような補助説明。
- `practice/`  
  - 各章向けの C 実装テンプレートとテスト。`README.md` に使い方を記載。
- `examples/`  
  - 各要素タイプに対応した片持ち梁の入力データがあり、`bin/fem4c` への引数として指定して解析する。
    - 例: `./bin/fem4c examples/t6_cantilever_beam.dat output.dat`
- `test/data/`  
  - 単体テストや再現テストで使用する入力セット。リリース前の検証を継続する場合に利用。

## 推奨ワークフロー

1. `make clean`（必要に応じて）→ `make openmp` でビルド。  
2. `bin/fem4c` に対して `examples/*.dat` を入力ファイルとして解析実行。  
3. 出力（例: `output.dat`, `output.vtk`）はユーザー側で任意に保存・整理してください。

## 注意事項

- リリース版ではビルド成果物・中間ファイル・旧デバッグコードを削除済みです。不要なデータが混入しない状態で配布できます。  
- 追加で入力データを作る場合は `examples/` 配下に配置すると整理しやすくおすすめです。  
- `test/` 配下は任意利用ですが、回帰テストに役立つため基本的には残した構成としています。
