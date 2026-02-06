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

### B. 生成物・不要物（削除維持を推奨）
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
- `FEM4C/PHASE2_IMPLEMENTATION_REPORT.md`
- `FEM4C/T6_PROGRESS_REPORT.md`
- `FEM4C/docs/00_tutorial_requirements.md`
- `FEM4C/docs/01_requirements.md`
- `FEM4C/docs/02_file_structure.md`
- `FEM4C/docs/03_design.md`
- `FEM4C/docs/04_progress.md`
- `FEM4C/docs/05_handover_notes.md`
- `FEM4C/docs/06_fem4c_implementation_history.md`
- `FEM4C/docs/FEM_LEARNING_GUIDE.md`
- `FEM4C/docs/RELEASE_README.md`
- `FEM4C/docs/tutorial_manual.md`
- `FEM4C/docs/implementation_guide.md`
- `FEM4C/NastranBalkFile/3Dtria_example.dat`
- `FEM4C/USAGE_PARSER.md`
- `FEM4C/practice/README.md`
- `FEM4C/src/elements/t3/t3_element.c`
- `FEM4C/src/io/input.c`
- `FEM4C/src/solver/cg_solver.c`
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

## 2) `FEM4C/test/*` 削除群の暫定判定

### 復元候補
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

判定理由:
- `test/data` と `test/unit` は回帰入力/テストコード本体であり、削除確定すると検証導線を失うため、現時点は復元候補とする。

### 削除確定候補
- `FEM4C/test/output/mixed_t3_q4.vtk`
- `FEM4C/test/output/nastran_openmp_simple.vtk`
- `FEM4C/test/output/nastran_simple.vtk`
- `FEM4C/test/output/q4_simple.vtk`
- `FEM4C/test/output/simple_2d_test.vtk`
- `FEM4C/test/output/t3_simple.vtk`
- `FEM4C/test/output/t6_cantilever_phase2.vtk`
- `FEM4C/test/output/t6_simple_phase2.vtk`

判定理由:
- `test/output/*.vtk` は生成結果で再生成可能なため、削除維持を暫定採用。

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

## 4) 補足
- 本レポートは「誤コミット防止」を優先した暫定判定。`意図不明` 群は PM レビュー後に再分類する。
- `chrono-2d` 側の dirty 差分（バイナリ/生成物含む）は今回スコープ外として、同一コミットへ混在させない。
