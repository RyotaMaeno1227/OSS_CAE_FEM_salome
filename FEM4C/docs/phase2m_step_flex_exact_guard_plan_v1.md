# phase2m_step_flex_exact_guard_plan_v1

## purpose

この文書は、Phase 2L で future actual split の first subtarget を
`coupled_run2d_step_flex_iteration_column_name` に固定した結果を受け、
actual split acceptance に必要な exact `step_flex_counter_columns` guard を
どこでどう担保するかを docs-only で固定するための plan である。

今回の目的は次である。

- `step_flex_counter_columns` current emission point を確認する
- exact guard を追加する future checker surface を比較する
- guard-hardening phase を narrow に保てるかを整理する
- future actual split acceptance condition を固定する
- actual checker edit / C edit 前の approval gate を明文化する

## scope and non-goals

今回の scope は次である。

- `src/coupled/coupled_run2d.c`
- `scripts/check_coupled_integrators.sh`
- `scripts/check_year1_coupled_route_matrix.sh`
- related guard/planning docs

今回の non-goals は次である。

- C source edit
- header edit
- function move
- checker/script edit
- build system edit
- route/output behavior change
- artifact schema change
- `git add`
- `git commit`
- `git push`
- Phase 2M actual patch execution

## prerequisites

前提は次である。

- current HEAD は `5b6918d03944923ba3c265ca7efb6bfe97e98c1e`
- current branch は `release/year1-cleanup`
- docs checkpoint commit は history に存在する
- Phase 2 implementation checkpoint commit は history に存在する
- Phase 2L docs-only planning は完了している
- Phase 2L selected target は `coupled_run2d_step_flex_iteration_column_name`
- actual C edit remains blocked
- current repo には still broad cumulative dirty state が残る

## monolithic premise

monolithic premise は固定である。

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Implications:

- future guard path は Year1 route/preserve-only surface の中で完結させる
- monolithic を Year2 current-goal に戻すような checker ownership は採らない
- visible monolithic wording helper movement は引き続き対象外である

## current guard gap

current guard gap は「exact guard が完全に無い」ではなく、次の状態である。

- `scripts/check_coupled_integrators.sh` は oneway route に限り
  `step_flex_counter_columns` exact line を grep している
- しかし this helper が返す route-dependent label
  - `snapshot_iteration_index`
  - `communication_iteration_index`
  - `coupling_iteration_index`
  を multi-route で exact-check する surface は current workspace に無い
- したがって `coupled_run2d_step_flex_iteration_column_name` の future move を
  accept するには、oneway-only exact guard では不十分である

## current emission point

current emission point は `src/coupled/coupled_run2d.c` の
`coupled_run2d_write_output(...)` 本体である。

writer は次の line で header token を出力している。

- `step_flex_counter_columns,step_index,%s,body_id,full_reassembly_count,static_solve_count`

label selection は helper
`coupled_run2d_step_flex_iteration_column_name(run->time.scheme)` に委譲される。

current helper mapping は次である。

- `oneway_snapshot` -> `snapshot_iteration_index`
- `delayed_cosim_v1_5` -> `communication_iteration_index`
- other current schemes -> `coupling_iteration_index`

このため future exact guard が最小でも cover すべき route family は次である。

- `oneway_snapshot`
- `delayed_cosim_v1_5`
- `fixed_point_strong`
- `monolithic_strong_v1`

## current checker coverage

current checker coverage は次である。

### `scripts/check_coupled_integrators.sh`

- official one-way smoke checker
- exact grep for:
  - `step_columns`
  - `step_flex_counter_columns,step_index,snapshot_iteration_index,body_id,full_reassembly_count,static_solve_count`
  - `snapshot_columns`
  - `snapshot_record`
- ownership is explicitly one-way focused

### `scripts/check_year1_coupled_route_matrix.sh`

- Year1 route runtime/output contract checker
- exact grep for route-specific:
  - `comparison_role`
  - `solver_route_class`
  - `delay_semantics_status`
  - `coupling_metric`
  - `step_columns`
  - `compare_schema_version`
  - `compare_step_columns`
  - monolithic preserve-only distinction tokens
- does not currently exact-check `step_flex_counter_columns`

### current conclusion

- one-way exact guard exists
- multi-route exact guard does not exist
- future actual split acceptance still lacks a route-aware exact contract for the selected helper

## checker candidate comparison

### candidate A

future path:

- extend `scripts/check_coupled_integrators.sh`

pros:

- existing exact grep style already covers `step_flex_counter_columns`
- no new checker file would be introduced

cons:

- script ownership is deliberately official one-way only
- extending it into delayed/fixed-point/monolithic expectations would blur checker scope
- helper acceptance needs route-aware coverage beyond oneway

assessment:

- useful as supporting one-way evidence
- not the primary acceptance gate

### candidate B

future path:

- extend `scripts/check_year1_coupled_route_matrix.sh`

pros:

- already owns Year1 route-specific runtime/output contract comparisons
- already exact-checks adjacent writer fields such as `step_columns`,
  `compare_step_columns`, `comparison_role`, `solver_route_class`
- route-aware exact token comparison fits this checker's current design
- keeps monolithic handling inside the existing Year1 preserve-only route matrix,
  not Year2 current-goal checkers
- no new checker file is required

cons:

- requires future script edit
- increases route-matrix surface slightly

assessment:

- best ownership match
- best direct acceptance gate for the selected helper

### candidate C

future path:

- add a dedicated narrow checker just for `step_flex_counter_columns`

pros:

- very explicit single-purpose ownership
- future split acceptance condition would be easy to point to

cons:

- adds another checker surface in an already broad dirty repo
- duplicates route execution/setup that the existing Year1 route matrix already owns
- provenance and maintenance cost are higher than extending an existing route-aware checker

assessment:

- fallback option only

### candidate D

future path:

- accept future helper move using existing semantic coverage only

pros:

- no new checker work

cons:

- does not satisfy the exact-guard requirement already identified in Phase 2H/K/L
- weakens acceptance discipline for a writer-visible header token
- too easy to miss column-label drift

assessment:

- reject

## selected future guard path

selected future guard path は candidate B とする。

recommended checker surface:

- `scripts/check_year1_coupled_route_matrix.sh`

exact field/token to guard:

- `step_flex_counter_columns,step_index,<label>,body_id,full_reassembly_count,static_solve_count`

minimum route coverage:

- `oneway_snapshot` -> `snapshot_iteration_index`
- `delayed_cosim_v1_5` -> `communication_iteration_index`
- `fixed_point_strong` -> `coupling_iteration_index`
- `monolithic_strong_v1` -> `coupling_iteration_index`

reason:

- this checker already owns route-aware exact output contracts
- it already distinguishes delayed/fixed-point/monolithic preserve-only lanes
- adding one more exact header token per route is narrower than creating a new checker
- it does not blur Year2 scope because the checker is already Year1 route-matrix specific

## acceptance condition for future actual split

future actual split acceptance condition は次である。

- `coupled_run2d_step_flex_iteration_column_name` の actual move は、
  exact `step_flex_counter_columns` guard acceptance 後に限る
- preferred order は:
  1. approved checker-hardening phase
  2. approved actual C split phase
- combined phase is possible only if the user explicitly approves both
  checker/script edit and C edit in the same phase and the patch remains narrow
- default recommendation は separate phases とする
  - provenance clarity が高い
  - broad dirty state 下でも review scope を狭く保てる

## future approval gate

固定する approval gate は次である。

- Phase 2M is docs-only
- no C edit is performed
- no checker/script edit is performed
- no build system change is performed
- no commit/staging/push is performed
- future checker/script edit requires explicit user approval
- future C edit requires explicit user approval
- if checker/script edit and C edit are combined, both approvals are required
- actual split remains blocked until the guard acceptance condition is satisfied
- target surface の checkpoint discipline は future actual edit 前も維持する

## risks

主要 risk は次である。

- oneway-only exact guard を multi-route acceptance の代わりに扱うと、
  delayed/strong/monolithic label drift を見逃す
- dedicated new checker を増やすと provenance と maintenance cost が上がる
- Year2 checker surface に monolithic preserve-only guard を混ぜると scope boundary が曖昧になる
- checker hardening と helper move を一度に行うと、approval と review scope が広がる

## closeout criteria

Phase 2M docs-only planning は次を満たしたら closeout-ready とする。

- `step_flex_counter_columns` current emission point を確認できた
- current exact coverage が oneway-only であることを明記できた
- candidate A/B/C/D を比較できた
- selected future guard path を
  `scripts/check_year1_coupled_route_matrix.sh` extension に固定できた
- minimum route coverage を列挙できた
- future actual split acceptance condition を明記できた
- future approval gate を明記できた
