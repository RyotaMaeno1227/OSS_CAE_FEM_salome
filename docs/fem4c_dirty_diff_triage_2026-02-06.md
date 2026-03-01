# FEM4C Dirty Diff Triage (2026-02-06)

## Scope / Snapshot
- Scope: `FEM4C` 配下の未整理差分（`git status --short -- FEM4C`）
- Snapshot date: 2026-02-06
- 目的: 誤コミット防止のため、差分を 3分類し、安全な staging 手順を固定する。

## 1) 3分類（ファイルパス付き）

### A. 実装として残す（優先ステージ対象）
- `FEM4C/Makefile`
- `FEM4C/src/fem4c.c`
- `FEM4C/src/analysis/runner.c`
- `FEM4C/src/analysis/runner.h`
- `FEM4C/src/mbd/constraint2d.c`
- `FEM4C/src/mbd/constraint2d.h`
- `FEM4C/src/mbd/kkt2d.c`
- `FEM4C/src/mbd/kkt2d.h`
- `FEM4C/practice/ch09/native_probe.c`
- `FEM4C/practice/ch09/nastran_probe.c`
- `FEM4C/docs/00_tutorial_requirements.md`
- `FEM4C/docs/implementation_guide.md`
- `FEM4C/NastranBalkFile/3Dtria_example.dat`
- `FEM4C/USAGE_PARSER.md`

### B. 生成物・不要物（削除維持を推奨）
- `FEM4C/PHASE2_IMPLEMENTATION_REPORT.md`
- `FEM4C/T6_PROGRESS_REPORT.md`
- `FEM4C/docs/02_file_structure.md`
- `FEM4C/docs/04_progress.md`
- `FEM4C/docs/05_handover_notes.md`
- `FEM4C/docs/06_fem4c_implementation_history.md`
- `FEM4C/docs/RELEASE_README.md`
- `FEM4C/examples/q4_cantilever_results.dat`
- `FEM4C/examples/q4_cantilever_results.f06`
- `FEM4C/examples/q4_cantilever_results.vtk`
- `FEM4C/examples/t3_cantilever_results.dat`
- `FEM4C/examples/t3_cantilever_results.f06`
- `FEM4C/examples/t3_cantilever_results.vtk`
- `FEM4C/examples/t6_cantilever_results.dat`
- `FEM4C/examples/t6_cantilever_results.f06`
- `FEM4C/examples/t6_cantilever_results.vtk`
- `FEM4C/output.csv`
- `FEM4C/output.dat`
- `FEM4C/output.f06`
- `FEM4C/output.vtk`
- `FEM4C/q4_test.dat`
- `FEM4C/simple_t3_test.dat`
- `FEM4C/t6_correct_test.dat`
- `FEM4C/t6_fixed_test.dat`
- `FEM4C/t6_proper_test.dat`
- `FEM4C/t6_simple_test.dat`
- `FEM4C/t6_standard_test.dat`
- `FEM4C/test_parser_pkg/Boundary Conditions/boundary.dat`
- `FEM4C/test_parser_pkg/material/material.dat`
- `FEM4C/test_parser_pkg/mesh/mesh.dat`
- `FEM4C/test/output/mixed_t3_q4.vtk`
- `FEM4C/test/output/nastran_openmp_simple.vtk`
- `FEM4C/test/output/nastran_simple.vtk`
- `FEM4C/test/output/q4_simple.vtk`
- `FEM4C/test/output/simple_2d_test.vtk`
- `FEM4C/test/output/t3_simple.vtk`
- `FEM4C/test/output/t6_cantilever_phase2.vtk`
- `FEM4C/test/output/t6_simple_phase2.vtk`

### C. 意図不明（レビュー後に採否決定）
- `FEM4C/FEM4C_Reference_Manual.md`
- `FEM4C/docs/01_requirements.md`
- `FEM4C/docs/03_design.md`
- `FEM4C/docs/FEM_LEARNING_GUIDE.md`
- `FEM4C/docs/tutorial_manual.md`
- `FEM4C/practice/README.md`
- `FEM4C/src/elements/t3/t3_element.c`
- `FEM4C/src/io/input.c`
- `FEM4C/src/solver/cg_solver.c`
- `FEM4C/test/data/*` / `FEM4C/test/unit/*` は Section 2 の最終判定に移管。

## 2) `FEM4C/test/*` 削除群の最終判定（C-1）

### 復元確定（削除を採用しない）
- `FEM4C/test/data/cantilever_beam.dat`
- `FEM4C/test/data/cantilever_beam_fixed.dat`
- `FEM4C/test/data/comprehensive_2d_test.dat`
- `FEM4C/test/data/fixed_2d_test.dat`
- `FEM4C/test/data/mixed_t3_q4.dat`
- `FEM4C/test/data/nastran_mixed_openmp.nas`
- `FEM4C/test/data/nastran_openmp_simple.nas`
- `FEM4C/test/data/nastran_simple.nas`
- `FEM4C/test/data/q4_simple.dat`
- `FEM4C/test/data/simple_2d_test.dat`
- `FEM4C/test/data/simple_t6_test.dat`
- `FEM4C/test/data/t3_simple.dat`
- `FEM4C/test/data/t6_beam.dat`
- `FEM4C/test/data/t6_cantilever.dat`
- `FEM4C/test/data/t6_minimal.dat`
- `FEM4C/test/data/t6_simple.dat`
- `FEM4C/test/data/t6_simple_beam.dat`
- `FEM4C/test/data/t6_test_beam.dat`
- `FEM4C/test/data/validation_test.dat`
- `FEM4C/test/data/vtk_test.dat`
- `FEM4C/test/unit/test_t6_element.c`

最終判定理由:
- `test/data` と `test/unit` は回帰入力/テストコード本体であり、削除すると検証導線が喪失する。
- PM-3 の受入確認で必要な再現ケース維持のため、削除差分は採用しない（復元確定）。

### 削除確定（削除を採用する）
- `FEM4C/test/output/mixed_t3_q4.vtk`
- `FEM4C/test/output/nastran_openmp_simple.vtk`
- `FEM4C/test/output/nastran_simple.vtk`
- `FEM4C/test/output/q4_simple.vtk`
- `FEM4C/test/output/simple_2d_test.vtk`
- `FEM4C/test/output/t3_simple.vtk`
- `FEM4C/test/output/t6_cantilever_phase2.vtk`
- `FEM4C/test/output/t6_simple_phase2.vtk`

最終判定理由:
- `test/output/*.vtk` は生成結果で再生成可能なため、追跡対象から外す。

## 3) 安全な `git add` 手順（混在コミット回避）

### 手順A: PM-3 実装だけを先に積む
```bash
git add FEM4C/Makefile FEM4C/src/fem4c.c \
  FEM4C/src/analysis/runner.c FEM4C/src/analysis/runner.h \
  FEM4C/src/mbd/constraint2d.c FEM4C/src/mbd/constraint2d.h \
  FEM4C/src/mbd/kkt2d.c FEM4C/src/mbd/kkt2d.h
```

### 手順B: Cチーム成果（本レポート＋進捗ログ）を別枠で積む
```bash
git add docs/fem4c_dirty_diff_triage_2026-02-06.md docs/team_status.md docs/session_continuity_log.md
```

### 手順C: 生成物削除だけを独立コミットにする場合
```bash
git add -u FEM4C/examples FEM4C/output.csv FEM4C/output.dat FEM4C/output.f06 FEM4C/output.vtk FEM4C/test/output
```

### 手順D: 誤ステージの巻き戻し
```bash
git restore --staged FEM4C/test/data FEM4C/test/unit FEM4C/test_parser_pkg FEM4C/q4_test.dat FEM4C/simple_t3_test.dat
```

### 手順E: 最終確認
```bash
git diff --cached --name-status
git status --short
```

## 4) 生成物除外ポリシー（C-2）
- 生成物はコミット対象外（`FEM4C/output*`, `FEM4C/out_*`, `FEM4C/examples/*_results.*`, `FEM4C/test/output/*`）。
- MBD検証の標準出力 `out_mbd.dat` は「ローカル確認専用」とし、コミット禁止。
- `.gitignore` で `FEM4C/out_*.dat|csv|vtk|f06` と `FEM4C/test/output/` を除外設定する。

運用チェックコマンド:
```bash
git check-ignore -v FEM4C/out_mbd.dat FEM4C/out_mbd.csv FEM4C/test/output/sample.vtk
git status --short --untracked-files=all -- FEM4C
```

## 5) 補足
- 本レポートの `FEM4C/test/*` 判定は最終版。`意図不明` 群のみ PM レビュー後に再分類する。
- `chrono-2d` 側の dirty 差分（バイナリ/生成物含む）は今回スコープ外として、同一コミットへ混在させない。

## 6) C-4 判定済み差分（本セッション）

最終判定を付与した差分（14件）:
- 残す（採用）:
  - `FEM4C/docs/00_tutorial_requirements.md`
  - `FEM4C/docs/implementation_guide.md`
  - `FEM4C/USAGE_PARSER.md`
  - `FEM4C/NastranBalkFile/3Dtria_example.dat`
- 削除維持（採用）:
  - `FEM4C/PHASE2_IMPLEMENTATION_REPORT.md`
  - `FEM4C/T6_PROGRESS_REPORT.md`
  - `FEM4C/docs/02_file_structure.md`
  - `FEM4C/docs/04_progress.md`
  - `FEM4C/docs/05_handover_notes.md`
  - `FEM4C/docs/06_fem4c_implementation_history.md`
  - `FEM4C/docs/RELEASE_README.md`
  - `FEM4C/test_parser_pkg/Boundary Conditions/boundary.dat`
  - `FEM4C/test_parser_pkg/material/material.dat`
  - `FEM4C/test_parser_pkg/mesh/mesh.dat`

判定根拠:
- `00_tutorial_requirements.md` / `implementation_guide.md` / `USAGE_PARSER.md` / `3Dtria_example.dat` は `README`・`tutorial_manual`・`practice/README` から参照されているため採用。
- 旧進捗/履歴系 docs は現行 docs index (`FEM4C/docs/README.md`) の canonical 対象外で参照されていないため、削除維持を採用。
- `test_parser_pkg/*` は parser生成物由来で現行参照がなく、追跡維持の根拠がないため削除維持を採用。

## 7) C-5 Blocker 詳細（履歴）

PM判断待ちの高リスク差分:
- `FEM4C/src/io/input.c`（論点 #1）
  - PM決定: Option A（旧 `SPC/FORCE` 互換維持、2026-02-07）。
  - 状態: 解決済み（未決 blocker から除外）。
- `FEM4C/src/solver/cg_solver.c`
  - 大半は改行正規化だが、零曲率判定が `TOLERANCE` から固定値 `1.0e-14` に変更。
  - 収束/特異判定の閾値変更が数値挙動に影響する可能性がある。
- `FEM4C/src/elements/t3/t3_element.c`
  - 負のヤコビアン時に節点順序を自動入れ替える補正ロジックを追加。
  - PM決定: Option B（既定は自動補正 + `--strict-t3-orientation` で即エラー、2026-02-07）。

## 8) C-5 試行結果と暫定判定（更新: #1解決済み）

### `FEM4C/src/io/input.c`
- 試行:
  - `cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/c5_parser_eval.dat`
  - `cd FEM4C && ./bin/fem4c /tmp/parser_pkg_old /tmp/c5_parser_old_eval.dat`
- 結果:
  - 新形式（`Fix/Force`）の parser package は計算成功（PASS）。
  - 旧形式（`SPC/FORCE`）の parser package でも `Applied 2 boundary conditions` と荷重反映を確認（PASS）。
- 判定:
  - `解決済み（Option A採用済み）`。
- PM決定（2026-02-07）:
  - `Option A` を採用（旧 `SPC/FORCE` 互換を維持）。
  - 旧 `NastranBalkFile` 入力は継続サポートし、無言無視は不合格とする。
  - `input_read_parser_boundary()` で旧/新形式を同一内部データへ正規化する実装を実施する。
- PM判断依頼:
  - なし（クローズ済み）。

### `FEM4C/src/solver/cg_solver.c`
- 試行:
  - `rg -n "#define\\s+TOLERANCE|TOLERANCE" FEM4C/src/common FEM4C/src/solver/cg_solver.c`
  - `git diff -w -- FEM4C/src/solver/cg_solver.c`
  - `make -C FEM4C`
  - `make -C FEM4C test`
  - `make -C FEM4C mbd_checks`
  - `cd FEM4C && ./bin/fem4c examples/t3_cantilever_beam.dat /tmp/c5_t3_cg_eval.dat`
  - `cd FEM4C && ./bin/fem4c examples/q4_cantilever_beam.dat /tmp/c5_q4_cg_eval.dat`
  - `cd FEM4C && ./bin/fem4c examples/t6_cantilever_beam.dat /tmp/c5_t6_cg_eval.dat`
  - `cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/c5_parser_eval.dat`
- 結果:
  - `TOLERANCE=1.0e-8`（`constants.h`）に対し、零曲率判定は一時的に `1.0e-14` へ変更されていた（`git diff -w` でロジック差分はこの1点）。
  - `fabs(pAp) < TOLERANCE` を試行すると、`3Dtria_example` で `Zero curvature in CG iteration 289` が再現（`pAp=9.615406e-09`）。
  - 安定性を優先して `fabs(pAp) < 1.0e-14` を維持した場合、`make -C FEM4C` / `make -C FEM4C test` / `mbd_checks` / 実行4ケース（T3/Q4/T6/parser）は PASS。
- 判定:
  - `解決済み（Option Aを採用）`。
- PM決定（2026-02-07）:
  - `Option B` 試行は実行退行（3Dtria parser fail）のため不採用。
  - `Option A` を採用（`fabs(pAp) < 1.0e-14` を維持）。
- PM判断依頼:
  - なし（クローズ済み）。

### `FEM4C/src/elements/t3/t3_element.c`
- 試行:
  - `rg -n "orientation corrected|det_J < 0|strict|t3_validate_element" FEM4C/src/elements/t3/t3_element.c`
  - `rg -n "strict|orientation" FEM4C/src FEM4C/docs FEM4C/practice -g'*.c' -g'*.h' -g'*.md'`
  - `cd FEM4C && ./bin/fem4c /tmp/t3_clockwise.dat /tmp/c5_t3_clockwise_eval.dat`
  - `cd FEM4C && ./bin/fem4c --strict-t3-orientation /tmp/t3_clockwise.dat /tmp/c5_t3_clockwise_strict_eval.dat`
  - `cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/c5_parser_eval.dat`
- 結果:
  - 既定モードでは clockwise 要素に `Warning: T3 element orientation corrected` を出して計算継続（PASS）。
  - strictモード（`--strict-t3-orientation`）では clockwise 要素で非0終了（PASS: 期待どおり失敗）。
  - parser 実行でも同警告付きで収束（PASS）。
- 判定:
  - `解決済み（Option Bを採用）`。
- PM決定（2026-02-07）:
  - 既定挙動は自動補正（警告ログ維持）を採用。
  - `--strict-t3-orientation` 指定時は clockwise 要素を即エラー（non-zero）とする。
- PM判断依頼:
  - なし（クローズ済み）。

## 9) C-5 PM判断オプション表（C-7）

### `FEM4C/src/io/input.c`
- Option A: 旧 `SPC/FORCE` 互換を復元して採用。
- Option B: 旧形式は明示エラー（non-zero + 行番号）に変更して採用。
- Option C: 現差分を破棄し、旧仕様を維持。
- PM決定: Option A（2026-02-07確定）。

### `FEM4C/src/solver/cg_solver.c`
- Option A: 固定閾値 `1.0e-14` を採用し、理由を docs に明記。
- Option B: `TOLERANCE` 連動へ戻して採用。
- Option C: 現差分を保留し、閾値だけ先行差し戻し。
- PM決定: Option A（2026-02-07確定, Option Bは3Dtria回帰で失敗）。

### `FEM4C/src/elements/t3/t3_element.c`
- Option A: 自動補正を既定動作として採用（警告ログ維持）。
- Option B: strict mode フラグで自動補正を切替可能にして採用。
- Option C: 従来通り即エラーへ戻す。
- 推奨: Option B（既定は互換維持、厳格運用にも対応可能）。
- PM決定: Option B（2026-02-07確定）。

PM回答待ちの安全運用（履歴）:
```bash
git restore --staged FEM4C/src/elements/t3/t3_element.c
git add docs/fem4c_dirty_diff_triage_2026-02-06.md docs/team_status.md docs/session_continuity_log.md
git diff --cached --name-status
```

## 10) C-5 即時反映プレイブック（PM決定後）

### 10.1 `FEM4C/src/io/input.c`（境界条件互換）
- 採用決定:
  - Option A（2026-02-07 PM確定）。
- 状態:
  - 解決済み（C-5未決 blocker から除外）。
- 差分案:
  - Option A: 旧 `SPC/FORCE` を `input_read_parser_boundary()` 内で併読し、`Fix/Force` と同じ内部データへ正規化。
  - Option B: 旧 `SPC/FORCE` を検出した時点で行番号付き `FEM_ERROR_INVALID_INPUT` を返す（無言無視を禁止）。
  - Option C: 当該ファイル差分を破棄し、現行仕様へ戻す。
- 検証コマンド:
```bash
make -C FEM4C
cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/fem4c_parser_new.dat
cd FEM4C && ./bin/fem4c /tmp/parser_pkg_old /tmp/fem4c_parser_old.dat
```
- pass/fail 判定:
  - Option A: 新旧入力とも non-zero にならず、旧入力でも BC/荷重が無視されない。
  - Option B: 旧入力が「行番号つきエラー + non-zero」で停止する。
  - Option C: 旧仕様挙動に戻ることを `git diff -- FEM4C/src/io/input.c` で確認。

### 10.2 `FEM4C/src/solver/cg_solver.c`（零曲率閾値）
- 採用決定:
  - Option A（2026-02-07 PM確定）。
- 状態:
  - 解決済み（C-5未決 blocker から除外）。
- 差分案:
  - Option A: `fabs(pAp) < 1.0e-14` を維持し、理由を docs へ追記。
  - Option B: `fabs(pAp) < TOLERANCE` へ戻し、既存定数に統一。
  - Option C: 改行正規化のみ採用し、閾値変更だけ差し戻し。
- 検証コマンド:
```bash
make -C FEM4C
make -C FEM4C mbd_checks
cd FEM4C && ./bin/fem4c examples/t3_cantilever_beam.dat /tmp/fem4c_t3_cg.dat
```
- pass/fail 判定:
  - 上記3コマンドが全て exit 0。
  - CG失敗時は `Zero curvature` 発生条件をログに残し、閾値選択を再判定。

### 10.3 `FEM4C/src/elements/t3/t3_element.c`（負ヤコビアン補正）
- 採用決定:
  - Option B（2026-02-07 PM確定）。
- 状態:
  - 解決済み（C-5未決 blocker から除外）。
- 差分案:
  - Option A: 現行の自動補正を採用（警告ログ維持）。
  - Option B: strict mode（自動補正を無効化可能）を追加して採用。
  - Option C: 従来どおり即エラーへ戻す。
- 検証コマンド:
```bash
make -C FEM4C
cd FEM4C && ./bin/fem4c /tmp/t3_clockwise.dat /tmp/t3_clockwise_check.dat
cd FEM4C && ./bin/fem4c --strict-t3-orientation /tmp/t3_clockwise.dat /tmp/t3_clockwise_strict_check.dat
cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/fem4c_t3_parser.dat
```
- pass/fail 判定:
  - Option A/B: clockwise ケースで期待どおり「補正継続」または strict エラーが再現する。
  - Option C: clockwise ケースが即エラーで停止する。

### 10.4 安全 staging（PM決定反映時）
```bash
# 1) 3ファイルを一旦ステージ解除（誤混在防止）
git restore --staged FEM4C/src/io/input.c FEM4C/src/solver/cg_solver.c FEM4C/src/elements/t3/t3_element.c

# 2) PM採用ファイルのみ個別に stage
git add FEM4C/src/io/input.c
git add FEM4C/src/solver/cg_solver.c
git add FEM4C/src/elements/t3/t3_element.c

# 3) Cチーム docs は別コミット候補として stage
git add docs/fem4c_dirty_diff_triage_2026-02-06.md docs/fem4c_team_next_queue.md docs/team_status.md docs/session_continuity_log.md

# 4) 最終確認（FEM4C/chrono-2d 混在禁止）
git diff --cached --name-status
git status --short
```

運用メモ:
- PMが「破棄」を選んだファイルは `git restore --worktree --staged <path>` で差分を戻す。
- PMが「採用」を選んだファイルのみ `git add <path>` し、3ファイル一括 stage を禁止する。

## 11) 最新 PM判断依頼（2026-02-07 更新）
- 論点 #1 `input.c`: 解決済み（Option A採用、判断依頼なし）。
- 論点 #2 `cg_solver.c`: 解決済み（Option A採用、`1.0e-14` 維持）。
- 論点 #3 `t3_element.c`: 解決済み（Option B採用、既定自動補正 + `--strict-t3-orientation`）。

## 12) C-11 回帰導線（strict orientation）
- 目的:
  - Option B（既定=自動補正継続、strict=即エラー）を1コマンドで再現できるよう固定する。
- 追加:
  - `FEM4C/scripts/check_t3_orientation_modes.sh`
  - `FEM4C/Makefile` ターゲット `t3_orientation_checks`
  - `FEM4C/practice/README.md` に実行導線を追記
- 実行コマンド:
```bash
make -C FEM4C t3_orientation_checks
```
- pass/fail 判定:
  - default 実行: clockwise T3 で補正警告が出て exit 0。
  - strict 実行: 同入力で non-zero（期待どおり失敗）。

## 13) C-12 安全 staging 最終確認（2026-02-07）
- 目的:
  - C-5確定済み3ファイル + Cチーム docs の staging セットが、混在なしで成立することを最終確認する。
- 実施:
  - `GIT_INDEX_FILE` を一時 index に切替え、実 index を汚さずに staging コマンドをドライラン。
  - `git diff --cached --name-status` を検査し、`chrono-2d/` と `.github/` が staged set に入らないことを確認。
- ドライラン結果（cached set）:
```text
M FEM4C/src/elements/t3/t3_element.c
M FEM4C/src/io/input.c
M FEM4C/src/solver/cg_solver.c
M docs/fem4c_dirty_diff_triage_2026-02-06.md
M docs/fem4c_team_next_queue.md
M docs/session_continuity_log.md
M docs/team_status.md
```
- 追加安全証跡（連続ソーク）:
  - `examples/t6_cantilever_beam.dat` を 220 回連続実行し、`Zero curvature` と non-zero 終了が発生しないことを確認。
  - 結果: `SOAK_DONE total=220`（失敗ログなし）。
- 結論:
  - C-12 の staging 手順は現行ワークツリーに対して安全に適用可能。

## 14) C-13 staging dry-run 定型手順（2026-02-08）
- 目的:
  - 一時 index（`GIT_INDEX_FILE`）を使う dry-run を毎回同じ手順で実行し、混在コミット検査の再現性を固定する。
- 前提:
  - 現在の作業ディレクトリが repo root（`/home/rmaen/highperformanceFEM`）であること。
  - 対象セットは `FEM4C/src/io/input.c`, `FEM4C/src/solver/cg_solver.c`, `FEM4C/src/elements/t3/t3_element.c` と Cチーム docs 一式。
- コマンド（定型）:
```bash
TMP_INDEX=$(mktemp /tmp/c_stage_dryrun.index.XXXXXX)
cp .git/index "$TMP_INDEX"
GIT_INDEX_FILE="$TMP_INDEX" git restore --staged FEM4C/src/io/input.c FEM4C/src/solver/cg_solver.c FEM4C/src/elements/t3/t3_element.c || true
GIT_INDEX_FILE="$TMP_INDEX" git add FEM4C/src/io/input.c FEM4C/src/solver/cg_solver.c FEM4C/src/elements/t3/t3_element.c
GIT_INDEX_FILE="$TMP_INDEX" git add docs/fem4c_dirty_diff_triage_2026-02-06.md docs/fem4c_team_next_queue.md docs/team_status.md docs/session_continuity_log.md
GIT_INDEX_FILE="$TMP_INDEX" git diff --cached --name-status
```
- コマンド（推奨ラッパー）:
```bash
scripts/c_stage_dryrun.sh --add-target docs/team_runbook.md --log /tmp/c_stage_dryrun_YYYYMMDD.log
```
- 判定基準:
  - `PASS`: cached staged set に `chrono-2d/` と `.github/` が含まれない。
  - `PASS`: 必須7ファイル（3実装 + 4docs）がすべて staged set に存在する。
  - `FAIL`: 上記どちらかを満たさない。
- `team_status` 記録フォーマット（定型）:
```text
dryrun_method=GIT_INDEX_FILE
dryrun_cached_list=<git diff --cached --name-status の結果>
forbidden_check=pass|fail
required_set_check=pass|fail
dryrun_result=pass|fail
```
- 運用メモ:
  - dry-run の出力保存先は `/tmp/c_stage_dryrun_<date>.log` を推奨する。
  - 実 index を変更しないため、通常の `git status` と結果が異なることは正常。
  - failパス検証（混在検出）は以下で再現可能:
```bash
scripts/c_stage_dryrun.sh --add-target chrono-2d/tests/test_coupled_constraint
```

## 15) C-14 failパス検証（2026-02-08）
- 実行コマンド（passケース）:
```bash
scripts/c_stage_dryrun.sh --add-target docs/team_runbook.md --log /tmp/c14_dryrun_pass.log
```
- 結果:
  - `forbidden_check=pass`
  - `required_set_check=pass`
  - `dryrun_result=pass`
- 実行コマンド（failケース）:
```bash
scripts/c_stage_dryrun.sh --add-target chrono-2d/tests/test_coupled_constraint --log /tmp/c14_dryrun_fail.log
```
- 結果:
  - `forbidden_check=fail`
  - `dryrun_result=fail`
  - exit code `1`（期待どおり）
- 結論:
  - 定型 dry-run は pass/fail の両経路で期待どおり判定できる。

## 16) C-18 最終判定（2026-02-08, 短時間スモーク + staging運用）
- 短時間スモーク（最大3コマンド）:
```bash
make -C FEM4C
cd FEM4C && ./bin/fem4c examples/t6_cantilever_beam.dat /tmp/c18_t6_smoke.dat
cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/c18_parser_smoke.dat
```
- スモーク結果:
  - 3コマンドすべて exit 0（PASS）。
  - `/tmp/c18_t6_smoke.log` と `/tmp/c18_parser_smoke.log` で `Zero curvature` 未発生（PASS）。
  - parserケースでは `Warning: T3 element orientation corrected` を確認（Option B 既定挙動どおり）。
- `scripts/c_stage_dryrun.sh` 実行結果:
```bash
scripts/c_stage_dryrun.sh --add-target docs/team_runbook.md --log /tmp/c18_dryrun_pass.log
scripts/c_stage_dryrun.sh --add-target chrono-2d/tests/test_coupled_constraint --log /tmp/c18_dryrun_fail.log
```
  - passケース: `forbidden_check=pass`, `required_set_check=pass`, `dryrun_result=pass`。
  - failケース: `forbidden_check=fail`, `required_set_check=pass`, `dryrun_result=fail`（exit code `1`）。
- 最終判定（C-18）:
  - `FEM4C/src/io/input.c` -> 採用
    理由: Option A（旧 `SPC/FORCE` 互換）方針に整合し、parserスモークで non-zero/退行なし。
  - `FEM4C/src/solver/cg_solver.c` -> 採用
    理由: 短時間スモーク2ケースで `Zero curvature` 未発生、`1.0e-14` 維持方針と整合。
  - `FEM4C/src/elements/t3/t3_element.c` -> 採用
    理由: parserスモークで補正警告付き完走、既定補正 + strict切替の Option B と整合。
  - 破棄: なし（本ラウンドでは採用側のみ更新）。
- 安全 staging コマンド（実施手順）:
```bash
git add FEM4C/src/io/input.c FEM4C/src/solver/cg_solver.c FEM4C/src/elements/t3/t3_element.c
git add docs/fem4c_dirty_diff_triage_2026-02-06.md docs/fem4c_team_next_queue.md docs/team_status.md docs/session_continuity_log.md
git diff --cached --name-status
```
- C-19 への遷移メモ:
```bash
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze
```
  - 上記が PASS であることを、C-team報告の位置（`## Cチーム` 配下）と coupled凍結ポリシーを含む受入条件とする。

## 17) C-19 監査導線（Done, 2026-02-08）
- 目的:
  - C-team の dry-run 記録を PMが1コマンドで機械監査できる状態を固定する。
- 追加/更新:
  - `scripts/check_c_team_dryrun_compliance.sh`（新規）
  - `scripts/run_c_team_staging_checks.sh`（新規）
  - `scripts/audit_c_team_staging.py`（`--require-c-section`, `--require-coupled-freeze`, `--global-fallback`, `--coupled-freeze-file` 追加）
  - `scripts/run_team_audit.sh`（C dry-run policy: `pass|pass_section|pass_section_freeze|both|both_section|both_section_freeze|none`）
- 実行コマンド:
```bash
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze
bash scripts/run_c_team_staging_checks.sh docs/team_status.md
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_run_team_audit.py
python scripts/test_run_c_team_staging_checks.py
```
- 結果:
  - `pass`: PASS（dry-run 結果が最新C報告に記録されていることを確認）。
  - `pass_section`: PASS（最新C報告が `## Cチーム` 配下に存在することを確認）。
  - `pass_section_freeze`: PASS（`## Cチーム` 配下 + coupled凍結対象パスを含まないことを確認）。
  - テスト4本: PASS。

## 18) C-20 coupled凍結禁止パスの外部定義化（Done, 2026-02-09）
- 目的:
  - coupled凍結監査の禁止パス集合をコード外に分離し、PM運用で安全に更新できるようにする。
- 追加/更新:
  - `scripts/c_coupled_freeze_forbidden_paths.txt`（新規、禁止パス定義）
  - `scripts/check_c_coupled_freeze_file.py`（新規、禁止パス定義の品質検査）
  - `scripts/check_c_stage_dryrun_report.py`（新規、dry-runログ契約検査）
  - `scripts/audit_c_team_staging.py`（禁止パスファイル読込、構造化パス抽出、監査出力の追跡情報追加）
  - `scripts/check_c_team_dryrun_compliance.sh`（`COUPLED_FREEZE_FILE` 対応）
  - `scripts/run_team_audit.sh`（一時JSONを `mktemp` 化、`COUPLED_FREEZE_FILE` + timer strict policy連携）
  - `scripts/c_stage_dryrun.sh`（`coupled_freeze_check` と `safe_stage_command` 出力を追加）
  - `scripts/run_c_team_staging_checks.sh`（禁止パスファイル precheck + dry-runログ契約検査 + 検査テスト実行を追加）
  - `scripts/test_audit_c_team_staging.py` / `scripts/test_check_c_team_dryrun_compliance.py` / `scripts/test_check_c_coupled_freeze_file.py` / `scripts/test_check_c_stage_dryrun_report.py` / `scripts/test_c_stage_dryrun.py` / `scripts/test_run_c_team_staging_checks.py`（回帰追加）
- 実行コマンド:
```bash
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_check_c_coupled_freeze_file.py
python scripts/test_check_c_stage_dryrun_report.py
python scripts/test_c_stage_dryrun.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_run_team_audit.py
bash scripts/run_c_team_staging_checks.sh docs/team_status.md
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer
C_DRYRUN_POLICY=pass_section_freeze_timer_safe bash scripts/run_c_team_staging_checks.sh docs/team_status.md
C_DRYRUN_POLICY=pass_section_freeze_timer bash scripts/run_c_team_staging_checks.sh docs/team_status.md
scripts/c_stage_dryrun.sh --log /tmp/c_stage_dryrun_auto.log
python scripts/check_c_stage_dryrun_report.py /tmp/c_stage_dryrun_auto.log --policy pass
```
- 中間結果:
  - 構造化パス抽出で、実行コマンド中の参照パスによる coupled凍結誤検知を抑制。
  - 並列テスト時の `run_team_audit.sh` 一時JSON衝突を `mktemp` 化で解消。
  - `c_stage_dryrun` と coupled凍結禁止パターンの同期を完了し、dry-run出力から安全 staging コマンドを再利用可能にした。

## 19) C-21 strict-safe 監査既定化（Done, 2026-02-09）
- 目的:
  - C-team 提出を `pass_section_freeze_timer_safe` 基準で受入可能にし、safe staging 記録を監査必須にする。
- 現状:
  - `pass_section_freeze_timer_safe` は最新C報告に `dryrun_result` / `safe_stage_command` / timer完了値が未記録のため FAIL（想定どおり）。
  - 本セッションの C-team 報告へ raw 出力（`dryrun_result=pass`, `safe_stage_command=git add ...`, timer end）を反映後、strict-safe 監査を PASS 化して既定運用へ移行する。
- 追加/更新:
  - `scripts/audit_c_team_staging.py`（`safe_stage_command` の値を抽出し、`git add` 形式でない場合は `safe_stage_command_not_git_add` を返すよう更新）
  - `scripts/check_c_stage_dryrun_report.py`（`safe_stage_command` が `git add` 形式で、`safe_stage_targets` と同一集合であることを検査）
  - `scripts/render_c_stage_team_status_block.py`（`c_stage_dryrun` ログから `team_status` 用の記録ブロックを生成）
  - `scripts/check_c_team_submission_readiness.sh`（strict-safe + C単独30分監査の提出前一括確認）
  - `scripts/test_check_c_stage_dryrun_report.py`（`safe_stage_command_not_git_add` / target mismatch の回帰追加）
  - `scripts/test_render_c_stage_team_status_block.py`（記録ブロック生成の回帰追加）
  - `scripts/test_audit_c_team_staging.py`（`safe_stage_command_not_git_add` の回帰追加）
  - `scripts/test_check_c_team_dryrun_compliance.py`（strict-safe での `git add` 形式必須回帰を追加）
  - `scripts/test_check_c_team_submission_readiness.py`（strict-safe 提出前ゲートの回帰維持）
- 実行コマンド:
```bash
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_stage_dryrun_report.py
python scripts/test_render_c_stage_team_status_block.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_run_team_audit.py
```
- 結果:
  - 追加した strict-safe 条件（`safe_stage_command=git add ...`）に対する回帰テストは PASS。
  - `team_status` 最新Cエントリへ raw 出力（`dryrun_result`/`safe_stage_command`/timer）反映後、`pass_section_freeze_timer_safe` / `check_c_team_submission_readiness.sh docs/team_status.md 30` が PASS。
  - `docs/fem4c_team_next_queue.md` に C-22（dry-run 記録ブロック自動生成）を Auto-Next として追加済み。

## 20) C-22 dry-run 記録ブロック自動生成（Done, 2026-02-09）
- 目的:
  - `c_stage_dryrun` ログを `team_status` 記録形式へ自動変換し、転記ミスで strict-safe 監査が落ちるリスクを下げる。
- 追加/更新:
  - `scripts/render_c_stage_team_status_block.py`
  - `scripts/test_render_c_stage_team_status_block.py`
  - `scripts/run_c_team_staging_checks.sh`（render step + 出力先環境変数対応を追加）
- 実行コマンド:
```bash
python scripts/test_render_c_stage_team_status_block.py
python scripts/render_c_stage_team_status_block.py /tmp/c21_dryrun_20260209T0944Z.log
python scripts/render_c_stage_team_status_block.py /tmp/c_stage_dryrun_auto.log --output /tmp/c_stage_team_status_block.md
C_DRYRUN_POLICY=pass_section_freeze_timer_safe bash scripts/run_c_team_staging_checks.sh docs/team_status.md
C_TEAM_STATUS_BLOCK_OUT=/tmp/c22_team_status_block_from_checks.md bash scripts/run_c_team_staging_checks.sh docs/team_status.md
```
- 結果:
  - 記録ブロック生成は PASS。
  - `--output` 指定時に貼り付け用 markdown を再利用可能なファイルとして出力できることを確認。
  - staging bundle の既定チェックに render step と `team_status_block_output` 出力を追加後も PASS を確認。

## 21) C-23 C-team 報告ブロック適用自動化（Done, 2026-02-09）
- 目的:
  - 生成済み dry-run 記録ブロックを `team_status` 最新Cエントリへ反映する手作業をさらに削減する。
- 現状:
  - C-22 で「生成」までは自動化済み。C-23 では「安全適用」の補助スクリプトを追加して運用固定を進める。
- 追加/更新:
  - `scripts/apply_c_stage_block_to_team_status.py`
  - `scripts/test_apply_c_stage_block_to_team_status.py`
  - `scripts/apply_c_stage_block_to_team_status.py` に `--target-start-epoch` を追加（適用先を明示）
  - `scripts/run_c_team_staging_checks.sh`（`C_APPLY_BLOCK_TO_TEAM_STATUS=1` で自動適用可能に拡張）
  - `docs/team_runbook.md`（適用コマンドを追記）
  - `docs/fem4c_team_next_queue.md`（C-23 Scope/Acceptance/Verification を詳細化）
- 実行コマンド:
```bash
python scripts/test_apply_c_stage_block_to_team_status.py
python scripts/apply_c_stage_block_to_team_status.py --team-status docs/team_status.md --block-file /tmp/c22_team_status_block.md --in-place
python scripts/apply_c_stage_block_to_team_status.py --team-status docs/team_status.md --block-file /tmp/c22_team_status_block.md --target-start-epoch 1770628846 --in-place
C_APPLY_BLOCK_TO_TEAM_STATUS=1 C_TEAM_STATUS_BLOCK_OUT=/tmp/c23_team_status_block.md bash scripts/run_c_team_staging_checks.sh docs/team_status.md
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer_safe
```
- 結果:
  - 最新 C エントリの dry-run ブロックを自動差し替えできることを確認。
  - `--target-start-epoch` で適用対象を固定して更新できることを確認。
  - run_c staging checks からの自動適用（`team_status_block_apply=updated`）でも PASS を維持することを確認。

## 22) C-24 C-team セッション記録雛形の自動生成（Done, 2026-02-14）
- 目的:
  - `session_timer` の start/end 生出力と dry-run 記録から `team_status` セッション雛形を自動生成し、報告作成を標準化する。
- 追加/更新:
  - `scripts/render_c_team_session_entry.py`
  - `scripts/test_render_c_team_session_entry.py`
  - `scripts/render_c_team_session_entry.py` に `--collect-timer-end` / `--timer-end-output` を追加
  - `scripts/render_c_team_session_entry.py` に `--timer-guard-file` / `--collect-timer-guard` / `--guard-minutes` / `--timer-guard-output` を追加
  - `scripts/collect_c_team_session_evidence.sh`（dry-run + guard + end + entry 生成の一括実行）
  - `scripts/test_collect_c_team_session_evidence.py`
  - `scripts/append_c_team_entry.py`（生成済み entry を `## Cチーム` へ追記）
  - `scripts/test_append_c_team_entry.py`
  - `docs/team_runbook.md`（雛形生成コマンドを追記）
  - `docs/fem4c_team_next_queue.md`（C-24 Auto-Next 起票）
- 実行コマンド:
```bash
python scripts/test_render_c_team_session_entry.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_append_c_team_entry.py
python scripts/render_c_team_session_entry.py --task-title "C-24 着手" --session-token <token> --timer-end-file <end_file> --dryrun-block-file /tmp/c_stage_team_status_block.md --output /tmp/c_team_session_entry.md
python scripts/render_c_team_session_entry.py --task-title "C-24 着手" --session-token <token> --collect-timer-end --collect-timer-guard --guard-minutes 0 --timer-end-output /tmp/c_team_timer_end.txt --timer-guard-output /tmp/c_team_timer_guard.txt --output /tmp/c_team_session_entry.md
bash scripts/collect_c_team_session_evidence.sh --task-title "C-24 着手" --session-token <token> --guard-minutes 0 --entry-out /tmp/c_team_session_entry.md
bash scripts/collect_c_team_session_evidence.sh --task-title "C-24 着手" --session-token <token> --guard-minutes 0 --entry-out /tmp/c_team_session_entry.md --team-status docs/team_status.md --append-to-team-status
```
- 結果:
  - start/end 生出力と dry-run ブロックを含む `team_status` 向け雛形を生成できることを確認。
  - guard 出力（`SESSION_TIMER_GUARD`）を雛形へ含められることを確認。
  - `--collect-timer-end` で end 出力を自動取得し、雛形生成と同時に保存できることを確認。
  - `collect_c_team_session_evidence.sh` により dry-run/guard/end/entry を 1 コマンドで収集できることを確認。
  - 生成 entry に `scripts/c_stage_dryrun.sh --log <path>` の証跡が入り、strict-safe 監査要件を満たせることを確認。
  - `render_c_team_session_entry.py` の `--done-line/--in-progress-line/--command-line/--pass-fail-line` で報告雛形の手編集を削減できることを確認。
  - `append_c_team_entry.py` により生成済み entry を Cセクションへ安全に追記できることを確認。
  - `audit_c_team_staging.py` の timer完了監査で `<pending>` / `token missing` を検知して FAIL 化できることを確認。
  - 必須キー欠落時に non-zero fail となる回帰を追加した。

## 23) C-25 token-missing 復旧テンプレの半自動化（Done, 2026-02-14）
- 目的:
  - `token missing` 発生時の旧エントリ無効化を定型化し、再実行セッションへの切替を安全化する。
- 追加/更新:
  - `scripts/mark_c_team_entry_token_missing.py`
  - `scripts/test_mark_c_team_entry_token_missing.py`
  - `scripts/recover_c_team_token_missing_session.sh`
  - `scripts/test_recover_c_team_token_missing_session.py`
  - `scripts/audit_c_team_staging.py`（`<pending>` / `token missing` の監査失敗化）
  - `scripts/test_audit_c_team_staging.py`
  - `scripts/test_check_c_team_dryrun_compliance.py`
  - `scripts/test_check_c_team_submission_readiness.py`
- 実行コマンド:
```bash
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_mark_c_team_entry_token_missing.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/mark_c_team_entry_token_missing.py --team-status docs/team_status.md --target-start-epoch <start_epoch> --token-path <missing_token> --in-place
bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --target-start-epoch <start_epoch> --token-path <missing_token> --new-team-tag c_team
bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <new_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe
```
- 結果:
  - strict-safe（timer完了）監査で `<pending>` / `token missing` を確実に FAIL 化できることを確認。
  - `mark_c_team_entry_token_missing.py` により旧エントリ無効化（`<pending>` 解消 + FAIL明示）を機械化できることを確認。
  - `recover_c_team_token_missing_session.sh` の `start` モードで「旧エントリ無効化 + 新規timer開始」を 1 コマンド化できることを確認。
  - `recover_c_team_token_missing_session.sh` の `finalize` モードで「証跡収集 + Cエントリ追記 + strict-safe確認」を 1 コマンド化できることを確認。
  - 復旧フロー全体を「開始1コマンド + 最終反映1コマンド」で再現でき、受入条件（3コマンド以内）を満たした。

## 24) C-26 token-missing 復旧運用の提出前ゲート固定（Done, 2026-02-14）
- 目的:
  - 復旧セッションの最終反映時に `strict-safe + elapsed` の提出前ゲートを同一コマンドに統合し、再提出品質を固定する。
- 追加/更新:
  - `scripts/render_c_team_session_entry.py`
    - `safe_stage_targets` を `変更ファイル` へ自動反映（changes evidence を自動付与）。
    - `c_stage_dryrun` 記録済み時に `実行コマンド` の `<記入>` を出さないよう改善。
    - `safe_stage_targets` 自動反映時は複数行へ分割し、`team_status` 記録の可読性を改善。
  - `scripts/collect_c_team_session_evidence.sh`
    - `--check-submission-readiness-minutes` を追加（`check_c_team_submission_readiness.sh` を任意実行）。
    - `--check-submission-readiness-minutes` は `--append-to-team-status` 必須にし、誤運用を防止。
    - readiness 実行時は Done/In Progress/pass-fail を既定値で自動補完。
    - strict-safe/readiness 失敗時に監査出力を stderr へそのまま表示し、失敗理由の切り分けを容易化。
  - `scripts/recover_c_team_token_missing_session.sh`
    - finalize モードに `--check-submission-readiness-minutes` を追加し、復旧最終反映から提出前ゲートまで一括実行可能化。
    - start モードで `next_finalize_command` を出力し、復旧2段目コマンドの転記ミスを削減。
  - `scripts/check_c_team_submission_readiness.sh`
    - `C_TEAM_SKIP_STAGING_BUNDLE=1` で staging bundle をスキップできるテストフックを追加。
  - `scripts/audit_c_team_staging.py` / `scripts/check_c_team_dryrun_compliance.sh`
    - strict-safe（`pass_section_freeze_timer_safe`）で `<記入>` などテンプレ残骸を `template_placeholder_detected` として FAIL 化。
  - `scripts/test_collect_c_team_session_evidence.py`
    - `--check-submission-readiness-minutes` の入力バリデーション（整数必須/append必須）を追加。
    - submission readiness 実行時の既定値補完（Done/pass-fail/changes evidence）を回帰化。
  - `scripts/test_recover_c_team_token_missing_session.py`
    - finalize モードの `--check-submission-readiness-minutes` バリデーションを追加。
    - finalize + submission readiness 実行の統合ケースを追加。
  - `scripts/test_render_c_team_session_entry.py`
    - `変更ファイル` 自動反映と `--change-line` の回帰を追加。
  - `scripts/test_check_c_team_submission_readiness.py`
    - `C_TEAM_SKIP_STAGING_BUNDLE=1` の挙動回帰を追加。
  - `scripts/test_audit_c_team_staging.py`
  - `scripts/test_check_c_team_dryrun_compliance.py`
    - template placeholder 残留時の strict-safe FAIL 回帰を追加。
- 実行コマンド:
```bash
python scripts/test_render_c_team_session_entry.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
```
- 現在の結果:
  - 新規オプションの入力バリデーション回帰は PASS。
  - `collect`/recover finalize から提出前ゲートを呼び出せることを確認。
  - strict-safe 監査に template 残骸 FAIL（`template_placeholder_detected`）を追加し、生成雛形側と整合したことを確認。
  - `run_c_team_staging_checks.sh` / `check_c_team_submission_readiness.sh` の回帰が PASS し、提出前ゲートの固定化を完了。

## 25) C-27 token-missing 復旧報告のテンプレ整合監査強化（Done, 2026-02-14）
- 目的:
  - 復旧報告のテンプレ整合（変更ファイル・実行コマンド・pass/fail）を strict-safe 監査と常に一致させ、差し戻し率を下げる。
- 追加/更新:
  - `scripts/audit_c_team_staging.py`
    - template placeholder 検知を `<記入>` 固定から汎用トークン検出（`<...>`）へ拡張し、`<PASS|FAIL>` など判定未確定の残骸も FAIL 化。
    - 監査レポートに `template_placeholders` を追加し、失敗時の具体トークンを即確認できるよう改善。
  - `scripts/collect_c_team_session_evidence.sh`
    - compliance/readiness 監査時は validation 用 `team_status`（一時ファイル）へ追記して先に監査し、PASS 時のみ本番 `team_status` を更新する preflight フローを追加。
    - strict-safe のみ実行時の既定 `pass/fail` 文言を `PASS（strict-safe compliance）` へ調整。
  - `scripts/test_audit_c_team_staging.py`
    - `<PASS|FAIL>` 残骸で strict-safe FAIL になる回帰を追加。
  - `scripts/test_check_c_team_dryrun_compliance.py`
    - `pass_section_freeze_timer_safe` で `<PASS|FAIL>` を reject する回帰を追加。
  - `scripts/test_check_c_team_submission_readiness.py`
    - readiness 監査で `<PASS|FAIL>` を reject する回帰を追加。
  - `scripts/test_collect_c_team_session_evidence.py`
    - compliance FAIL 時に `team_status` が無汚染（追記されない）であることを回帰テストで固定。
- 実行コマンド:
```bash
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
```
- 結果:
  - strict-safe/readiness 監査で `<PASS|FAIL>` を含むテンプレ残骸を確実に FAIL 化できることを確認。
  - `collect` / `recover finalize` の監査失敗時でも本番 `team_status` を更新しない preflight フローが PASS することを確認。
  - C-27 の受入条件（テンプレ整合監査 + 失敗時無汚染）を充足し、Done 化。

## 26) C-28 token-missing 復旧報告の preflight 認証ログ固定（Done, 2026-02-14）
- 目的:
  - preflight 監査結果（validation対象/最終反映）を運用ログへ明示し、復旧時の切り戻し判断を短時間化する。
- 追加/更新:
  - `scripts/collect_c_team_session_evidence.sh` に `preflight_mode` / `preflight_team_status` / `preflight_result` を追加し、validation対象と結果を出力に固定。
  - preflight 有効時は `check_c_team_dryrun_compliance.sh` / `check_c_team_submission_readiness.sh` の実行コマンドを entry へ自動追記するよう更新。
  - `scripts/check_c_team_collect_preflight_report.py` を追加し、collect出力の preflight 契約（mode/result/team_status）を機械検証可能化。
  - `scripts/recover_c_team_token_missing_session.sh` に `--collect-log-out` を追加し、finalize 実行時の collect ログ固定 + preflight 契約チェックを一括実行可能化。
  - `scripts/test_check_c_team_collect_preflight_report.py` を追加し、enabled/disabled/欠落キーfailを回帰化。
  - `python scripts/test_collect_c_team_session_evidence.py` へ preflight-only（appendなし）回帰を追加し、本番 `team_status` 無変更を確認。
  - `python scripts/test_recover_c_team_token_missing_session.py` で finalize 出力に preflight ログ + collect log 出力が含まれることを確認。
  - 2026-02-15 追補: `preflight_team_status` を append先の canonical path で記録するよう更新し、復旧ログの参照先を固定。
  - 2026-02-15 追補: `check_c_team_collect_preflight_report.py` に `--expect-team-status` を追加し、`recover --collect-log-out` で team_status 一致を機械検証可能化。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_check_c_team_collect_preflight_report.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - `collect` / `recover finalize` の preflight 契約ログ固定（mode/result/team_status）が機械検証で PASS。
  - C-28 の受入条件（preflight 判定ログ固定 + preflight-only運用 + 無汚染反映）を充足し、Done 化。

## 27) C-29 preflight 認証ログの staging bundle 統合（Done, 2026-02-15）
- 目的:
  - preflight 契約チェックを C-team の staging bundle 導線へ統合し、提出前品質を 1 導線で評価できるようにする。
- 進捗:
  - C-28 完了後の Auto-Next として C-29 を起票。
  - `docs/fem4c_team_next_queue.md` の C先頭を C-29 へ更新。
  - `scripts/run_c_team_staging_checks.sh` に `C_COLLECT_PREFLIGHT_LOG` / `C_REQUIRE_COLLECT_PREFLIGHT_ENABLED` を追加し、collectログ契約検査を optional step として統合。
  - `scripts/check_c_team_submission_readiness.sh` に collect preflight 直接検証ステップを追加し、`C_TEAM_SKIP_STAGING_BUNDLE=1` 時でも preflight 契約を同入口で確認可能化。
  - `scripts/test_run_c_team_staging_checks.py` に preflightログ検査の pass/fail ケースを追加し、bundle 統合の回帰を固定。
  - `scripts/test_check_c_team_submission_readiness.py` に collect preflight 連携の pass/fail 回帰を追加。
  - `scripts/run_c_team_staging_checks.sh` / `scripts/check_c_team_submission_readiness.sh` に `C_COLLECT_EXPECT_TEAM_STATUS` を追加し、bundle/readiness で preflight team_status の一致チェックを標準化。
  - `scripts/run_c_team_staging_checks.sh` に `C_SKIP_NESTED_SELFTESTS` を追加し、nested self-test の環境依存失敗を切り分けた上で preflight 契約検証を安定実行できるようにした。
  - `scripts/test_run_c_team_staging_checks.py` / `scripts/test_check_c_team_submission_readiness.py` に `C_COLLECT_EXPECT_TEAM_STATUS` の上書き運用（一致/不一致）回帰を追加。
  - `scripts/run_c_team_collect_preflight_check.sh` を追加し、collect preflight 検証ロジックを helper へ集約。
  - `scripts/run_c_team_staging_checks.sh` / `scripts/check_c_team_submission_readiness.sh` から helper を呼び出すよう更新し、`collect_preflight_check=skipped|pass` を共通出力化。
  - `scripts/test_run_c_team_collect_preflight_check.py` を追加し、helper の skip/pass/fail を回帰固定。
- 実行コマンド:
```bash
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_check_c_team_collect_preflight_report.py
C_SKIP_NESTED_SELFTESTS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
```
- 結果:
  - preflight 契約検証が bundle/readiness で同一導線となり、重複実装を解消。
  - `preflight_team_status` 一致検証を含む staging bundle/readiness 回帰が PASS。
  - C-29 の受入条件（bundle統合 + 明確FAIL理由 + team_status一致検証）を充足し、Done 化。

## 28) C-30 collect preflight ログ参照導線の自動化（Done, 2026-02-15）
- 目的:
  - collect preflight ログ参照の運用を標準化し、日次運用での手入力パラメータを削減する。
- 進捗:
  - C-29 完了後の Auto-Next として C-30 を起票。
  - `docs/fem4c_team_next_queue.md` と `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-30 へ更新。
  - `scripts/extract_c_team_latest_collect_log.py` を追加し、最新 C エントリから collect ログ候補（checker command / env / collect_log_out）を抽出可能化。
  - `scripts/run_c_team_collect_preflight_check.sh` に `C_COLLECT_PREFLIGHT_LOG=latest` と `C_COLLECT_LATEST_REQUIRE_FOUND=1` を追加。
  - `scripts/run_c_team_staging_checks.sh` の既定 dry-run/block 出力を `mktemp` 化し、並列実行時の `/tmp` 衝突を低減。
  - `scripts/render_c_team_session_entry.py` / `scripts/collect_c_team_session_evidence.sh` / `scripts/recover_c_team_token_missing_session.sh` を拡張し、生成エントリへ collect preflight ログ証跡を残せるようにした。
  - `scripts/test_extract_c_team_latest_collect_log.py` / `scripts/test_run_c_team_collect_preflight_check.py` / `scripts/test_run_c_team_staging_checks.py` / `scripts/test_check_c_team_submission_readiness.py` を拡張し、latest 解決の pass/skip/fail を回帰化。
  - 2026-02-15 追補: `run_c_team_staging_checks.sh` / `check_c_team_submission_readiness.sh` の `C_COLLECT_PREFLIGHT_LOG` 既定を `latest` 自動解決へ変更。
  - 2026-02-15 追補: `run_c_team_collect_preflight_check.sh` で latest 解決先が契約不一致のとき、既定は `collect_preflight_check=skipped`、厳格モード（`C_COLLECT_LATEST_REQUIRE_FOUND=1`）は `collect_preflight_check=fail` へ分離。
- 実行コマンド:
```bash
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_extract_c_team_latest_collect_log.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_render_c_team_session_entry.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
C_SKIP_NESTED_SELFTESTS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
C_TEAM_SKIP_STAGING_BUNDLE=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
```
- 結果:
  - collect preflight latest 参照が bundle/readiness の既定導線となり、手入力なしで運用可能化。
  - latest 解決不能/契約不一致の既定挙動（skip）と厳格挙動（fail-fast）が分離され、C-30 の受入条件を充足。

## 29) C-31 latest preflight 解決の厳格モード分離（Done, 2026-02-15）
- 目的:
  - latest 自動解決の利便性を維持しつつ、提出直前だけ fail-fast を強制できる運用ノブを固定する。
- 進捗:
  - Auto-Next として C-31 を起票し、`docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-31 へ更新。
  - `docs/team_runbook.md` に latest 自動解決の既定（skip）と strict（fail）運用を追記。
  - `scripts/run_c_team_staging_checks.sh` / `scripts/check_c_team_submission_readiness.sh` に `C_COLLECT_LATEST_REQUIRE_FOUND` の明示受け渡しを追加し、strictノブの挙動を経路依存なく固定。
  - `scripts/test_run_c_team_staging_checks.py` / `scripts/test_check_c_team_submission_readiness.py` に「latest は解決できるが preflight 契約不一致」の default skip / strict fail 回帰を追加。
- 実行コマンド:
```bash
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - latest 自動解決の既定（skip）と strict ノブ（fail-fast）が helper/staging/readiness 全経路で一致し、C-31 の受入条件を充足。

## 30) C-32 collect証跡導線で strict latest ノブ固定（Done, 2026-02-15）
- 目的:
  - collect/recover 提出導線で strict latest ノブを明示指定できる形へ昇格し、運用時の設定漏れを低減する。
- 進捗:
  - Auto-Next として C-32 を起票し、`docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-32 へ更新。
  - `scripts/collect_c_team_session_evidence.sh` に `--collect-latest-require-found 0|1` を追加し、提出前ゲート実行時に strict latest ノブを `check_c_team_submission_readiness.sh` へ伝播するよう更新。
  - `scripts/recover_c_team_token_missing_session.sh` に `--collect-latest-require-found 0|1` を追加し、finalize 時に collect 導線へ strict ノブを引き継げるよう更新。
  - `scripts/render_c_team_session_entry.py` に `--collect-latest-require-found` を追加し、strict ノブ有効時は preflight コマンド証跡へ `C_COLLECT_LATEST_REQUIRE_FOUND=1` を出力。
  - `scripts/test_collect_c_team_session_evidence.py` に strict ノブの入力検証（異常値）と strict fail-fast（latest未解決）の回帰を追加。
  - `scripts/test_recover_c_team_token_missing_session.py` に strict ノブ異常値/strict fail-fast の回帰を追加。
  - `scripts/test_render_c_team_session_entry.py` に strict ノブ証跡出力の回帰を追加。
  - `docs/team_runbook.md` に strict latest 提出モード（collect/recover）コマンド例を追記。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_render_c_team_session_entry.py
```
- 結果:
  - collect/recover で strict latest ノブを明示指定でき、entry に strict ノブ証跡を残せる状態を固定。
  - strictノブ異常値と latest未解決 fail-fast の回帰を追加し、C-32 の受入条件を充足。

## 31) C-33 latest候補優先順位とstrict運用境界の固定（Done, 2026-02-15）
- 目的:
  - latest 参照候補の優先順位を安定化し、strict運用時の誤解決リスクを低減する。
- 進捗:
  - Auto-Next として C-33 を起票し、`docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-33 へ更新。
  - `scripts/extract_c_team_latest_collect_log.py` で候補優先順位（checker command > env > collect_log_out）を導入。
  - `scripts/test_extract_c_team_latest_collect_log.py` に優先順位回帰（checker優先）を追加。
  - `scripts/test_run_c_team_collect_preflight_check.py` / `scripts/test_run_c_team_staging_checks.py` / `scripts/test_check_c_team_submission_readiness.py` に、`checker + collect_log_out` 併記時の checker優先回帰を追加。
  - `scripts/run_c_team_collect_preflight_check.sh` に latest境界理由キー（`collect_preflight_check_reason=*`）を追加し、default skip / strict fail の分岐理由をログで追跡可能化。
  - `scripts/test_run_c_team_staging_checks.py` / `scripts/test_check_c_team_submission_readiness.py` に `collect_preflight_check_reason` の回帰を追加し、上位ラッパー経路でも理由キーを保証。
  - same tests に `latest_not_found_default_skip` 正常系回帰を追加し、未検出時の既定継続挙動を固定。
  - `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に `--collect-latest-require-found` ヘルプ導線の回帰を追加。
- 実行コマンド:
```bash
python scripts/test_extract_c_team_latest_collect_log.py
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - latest 候補優先順位（checker優先）と strict/default 境界理由キーを helper/staging/readiness で固定し、C-33 の受入条件を充足。

## 32) C-34 strict latest 提出テンプレの最終固定（Done, 2026-02-15）
- 目的:
  - strict latest 提出運用（`C_COLLECT_LATEST_REQUIRE_FOUND=1`）を dispatch/runbook テンプレへ固定し、運用手順の揺れをなくす。
- 進捗:
  - Auto-Next として C-34 を起票し、strict latest 提出テンプレを collect/recover の 2導線で固定。
  - `docs/fem4c_team_dispatch_2026-02-06.md` の Team C テンプレを C-34 向けに更新し、strict latest 既定コマンド（`--collect-latest-require-found 1`）を明記。
  - `docs/team_runbook.md` へ C-34 の固定運用（strictテンプレ既定 + `collect_preflight_check_reason=*` の fail理由追跡）を追記。
  - `scripts/render_c_team_session_entry.py` に `preflight_latest_require_found=0|1` の常時出力を追加し、team_status エントリ側で strict/default 境界を記録可能化。
  - `scripts/recover_c_team_token_missing_session.sh` に `next_finalize_command_strict_latest=... --collect-latest-require-found 1` を追加し、復旧開始時点で strict finalize テンプレを即取得可能化。
  - `scripts/test_render_c_team_session_entry.py` / `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に回帰を追加し、上記挙動を固定。
- 実行コマンド:
```bash
python scripts/test_render_c_team_session_entry.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/check_doc_links.py docs/team_runbook.md docs/fem4c_team_dispatch_2026-02-06.md docs/fem4c_team_next_queue.md docs/abc_team_chat_handoff.md docs/fem4c_dirty_diff_triage_2026-02-06.md
```
- 結果:
  - dispatch/runbook の提出テンプレを strict latest 既定で整合。
  - collect/recover 経路の `team_status` エントリに strictノブ記録（`preflight_latest_require_found`）を残せることを回帰で固定。
  - C-34 の受入条件を充足。

## 33) C-35 strict latest 失敗理由の提出ログ固定（Done, 2026-02-16）
- 目的:
  - strict latest fail-fast 時の理由キー（`collect_preflight_check_reason=*`）を提出ログへ自動転記し、切り戻し判断をログのみで完結可能にする。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` に preflight 理由抽出を追加し、`run_c_team_collect_preflight_check.sh` 出力から `collect_preflight_check_reason=*` を収集して entry の実行コマンド欄へ自動転記するよう更新。
  - `scripts/test_collect_c_team_session_evidence.py` の strict fail 回帰を更新し、`collect_preflight_check_reason=latest_not_found_strict` が entry 出力へ残ることを固定。
  - `scripts/run_c_team_staging_checks.sh` を更新し、nested self-test 前に `C_COLLECT_LATEST_REQUIRE_FOUND` を `unset` することで strict 提出モード環境変数の自己テスト波及を防止。
  - `scripts/test_run_c_team_staging_checks.py` に `C_COLLECT_LATEST_REQUIRE_FOUND=1` かつ nested self-test 実行（`C_SKIP_NESTED_SELFTESTS=0`）の回帰を追加し PASS を確認。
  - `scripts/test_recover_c_team_token_missing_session.py` に strict fail 理由キー出力の回帰を追加。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_run_c_team_staging_checks.py
C_COLLECT_PREFLIGHT_LOG=/tmp/c35_preflight_seed.log C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
```
- 結果:
  - strict fail時の理由キーを提出ログへ自動転記でき、C-35 の受入条件を充足。
  - strict提出モード環境変数による nested self-test 誤失敗を解消。

## 34) C-36 strict latest 理由ログ運用の提出前安定化（Done, 2026-02-16）
- 目的:
  - strict latest 失敗時に、理由キーと再実行コマンドを提出ログから一意に追跡できる運用を固定する。
- 進捗:
  - C-35 完了後の Auto-Next として C-36 を起票し、`docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-36 へ更新。
  - `scripts/collect_c_team_session_evidence.sh` に `submission_readiness_retry_command=...` 出力を追加し、strict fail 時に再実行コマンドをログへ自動提示するよう更新。
  - 同スクリプトの事前埋め込みコマンド表記を `-> RUN（preflight gate）` へ変更し、失敗時に PASS 誤表示されないよう修正。
  - strict fail 経路で `collect_preflight_check_reason=*` を標準エラーへ再掲し、理由キーの見落としを防止。
  - `scripts/collect_c_team_session_evidence.sh` に `collect_preflight_reasons=*` 集約出力（未検出時は `-`）を追加し、理由収集有無の判別を標準化。
  - 回帰更新: `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に retry_command 出力検証を追加。
  - 回帰更新: `scripts/test_collect_c_team_session_evidence.py` に `collect_preflight_reasons=-` / strict理由集約の検証を追加。
  - 回帰更新: `scripts/test_run_c_team_staging_checks.py` に strict env隔離（`unset C_COLLECT_LATEST_REQUIRE_FOUND`）の契約テストを追加。
  - `scripts/collect_c_team_session_evidence.sh` の strict失敗時 `submission_readiness_retry_command=...` を安定パス化し、一時validationファイルではなく `team_status` の実パスを返すよう修正。
  - 回帰更新: `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に retry_command が `team_status` 実パスを指すことを追加。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_check_c_team_submission_readiness.py
C_COLLECT_PREFLIGHT_LOG=latest C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=1 C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md
C_TEAM_SKIP_STAGING_BUNDLE=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
```
- 結果:
  - strict失敗時に理由キー（`latest_invalid_report_strict`）と再実行コマンドを追跡でき、retry command の安定パス化まで完了。
  - default運用では同条件で `collect_preflight_check=skipped` を維持し、strict/default 境界の挙動を再確認。

## 35) C-37 latest preflight strict運用の欠落ログ境界固定（Done, 2026-02-16）
- 目的:
  - `latest` 解決先が消失ログを指す場合でも、strict/default 境界の判定理由を提出ログから誤読なく追跡できる形へ固定する。
- 進捗:
  - Auto-Next として C-37 を起票し、`docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` の C先頭タスク参照を C-37 へ更新。
  - `scripts/run_c_team_collect_preflight_check.sh` に latest解決後の存在確認を追加し、解決先ログが消失している場合は `collect_preflight_log_missing=...` を出力するよう更新。
  - latest解決後ログ消失の境界キーを追加:
    - default: `collect_preflight_check_reason=latest_resolved_log_missing_default_skip`
    - strict: `collect_preflight_check_reason=latest_resolved_log_missing_strict`
  - explicitログ指定（`C_COLLECT_PREFLIGHT_LOG=/path.log`）でログ消失時は `collect_preflight_check_reason=explicit_log_missing` を返す fail-fast を追加。
  - 回帰更新:
    - `scripts/test_run_c_team_collect_preflight_check.py` に missing-log の default skip / strict fail ケースを追加。
    - `scripts/test_check_c_team_submission_readiness.py` に missing-log の default skip / strict fail ケースを追加。
    - `scripts/test_run_c_team_staging_checks.py` に missing-log の default skip / strict fail ケースを追加。
    - `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に missing-log strict 経路の回帰を追加。
    - `scripts/test_extract_c_team_latest_collect_log.py` に `--require-existing` の missing/pass 回帰を追加。
  - 運用文書同期: `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` へ `latest_resolved_log_missing_*` reason key を追記。
  - `scripts/extract_c_team_latest_collect_log.py` に `--require-existing` を追加し、latest解決先ログの存在有無（`collect_log_exists=0|1`）を helper単体で判定できるよう更新。
- 実行コマンド:
```bash
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_extract_c_team_latest_collect_log.py
C_COLLECT_PREFLIGHT_LOG=latest C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=1 C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md
C_COLLECT_PREFLIGHT_LOG=latest C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=1 C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md
C_COLLECT_PREFLIGHT_LOG=/tmp/c37_explicit_missing.log C_REQUIRE_COLLECT_PREFLIGHT_ENABLED=1 C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md
```
- 結果:
  - missing-log 条件で strict/default が `latest_resolved_log_missing_*` に分離され、提出ログから境界を誤読なく追跡できる状態を確認。
  - explicitログ指定欠落で `collect_preflight_check_reason=explicit_log_missing` の fail-fast を確認。
  - C-37 を Done 化し、Auto-Next として C-38（missing-log 境界の提出エントリ固定）を起票。

## 36) C-38 missing-log 境界の提出エントリ固定（Done, 2026-02-19）
- 目的:
  - C-37で確定した reason key と欠落ログパスを `team_status` エントリへ常時残し、提出ログのみで strict/default 境界を追跡できる状態を固定する。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` の preflight probe 解析を拡張し、`collect_preflight_log_resolved=*` / `collect_preflight_log_missing=*` を `command_lines` として `team_status` エントリへ転記するよう更新。
  - 回帰 `scripts/test_collect_c_team_session_evidence.py` に explicit missing-log ケースを追加し、`collect_preflight_log_missing` と `collect_preflight_check_reason=explicit_log_missing` の転記を固定。
  - 回帰 `scripts/test_collect_c_team_session_evidence.py` に latest resolved missing の default skip / strict fail ケースを追加し、`collect_preflight_log_resolved` / `collect_preflight_log_missing` と reason key の同時転記を固定。
  - 回帰 `scripts/test_render_c_team_session_entry.py` に missing-log 境界キーの出力検証を追加し、entry 生成段階の欠落を検知可能にした。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_render_c_team_session_entry.py
python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - latest resolved missing（strict/default）と explicit missing の3経路で、境界キーが提出エントリへ残ることを確認（PASS）。
  - C-38 を Done 化し、Auto-Next として C-39（missing-log 境界ログの失敗経路固定）を起票。

## 37) C-39 missing-log 境界ログの失敗経路固定（Done, 2026-02-19）
- 目的:
  - strict fail 経路でも missing-log 境界キーを復旧導線へ残し、`token missing` 復旧時に判定材料を欠落させない。
- 進捗:
  - `scripts/recover_c_team_token_missing_session.sh` を更新し、finalize 失敗時に `entry_out=...`（`--collect-log-out` 指定時は `collect_log_out=...` も）を stderr へ出力して復旧ログ探索を固定。
  - `scripts/test_recover_c_team_token_missing_session.py` を更新し、strict fail 時に `entry_out` の存在と missing-log 境界キー（`collect_preflight_log_resolved` / `collect_preflight_log_missing` / `collect_preflight_check_reason=*`）が entry に残ることを回帰固定。
  - strict latest missing の precheck seed を追加し、`latest_not_found_strict` ではなく `latest_resolved_log_missing_strict` を確実に通る失敗経路をテストで固定。
- 実行コマンド:
```bash
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_render_c_team_session_entry.py
python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - recover strict fail 時に復旧ログ（stdout/stderr）と entry_out の両方で missing-log 境界キーが復元できることを確認（PASS）。
  - C-39 を Done 化し、Auto-Next として C-40（missing-log 境界ログの提出テンプレ固定）を起票。

## 38) C-40 missing-log 境界ログの提出テンプレ固定（Done, 2026-02-21）
- 目的:
  - C-39 で固定した失敗経路ログの確認手順を runbook/dispatch/handoff へ同期し、運用時の見落としを防ぐ。
- 進捗:
  - `scripts/recover_c_team_token_missing_session.sh` に review command 出力を追加し、finalize 失敗時に `missing_log_review_command=...`（`--collect-log-out` 指定時は `collect_report_review_command=...` も）を stderr へ出力するよう更新。
  - 同スクリプトの token-missing 復旧開始出力へ `next_finalize_review_keys` / `next_finalize_review_command` を追加し、提出テンプレ側の確認キーを即時参照できるよう固定。
  - 回帰 `scripts/test_recover_c_team_token_missing_session.py` を更新し、start/finalize strict fail の両経路で review command 出力を検知するようにした。
  - `docs/team_runbook.md`, `docs/fem4c_team_dispatch_2026-02-06.md`, `docs/abc_team_chat_handoff.md` を C-40 受入基準へ同期した。
- 実行コマンド:
```bash
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/check_doc_links.py docs/team_runbook.md docs/fem4c_team_dispatch_2026-02-06.md docs/abc_team_chat_handoff.md
```
- 結果:
  - recover strict fail の確認対象（`entry_out` / `collect_log_out` / `submission_readiness_retry_command` / review command）が復旧ログから一意に復元できることを確認（PASS）。
  - C-40 を Done 化し、Auto-Next として C-41（missing-log 境界確認コマンドの提出エントリ連携）を起票。

## 39) C-41 missing-log 境界確認コマンドの提出エントリ連携（Done, 2026-02-21）
- 目的:
  - C-40 で固定した review command を `team_status` 提出エントリ運用へ接続し、復旧時の手作業再構成を不要化する。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` に `missing_log_review_command=...` の自動転記を追加し、preflight 有効時は提出エントリへ review command を残すよう更新。
  - `--collect-preflight-log` 指定時は `collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py <log> --require-enabled --expect-team-status <team_status>` を同エントリへ追記するよう更新。
  - 回帰 `scripts/test_collect_c_team_session_evidence.py` を更新し、submission readiness / strict fail / collect-log 経路で review command が entry に残ることを固定。
  - `scripts/check_c_team_review_commands.py` と `scripts/test_check_c_team_review_commands.py` を追加し、最新Cエントリの review command 欠落を機械検出できるようにした。
  - `docs/team_runbook.md` / `docs/fem4c_team_dispatch_2026-02-06.md` / `docs/abc_team_chat_handoff.md` を C-41 前提へ同期した。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_render_c_team_session_entry.py
python scripts/test_check_c_team_review_commands.py
```
- 結果:
  - 提出エントリから missing-log 境界確認コマンドを直接再実行できる状態を確認（PASS）。
  - C-41 を Done 化し、Auto-Next として C-42（review-command 監査の提出前ゲート統合）を起票。

## 40) C-42 review-command 監査の提出前ゲート統合（Done, 2026-02-21）
- 目的:
  - C-41 で追加した review-command 監査を readiness/staging の提出前ゲートへ組み込み、必要時に fail-fast できる運用を固定する。
- 進捗:
  - `scripts/check_c_team_submission_readiness.sh` に `C_REQUIRE_REVIEW_COMMANDS=1` を追加し、指定時は `check_c_team_review_commands.py` を提出前ゲートで実行するよう更新。
  - `scripts/run_c_team_staging_checks.sh` に optional review-command 監査ステップを追加し、`C_REQUIRE_REVIEW_COMMANDS=1` で bundle 実行内監査が可能になった。
  - 同スクリプトの nested self-test に `scripts/test_check_c_team_review_commands.py` を追加し、bundle 実行時の退行検知を補強。
  - 回帰 `scripts/test_check_c_team_submission_readiness.py` / `scripts/test_run_c_team_staging_checks.py` を更新し、監査有効時の pass/fail 境界を固定。
  - `review_command_check=pass|skipped|fail` を readiness/staging 両スクリプトへ追加し、監査有効/無効の判定をログ上で明示できるようにした。
- 実行コマンド:
```bash
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_check_c_team_review_commands.py
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
```
- 結果:
  - review-command 監査を有効化した提出前ゲートの pass/fail 分岐、および `review_command_check` 出力を確認（PASS）。
  - C-42 を Done 化し、Auto-Next として C-43（strict latest collect-report 検証パス整合）を起票。

## 41) C-43 strict latest collect-report 検証パス整合（Done, 2026-02-21）
- 目的:
  - `collect_c_team_session_evidence.sh` の validation用一時 `team_status` と strict latest preflight 検証（`--collect-preflight-log`）の期待パス不一致を解消し、`latest_invalid_report_strict` の誤検知を防ぐ。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` の submission readiness 呼び出しを更新し、`--collect-preflight-log` 指定時は expected team_status を canonical `team_status` 側へ合わせるようにした（validation一時ファイルとの不一致を回避）。
  - `scripts/recover_c_team_token_missing_session.sh` を更新し、`--collect-log-out` 利用時に実行中ログの自己参照 preflight を避けつつ、`collect_report_review_command` を提出エントリへ明示的に残すようにした。
  - `scripts/collect_c_team_session_evidence.sh` の readiness prefix 生成を共通化し、strict latest + review-required 併用時の retry/prefill command 記録が同一規約で出力されるようにした。
  - `scripts/test_collect_c_team_session_evidence.py` に canonical team_status を使う回帰に加えて、strict latest + explicit collect-log + review-required 併用の再発防止ケースを追加した。
  - `scripts/test_recover_c_team_token_missing_session.py` の strict retry-command 期待値を review-required 併用でも崩れない形へ修正した。
  - `scripts/test_check_c_team_submission_readiness.py` の `run_script` で `C_REQUIRE_REVIEW_COMMANDS=0` を初期化し、親環境変数混入による偽陽性を防止した。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_collect_preflight_check.py
python scripts/test_check_c_team_submission_readiness.py
C_REQUIRE_REVIEW_COMMANDS=1 python scripts/test_check_c_team_submission_readiness.py
```
- 結果:
  - C-43 受入条件（canonical path整合、strict fail-fast維持、review-required併用、回帰固定）を満たしたため Done 判定。
  - Auto-Next として C-44（review-required 環境混入時の提出ゲート再現性固定）へ遷移。

## 42) C-44 review-required 環境混入時の提出ゲート再現性固定（Done, 2026-02-22）
- 目的:
  - 提出前ゲート回帰が親シェル環境（`C_REQUIRE_REVIEW_COMMANDS=1`, strict/preflight 変数）に依存せず再現するように初期化境界を固定する。
- 進捗:
  - `scripts/test_check_c_team_submission_readiness.py` の `run_script` に環境サニタイズを追加し、外部環境混入（`C_COLLECT_PREFLIGHT_LOG`, `C_COLLECT_EXPECT_TEAM_STATUS`, `C_REQUIRE_REVIEW_COMMANDS`, `C_REQUIRE_COLLECT_PREFLIGHT_ENABLED`, `C_COLLECT_LATEST_REQUIRE_FOUND`）をデフォルト初期化で遮断した。
  - 同テストへ親環境汚染ケースを追加し、sanitization 後に `latest_not_found_default_skip` + `review_command_check=skipped` + `PASS` が維持されることを固定した。
  - 最新Cエントリを対象に `C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30` を再実行し、review-command 監査・strict-safe 監査・elapsed監査の共存 PASS を確認した。
- 実行コマンド:
```bash
python scripts/test_check_c_team_submission_readiness.py
C_REQUIRE_REVIEW_COMMANDS=1 python scripts/test_check_c_team_submission_readiness.py
python scripts/test_collect_c_team_session_evidence.py
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
```
- 結果:
  - C-44 受入条件（親環境混入耐性、strict/review 併用整合、提出ゲート共存）を満たしたため Done 判定。
  - Auto-Next として C-45（latest preflight 一時ログ消失時の提出ゲート境界固定）へ遷移。

## 43) C-45 latest preflight 一時ログ消失時の提出ゲート境界固定（Done, 2026-02-22）
- 目的:
  - latest 解決先ログが `/tmp` から消失した境界で、strict/default 判定と review-command 監査結果を提出ログから機械的に追跡できる状態を固定する。
- 進捗:
  - `scripts/check_c_team_submission_readiness.sh` を更新し、collect preflight 判定要約を `submission_readiness_collect_preflight_check` / `submission_readiness_collect_preflight_reason` として常時出力するようにした。
  - 同スクリプトの strict fail 経路へ `submission_readiness_retry_command=...` と `submission_readiness_fail_step=collect_preflight` を追加し、reason + retry + fail-step の3点を提出ログへ残すようにした。
  - `scripts/test_check_c_team_submission_readiness.py` を更新し、latest resolved missing の default/strict（`C_REQUIRE_REVIEW_COMMANDS=1` 併用を含む）で reason/retry/fail-step 境界が固定される回帰を追加した。
  - strict fail 時の出力経路変更（stderr）に合わせ、collect preflight 失敗系テストを `stdout+stderr` 判定へ調整した。
- 実行コマンド:
```bash
python scripts/test_check_c_team_submission_readiness.py
C_REQUIRE_REVIEW_COMMANDS=1 python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_collect_preflight_check.py
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer_safe
```
- 結果:
  - default missing-log で `latest_resolved_log_missing_default_skip` + review監査PASS を維持し、strict missing-log で fail-fast + reason/retry/fail-step 出力を確認した。
  - C-45 受入条件（strict/default 境界、review-required 併用、retry trace 欠落防止）を満たしたため Done 判定。
  - Auto-Next として C-46（strict latest fail-step/retry trace の提出ログ固定）を起票。

## 44) C-46 strict latest fail-step/retry trace の提出ログ固定（Done, 2026-02-22）
- 目的:
  - strict latest fail-fast の追跡キー（reason/summary/retry/fail-step）を readiness/staging 両経路で揃え、監査導線の機械抽出を安定化する。
- 進捗:
  - `scripts/run_c_team_staging_checks.sh` の strict fail 経路に `submission_readiness_collect_preflight_check` / `submission_readiness_collect_preflight_reason` 要約出力を追加。
  - 同スクリプトの strict fail 経路で `submission_readiness_retry_command=...` と `submission_readiness_fail_step=collect_preflight` を出力し、readiness 側と同じ再実行導線を固定した。
  - `scripts/test_run_c_team_staging_checks.py` に default missing（review-required 併用）で retry/fail-step 非出力、strict missing で retry/fail-step 出力となる回帰を追加。
  - `scripts/test_check_c_team_submission_readiness.py` に default missing 経路で retry/fail-step を出さない回帰を追加し、strict/default 境界の対称性を補強した。
- 実行コマンド:
```bash
python scripts/test_check_c_team_submission_readiness.py
C_REQUIRE_REVIEW_COMMANDS=1 python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_run_c_team_collect_preflight_check.py
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md
bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer_safe
```
- 結果:
  - default missing で skip reason + summary を維持し、retry/fail-step が非出力であることを確認。
  - strict missing で readiness/staging とも `submission_readiness_retry_command` + `submission_readiness_fail_step=collect_preflight` が出力されることを確認。
  - C-46 受入条件（strict/default 境界、review-required 併用、fail-step/retry trace 同期）を満たしたため Done 判定。
  - Auto-Next として C-47（strict latest fail trace 出力順の提出ログ固定）を起票。

## 45) C-47 strict latest fail trace 出力順の提出ログ固定（Done, 2026-02-22）
- 目的:
  - strict/default の fail trace を監査側で同一パーサ抽出できるよう、readiness/staging の出力順を固定する。
- 進捗:
  - `scripts/check_c_team_fail_trace_order.py` を新規追加し、strict/default ログの必須キー有無と出力順を機械判定できるようにした。
  - `scripts/test_check_c_team_fail_trace_order.py` を追加し、strict pass/fail と default pass/fail の回帰を固定した。
  - `scripts/test_check_c_team_submission_readiness.py` / `scripts/test_run_c_team_staging_checks.py` に strict経路の順序アサーション（reason -> summary -> retry -> fail-step）を追加した。
  - `scripts/check_c_team_fail_trace_order.py` に review-command 出力境界チェックを追加し、fail-trace ブロックへの混線を検知可能にした。
  - `scripts/test_check_c_team_fail_trace_order.py` に review trace 混線ケースを追加し、strict/default 双方で FAIL を固定した。
  - `scripts/run_c_team_fail_trace_audit.sh` に `C_FAIL_TRACE_SKIP_NESTED_SELFTESTS`（既定1）を追加し、短時間で安定した strict/default ログ採取を固定した。
  - `scripts/test_run_c_team_fail_trace_audit.py` を更新し、staging 呼び出し時に `C_SKIP_NESTED_SELFTESTS=1` が必須で伝播することを回帰固定した。
  - `docs/fem4c_team_next_queue.md` / `docs/abc_team_chat_handoff.md` / `docs/team_runbook.md` に fail trace 順序監査コマンドを追記した。
- 実行コマンド:
```bash
python scripts/test_check_c_team_fail_trace_order.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30 > /tmp/c47_readiness_default.log 2>&1
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30 > /tmp/c47_readiness_strict.log 2>&1
C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md > /tmp/c47_staging_default.log 2>&1
C_COLLECT_LATEST_REQUIRE_FOUND=1 C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md > /tmp/c47_staging_strict.log 2>&1
python scripts/check_c_team_fail_trace_order.py /tmp/c47_readiness_default.log --mode default
python scripts/check_c_team_fail_trace_order.py /tmp/c47_readiness_strict.log --mode strict
python scripts/check_c_team_fail_trace_order.py /tmp/c47_staging_default.log --mode default
python scripts/check_c_team_fail_trace_order.py /tmp/c47_staging_strict.log --mode strict
```
- 現状:
  - readiness/staging の strict/default ログで fail trace 順序監査は PASS（review-required 併用を含む）。
  - C-47 受入条件（strict/default 境界、順序一致、review混線なし）を満たしたため Done 判定。
  - Auto-Next として C-48（collect/recover 提出ログへの fail-trace 監査導線固定）へ遷移。

## 46) C-48 collect/recover 提出ログへの fail-trace 監査導線固定（Done, 2026-02-23）
- 目的:
  - collect/recover 経由で生成される提出エントリに fail-trace order 監査導線を固定し、token-missing 復旧後でも strict/latest 境界を同じ手順で再検証できる状態にする。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` に `fail_trace_audit_command=scripts/run_c_team_fail_trace_audit.sh <team_status> <minutes>` の自動追記を追加した。
  - `scripts/collect_c_team_session_evidence.sh` に `--fail-trace-audit-log` を追加し、fail-trace監査ログから `readiness/staging default+strict` のログパスと再検証コマンドを提出エントリへ自動転記できるようにした。
  - `scripts/recover_c_team_token_missing_session.sh` start モード出力へ `next_finalize_fail_trace_audit_command=...` を追加し、復旧直後の監査導線を固定した。
  - `scripts/recover_c_team_token_missing_session.sh` finalize モードへ `--fail-trace-audit-log` を追加し、collect へ監査ログを引き渡せるようにした。
  - `scripts/recover_c_team_token_missing_session.sh` start モードへ `next_finalize_fail_trace_audit_log=...` / `next_finalize_command_with_fail_trace_log=...` / `next_finalize_fail_trace_embed_command=...`（strict latest 版含む）を追加し、token-missing 復旧テンプレから fail-trace 監査結果埋め込みまでを 1 系列コマンドで再現可能にした。
  - `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` を更新し、監査導線出力と監査ログ取り込みを回帰で固定した。
  - `docs/team_runbook.md` / `docs/abc_team_chat_handoff.md` / `docs/fem4c_team_next_queue.md` を C-48 先頭タスクへ同期した。
- 判定:
  - C-48 の受入条件（collect/recover の fail-trace 導線固定 + token-missing finalize テンプレへの監査結果埋め込み）を満たしたため Done。
  - Auto-Next は C-49（token-missing 復旧 finalize テンプレの fail-trace 失敗時再試行導線固定）へ遷移。

## 47) C-49 token-missing 復旧 finalize テンプレの fail-trace 失敗時再試行導線固定（Done, 2026-02-23）
- 目的:
  - fail-trace 監査ログが不完全/失敗でも、token-missing 復旧 finalize の提出ログから再実行導線を即時復元できるようにする。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` を更新し、`--fail-trace-audit-log` 指定時に `fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh <team_status> <minutes> | tee <log>` を提出エントリへ自動追記するようにした。
  - 同処理で `FAIL_TRACE_AUDIT_RESULT != PASS` の場合は `fail_trace_audit_retry_reason=...` を出力し、再試行理由をログで機械判読できるようにした。
  - `readiness/staging` 監査キー欠落時は `fail_trace_audit_missing_keys=...` を自動追記するようにした。
  - `scripts/recover_c_team_token_missing_session.sh` の finalize 失敗（`--fail-trace-audit-log` 欠落）時に `fail_trace_audit_retry_command=...` と `fail_trace_finalize_retry_command=...` を標準エラーへ出力するよう更新した。
  - 回帰更新:
    - `scripts/test_collect_c_team_session_evidence.py` に fail-trace 監査ログ不完全ケース（`FAIL` + キー欠落）を追加。
    - `scripts/test_recover_c_team_token_missing_session.py` に finalize 出力で retry command が残ることを追加。
    - `scripts/test_recover_c_team_token_missing_session.py` に missing fail-trace log の strict/latest retry command 回帰を追加。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_fail_trace_audit.py
bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30
```
- 判定:
  - C-49 の受入条件（fail-trace 監査欠落時 retry テンプレ提示 + strict latest 併用回帰固定 + required tests PASS）を満たしたため Done。
  - Auto-Next は C-50（fail-trace retry 導線の提出エントリ整合監査固定）へ遷移。

## 48) C-50 fail-trace retry 導線の提出エントリ整合監査固定（Done, 2026-02-28）
- 目的:
  - recover finalize 失敗ログの retry 導線と、collect で team_status に記録される retry 導線を同一規約で追跡できるようにする。
- 進捗:
  - `scripts/check_c_team_fail_trace_retry_consistency.py` を追加し、retry 導線（audit/finalize）の整合を latest C entry から機械判定できるようにした。
  - `scripts/test_check_c_team_fail_trace_retry_consistency.py` を追加し、欠落/不整合/許容モードの回帰を固定した。
  - `scripts/check_c_team_submission_readiness.sh` / `scripts/run_c_team_staging_checks.sh` に retry consistency 監査を統合し、提出前ゲート/日次bundle双方で同一判定を返すようにした。
  - `scripts/run_c_team_fail_trace_audit.sh` を 9-step 化し、fail-trace order 監査の末尾で retry consistency 監査を実行するようにした。
  - `scripts/collect_c_team_session_evidence.sh` の fail-trace 監査ログ取り込みへ `fail_trace_retry_consistency_*` 記録キーを追加し、提出エントリ単体で再検証導線を追跡できるようにした。
  - 回帰更新:
    - `scripts/test_run_c_team_fail_trace_audit.py` に retry consistency 必須/任意の境界テストを追加。
    - `scripts/test_collect_c_team_session_evidence.py` に retry consistency 記録キーの取り込み検証を追加。
  - `python -m unittest discover -s scripts -p 'test_*c_team*.py'` で C-team 系の総合回帰（189 tests）PASS を確認した。
- 実行コマンド:
```bash
python scripts/test_check_c_team_fail_trace_retry_consistency.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_fail_trace_audit.py
bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30
python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md
```
- 判定:
  - C-50 の受入条件（retry 導線整合監査 + strict/default 境界回帰 + 提出導線統合）を満たしたため Done。
  - Auto-Next は C-51（fail-trace retry consistency 証跡の strict/default 境界固定）へ遷移。

## 49) C-51 fail-trace retry consistency 証跡の strict/default 境界固定（Done, 2026-02-28）
- 目的:
  - fail-trace 監査ログで retry consistency 証跡が欠落/不整合な場合の strict/default 提出判断境界を、提出ログのみで追跡可能にする。
- 進捗:
  - `scripts/run_c_team_fail_trace_audit.sh` の `C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=0|1` 運用境界を固定し、readiness/staging へ同一ノブを伝搬するよう更新した。
  - `C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=0|1` / `C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=0|1` を追加し、`fail_trace_retry_consistency_check` 記録キー欠落を strict で fail-fast できるようにした。
  - `scripts/check_c_team_fail_trace_retry_consistency.py` に `--require-retry-consistency-check-key` を追加し、key 欠落/不正/fail 値を明示失敗として固定した。
  - `scripts/check_c_team_submission_readiness.sh` / `scripts/run_c_team_staging_checks.sh` で strict key-required 判定を統一し、`missing fail_trace_retry_consistency_check` 理由で fail-fast することを確認した。
  - `scripts/collect_c_team_session_evidence.sh` に `fail_trace_retry_consistency_command/check/retry_command` の常時追記を追加し、提出エントリへ strict 判定に必要なキーを先行付与できるようにした。
  - `scripts/run_c_team_fail_trace_audit.sh` の default capture 失敗時に fail理由とログパス (`readiness_default_log` / `staging_default_log`) を即出力し、`FAIL_TRACE_AUDIT_RESULT=FAIL` を残すよう改善した。
  - `scripts/test_run_c_team_fail_trace_audit.py` に default capture 失敗コンテキスト出力の回帰を追加した。
- 実行コマンド:
```bash
python scripts/test_run_c_team_fail_trace_audit.py
python scripts/test_check_c_team_fail_trace_retry_consistency.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python -m unittest discover -s scripts -p 'test_*c_team*.py'
C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45
C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=0 bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45
C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45
```
- 判定:
  - C-51 の受入条件（strict/default 境界固定、key-required fail-fast、readiness/staging/audit 判定整合）を満たしたため Done。
  - Auto-Next は C-52（strict-key fail-fast ログの collect/recover 連携固定）へ遷移。

## 50) C-52 strict-key fail-fast ログの collect/recover 連携固定（Done, 2026-02-28）
- 目的:
  - strict key-required 失敗時の fail理由・再試行導線を collect/recover 提出テンプレへ欠落なく埋め込み、token-missing 復旧後も提出ログ単体で再実行手順を誤読なく追跡できるようにする。
- 進捗:
  - `scripts/collect_c_team_session_evidence.sh` の fail-trace 取り込みで生成する `fail_trace_finalize_retry_command` に strict ノブ（`C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY` / `C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY`）を引き継ぐよう更新した。
  - `scripts/recover_c_team_token_missing_session.sh` の start 出力へ strict-key 運用テンプレを追加し、`next_finalize_fail_trace_audit_command_strict_key` / `next_finalize_fail_trace_embed_command_strict_key` / `next_finalize_fail_trace_embed_command_strict_latest_strict_key` を固定した。
  - `scripts/test_collect_c_team_session_evidence.py` で strict-key 再試行文脈の `fail_trace_finalize_retry_command` 出力を回帰固定した。
  - `scripts/test_recover_c_team_token_missing_session.py` で token-missing start 出力の strict-key テンプレを回帰固定した。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_run_c_team_fail_trace_audit.py
C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45
```
- 判定:
  - C-52 の受入条件（strict-key fail-fast 理由の collect/recover 連携、retry 導線の同一ログパス維持、回帰固定）を満たしたため Done。
  - Auto-Next は C-53（strict-key token-missing 復旧テンプレの監査再実行導線固定）へ遷移。

## 51) C-53 strict-key token-missing 復旧テンプレの監査再実行導線固定（Done, 2026-02-28）
- 目的:
  - token-missing 復旧テンプレの strict-key 経路で、監査再実行コマンド（audit/recover）の環境ノブ・ログパス・minutes が常に一致することを提出前ゲートで機械監査できるようにする。
- 進捗:
  - `scripts/check_c_team_fail_trace_retry_consistency.py` に `--require-strict-env-prefix-match` を追加し、`C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY*` の env prefix と entry key の不一致を fail-fast できるようにした。
  - `scripts/run_c_team_fail_trace_audit.sh` に `C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=0|1` を追加し、retry consistency checker へ strict-env 一致監査ノブを配線した。
  - `scripts/check_c_team_submission_readiness.sh` / `scripts/run_c_team_staging_checks.sh` に `C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=0|1` を追加し、提出前ゲート/staging bundle 双方で同じ strict-env 条件を渡せるようにした。
  - `scripts/collect_c_team_session_evidence.sh` の fail-trace 監査ログ取り込みを拡張し、`fail_trace_require_retry_consistency_strict_env` を `fail_trace_retry_consistency_command` / retry command / audit retry/finalize retry へ伝搬するようにした。
  - 回帰更新:
    - `scripts/test_check_c_team_fail_trace_retry_consistency.py`
    - `scripts/test_run_c_team_fail_trace_audit.py`
    - `scripts/test_check_c_team_submission_readiness.py`
    - `scripts/test_run_c_team_staging_checks.py`
    - `scripts/test_collect_c_team_session_evidence.py`
- 実行コマンド:
```bash
python scripts/test_check_c_team_fail_trace_retry_consistency.py
python scripts/test_run_c_team_fail_trace_audit.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
```
- 判定:
  - C-53 の受入条件（strict-key retry/finalize ノブ不一致の fail-fast、submission readiness/staging fail理由の一致、回帰PASS）を満たしたため Done。
  - Auto-Next は C-54（strict-env fail-fast 理由の collect/recover 提出ログ境界固定）へ遷移。

## 52) C-54 strict-env fail-fast 理由の collect/recover 提出ログ境界固定（Done, 2026-02-28）
- 目的:
  - strict-env 必須運用（`C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1`）で fail-trace 監査が失敗した場合に、collect/recover が生成する提出ログ単体で fail理由と再試行導線を再現可能にする。
- 進捗:
  - `scripts/check_c_team_fail_trace_retry_consistency.py` に `reason_codes=` 出力を追加し、strict-env fail-fast 理由を機械可読キーで残せるようにした。
  - `scripts/run_c_team_fail_trace_audit.sh` で `fail_trace_retry_consistency_retry_command` / `fail_trace_retry_consistency_reasons` / `fail_trace_retry_consistency_reason_codes` を出力し、fail時の再試行導線と理由コードを監査ログへ固定した。
  - `scripts/check_c_team_submission_readiness.sh` / `scripts/run_c_team_staging_checks.sh` で reason/reason_codes/retry_command の出力契約を同期し、strict-env 欠落境界の fail-fast 理由を一致させた。
  - `scripts/collect_c_team_session_evidence.sh` の fail-trace 取り込みに `reason_codes=` を追加し、監査ブロック欠落時も top-level fallback で復元できるようにした。
  - `scripts/recover_c_team_token_missing_session.sh` の finalize missing-log 診断に `fail_trace_retry_consistency_retry_command` を追加し、strict-env 条件つき再試行導線を欠落なく提示できるようにした。
- 実行コマンド:
```bash
python scripts/test_check_c_team_fail_trace_retry_consistency.py
python scripts/test_run_c_team_fail_trace_audit.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
```
- 判定:
  - C-54 の受入条件（strict-env fail-fast 理由の collect/recover 提出ログ境界固定、readiness/staging 判定整合、回帰PASS）を満たしたため Done。
  - Auto-Next は C-55（strict-env fail-fast 理由コードの latest/preflight 境界固定）へ遷移。

## 53) C-55 strict-env fail-fast 理由コードの latest/preflight 境界固定（Done, 2026-02-28）
- 目的:
  - latest preflight 解決先の欠落/不一致時にも、strict-env fail-fast 理由コード（`fail_trace_retry_consistency_reason_codes`）と再試行コマンドを提出ログから一意に復元できるようにする。
- 進捗:
  - `scripts/check_c_team_submission_readiness.sh` で collect preflight strict fail 時に fallback を追加し、`fail_trace_retry_consistency_reasons` / `fail_trace_retry_consistency_reason_codes` / `fail_trace_retry_consistency_retry_command` を `submission_readiness_fail_step=collect_preflight` より前に出力するよう更新。
  - `scripts/run_c_team_staging_checks.sh` へ同じ fallback 契約を追加し、readiness/staging で reason_codes の命名規約（`collect_preflight_<reason>_before_retry_consistency`）を統一。
  - 回帰更新:
    - `scripts/test_check_c_team_submission_readiness.py`
    - `scripts/test_run_c_team_staging_checks.py`
  - 新規回帰で `latest_resolved_log_missing_strict` 経路の fail 出力に reason/reason_codes/retry_command が欠落しないことを固定。
- 実行コマンド:
```bash
python scripts/test_check_c_team_submission_readiness.py
python scripts/test_run_c_team_staging_checks.py
```
- 判定:
  - C-55 の受入条件（latest/preflight 境界で reason_codes 契約維持、readiness/staging strict-env 経路整合、指定2テストPASS）を満たしたため Done。
  - Auto-Next は C-56（collect/recover への preflight strict-fail 理由コード転写固定）へ遷移。

## 54) C-56 collect/recover への preflight strict-fail 理由コード転写固定（Done, 2026-03-01）
- 目的:
  - preflight strict-fail で readiness/staging が終了した場合でも、collect/recover 提出テンプレへ reason_codes/retry_command を欠落なく転写し、token-missing 復旧後も同一境界を再現できるようにする。
- 進捗:
  - C-55 で readiness/staging の fallback reason_codes 契約を確定済み。次段で collect/recover 転写経路へ同等キーを連携する。
  - `collect_c_team_session_evidence.sh` / `recover_c_team_token_missing_session.sh` の strict-fail 失敗経路を起点に、reason_codes の復元手順を整理した。
  - `scripts/collect_c_team_session_evidence.sh` の既定 dryrun 出力を `mktemp` 化し、nested 回帰で `/tmp` 固定パス競合により strict-fail 診断が欠落する不安定要因を除去した。
  - `scripts/run_c_team_staging_checks.sh` が nested self-test 実行前に retry-consistency 系 env を `unset` するよう更新し、親環境混入による collect/recover 回帰の揺らぎを抑止した。
  - `scripts/test_recover_c_team_token_missing_session.py` の strict latest retry command 期待値を可変 prefix 許容へ更新し、追加 env ノブを含む collect/recover 連携経路でも回帰が落ちないようにした。
- 実行コマンド:
```bash
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
```
- 判定:
  - C-56 の受入条件（collect/recover strict-fail ログから `fail_trace_retry_consistency_reason_codes` / `fail_trace_retry_consistency_retry_command` を復元記録、指定2テストPASS）を満たしたため Done。
  - Auto-Next は C-57（collect/recover finalize strict-safe elapsed 算出境界の固定）へ遷移。

## 55) C-57 collect/recover finalize strict-safe elapsed 算出境界の固定（In Progress, 2026-03-01）
- 目的:
  - finalize エントリで guard/end の両タイマー出力を保持した場合でも、strict-safe 監査の elapsed 算出が一貫するように記録境界を固定する。
- 進捗:
  - C-56 完了時点で latest Cエントリの strict-safe 監査は PASS 済み（`pass_section_freeze_timer_safe`）。
  - C-57 では collect/recover の最終テンプレで elapsed 抽出に影響するタイマーブロック順序を固定化する。
  - `scripts/audit_c_team_staging.py` を更新し、`end_epoch` / `elapsed_min` を最新一致で抽出するように変更（途中 guard 値より終端タイマー値を優先）。
  - `scripts/test_audit_c_team_staging.py` と `scripts/test_check_c_team_dryrun_compliance.py` に、複数 `elapsed_min` / `end_epoch` を含む Cエントリ境界ケースの回帰を追加。
  - `scripts/render_c_team_session_entry.py` を更新し、`SESSION_TIMER_END` / `SESSION_TIMER_GUARD` の「最新かつ完全な」ブロックを優先抽出するように変更（複数ブロック混在や末尾不完全ブロックを許容）。
  - `scripts/test_render_c_team_session_entry.py` に、複数 end/guard ブロック混在と最新不完全 end/guard ブロック混在の回帰を追加。
  - `scripts/collect_c_team_session_evidence.sh` / `scripts/recover_c_team_token_missing_session.sh` に `--guard-checkpoint-minutes`（複数指定可）を追加し、中間guardは block を許容して記録、最終guardのみ厳格判定する finalize 境界を実装。
  - checkpoint 指定時は `guard_checkpoints=<csv>` を提出エントリへ自動転記するよう更新し、提出ログ単体で checkpoint 実施条件を追跡可能化。
  - `scripts/test_collect_c_team_session_evidence.py` / `scripts/test_recover_c_team_token_missing_session.py` に guard-checkpoint 混在の回帰を追加。
  - `scripts/test_check_c_team_dryrun_compliance.py` に最新 `SESSION_TIMER_END` が不完全な場合の fallback 回帰を追加し、strict-safe 判定の終端値採用を補強。
  - C-57再実行セッション（`/tmp/c_team_session_20260301T130243Z_3408716.token`）で `session_timer_guard 10/20/30` と `session_timer end` を同一tokenで取得し、`elapsed_min=30` 証跡を `docs/team_status.md` へ反映。
- 実行コマンド:
```bash
python scripts/test_audit_c_team_staging.py
python scripts/test_check_c_team_dryrun_compliance.py
python scripts/test_render_c_team_session_entry.py
python scripts/test_collect_c_team_session_evidence.py
python scripts/test_recover_c_team_token_missing_session.py
```
