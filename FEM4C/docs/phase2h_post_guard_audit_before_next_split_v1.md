# Phase 2H Post-Guard Audit Before Next Split v1

## purpose

Phase 2G で `coupled_run2d` output/report surface に対する first checker-hardening subset を
入れたあと、Phase 2F の minimum guard set と current checker coverage を read-only で照合し、
次に split planning へ下げられる helper と、まだ `no-split-yet` に残す helper を再固定する。

今回は docs-only の post-guard audit とし、actual C split / function move / checker edit は行わない。

## scope and non-goals

- scope は `src/coupled/coupled_run2d.c` と、その output/report contract を guard している
  checker/doc surface の read-only audit に限定する。
- `scripts/check_coupled_integrators.sh` と
  `scripts/check_year1_coupled_route_matrix.sh` の current coverage を source of truth として読む。
- year2 / same-time file-exchange / mixed-bounded checker は indirect semantic guard として扱う。
- non-goals は additional C split、function move、header split、checker/script edit、
  build system change、route/output semantics change。

## prerequisites

Phase 2H audit 時点で次を current workspace 上で確認した。

- `docs/phase2a_c_source_split_planning_v1.md`
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md`
- `docs/phase2c_c_split_verification_plan_v1.md`
- `docs/phase2e_coupled_run2d_output_contract_audit_v1.md`
- `docs/phase2f_coupled_run2d_guard_hardening_plan_v1.md`
- `src/coupled/coupled_run2d_config.c` exists
- `Makefile` includes `src/coupled/coupled_run2d_config.c`
- `scripts/check_year1_coupled_route_matrix.sh` contains the Phase 2G exact guard additions

## monolithic premise

This phase keeps the monolithic premise fixed exactly as follows.

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Therefore, monolithic-related output/report wording is audited only for no-regression
and must not be reinterpreted as Year2 active scope or completion work.

## Phase 2G guard coverage summary

### achieved exact coverage

`scripts/check_coupled_integrators.sh` remains the canonical one-way exact checker and
already guards:

- `step_runner=coupled_step_oneway2d_run`
- `comparison_role=official_reference`
- `solver_route_class=accepted_snapshot_replay`
- `delay_semantics_status=not_applicable`
- exact one-way `step_columns`
- exact `snapshot_columns`
- path-normalized `snapshot_record`

Phase 2G extended `scripts/check_year1_coupled_route_matrix.sh` so that year1 route
fixtures now exact-check:

- runtime `step_runner` for oneway / delayed / fixed-point strong / monolithic
- runtime `comparison_role`
- runtime `solver_route_class` for oneway / fixed-point strong / monolithic
- runtime `delay_semantics_status` for oneway / delayed
- preserve-only distinction tokens
  - `year1_experimental_only=1`
  - `fixed_point_strong!=monolithic_strong_v1`
  - `monolithic_strong_v1!=delayed_cosim_v1_5`
- output-file `solver_route_class`, `delay_semantics_status`, `coupling_metric`,
  `step_columns`, `compare_schema_version`, `compare_step_columns`
- monolithic-only `physical_status_columns` and `step_status_columns`

### indirect semantic coverage

The following checker families still matter, but they are indirect guards rather than
exact route-contract checkers.

- `tools/checkers/year2/check_year2_oneway_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_lagged_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_stub.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_bundle.sh`
- `tools/checkers/mixed_bounded/check_mixed_macro_local_replay_split_generic_v1.sh`

These protect route behavior, artifact presence, downstream pipeline semantics, and the
Year2 "monolithic remains out of scope" doc contract, but they do not fully replace
dedicated exact field guards for writer/header helper movement.

## achieved / remaining guard table

| guard target | intended equality mode | current checker coverage | achieved by Phase 2G yes/no | remaining gap | blocks next split planning | next action |
| --- | --- | --- | --- | --- | --- | --- |
| strong-route output field exact guard | exact | `scripts/check_year1_coupled_route_matrix.sh` exact-checks strong runtime/output tokens including `comparison_role`, `solver_route_class`, `coupling_metric`, strong `step_columns` | yes | no dedicated checker separates strong output from writer body movement | no | treat pure strong metric/compare-schema helpers as split-planning candidates |
| delayed-cosim output summary guard | exact | `scripts/check_year1_coupled_route_matrix.sh` exact-checks delayed `step_runner`, `comparison_role`, `delay_semantics_status`, delayed `step_columns`, `compare_schema_version`, `compare_step_columns` | yes | delayed runtime `solver_route_class` is still guarded mainly through output contract, not its own dedicated runtime checker | no | keep delayed mapping helpers eligible for planning, but not writer movement |
| Year1 compare-schema guard | exact | `scripts/check_year1_coupled_route_matrix.sh` exact-checks `compare_schema_version` and `compare_step_columns` on routes using the year1 schema | yes | compare-schema-related helper outputs are still bundled inside the main writer | no | planning may isolate pure compare-schema helper cluster first |
| exported CSV header getter guard | exact | no dedicated checker for `coupled_run2d_artifact_metadata_columns_csv`, `coupled_run2d_interface_centers_header_csv`, `coupled_run2d_reaction_map_header_csv`, `coupled_run2d_observation_points_header_csv` | no | header getters have no direct exact guard | no, but blocks header-getter split | add dedicated exact header-getter guard before moving this cluster |
| legacy fallback wording guard | exact | no dedicated checker; only indirect route behavior coverage | no | fallback wording and preserve-only stub wording can still drift unnoticed | yes for fallback-related split planning | keep fallback body `no-split-yet` and add wording checker first |
| artifact-writer contract audit gate | exact + path-normalized | one-way has exact `snapshot_columns` and path-normalized `snapshot_record`; year1 route matrix exact-checks many writer fields; year2/mixed/file-exchange give indirect semantic coverage | no | no dedicated multi-route artifact-writer audit for `artifact_route_class`, `artifact_family`, `artifact_preferred_compare_source`, `artifact_aux_exports`, multi-route `snapshot_record` | yes for writer/body movement | do not move `coupled_run2d_write_output(...)` yet |
| monolithic preserve-only distinction guard | exact | `scripts/check_year1_coupled_route_matrix.sh` exact-checks preserve-only distinction tokens and monolithic route tokens | yes | wording still sits near main summary/writer surfaces | no for pure mapping helper planning | keep visible monolithic wording outside upcoming split targets |
| Year2 monolithic out-of-scope guard | semantic + doc exact | `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh` greps docs that keep monolithic out of Year2 current-goal scope | yes | checker is docs-based, not runtime-output based | no for next planning step | retain docs-based guard until a stronger need appears |

## updated move-readiness table

| helper / region | current responsibility | now-covered surfaces | remaining uncovered surfaces | updated readiness | required extra guard before move |
| --- | --- | --- | --- | --- | --- |
| `coupled_run2d_coupling_metric_from_scheme` | strong/monolithic coupling metric label mapping | exact strong/monolithic output coverage | none that materially blocks helper extraction | safe-next-planning | keep writer body in place |
| `coupled_run2d_delay_semantics_status_from_scheme` | delay semantics label mapping | exact oneway/delayed runtime and output coverage | no dedicated same-time exact field checker | safe-next-planning | retain semantic year2 guards during planning |
| `coupled_run2d_scheme_uses_strong_metrics` | predicate controlling strong metric branch | exact strong-route field coverage | still tied to writer branch shape | safe-next-planning | move only as a pure helper, not with writer-body edits |
| `coupled_run2d_scheme_uses_year1_compare_schema` | predicate controlling year1 compare schema emission | exact compare-schema coverage | bundled with writer control flow | safe-next-planning | move only with the compare-schema helper cluster |
| `coupled_run2d_compare_step_*` | compare-step row field derivation | exact `compare_step_columns` contract and row-family presence | no isolated checker per sub-helper | safe-next-planning | keep `compare_step` row write site in the main writer |
| `coupled_run2d_delay_buffer_scope_from_run` | delayed snapshot/provenance label mapping | delayed output contract partially exact | no dedicated runtime/output checker for this field alone | later | add dedicated delayed audit or field checker first |
| `coupled_run2d_step_flex_iteration_column_name` | `step_flex_counter_columns` header label selection | indirect writer coverage only | no exact checker for `step_flex_counter_columns` | later | add exact header/column guard |
| exported CSV header getters | interface/reaction/observation/artifact header CSV constants | indirect artifact presence only | no direct exact guard | later | add exact header-getter checker |
| `coupled_run2d_comparison_role_from_scheme` | user-visible comparison role wording | runtime exact coverage exists, but wording is central to summary and artifact semantics | tied to visible summary/writer wording and preserve-only distinctions | no-split-yet | add dedicated summary/output wording audit before move |
| `coupled_run2d_solver_route_class_from_run` | route-class wording emitted to stdout and artifacts | some exact runtime/output coverage | still coupled to writer semantics, body/interface count, and route explanation | no-split-yet | add stronger cross-route exact guard before move |
| `coupled_run2d_step_coupling_reason` | monolithic `step_status` wording | indirect exact coverage through `step_status_columns` only | no dedicated reason-string guard | no-split-yet | add exact `step_status` reason audit first |
| `coupled_run2d_write_output(...)` | main artifact writer | many output fields guarded exactly or semantically | dedicated multi-route writer audit missing | no-split-yet | complete artifact-writer contract hardening first |
| legacy fallback body | preserve-only no-flex stub path and wording | indirect route/error behavior coverage only | no dedicated wording guard | no-split-yet | add fallback wording checker first |
| visible monolithic preserve-only distinction wording | `year1_experimental_only` and distinction lines in runtime summary | exact preserve-only token coverage | wording still coupled to runtime summary block | no-split-yet | do not move before summary/writer audit is stronger |

## recommended next safe split planning target

The next safe split planning target should be the pure route-metric / compare-schema
helper cluster, not the writer or wording surfaces.

Recommended target cluster:

- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_*`

Why this cluster is the safest next planning target:

- its externally visible outputs are now guarded more directly than before Phase 2G
- it does not require public API expansion
- it does not require moving `coupled_run2d_write_output(...)`
- it does not touch monolithic preserve-only wording directly

`coupled_run2d_delay_buffer_scope_from_run` can stay queued behind this cluster as
`later`, but it should not be promoted to the first target yet.

## deferred surfaces

The following surfaces should remain deferred after Phase 2H.

- `coupled_run2d_write_output(...)`
- `coupled_run2d_summary_title_from_scheme`
- `coupled_run2d_step_runner_name_from_scheme`
- `coupled_run2d_path_class_from_scheme`
- `coupled_run2d_role_from_scheme`
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- visible monolithic preserve-only distinction wording
- exported CSV header getter cluster

## known gaps

- no dedicated exact checker yet for exported CSV header getters
- no dedicated exact checker yet for legacy fallback wording
- no dedicated multi-route artifact-writer audit gate yet
- `snapshot_record` path-normalized coverage is still canonical oneway-focused
- `step_flex_counter_columns` still lacks its own exact contract checker
- route summary title / path-class wording still relies on indirect coverage

## risks

- moving a helper that appears pure but still influences visible wording can silently
  weaken monolithic preserve-only boundaries
- over-trusting year2 semantic checkers can hide writer/header schema drift
- splitting writer-adjacent helpers too early can entangle future header getter guards
- treating docs-based Year2 monolithic guards as runtime guards can create false confidence

## closeout criteria

Phase 2H is closeout-ready only if all of the following hold.

- Phase 2F minimum guard set has been re-audited against Phase 2G actual coverage
- achieved vs remaining guard status is explicitly fixed
- helper readiness is reclassified into `safe-next-planning` / `later` / `no-split-yet`
- a single recommended next safe split planning target is named
- monolithic preserve-only / Year2 out-of-scope premises remain unchanged
