# Phase 2K Post-Route-Meta Split Audit v1

## purpose

Phase 2J で実施した `src/coupled/coupled_run2d.c` route-metric /
compare-schema helper cluster の narrow split patch について、Phase 2I plan と
actual patch / checker coverage を read-only で照合し、acceptability と
remaining boundary を docs-only で固定する。

Phase 2K では additional C edit は行わない。目的は、Phase 2J が
「move-first helper set だけを移した narrow split」として受理可能であること、
および次に planning へ進める helper boundary を再固定することにある。

## scope and non-goals

This phase covers only:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_route_meta.c`
- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d.h`
- `Makefile`
- `scripts/check_coupled_integrators.sh`
- `scripts/check_year1_coupled_route_matrix.sh`
- Year2 / same-time file-exchange / mixed-bounded checker surfaces
- related docs:
  - `docs/phase2f_coupled_run2d_guard_hardening_plan_v1.md`
  - `docs/phase2h_post_guard_audit_before_next_split_v1.md`
  - `docs/phase2i_coupled_run2d_route_meta_split_plan_v1.md`
  - `docs/release_architecture_reorganization_plan_v1.md`

This phase does not:

- edit C source or headers
- move functions
- edit scripts/checkers
- change the build graph
- rerun broad architecture planning outside the `coupled_run2d` split lane

## prerequisites

Current workspace prerequisites confirmed for this audit:

- `docs/phase2a_c_source_split_planning_v1.md`
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md`
- `docs/phase2c_c_split_verification_plan_v1.md`
- `docs/phase2e_coupled_run2d_output_contract_audit_v1.md`
- `docs/phase2f_coupled_run2d_guard_hardening_plan_v1.md`
- `docs/phase2h_post_guard_audit_before_next_split_v1.md`
- `docs/phase2i_coupled_run2d_route_meta_split_plan_v1.md`
- `src/coupled/coupled_run2d_config.c` exists and `Makefile` includes it
- `src/coupled/coupled_run2d_route_meta.c` exists and `Makefile` includes it
- `src/coupled/coupled_run2d.c` retains compile-preserving declarations and
  call-sites for the moved helper set
- `scripts/check_year1_coupled_route_matrix.sh` still contains the Phase 2G
  exact route/output guards

Carry-forward acceptance premise:

- Phase 2J route-meta split is accepted only because the required verification
  matrix passed
- the current audit does not re-run that matrix; it treats the accepted Phase 2J
  result as the baseline being reviewed

## monolithic premise

The monolithic premise remains fixed exactly as follows:

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Implications for Phase 2K:

- Phase 2J remains acceptable only if preserve-only wording and behavior were not
  widened or reinterpreted
- remaining no-split-yet surfaces must continue to exclude visible monolithic
  preserve-only wording
- no next planning target may imply monolithic re-entry into Year2 current-goal
  scope

## Phase 2J patch audit summary

### planned vs actual moved helper set

Phase 2I planned the following move-first helper set:

- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_iteration_count`
- `coupled_run2d_compare_step_coupling_converged`
- `coupled_run2d_compare_step_exchange_lag_steps`
- `coupled_run2d_compare_step_sample_hold_active`
- `coupled_run2d_compare_step_delayed_snapshot_step`

Current source confirms that exactly this set now lives in
`src/coupled/coupled_run2d_route_meta.c`, with `src/coupled/coupled_run2d.c`
retaining only compile-preserving declarations and call-sites.

### declaration surface

Current source also confirms:

- `src/coupled/coupled_run2d.h` was not widened for the route-meta split
- no private internal header was introduced
- `src/coupled/coupled_run2d.c` keeps local forward declarations for the moved
  helper set
- `src/coupled/coupled_run2d_route_meta.c` includes `coupled_run2d.h` and owns
  the moved definitions

### untouched boundaries

Current source still keeps the following in `src/coupled/coupled_run2d.c`:

- `coupled_run2d_write_output(...)`
- summary/report wording helpers
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- scheme dispatch body
- main orchestration body

This matches the Phase 2I no-split-yet boundary.

### build graph

`Makefile` confirms a minimal build-graph change:

- `$(SRCDIR)/coupled/coupled_run2d_route_meta.c` was added to `COUPLED_SRCS`
- no include-path change is visible
- no public header split is visible

## checker coverage summary

### direct exact coverage supporting Phase 2J acceptance

The strongest exact guards remain:

- `scripts/check_coupled_integrators.sh`
  - exact one-way runtime tokens including `step_runner`
  - exact one-way output-file tokens for `comparison_role`,
    `solver_route_class`, `delay_semantics_status`
  - exact one-way `step_columns`
  - exact `snapshot_columns`
  - path-normalized `snapshot_record`
- `scripts/check_year1_coupled_route_matrix.sh`
  - exact runtime tokens for oneway / delayed / fixed-point strong /
    monolithic routes
  - exact output tokens for strong/delayed/year1 compare-schema surfaces
  - exact guards for `coupling_metric`, `delay_semantics_status`,
    `compare_schema_version`, `compare_step_columns`
  - exact preserve-only distinction tokens
    - `year1_experimental_only=1`
    - `fixed_point_strong!=monolithic_strong_v1`
    - `monolithic_strong_v1!=delayed_cosim_v1_5`

These exact guards are sufficient to accept the moved Phase 2J cluster because
the cluster only emits route-metric / compare-schema tokens and compare-step
derived numeric fields.

### indirect semantic coverage that still matters

Indirect semantic backstops remain:

- `tools/checkers/year2/check_year2_oneway_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_lagged_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_stub.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_bundle.sh`
- `tools/checkers/mixed_bounded/check_mixed_macro_local_replay_split_generic_v1.sh`

These support the Phase 2J acceptance claim that route/output semantics stayed
unchanged, but they do not replace dedicated exact guards for visible wording,
header getters, or the writer body.

### Year2 monolithic out-of-scope guard

The Year2 out-of-scope guard remains docs-based and semantic:

- `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh` still greps docs
  that keep monolithic outside Year2 current-goal scope
- this is sufficient to retain the planning boundary
- it is not a substitute for runtime wording guards inside `coupled_run2d.c`

## updated move-readiness table

| helper / region | current role after Phase 2J | current guard support | updated readiness | reason | required extra guard before move |
| --- | --- | --- | --- | --- | --- |
| moved route-meta helper cluster (`coupling_metric_from_scheme`, `delay_semantics_status_from_scheme`, `scheme_uses_strong_metrics`, `scheme_uses_year1_compare_schema`, `compare_step_*`) | already split into `coupled_run2d_route_meta.c` | exact + semantic guard floor accepted by Phase 2J matrix | accepted / closed for this split | Phase 2I plan matched actual move set and Phase 2J matrix passed | none for current acceptance; future edits still need normal regression matrix |
| `coupled_run2d_delay_buffer_scope_from_run` | delayed-only scope token emitted by writer | partial delayed output coverage only | later | still writer-adjacent and lacks dedicated exact field guard | add dedicated delayed scope/output checker |
| `coupled_run2d_step_flex_iteration_column_name` | selects `step_flex_counter_columns` label | indirect only | later | no exact guard for `step_flex_counter_columns` | add exact step-flex column checker |
| exported CSV header getters | public exported artifact header constants | indirect only | later | public API surface plus no direct exact header checker | add dedicated header-getter checker before any move |
| `coupled_run2d_comparison_role_from_scheme` | visible comparison-role wording | exact visible token coverage, but wording-sensitive | no-split-yet | visible wording remains central to runtime summary and writer semantics | dedicated wording audit first |
| `coupled_run2d_solver_route_class_from_run` | visible route-class wording | some exact coverage, but topology-sensitive | no-split-yet | still couples visible wording to route topology/body count | stronger cross-route exact guard first |
| `coupled_run2d_step_coupling_reason` | monolithic `step_status` wording | indirect via `step_status_columns` | no-split-yet | no dedicated exact reason-string guard | exact step-status reason checker first |
| `coupled_run2d_write_output(...)` | main artifact writer | many exact/semantic guards, but no dedicated writer audit | no-split-yet | artifact contract remains too dense for another narrow split | dedicated multi-route writer audit first |
| summary/report wording helpers | visible runtime summary wording | exact runtime token coverage only in selected routes | no-split-yet | wording remains coupled to preserve-only distinctions and user-visible surface | dedicated wording audit first |
| legacy fallback body | preserve-only no-flex fallback path | indirect only | no-split-yet | fallback wording has no dedicated checker | fallback wording checker first |
| visible monolithic preserve-only distinction wording | preserve-only summary tokens | exact token guards exist | no-split-yet | wording must remain outside any helper move that could blur Year2 scope | keep out of move candidates |
| scheme dispatch body | route step dispatch core | indirect via full route matrix | no-split-yet | not a narrow helper split target | separate dispatch plan only |
| main orchestration body | end-to-end run orchestration | indirect via full route matrix | no-split-yet | still the main behavior nexus | separate orchestration plan only |
| model-load helpers | master/flex model load and validation | behavior guarded only indirectly | later | not wording-sensitive, but larger behavior surface than current narrow split | dedicated model-load split planning first |
| snapshot helpers | snapshot write/capture path handling | one-way path-normalized coverage is partial | later | snapshot path and timing semantics still need wider route coverage | stronger multi-route snapshot audit first |
| artifact writer helpers | writer-adjacent artifact field/header helpers | uneven exact coverage | no-split-yet | too close to `coupled_run2d_write_output(...)` and public header getters | complete artifact-writer guard hardening first |

## recommended next planning target

The next safe planning target should stay narrow and avoid visible wording,
fallback, and writer-body movement.

Recommended next planning target:

- writer-adjacent metadata helper mini-cluster
  - `coupled_run2d_delay_buffer_scope_from_run`
  - `coupled_run2d_step_flex_iteration_column_name`

Why this is the best next planning target:

- both helpers are materially smaller than writer-body or wording surfaces
- neither requires public header widening by default
- both remain blocked mainly by missing exact guards, not by orchestration or
  dispatch coupling
- planning this cluster next keeps exported CSV header getters outside scope

What should not be the next target:

- `coupled_run2d_write_output(...)`
- summary/report wording helpers
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- legacy fallback body
- any visible monolithic preserve-only wording

## deferred surfaces

The following remain explicitly deferred after Phase 2K:

- `coupled_run2d_write_output(...)`
- summary/report wording helpers
- `coupled_run2d_summary_title_from_scheme`
- `coupled_run2d_step_runner_name_from_scheme`
- `coupled_run2d_path_class_from_scheme`
- `coupled_run2d_role_from_scheme`
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- visible monolithic preserve-only distinction wording
- exported CSV header getters
- scheme dispatch body
- main orchestration body

## known gaps

- no dedicated exact checker yet for `delay_buffer_scope`
- no dedicated exact checker yet for `step_flex_counter_columns`
- no dedicated exact checker yet for exported CSV header getters
- no dedicated exact checker yet for legacy fallback wording
- no dedicated multi-route artifact-writer audit gate yet
- snapshot path coverage is still strongest only on the canonical one-way route
- visible route summary title/path-class wording still relies on indirect guard

## risks

- moving a writer-adjacent helper without first adding its exact field guard can
  silently widen the effective split scope
- public exported header getters remain higher-risk than they appear because
  they are API surface, not just internal constants
- over-trusting semantic year2/mixed/file-exchange checkers can hide narrow
  output/header drift
- preserve-only monolithic wording can still be harmed by a wording-adjacent
  refactor even when the moved helper itself looks pure

## closeout criteria

Phase 2K is closeout-ready when all of the following are true:

- Phase 2I planned move-first helper set is confirmed to match the Phase 2J
  actual moved helper set
- current source confirms no public API expansion and no private internal header
  addition
- current source confirms writer / wording / fallback / dispatch /
  orchestration boundaries remain untouched
- checker coverage is split into direct exact versus indirect semantic support
- remaining readiness is re-fixed as `later` versus `no-split-yet`
- a single next planning target is named
- monolithic remains explicitly Year1-complete, Year2-out-of-scope,
  preserve-only
