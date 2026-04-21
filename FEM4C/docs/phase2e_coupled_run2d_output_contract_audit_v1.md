# Phase 2E Coupled Run2D Output Contract Audit v1

## purpose

This document records the Phase 2E read-only audit of
`src/coupled/coupled_run2d.c` after the Phase 2D safe-first split moved only
config/string helpers into `src/coupled/coupled_run2d_config.c`.

Phase 2E is read-only and docs-only.

- no additional C split
- no function move
- no header split
- no build-system change
- no include-path change
- no solver behavior change
- no parser behavior change

The goal is to inventory current output/summary/artifact contracts, identify
downstream consumer dependence, and re-fix which artifact-sensitive helper
regions remain `later` versus `no-split-yet` before any further native split.

## scope and non-goals

This phase covers only:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d.h`

Primary focus:

- `coupled_run2d_write_output(...)`
- summary / report stdout surfaces in `coupled_run2d(...)`
- route-class / comparison-role / solver-route-class mapping helpers
- legacy fallback wording
- Year1 monolithic preserve-only reporting surface
- downstream scripts/tools/docs that inspect coupled output/report surfaces

Out of scope:

- modifying any C source or header
- hardening guards in code
- creating new runtime checkers
- redefining monolithic scope
- broad split planning beyond the audited output/report cluster

## prerequisites

Current workspace prerequisites confirmed for this audit:

- `docs/phase2a_c_source_split_planning_v1.md` exists
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md` exists
- `docs/phase2c_c_split_verification_plan_v1.md` exists
- `src/coupled/coupled_run2d_config.c` exists
- `Makefile` includes `src/coupled/coupled_run2d_config.c`

Relevant carry-forward conclusions:

- Phase 2D changed only the safe-first config/string helper cluster
- `coupled_run2d_dispatch_step_by_scheme`
- `coupled_run2d(...)`
- `coupled_run2d_write_output(...)`
- legacy fallback wording
- Year1 monolithic preserve-only branches

all remain in `src/coupled/coupled_run2d.c`

## monolithic premise

Monolithic handling remains fixed as follows:

- Year1 monolithic is fully implemented
- Year2 monolithic is out of current scope
- existing Year1 monolithic behavior is preserve-only

Implications for this audit:

- any `monolithic_strong_v1` wording visible in stdout/output is part of an
  existing preserve-only runtime surface
- Phase 2E must not reframe monolithic as Year2 active completion work
- helper movement that could alter monolithic report wording remains blocked
  unless stronger guards exist first

## output/summary contract inventory

### stdout summary surface

`coupled_run2d(...)` emits the high-level run summary before step execution.
Current visible surface includes:

- summary title from `coupled_run2d_summary_title_from_scheme`
- `scheme`
- `path_class`
- `step_dispatch_basis`
- `step_runner`
- `scheme_source`
- `integrator`
- `coupling_role`
- `comparison_role`
- `solver_route_class`
- `delay_semantics_status`
- `v2_decision_state`
- `year1_experimental_only`
- preserve-only distinction lines such as
  `fixed_point_strong!=monolithic_strong_v1`
  and
  `monolithic_strong_v1!=delayed_cosim_v1_5`
- `feedback_to_mbd`
- `body_count` / `interface_count` on year1 compare-schema routes
- time-control values
- `snapshot_skip` warning lines on strong-route nonconvergence

### output file summary surface

`coupled_run2d_write_output(...)` writes a flat CSV-like summary/output file
containing:

- `integrator`
- `mbd_integrator`
- `coupling_scheme`
- `coupling_path_class`
- `coupling_scheme_source`
- `coupling_role`
- `comparison_role`
- `feedback_to_mbd`
- `solver_route_class`
- `delay_semantics_status`
- `v2_decision_state`
- `artifact_route_class`
- `artifact_family`
- `artifact_preferred_compare_source`
- `artifact_aux_exports`
- `steps_requested`
- `steps_executed`
- `flex_body_count`
- optionally `body_count`, `interface_count`, `iteration_metric_name`
- `snapshot_policy`
- delayed-cosim-only delay metadata
- strong-route-only convergence metadata
- `step_columns`
- optional `physical_status_columns`
- optional `step_status_columns`
- optional `compare_schema_version`
- optional `compare_step_columns`
- `flex_body_counter_columns`
- `step_flex_counter_columns`
- `snapshot_columns`
- row families:
  - `flex_body`
  - `flex_body_counter`
  - `step`
  - `physical_status`
  - `step_status`
  - `compare_step`
  - `step_flex_counter`
  - `snapshot_record`

### artifact-adjacent helper surface

The following helpers directly shape the contract above:

- `coupled_run2d_summary_title_from_scheme`
- `coupled_run2d_step_runner_name_from_scheme`
- `coupled_run2d_feedback_to_mbd_from_scheme`
- `coupled_run2d_path_class_from_scheme`
- `coupled_run2d_role_from_scheme`
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_body_count_from_run`
- `coupled_run2d_interface_count_from_run`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_delay_buffer_scope_from_run`
- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_*`
- `coupled_run2d_step_flex_iteration_column_name`
- `coupled_run2d_step_coupling_reason`
- exported CSV header getters:
  - `coupled_run2d_artifact_metadata_columns_csv`
  - `coupled_run2d_interface_centers_header_csv`
  - `coupled_run2d_reaction_map_header_csv`
  - `coupled_run2d_observation_points_header_csv`

### legacy fallback surface

`coupled_legacy_no_flex_fallback_error(...)` remains a separate compatibility
surface. It emits:

- `Coupled mode contract snapshot (stub):`
- `legacy_stub_role=non_default_no_flex_fallback`
- `default_path_requires_flex_bodies=1`
- integrator / coupling scheme / source / time parameters
- a stable error message describing the preserved legacy no-flex fallback

## downstream consumer matrix

| consumer | consumed surface | dependence type | note |
| --- | --- | --- | --- |
| `scripts/check_coupled_integrators.sh` | summary stdout and output file header/row lines | direct exact grep / regex | strongest current exact guard for one-way route output file shape |
| `scripts/check_year1_coupled_route_matrix.sh` | runtime log tokens including `step_runner`, `comparison_role`, `monolithic_iteration=` | direct exact substring | strongest current preserve-only monolithic runtime surface guard |
| `tools/checkers/year2/check_year2_oneway_mbd_fem_v1.sh` | one-way route outcome and downstream artifact flow | semantic route checker | protects current-goal route behavior, not every summary key directly |
| `tools/checkers/year2/check_year2_lagged_mbd_fem_v1.sh` | lagged route behavior and delayed semantics | semantic route checker | indirect guard on delayed report/output semantics |
| `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh` | same-time route behavior and subordinate oneway/lagged surfaces | semantic route checker | protects current-goal same-time lane; also confirms docs keep monolithic out of scope |
| `tools/checkers/mixed_bounded/check_mixed_macro_local_replay_split_generic_v1.sh` | cross-lane replay/export/report stability | indirect semantic consumer | catches downstream compare/report fallout rather than exact coupled stdout |
| `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_stub.sh` | same-time file-exchange route compatibility | indirect semantic consumer | route-adjacent guard, not a direct parser of `coupled_run2d` output file |
| `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_bundle.sh` | same-time file-exchange bundle/output stability | indirect semantic consumer | bundle-level guard for same-time lane |
| docs such as `docs/year2_milestone_spec_v1.md`, `docs/b_lane_revised_roadmap_v1.md`, `docs/year2_same_time_mbd_fem_v1.md` | route-class wording and monolithic scope statements | specification consumer | protect interpretation of output/report lanes even when not parsing runtime files |
| `src/fem4c.c` CLI/help | scheme names and monolithic wording | exact user-facing string surface | split must not drift from CLI scheme vocabulary |

## equality strictness table

| surface / field | producer region | downstream consumers | equality strictness | current guard | gap | move readiness |
| --- | --- | --- | --- | --- | --- | --- |
| summary title (`Coupled ... run summary:`) | `coupled_run2d_summary_title_from_scheme` | `scripts/check_coupled_integrators.sh`, human/operator docs | exact | direct grep for one-way title | no dedicated guards for all schemes | no-split-yet |
| `step_runner` | `coupled_run2d_step_runner_name_from_scheme` + step-module logs | `scripts/check_coupled_integrators.sh`, `scripts/check_year1_coupled_route_matrix.sh` | exact | direct grep | no dedicated all-route output-file guard | no-split-yet |
| `coupling_scheme` / `scheme` / parse vocabulary | config helpers + summary/output writers | `scripts/check_coupled_integrators.sh`, CLI help, route docs | exact | direct grep + CLI wording | no explicit split-specific scheme-vocabulary guard | no-split-yet |
| `comparison_role` | `coupled_run2d_comparison_role_from_scheme` | `scripts/check_year1_coupled_route_matrix.sh`, downstream compare interpretation | exact on visible tokens | direct grep for monolithic route | no dedicated delayed/oneway compare-role guard | no-split-yet |
| `solver_route_class` | `coupled_run2d_solver_route_class_from_run` | delayed/monolithic runtime surfaces, docs | semantic with exact visible tokens on some routes | indirect route checkers + route matrix tokens | no dedicated exact output-file guard | no-split-yet |
| `delay_semantics_status` | `coupled_run2d_delay_semantics_status_from_scheme` | delayed route logs, docs | semantic | delayed route checker + delayed step log | no exact file-level guard found | later |
| `coupling_metric` | `coupled_run2d_coupling_metric_from_scheme` | strong-route output files and docs | semantic with exact presence/absence | one-way checker forbids its appearance in oneway output | no exact strong-route value audit | later |
| `artifact_route_class` / `artifact_family` / `artifact_preferred_compare_source` / `artifact_aux_exports` | `coupled_run2d_write_output(...)` | downstream compare/report tooling, docs | semantic | indirect through route/report flows | no dedicated exact contract test | no-split-yet |
| `step_columns` / `compare_step_columns` / `step_flex_counter_columns` / `snapshot_columns` | `coupled_run2d_write_output(...)` + helper selection | `scripts/check_coupled_integrators.sh`, compare/report tools | exact | one-way checker exact grep | limited coverage outside one-way route | no-split-yet |
| `snapshot_record` path field | `coupled_run2d_write_output(...)` + snapshot path builder | `scripts/check_coupled_integrators.sh` | path-normalized | regex checks filename suffix | no cross-route path contract checker | later |
| `compare_schema_version` | `coupled_run2d_write_output(...)` | compare/report tooling | exact semantic identifier | indirect via year1 compare-schema use | no dedicated direct checker found | later |
| `year1_experimental_only` and distinction lines | summary stdout and step-module logs | year1/monolithic route checks, docs | exact | route matrix and route docs | no dedicated file-output guard | no-split-yet |
| legacy fallback wording | `coupled_legacy_no_flex_fallback_error(...)` | legacy stub checks and compatibility docs | exact semantic wording | preserved by staying in place | no dedicated isolated checker in this phase | no-split-yet |
| exported CSV header getters | `coupled_run2d_*header_csv` functions | artifact consumers not directly audited here | exact | no direct dedicated checker found | missing dedicated guard | later |

## move-readiness reassessment

### remain `no-split-yet`

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

Reason:

- these helpers either feed exact-grep runtime/output surfaces already in use
  or define wording that is critical to preserve-only monolithic interpretation
- current guards are strong enough to prove sensitivity, but not strong enough
  yet to make movement cheap

### may downgrade to `later` after guard hardening

- `coupled_run2d_delay_buffer_scope_from_run`
- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_*`
- `coupled_run2d_step_flex_iteration_column_name`
- exported CSV header getters

Reason:

- these are relatively pure mappings or schema selectors
- they still influence audited output contracts, but their downstream use is
  currently less direct than the no-split-yet cluster
- movement should wait until field-level guards exist for strong/delayed/year1
  compare schemas and exported header strings

### keep `later` but not first next target

- `coupled_run2d_body_count_from_run`
- `coupled_run2d_interface_count_from_run`

Reason:

- they are simple derivations, but they affect monolithic/delayed route-class
  metadata and year1 compare-schema fields
- they should move only with surrounding report helper guards in place

## additional guard recommendations

Before moving any artifact-sensitive helper cluster, add or strengthen:

- an exact contract checker for strong-route output file fields written by
  `coupled_run2d_write_output(...)`
- a dedicated delayed-cosim output summary guard for
  `delay_semantics_status`, `exchange_lag_steps`, and delayed snapshot fields
- a dedicated year1 compare-schema guard covering
  `compare_schema_version`, `compare_step_columns`, and `iteration_metric_name`
- an exported CSV-header guard for
  `coupled_run2d_interface_centers_header_csv`,
  `coupled_run2d_reaction_map_header_csv`,
  and `coupled_run2d_observation_points_header_csv`
- a dedicated legacy fallback wording checker if that path is ever considered
  for movement
- an explicit artifact writer contract audit if `coupled_run2d_write_output(...)`
  is ever proposed for split

## risks

Main risks after the Phase 2D split are:

- overestimating how much of the output contract is already directly guarded
- breaking exact route wording while touching apparently pure mapping helpers
- moving monolithic preserve-only wording into a helper file without enough
  regression evidence
- drifting delayed-cosim metadata fields that currently have only indirect
  protection
- changing artifact-file schema selectors without a dedicated compare-schema
  contract guard

## closeout criteria

Phase 2E is closeout-ready when all of the following are true:

- the post-Phase-2D output/summary contract inventory is fixed
- downstream consumer dependence is fixed
- equality strictness is classified by surface
- `later` versus `no-split-yet` is re-fixed for artifact-sensitive helpers
- additional guard recommendations are recorded
- monolithic remains explicitly Year1-complete, Year2-out-of-scope,
  preserve-only
