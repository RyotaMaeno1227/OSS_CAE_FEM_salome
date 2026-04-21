# phase2q_checkpoint_plan_for_phase2n_2o_2p_v1

## purpose

この文書は、accepted だが still worktree-only な
Phase 2N / Phase 2O / Phase 2P family を、
another actual C split の前にどう checkpoint 化するかを
docs-only で固定するための plan である。

今回の目的は次である。

- Phase 2N / 2O / 2P family の minimum coherent checkpoint shape を固定する
- include / exclude boundary を明記する
- unrelated broad dirty state を巻き込まない explicit-path discipline を固定する
- future approval-gated checkpoint commit task にそのまま渡せる形へ落とす

## scope and non-goals

今回の scope は次である。

- `scripts/check_year1_coupled_route_matrix.sh`
- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_writer_meta.c`
- `Makefile`
- `docs/phase2l_next_safe_helper_split_planning_v1.md`
- `docs/phase2m_step_flex_exact_guard_plan_v1.md`
- `docs/phase2p_post_writer_meta_first_split_audit_v1.md`
- `docs/release_architecture_reorganization_plan_v1.md`
- this planning doc

今回の non-goals は次である。

- C source edit
- header edit
- checker/script edit
- build system edit
- route/output behavior change
- artifact schema change
- `git add`
- `git commit`
- `git push`
- next actual C split planning beyond checkpoint discipline

## prerequisites

前提は次である。

- current HEAD は `5b6918d03944923ba3c265ca7efb6bfe97e98c1e`
- current branch は `release/year1-cleanup`
- docs checkpoint commit `bbf76335724826e875634bd2f0d829cb16079165` が history に存在する
- Phase 2 implementation checkpoint commit `5b6918d03944923ba3c265ca7efb6bfe97e98c1e` が history に存在する
- Phase 2N checker hardening is accepted in worktree
- Phase 2O first writer-meta split is accepted in worktree
- Phase 2P docs-only audit is complete
- current repo remains broad cumulative dirty state
- next actual C edit is not allowed in this phase

## accepted worktree-only state summary

accepted worktree-only state は次である。

- Phase 2N:
  `scripts/check_year1_coupled_route_matrix.sh` に
  route-aware exact `step_flex_counter_columns` guard が追加されている
- Phase 2O:
  `src/coupled/coupled_run2d.c`,
  `src/coupled/coupled_run2d_writer_meta.c`,
  `Makefile`
  で first writer-meta split が反映されている
- Phase 2P:
  accepted / closed judgment と next-action checkpoint-first judgment が
  docs-only audit として固定されている

重要なのは次である。

- technical acceptance は取れている
- しかし script / C / docs surfaces は still worktree-only である
- この状態で next actual C split を積むと provenance / review / rollback が再び悪化する

## checkpoint family candidates

### candidate A: one combined checkpoint

include:

- Phase 2N script hardening
- Phase 2O C/build changes
- Phase 2L / 2M / 2P / release architecture / this planning doc

pros:

- one roundで終わる
- path list は一度で済む

cons:

- checker/script, C/build, docs/audit が 1 commit に混ざる
- review scope が広い
- rollback granularity が粗い
- broad dirty repo では accidental scope inflation を見つけにくい

assessment:

- coherent ではある
- ただし minimum safe shape としては非推奨

### candidate B: two-commit family

commit 1:

- implementation/guard checkpoint
- `scripts/check_year1_coupled_route_matrix.sh`
- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_writer_meta.c`
- `Makefile`

commit 2:

- docs/audit/checkpoint-plan checkpoint
- `docs/phase2l_next_safe_helper_split_planning_v1.md`
- `docs/phase2m_step_flex_exact_guard_plan_v1.md`
- `docs/phase2p_post_writer_meta_first_split_audit_v1.md`
- `docs/phase2q_checkpoint_plan_for_phase2n_2o_2p_v1.md`
- `docs/release_architecture_reorganization_plan_v1.md`

pros:

- implementation review と docs review を分離できる
- scope が still narrow
- explicit path discipline を保ちやすい
- future rollback も 2 layers に分かれる

cons:

- commit が 2 本になる
- docs planning と implementation chronology は exact reconstruction ではない

assessment:

- minimum coherent checkpoint shape として最適
- `recommended`

### candidate C: three-commit family

commit 1:

- planning docs (`phase2l`, `phase2m`)

commit 2:

- implementation/guard (`Phase 2N`, `Phase 2O`)

commit 3:

- post-split audit + checkpoint plan (`phase2p`, `phase2q`, release architecture)

pros:

- chronology の説明は最もきれい

cons:

- commit friction が高い
- broad dirty repo では staging/verification mistake risk が増える
- minimum shape ではない

assessment:

- optional only
- current workspace には over-specified

## include/exclude boundary

### required include paths

implementation / guard commit:

- `scripts/check_year1_coupled_route_matrix.sh`
- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_writer_meta.c`
- `Makefile`

docs / audit commit:

- `docs/phase2l_next_safe_helper_split_planning_v1.md`
- `docs/phase2m_step_flex_exact_guard_plan_v1.md`
- `docs/phase2p_post_writer_meta_first_split_audit_v1.md`
- `docs/phase2q_checkpoint_plan_for_phase2n_2o_2p_v1.md`
- `docs/release_architecture_reorganization_plan_v1.md`

### explicit exclude paths

- `scripts/check_coupled_integrators.sh`
- `src/coupled/coupled_run2d.h`
- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d_route_meta.c`
- `docs/phase2_implementation_checkpoint_hold_resolution_v1.md`
- `docs/git_checkpoint_grouping_before_phase2l_v1.md`
- all `tools/`
- all unrelated `src/`, `scripts/`, `examples/`, `docs/`, parent repo PM/team-run surfaces

### why excluded files stay out

- `scripts/check_coupled_integrators.sh`
  - dirty だが Phase 2N family の edited surface ではない
  - acceptance evidence として使っただけで、この family に混ぜる理由がない
- `src/coupled/coupled_run2d.h`
  - Phase 2O で public API unchanged が acceptance 条件だった
  - include すると checkpoint scope が misleading になる
- `src/coupled/coupled_run2d_config.c`
  - Phase 2D artifact であり current family の changed surface ではない
- `src/coupled/coupled_run2d_route_meta.c`
  - Phase 2J artifact であり current family の changed surface ではない
- `docs/phase2_implementation_checkpoint_hold_resolution_v1.md`
  - earlier checkpoint-resolution family の doc であり current acceptance family の direct deliverable ではない
- `docs/git_checkpoint_grouping_before_phase2l_v1.md`
  - broader provenance family に属するため、ここへ混ぜると checkpoint boundary がぼける

## recommended checkpoint shape

recommended checkpoint shape は candidate B とする。

結論:

- one combined checkpoint is `not preferred`
- docs/script/C should be split into `two explicit-path commits`

recommended order:

1. implementation/guard commit
2. docs/audit/checkpoint-plan commit

reason:

- current accepted worktree-only state の実体は Phase 2N + 2O の code/script side にある
- Phase 2P / 2Q docs はその accepted result を記録・固定する層である
- docs を separate commit にすることで accidental inclusion を減らし、review scope を保てる

## explicit path discipline

actual checkpoint commit phase で使う discipline は次で固定する。

### pre-commit review

- `git status --short -- <required include paths>`
- `git diff --name-status -- <required include paths>`
- non-target path が混じらないことを確認する

### staging

- `git add -- <explicit include paths only>`
- do not use:
  - `git add .`
  - `git add -A`
  - `git add -u`
  - broad directory adds

### commit

- staged unrelated files が既に存在する前提で、
  actual commit は必ず `git commit --only -- <explicit paths>` を使う
- plain `git commit -m` は使わない

### post-commit verification

- `git diff --stat HEAD~1..HEAD`
- `git diff --name-status HEAD~1..HEAD`
- committed paths are exactly the intended include list

## approval gate for actual checkpoint commit

actual checkpoint commit の approval gate は次である。

- Phase 2Q is docs-only
- no commit/staging/push is performed in Phase 2Q
- future checkpoint commit phase requires explicit user approval
- because the recommended shape is two commits, the future task should explicitly say whether both commits are approved in one pass
- no further actual C split should start before this checkpoint family is committed, unless the user explicitly approves the additional provenance risk

## risks

主要 risk は次である。

- Phase 2N / 2O / 2P work を未コミットのまま next split に積むと review scope が再度膨らむ
- one combined checkpoint にすると docs / checker / C/build scope が混ざりすぎる
- excluded dirty files を accidental に巻き込むと checkpoint family の意味が壊れる
- broad repo-wide dirty state は explicit path discipline を崩すとすぐ scope contamination を起こす

## closeout criteria

Phase 2Q checkpoint planning は次を満たしたら closeout-ready とする。

- accepted worktree-only family を明示できた
- candidate A/B/C を比較できた
- include / exclude boundary を固定できた
- recommended checkpoint shape を 2-commit family に固定できた
- explicit path discipline を明記できた
- future approval gate を明記できた
