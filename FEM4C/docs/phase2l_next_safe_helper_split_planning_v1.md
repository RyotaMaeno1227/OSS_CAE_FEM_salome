# phase2l_next_safe_helper_split_planning_v1

## purpose

この文書は、docs checkpoint と Phase 2 implementation checkpoint 完了後の
current workspace を source of truth として、Phase 2K post-route-meta split
audit を baseline に次の safe helper split candidate を docs-only で再評価し、
Phase 2M 以降で actual C edit に進む前の planning target を固定するための
plan である。

今回の目的は次である。

- Phase 2K の updated move-readiness table を current checkpoint 状態の上で再確認する
- next safe helper split candidate を 1 cluster に絞る
- future actual split 前に必要な guard / checker / verification を再確認する
- public API / private declaration / Makefile touch points を docs-only で整理する
- Phase 2M or later actual C edit requires explicit user approval を固定する

## scope and non-goals

今回の scope は次である。

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d.h`
- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d_route_meta.c`
- `Makefile`
- Phase 2A / 2B / 2C / 2E / 2F / 2H / 2I / 2K docs

今回の non-goals は次である。

- C source edit
- header edit
- function move
- checker/script edit
- build system change
- include path change
- route/output behavior change
- artifact schema change
- `git add`
- `git commit`
- `git push`
- Phase 2M actual split execution

## prerequisites

前提は次である。

- current HEAD は `5b6918d03944923ba3c265ca7efb6bfe97e98c1e`
- current branch は `release/year1-cleanup`
- docs checkpoint commit `bbf76335724826e875634bd2f0d829cb16079165` が history に存在する
- Phase 2 implementation checkpoint commit
  `5b6918d03944923ba3c265ca7efb6bfe97e98c1e` が history に存在する
- Remaining checkpoint requirement audit により、Phase 2L は docs-only planning としてのみ許可されている
- current repo には still broad cumulative dirty state が残る
- Phase 2 implementation 6 paths
  - `src/coupled/coupled_run2d.c`
  - `src/coupled/coupled_run2d.h`
  - `src/coupled/coupled_run2d_config.c`
  - `src/coupled/coupled_run2d_route_meta.c`
  - `Makefile`
  - `scripts/check_year1_coupled_route_matrix.sh`
  は current HEAD で coherent checkpoint として既に固定され、現在 clean である

## current checkpoint status

current checkpoint status は次である。

- docs checkpoint は release / closeout / Phase 2 planning docs を history に固定済み
- Phase 2 implementation checkpoint は `coupled_run2d.[ch]`,
  `coupled_run2d_config.c`, `coupled_run2d_route_meta.c`, `Makefile`,
  `scripts/check_year1_coupled_route_matrix.sh` を 1 intentional squash
  checkpoint として history に固定済み
- Phase 2N checker hardening は current worktree に残っており、
  `step_flex_counter_columns` route-aware exact guard が満たされている
- Phase 2O narrow writer-meta split patch により
  `coupled_run2d_step_flex_iteration_column_name` は
  `src/coupled/coupled_run2d_writer_meta.c` へ移動済みである
- したがって `coupled_run2d` split lane 自体は current HEAD で coherent な
  baseline を持つ
- ただし repo 全体では parent repo surfaces, tooling relocation surfaces,
  broad baseline additions, archive/backup surfaces が still dirty である

## remaining dirty-state constraint

remaining dirty-state constraint は次である。

- Phase 2L is docs-only
- actual C edit remains blocked by default while broad cumulative dirty state persists
- ただし `coupled_run2d` split lane は current HEAD で checkpoint 済みであり、
  future actual split が narrow かつ explicit approval 付きであれば
  provenance discipline を保てる可能性がある
- その場合でも、exact guard gap が残る helper は actual split 前に
  checker hardening または equivalent acceptance condition を要求する

## monolithic premise

monolithic premise は固定である。

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Implications:

- visible monolithic preserve-only wording は move candidate にしない
- Year2 current-goal scope に monolithic を再導入するような split は行わない
- visible wording / fallback / dispatch / orchestration は current lane の対象外に据え置く

## Phase 2K readiness baseline

Phase 2K baseline として current source/doc から再確認できる点は次である。

- route-meta helper cluster は Phase 2J で accepted / closed
- next planning lane として writer-adjacent metadata mini-cluster が推奨されている
- `coupled_run2d_delay_buffer_scope_from_run` は `later`
  - delayed-only scope token だが dedicated exact checker が未整備
- `coupled_run2d_step_flex_iteration_column_name` は `later`
  - `step_flex_counter_columns` の exact guard が未整備
- exported CSV header getter helpers は `later`
  - public exported declaration surface であり direct exact checker がない
- 次は planning で narrow cluster を固定し、writer body / wording / fallback /
  dispatch / orchestration movement を避けるべきである

## candidate cluster comparison

### candidate A

target:

- `coupled_run2d_delay_buffer_scope_from_run`

assessment:

- helper 自体は小さい
- ただし delayed-only scope token を直接返し、route topology と
  delayed semantics の visible output に近い
- current checker coverage は partial delayed output coverage に留まり、
  dedicated exact delayed-scope checker がない
- future split 前に dedicated delayed scope/output checker を要求する

status:

- keep in selected planning cluster as adjacent surface
- not the first actual split subtarget

### candidate B

target:

- `coupled_run2d_step_flex_iteration_column_name`

assessment:

- helper 自体は最小に近く、`step_flex_counter_columns` label selection に限定される
- current source では static helper であり public header widening を要求しない
- writer body には隣接するが、route topology wording や monolithic preserve-only
  distinction には直接触らない
- current blocker は exact `step_flex_counter_columns` checker がないことにほぼ限定される

status:

- best first actual split subtarget once exact guard condition is satisfied

### candidate C

target:

- exported CSV header getters
  - `coupled_run2d_artifact_metadata_columns_csv`
  - `coupled_run2d_interface_centers_header_csv`
  - `coupled_run2d_reaction_map_header_csv`
  - `coupled_run2d_observation_points_header_csv`

assessment:

- helper bodiesは小さいが public exported declaration surface である
- `src/coupled/coupled_run2d.h` の exported API を跨ぐため、private/public
  declaration boundary の整理が必要になる
- current exact checker floor では dedicated header-getter guard が未整備
- writer-adjacent metadata mini-cluster より先に選ぶ理由が薄い

status:

- later

### candidate D

target:

- no actual split candidate is safe yet

assessment:

- broad dirty state 自体は caution 要因である
- ただし `coupled_run2d` split lane は checkpoint 済みであり、
  planning baseline は十分に固定されている
- candidate B は small/static/non-exported helper として、
  A/C より narrow で planning target として成立する

status:

- reject as the selected target for Phase 2L

## selected next planning target

selected next planning target は writer-adjacent metadata mini-cluster とし、
future actual split の first subtarget は
`coupled_run2d_step_flex_iteration_column_name` に固定する。

理由は次である。

- Phase 2K が推奨した writer-adjacent metadata lane と一致する
- `coupled_run2d_step_flex_iteration_column_name` は current candidates の中で
  最も narrow で、public header expansion を要求しない
- writer body / wording / fallback / dispatch / orchestration を動かさずに
  future split patch を組める可能性が高い
- `coupled_run2d_delay_buffer_scope_from_run` は同 cluster に属するが、
  exact delayed-scope guard が未整備のため first target にはしない
- exported CSV header getters は public/exported surface なので Phase 2L target から外す

## proposed future file map

actual split は今回行わないが、Phase 2M or later の proposal-only file map は次とする。

- keep:
  - `src/coupled/coupled_run2d.c`
  - `src/coupled/coupled_run2d.h`
  - `src/coupled/coupled_run2d_config.c`
  - `src/coupled/coupled_run2d_route_meta.c`
- possible future addition:
  - `src/coupled/coupled_run2d_writer_meta.c`

proposed use of `src/coupled/coupled_run2d_writer_meta.c`:

- first target:
  - `coupled_run2d_step_flex_iteration_column_name`
- possible later same-lane helper:
  - `coupled_run2d_delay_buffer_scope_from_run`
    only after dedicated delayed-scope exact guard exists

guardrails:

- no new public header by default
- no widening of `src/coupled/coupled_run2d.h` unless proven unavoidable
- `coupled_run2d.c` may keep local declarations/call-sites, as Phase 2J did
- exported CSV header getters remain in `coupled_run2d.c` for now
- `Makefile` touch should stay at one narrow source-list addition if a future file is created

## keep-vs-move table

| current helper / region | current responsibility | proposed future home | readiness | guard coverage | reason | risk | verification needed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `coupled_run2d_step_flex_iteration_column_name` | selects `step_flex_counter_columns` label in writer output | `src/coupled/coupled_run2d_writer_meta.c` | accepted / closed in worktree | exact route-aware guard present | first narrow writer-meta helper move succeeded without public header expansion | still uncommitted relative to repo-wide dirty state | restore checkpoint discipline before another actual C split |
| `coupled_run2d_delay_buffer_scope_from_run` | emits delayed buffer scope token in writer output | `src/coupled/coupled_run2d_writer_meta.c` later, or keep in `coupled_run2d.c` | later | partial delayed output coverage | same lane as selected target but more route-semantics sensitive | delayed scope token drift across delayed routes | add dedicated delayed scope/output checker before move |
| exported CSV header getters | expose artifact/interface/reaction/observation header constants | keep in `src/coupled/coupled_run2d.c` for now | later | indirect only | public exported API surface is not the next narrow move | header/public API drift | add dedicated exact header-getter checker before any move |
| `coupled_run2d_comparison_role_from_scheme` | visible comparison-role wording | keep in `src/coupled/coupled_run2d.c` | no-split-yet | exact tokens exist but wording-sensitive | visible wording still tied to writer semantics | wording regression | dedicated wording audit first |
| `coupled_run2d_solver_route_class_from_run` | visible route-class wording | keep in `src/coupled/coupled_run2d.c` | no-split-yet | partial exact guard | route topology and body/interface count remain coupled | cross-route wording drift | stronger cross-route exact guard first |
| `coupled_run2d_step_coupling_reason` | visible `step_status` reason string | keep in `src/coupled/coupled_run2d.c` | no-split-yet | indirect only | no exact reason-string guard | preserve-only or delayed wording drift | exact step-status reason checker first |
| `coupled_run2d_write_output(...)` | main artifact writer body | keep in `src/coupled/coupled_run2d.c` | no-split-yet | mixed exact/semantic | still the dense contract nexus | artifact schema regression | dedicated multi-route writer audit first |
| summary / report wording helpers | visible summary tokens | keep in `src/coupled/coupled_run2d.c` | no-split-yet | selected route exact tokens only | preserve-only wording remains sensitive | user-visible wording regression | dedicated wording audit first |

## no-split-yet confirmation

Phase 2L でも次は対象外のまま維持する。

- `coupled_run2d_write_output(...)`
- summary / report wording helpers
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- visible monolithic preserve-only distinction wording
- scheme dispatch body
- main orchestration body
- model-load helpers
- snapshot helpers
- artifact writer body
- anything that risks treating monolithic as Year2 current-goal scope

## future approval gate

固定する approval gate は次である。

- Phase 2L is docs-only
- no C edit is performed in Phase 2L
- no function move is performed in Phase 2L
- no checker/script edit is performed in Phase 2L
- no build system change is performed in Phase 2L
- Phase 2M or later actual C edit requires explicit user approval
- if future phase also hardens checkers/scripts, that phase requires explicit approval for script edits as well
- no C edit should occur while remaining dirty state makes provenance ambiguous unless the target surface is coherent, checkpointed, and explicitly approved

## minimum verification matrix for future actual split

future actual split に進む場合の minimum matrix は次である。

- `make -j`
- `bash scripts/check_coupled_integrators.sh`
- `bash scripts/check_year1_coupled_route_matrix.sh`
- `bash scripts/check_year2_oneway_mbd_fem_v1.sh`
- `bash scripts/check_year2_lagged_mbd_fem_v1.sh`
- `bash scripts/check_year2_same_time_mbd_fem_v1.sh`
- `bash scripts/check_same_time_file_exchange_stub.sh`
- `bash scripts/check_same_time_file_exchange_bundle.sh`
- `bash scripts/check_mixed_macro_local_replay_split_generic_v1.sh`

conditional requirements:

- if `coupled_run2d_step_flex_iteration_column_name` is the first actual split target,
  add or otherwise satisfy an exact `step_flex_counter_columns` guard before accepting the move;
  the preferred guard path is a future route-aware extension of
  `scripts/check_year1_coupled_route_matrix.sh`, not a one-way-only smoke guard
- if `coupled_run2d_delay_buffer_scope_from_run` is included, add a dedicated delayed
  scope/output exact checker before accepting the move

## risks

主要 risk は次である。

- `step_flex_counter_columns` exact guard を持たないまま helper を移すと
  visible column-label drift を取り逃がす
- `delay_buffer_scope` を first target にすると delayed route semantics と
  visible token drift を招きやすい
- exported CSV header getters を先に動かすと public/private declaration surface と
  exact checker gap が同時に膨らむ
- broad dirty state 下で approval/disciplined path selection なしに actual C edit を行うと
  provenance が再び曖昧になる

## closeout criteria

Phase 2L docs-only planning は次を満たしたら closeout-ready とする。

- current checkpoint status を前提に Phase 2K baseline を再確認できた
- next planning cluster を 1 cluster に絞れた
- first actual split subtarget を
  `coupled_run2d_step_flex_iteration_column_name` に固定できた
- `coupled_run2d_delay_buffer_scope_from_run` と exported CSV header getters の
  defer reason を明文化できた
- no-split-yet surfaces を再確認できた
- future approval gate と minimum verification matrix を明記できた
