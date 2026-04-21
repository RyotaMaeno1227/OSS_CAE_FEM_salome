# git_checkpoint_grouping_before_phase2l_v1

## purpose

この文書は、Phase 2L へ進む前に current workspace の cumulative dirty
state を checkpoint grouping と hold bucket に凍結するための docs-only
plan である。

今回の目的は次である。

- current git state が phase-local diff ではなく cumulative dirty state
  であることを前提に、commit grouping を先に固定する
- checkpoint A/B/C/D/E の include/exclude candidates を整理する
- provenance が曖昧な mixed surface を hold bucket として分離する
- actual commit 前に必要な minimum check matrix を固定する
- Phase 2L の追加 C split より checkpoint strategy を優先する

## scope and non-goals

今回の scope は次である。

- current git state の grouping freeze
- include/exclude rule の固定
- ambiguous file の hold bucket 化
- recommended commit order の固定

今回の non-goals は次である。

- `git add`
- `git commit`
- `git reset`
- `git stash`
- `git clean`
- repo file move
- C source edit
- checker/script edit
- build system edit
- route behavior change
- artifact schema change
- Phase 2L 実行

## prerequisites

前提は次である。

- source of truth は current Codex workspace
- Git baseline / provenance audit before Phase 2L は完了済み
- current git state は phase-local ではなく cumulative dirty state
- monolithic premise は固定する
  - Year1 monolithic is fully implemented
  - Year2 monolithic is out of current scope
  - existing Year1 monolithic behavior is preserve-only
- audit artifact は `/tmp` に存在してよい
  - `/tmp/fem4c_unstaged_worktree.patch`
  - `/tmp/fem4c_staged_worktree.patch`
  - `/tmp/fem4c_git_status_short.txt`

## current git ambiguity summary

Git provenance audit で確認できた ambiguity は次である。

- `staged_only=111`
- `unstaged_only=83`
- `both_staged_and_unstaged=159`
- `untracked=3448`

status code の分布は次である。

- `A  88`
- `AD 115`
- `AM 34`
- `M  23`
- `MM 10`
- ` M 26`
- ` D 57`
- `?? 3448`

この state は、phase 単位の clean diff ではなく、少なくとも次が重なった
累積状態として扱う。

- staged native baseline
- Phase 1 tooling relocation / wrapper coexistence
- release / closeout / architecture docs
- Phase 2 planning / audit docs
- Phase 2 implementation deltas
- parent repo 側の PM/team-run surfaces
- archive / old_autorun / imported backup-like surfaces

したがって current dirty worktree は、phase-local reconstruction ではなく
grouping freeze と hold 分離で扱う。

## staged / unstaged / untracked summary

grouping freeze での default interpretation は次とする。

- staged-only surface
  - checkpoint candidate として最も扱いやすい
  - ただし current workspace の目的と無関係な parent repo PM surfaces は除外する
- unstaged-only surface
  - current worktree artifact として candidate になりうる
  - ただし large cumulative file は hold に回す
- both staged and unstaged surface
  - provenance ambiguity が高いため default hold とする
- untracked surface
  - directory taxonomy や phase docs のように clean artifact なら最優先
    candidate にできる
  - archive/import/backup-like surface は hold に送る

## proposed checkpoint groups

### checkpoint A

checkpoint A は staged native baseline candidates である。

含める候補:

- staged-only native additions
- staged-only example / baseline input additions
- staged-only initial native module additions
- provenance が Phase 1 / Phase 2 より前の native baseline に見えるもの

原則:

- `A ` または staged-only `M ` のうち、unstaged overlay を持たないものを優先する
- `AM` / `MM` / `AD` は入れない
- parent repo 側 PM/team-run files は入れない

### checkpoint B

checkpoint B は Phase 1 tooling relocation / wrappers である。

含める候補:

- `tools/` tree
- old `scripts/` compatibility wrappers
- Phase 1 taxonomy / migration docs
  - `docs/phase1d_*` から `docs/phase1o_*`
- runner / checker / exporter / helper / bridge / report / compare / validator
  relocations

原則:

- `tools/` 実体と `scripts/` compatibility surface を同じ family ごとに扱う
- archive / old_autorun / imported backup-like copy は含めない
- PM/team-run repo root scripts は current FEM4C checkpoint から外す

### checkpoint C

checkpoint C は release / closeout / architecture docs である。

含める候補:

- `docs/current_goal_closeout_memo_v1.md`
- `docs/canonical_bounded_route_closeout_memo_v1.md`
- `docs/release_architecture_reorganization_plan_v1.md`
- `docs/solver_core_vs_tooling_boundary_v1.md`
- release note / policy / support / manifest draft docs

原則:

- release-ready status を説明する docs を Phase 2 implementation から分離する
- Phase 2 planning docs はここに入れない

### checkpoint D

checkpoint D は Phase 2 planning / audit docs である。

含める候補:

- `docs/phase2a_*`
- `docs/phase2b_*`
- `docs/phase2c_*`
- `docs/phase2e_*`
- `docs/phase2f_*`
- `docs/phase2h_*`
- `docs/phase2i_*`
- `docs/phase2k_*`
- `docs/git_checkpoint_grouping_before_phase2l_v1.md`

原則:

- planning / audit / split-boundary / guard-hardening docs を code delta から分離する
- release/closeout docs は checkpoint C に残す

### checkpoint E

checkpoint E は Phase 2 implementation deltas である。

含める候補:

- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d_route_meta.c`
- `scripts/check_year1_coupled_route_matrix.sh`

conditional include:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d.h`
- `Makefile`
- `scripts/check_coupled_integrators.sh`

原則:

- clean untracked artifacts は checkpoint E candidate として扱える
- cumulative `AM` / `M` companions は default では hold に残す
- `src/coupled/coupled_run2d.c` / `.h` / `Makefile` を E に入れるのは、
  intentional squash checkpoint として user approval を得た場合だけにする

## include/exclude rules

default rules は次である。

### include rules

- clean untracked phase docs は docs checkpoint に入れてよい
- clean untracked split artifact は implementation checkpoint candidate とする
- staged-only native baseline は baseline checkpoint candidate とする
- `tools/` 実体と対応 wrapper がそろう relocation family は checkpoint B に寄せる

### exclude rules

- `AM`, `MM`, `AD` は default hold
- parent repo 側 PM/team-run surfaces は current FEM4C checkpoint から除外
- archive / old_autorun / imported backup-like surface は hold
- build system の large cumulative file は phase-local provenance が取れない限り hold

## special handling for `src/coupled/coupled_run2d.h`

`src/coupled/coupled_run2d.h` は current では `AM` である。

interpretation は次で固定する。

- staged 側は initial header addition
- unstaged 側は later route / artifact / dynamic storage expansion
- dirty state は Phase 2J 固有ではない
- current build/checks は worktree-side expansion を前提としている

handling は次とする。

- default は hold bucket
- checkpoint E に直ちに入れない
- `src/coupled/coupled_run2d.c` と `Makefile` の cumulative delta と一緒に
  intentional squash として扱う場合だけ inclusion を検討する
- phase-local commit として分離再構成できない限り、単独 checkpoint 化しない

## Phase 2 C split file handling

Phase 2 C split surface の扱いは次で固定する。

### `src/coupled/coupled_run2d_config.c`

- interpretation
  - Phase 2D safe-first config split artifact
- handling
  - checkpoint E include candidate

### `src/coupled/coupled_run2d_route_meta.c`

- interpretation
  - Phase 2J route-meta split artifact
- handling
  - checkpoint E include candidate

### `src/coupled/coupled_run2d.c`

- interpretation
  - cumulative `AM`
  - staged native baseline の上に Phase 2D / 2J compile-preserving updates が重なっている
- handling
  - default hold
  - checkpoint E へは intentional squash approval がある場合のみ

### `src/coupled/coupled_run2d.h`

- interpretation
  - cumulative `AM`
  - initial header addition + later route/artifact expansion
- handling
  - hold

### `Makefile`

- interpretation
  - cumulative unstaged `M`
  - current lines に Phase 2D / 2J source-list additionsは見えるが、diff 全体は
    phase-local ではない
- handling
  - default hold
  - minimal source-list extraction が済むまで independent checkpoint にしない

### `scripts/check_year1_coupled_route_matrix.sh`

- interpretation
  - Phase 2G checker guard hardening artifact
- handling
  - checkpoint E include candidate

## hold bucket

hold bucket は provenance ambiguous / mixed-surface files の一時隔離先とする。

hold bucket に入れるべきものは次である。

- all `AM`
- all `MM`
- all `AD`
- large cumulative `M`
- archive / old_autorun / imported backup-like surfaces
- parent repo 側 PM/team-run surfaces
- phase ownershipが不明な `D`
- current build に必要だが phase-local attribution ができない cumulative files

special hold examples:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d.h`
- `Makefile`
- parent repo の `../docs/*`, `../scripts/*`, `../AGENTS.md` など current FEM4C
  checkpoint から独立して扱うべき files

## minimum pre-commit check matrix

checkpoint commit 前の minimum matrix は次で固定する。

- `make -j`
- `bash scripts/check_coupled_integrators.sh`
- `bash scripts/check_year1_coupled_route_matrix.sh`
- `bash scripts/check_year2_oneway_mbd_fem_v1.sh`
- `bash scripts/check_year2_lagged_mbd_fem_v1.sh`
- `bash scripts/check_year2_same_time_mbd_fem_v1.sh`
- `bash scripts/check_same_time_file_exchange_stub.sh`
- `bash scripts/check_same_time_file_exchange_bundle.sh`
- `bash scripts/check_mixed_macro_local_replay_split_generic_v1.sh`

この matrix は checkpoint E の code / checker surfaces だけでなく、
current release-safe state を壊していないことの minimum evidence とする。

## recommended commit order

recommended order は次で固定する。

1. checkpoint A
2. checkpoint B
3. checkpoint C
4. checkpoint D
5. hold resolution checkpoint
6. checkpoint E

理由は次である。

- baseline / tooling / docs を先に分離した方が provenance が安定する
- `src/coupled/coupled_run2d.c` / `.h` / `Makefile` の cumulative surface を
  hold resolution で先に扱わないと、checkpoint E が phase-local implementation
  commit にならない
- Phase 2L は checkpoint E 実行前に hold が unresolved のまま進めない

## risks

主な risk は次である。

- cumulative `AM` / `MM` / `AD` を無理に A-E へ押し込むと provenance が壊れる
- `src/coupled/coupled_run2d.h` を単独 checkpoint 化すると current build dependency
  を誤る
- `Makefile` の large cumulative diff を Phase 2 source-list addition と誤認する
- `tools/` relocation 実体と `scripts/` wrapper surface を別 checkpoint に分けると
  migration intent が壊れる
- parent repo PM/team-run surfaces を FEM4C code checkpoint に混ぜると review scope が
  崩れる

## closeout criteria

この checkpoint freeze は次を満たしたら closeout とする。

- checkpoint A/B/C/D/E の role が固定されている
- `src/coupled/coupled_run2d.h` の扱いが hold として明示されている
- Phase 2 implementation clean artifacts と cumulative companion files が分離されている
- minimum pre-commit check matrix が固定されている
- Phase 2L is blocked until checkpoint grouping is resolved and actual checkpoint
  commit strategy is accepted
