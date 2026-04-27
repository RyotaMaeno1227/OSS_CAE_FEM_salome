# Git Cleanup Split Plan

最終更新: 2026-03-08

## 1. 目的

現在の worktree は、solver core、examples、compare/wrapper、PM運用 docs、C-team review/staging scripts が混在している。
この文書は、非破壊で commit 単位を切るための分類表である。

## 2. 今回の基本方針

1. 他チームの source 差分は revert しない。
2. generated noise だけを先に除去する。
3. commit は「1 commit = 1 merge reason」で切る。
4. `main` へは、solver core と wrapper 過多を一緒に載せない。
5. まず `backup/20260302-pre-move-0424` で整理し、`main` は後段で行う。

## 3. commit 候補の分割

### Group A: PM / Review-Spec / Learning Docs

対象:
- `docs/10_review_spec_priority_plan.md`
- `docs/MBD_00_learning_map.md`
- `docs/MBD_01_rigid_body_2d_basics.md`
- `docs/MBD_02_constraints_and_jacobians.md`
- `docs/mbd_learning_dod.md`
- `docs/fem4c_team_next_queue.md`
- `docs/abc_team_chat_handoff.md`
- `docs/team_runbook.md`
- `FEM4C/00_GPT_HANDOFF.md`
- `FEM4C/01_PRO_REVIEW_BRIEF.md`
- `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md`
- `FEM4C/README.md`

理由:
- review-spec 採用と learning docs 骨格を固定するため

### Group B: MBD Core (Run 1 / Run 2 本体)

対象:
- `FEM4C/src/mbd/*`
- `FEM4C/src/io/input.*`
- `FEM4C/src/analysis/runner.*`
- `FEM4C/src/fem4c.c`
- `FEM4C/src/elements/q4/q4_element.c`
- `FEM4C/src/elements/t3/t3_element.c`
- `FEM4C/src/elements/t6/t6_element.c`
- `FEM4C/Makefile`

理由:
- warning cleanup / rigid M1 closure の本丸

### Group C: Coupled / Examples / Practice Probes

対象:
- `FEM4C/src/coupled/`
- `FEM4C/examples/*`
- `FEM4C/practice/ch09/*`

理由:
- meaningful case / debug probe / coupled evolution を solver core から分けるため

### Group D: FEM4C compare / acceptance wrapper 群

対象:
- `FEM4C/scripts/check_*`
- `FEM4C/scripts/compare_*`
- `FEM4C/scripts/run_2d_coupled_*`
- `FEM4C/scripts/test_*`
- `docs/06_acceptance_matrix_2d.md`
- `docs/07_input_spec_coupled_2d.md`
- `docs/09_compare_schema_2d.md`

理由:
- wrapper 過多を評価しやすくし、default / non-default を切り分けるため

### Group E: Root scripts の C-team review/staging 群

対象:
- `scripts/c_stage_dryrun.sh`
- `scripts/c_team_review_reason_utils.sh`
- `scripts/check_c_team_collect_preflight_report.py`
- `scripts/check_c_team_submission_readiness.sh`
- `scripts/collect_c_team_session_evidence.sh`
- `scripts/recover_c_team_token_missing_session.sh`
- `scripts/run_c_team_collect_preflight_check.sh`
- `scripts/run_c_team_staging_checks.sh`
- `scripts/test_*c_team*`

理由:
- solver product ではなく team運用/提出補助なので、製品本体と commit を分けるべき

## 4. 先に落としてよい generated noise

- `FEM4C/scripts/__pycache__/`

## 5. いま触らないもの

- `docs/team_status.md`
- `docs/session_continuity_log.md`

理由:
- ラン中や各チーム更新と競合しやすい
- commit 整理より先に source/doc の意味単位を切る方が安全

## 6. 推奨順序

1. Group A を独立 commit
2. Group B を独立 commit
3. Group C を独立 commit
4. Group D を独立 commit
5. Group E は必要なら別 branch または backup 専用 commit
