# FEM4C - 高性能有限要素法（C言語版）

## 概要
FEM4Cは、山田貴博氏著「高性能有限要素法」に基づく有限要素解析ソルバーをFortranからC言語に変換したものです。本リポジトリのFEM4Cは「研究者(初心者)が自分でFEMを実装できるようになる」ための練習用マテリアルであり、実装の流れを追体験できる構成を重視しています。

## 特徴
- **線形静解析**: 現状は2次元構造の静的解析に対応
- **要素タイプ**: T6（6節点三角形要素）を中心とした実装、将来的にQ4、H8、T4、T10等に対応予定
- **OpenMP対応**: 並列処理による高速化（準備完了）
- **スカイライン行列格納**: 全体剛性をバンド構造で保持しメモリ使用量を削減
- **包括的エラー処理**: 戻り値方式による確実なエラーハンドリング

## 現在の実装状況: Phase 1 完了 ✅

### 実装済み機能
- ✅ ディレクトリ構造とビルドシステム
- ✅ 共通モジュール（定数、型定義、グローバル変数）
- ✅ エラーハンドリングシステム
- ✅ 入出力モジュール（ネイティブ形式対応）
- ✅ 入出力モジュール（Nastran Bulk入力の一部対応）
- ✅ parser出力パッケージ入力対応（mesh/material/boundary）
- ✅ T6要素完全実装
- ✅ 剛性行列組み立て
- ✅ 共役勾配法ソルバー
- ✅ 境界条件処理
- ✅ 統合解析ドライバー

### 次期実装予定
- 🔄 2D要素拡張（Q4、T3、Q9等）
- 🔄 3D要素対応（H8、T4、T10等）
- 🔄 OpenMP並列化

## ビルド方法

### 必要環境
- GCC コンパイラ（C99対応）
- Make ユーティリティ
- OpenMPライブラリ（オプション）

### クイックビルド
```bash
make                # 標準ビルド
make release        # OpenMP対応最適化ビルド
make debug          # デバッグビルド
```

### その他のビルドオプション
```bash
make openmp         # OpenMP対応ビルド
make clean          # ビルドファイル削除
make help           # 全ビルドターゲット表示
```

## 使用方法
```bash
./bin/fem4c [--mode=fem|mbd|coupled] [--strict-t3-orientation] [入力ファイル] [出力ファイル]
```

### 実行例
```bash
# テストケース実行
./bin/fem4c examples/t6_cantilever_beam.dat results.dat

# ファイル指定なしの場合
./bin/fem4c  # input.dat → output.dat
```

### parser一体実行（Nastran入力 → parser → solver）
```bash
./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 output.dat
```

### parser出力パッケージの実行例
```bash
./bin/fem4c <parser出力ディレクトリ>
```

### T3要素の strict orientation チェック（任意）
```bash
# clockwise要素を自動補正せず、入力エラーで停止
./bin/fem4c --strict-t3-orientation /tmp/t3_clockwise.dat out.dat
```

### MBD回帰ラッパー（B-team運用）
```bash
# B-8 smoke/contract regression entrypoints
make -C FEM4C mbd_b8_knob_matrix_smoke_test
make -C FEM4C mbd_b8_regression_test
make -C FEM4C mbd_ci_contract_test
```

### A-24 serial acceptance（A-team運用）
```bash
# A-24 acceptance 3コマンドを直列実行
make -C FEM4C mbd_a24_acceptance_serial

# 直列ラッパーの自己テスト
make -C FEM4C mbd_a24_acceptance_serial_test
```

`mbd_a24_acceptance_serial` は `mbd_a24_regression_full_test` →
`mbd_a24_batch_test` → `mbd_ci_contract_test` を直列実行し、
`A24_ACCEPT_SERIAL_SUMMARY` で結果を1行要約します。

`mbd_a24_regression` 側の主な lock ノブ:
- `A24_REGRESSION_SKIP_LOCK=0|1`（既定: `0`）
- `A24_REGRESSION_LOCK_DIR=<path>`（既定: `/tmp/fem4c_a24_regression.lock`）

主な運用ノブ:
- `A24_ACCEPT_SERIAL_RETRY_ON_137=0|1`（既定: `1`）
- `A24_ACCEPT_SERIAL_FAKE_137_STEP=none|full_test|batch_test|ci_contract_test`（自己テスト用途）
- `A24_ACCEPT_SERIAL_SUMMARY_OUT=<path>`（summary 1行をファイル出力。親ディレクトリ未作成/ディレクトリ指定は fail-fast）
- `A24_ACCEPT_SERIAL_STEP_LOG_DIR=<dir>`（ステップログを保存し、失敗時 `failed_log` で参照可能。既存ファイル指定や非 writable ディレクトリは fail-fast）

`mbd_b8_guard` 系は既定で `B8_LOCAL_TARGET=mbd_checks` を使い、再帰 make 呼び出し時に
`MAKEFLAGS/MFLAGS` を隔離して安定化しています。`mbd_b8_regression` 系は
`B8_B14_TARGET=mbd_ci_contract` を既定値にして B-14 連結を軽量化しつつ、必要時は
`B8_B14_TARGET=mbd_b14_regression` で重い経路へ切替できます。
さらに `mbd_b8_regression` / `mbd_b8_regression_full` は nested self-test 実行時に
`B8_LOCAL_TARGET` / `B8_B14_TARGET` / `B8_RUN_B14_REGRESSION` を隔離し、最終 guard 実行経路にのみ
ノブを受け渡すことで、自己テスト連鎖の再入安定性を維持します。
`B8_REGRESSION_SKIP_LOCK=0|1`（既定: `0`）と `B8_REGRESSION_LOCK_SCOPE=repo|global`（既定: `repo`）、
`B8_REGRESSION_LOCK_DIR=<path>`（任意 override）で再入ロック挙動を制御できます。
`B8_MAKE_TIMEOUT_SEC=<seconds>`（既定: `0` = 無効）を指定すると、`mbd_b8_regression` /
`mbd_b8_regression_full` の内部 make 呼び出しを fail-fast timeout で停止できます。
`repo` 既定では `/tmp/fem4c_b8_regression.<repo_hash>.lock` を使って別ワークツリー間の競合を避け、
`global` 指定時は `/tmp/fem4c_b8_regression.lock` を使います。
実行サマリには `lock_dir=<path>` と `lock_dir_source=env|scope_repo_default|scope_global_default`
（full wrapper は `b8_lock_dir_source=...`）を出力し、ロック経路の判定根拠を追跡できます。
`mbd_b8_regression_full` でも clean/all/test から lockノブを隔離したうえで、
最終 `mbd_b8_regression` 呼び出しにのみ受け渡します。
`mbd_ci_contract_test` では B-8自己テストの一時スクリプト生成契約
（`b8_regression_test_temp_copy_marker` など）に加えて、
`B8_TEST_TMP_COPY_DIR` の既定値/存在チェック/書込チェック契約
（`b8_*_temp_copy_dir_*_marker`）も静的検査します。

詳細な操作手順は `USAGE_PARSER.md` を参照してください。

## ドキュメント案内
- 参考書: `FEM4C_Reference_Manual.md`
- parser手順: `USAGE_PARSER.md`
- ドキュメント入口: `docs/README.md`
- 主教材: `docs/tutorial_manual.md`
- 実装ガイド: `docs/implementation_guide.md`
- 実装計画: `docs/solver_reorg_mbd_migration_plan.md`

## プロジェクト構成
```
FEM4C/
├── src/                # ソースコード
│   ├── common/         # 共通モジュール
│   ├── elements/       # 要素実装
│   │   └── t6/         # T6要素
│   ├── solver/         # 線形ソルバー
│   ├── io/             # 入出力処理
│   ├── analysis/       # 解析ドライバー
│   ├── mbd/            # 2D MBD 移植モジュール（段階導入）
│   └── fem4c.c         # メインプログラム
├── parser/             # Nastran Bulk -> parser出力パッケージ変換
├── docs/               # ドキュメント
├── examples/           # 使用例
├── practice/           # 学習用ハンズオン
├── USAGE_PARSER.md     # parser一体実行の手順
└── README.md           # このファイル
```

## 入力ファイル形式

### ネイティブ形式
```
# コメント行
タイトル

節点数 要素数
節点番号 X座標 Y座標
...
要素番号 節点1 節点2 節点3 節点4 節点5 節点6
...
ヤング率 ポアソン比
節点番号 X拘束 Y拘束 Z拘束 X変位 Y変位 Z変位
...
point loads
節点番号 X荷重 Y荷重 Z荷重
...
body
Fx Fy Fz

press
圧力値
節点1 節点2 節点3
...

tract
境界数
節点1 節点2 節点3 Tx Ty Tz
...
end
```

`press` セクションでは圧力値の後に外向き法線を得る順序で 3 節点（端点 2 節点と中点 1 節点）を列挙します。

## メモリ制限
- 最大節点数: 10,000
- 最大要素数: 5,000  
- 最大材料数: 100

これらの制限は `src/common/constants.h` で変更可能です。

## 実行例

### 基本テスト
```bash
# T6要素の実行
make && ./bin/fem4c examples/t6_cantilever_beam.dat output.dat

# 結果確認
cat output.dat
```

## 性能特性
- **固定配列**: 高速メモリアクセス
- **共役勾配法**: 大規模問題対応
- **OpenMP準備**: 並列処理対応（Phase 2で実装予定）
- **モジュール設計**: 保守性と拡張性

## 開発状況

### Phase 1 (完了): T6要素基本フレームワーク
- **期間**: 完了
- **成果**: 基本解析システム構築完了

### Phase 2 (予定): 2D要素拡張
- **期間**: 3週間予定
- **内容**: Q4、T3、Q9要素実装

### Phase 3 (予定): 3D要素対応
- **期間**: 3週間予定
- **内容**: H8、T4、T10要素実装

### Phase 4 (予定): Nastran対応・最終化
- **期間**: 2週間予定
- **内容**: Nastranファイル対応、OpenMP並列化

## トラブルシューティング

### コンパイルエラー
```bash
# 大規模配列でのリンクエラーの場合
make clean
# constants.hでMAX_NODESを削減するか
# -mcmodel=largeオプションを使用（Makefile設定済み）
```

### 実行時エラー
```bash
# ファイルが見つからない場合
ls examples/  # 入力データの確認
```

## ライセンス
本ソフトウェアは山田貴博氏の原著作をベースとしています。各ソースファイル内の著作権表示をご確認ください。

## 開発・貢献
本プロジェクトはPhase 1の基本実装が完了しており、Phase 2以降の拡張開発に向けた基盤が整っています。

## 連絡先
質問や貢献については、`docs/` ディレクトリ内のプロジェクトドキュメントをご参照ください。

---
**最終更新**: 2025-09-24  
**バージョン**: 1.0 (Phase 1 完了)  
**実装状況**: T6要素対応完全実装済み
