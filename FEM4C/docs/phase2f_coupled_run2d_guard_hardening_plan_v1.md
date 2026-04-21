# Phase 2F Coupled Run2D Guard Hardening Plan v1

## purpose

This document records the Phase 2F docs-only plan for hardening checker
coverage around `src/coupled/coupled_run2d.c` output / summary / artifact
surfaces after the Phase 2E audit fixed the current contract inventory.

Phase 2F is docs-only.

- no additional C split
- no function move
- no header split
- no build-system change
- no include-path change
- no solver behavior change
- no parser behavior change

The goal is to convert the Phase 2E guard gaps into concrete checker-hardening
tasks so that artifact-sensitive helpers can later move with a defined minimum
verification floor.

## scope and non-goals

This phase covers only:

- `src/coupled/coupled_run2d.c`
- current checker surfaces under `scripts/` and `tools/checkers/`
- related docs:
  - `docs/phase2e_coupled_run2d_output_contract_audit_v1.md`
  - `docs/release_architecture_reorganization_plan_v1.md`

This phase does not:

- implement new guards yet
- modify existing checkers
- change route/output semantics
- relax any `no-split-yet` boundary by code change
- reframe monolithic as Year2 active scope

## prerequisites

Current workspace prerequisites confirmed for this plan:

- `docs/phase2a_c_source_split_planning_v1.md` exists
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md` exists
- `docs/phase2c_c_split_verification_plan_v1.md` exists
- `docs/phase2e_coupled_run2d_output_contract_audit_v1.md` exists
- `src/coupled/coupled_run2d_config.c` exists
- `Makefile` includes `src/coupled/coupled_run2d_config.c`

Relevant carry-forward conclusions:

- Phase 2D moved only the safe-first config/string helper cluster
- Phase 2E re-fixed the output/summary/artifact contract inventory
- Phase 2E re-fixed `no-split-yet` versus `later` for artifact-sensitive
  helpers

## monolithic premise

Monolithic handling remains fixed as follows:

- Year1 monolithic is fully implemented
- Year2 monolithic is out of current scope
- existing Year1 monolithic behavior is preserve-only

Implications for guard hardening:

- guards may verify that preserve-only wording remains distinct
- guards must not turn monolithic into a Year2 completion condition
- any guard proposal that widens Year2 route acceptance onto monolithic is out
  of scope

## current checker coverage summary

Current checker coverage is uneven by surface.

Strongest direct exact guards:

- `scripts/check_coupled_integrators.sh`
  - exact one-way summary vocabulary
  - exact one-way output-file header and row-shape lines
  - exact/pattern guard for `step_columns`, `snapshot_columns`,
    `snapshot_record`
  - exact absence guard for strong-route-only output fields in one-way output
- `scripts/check_year1_coupled_route_matrix.sh`
  - exact runtime tokens for `step_runner`
  - exact preserve-only monolithic token
  - exact delayed exchange-lag token

Strongest indirect semantic guards:

- `tools/checkers/year2/check_year2_oneway_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_lagged_mbd_fem_v1.sh`
- `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh`
- `tools/checkers/mixed_bounded/check_mixed_macro_local_replay_split_generic_v1.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_stub.sh`
- `tools/checkers/routes/same_time_file_exchange/check_same_time_file_exchange_bundle.sh`

Current documentation/scope guards:

- `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh`
  already greps docs that keep monolithic outside Year2 current-goal scope
- `docs/year2_milestone_spec_v1.md`
- `docs/year2_same_time_mbd_fem_v1.md`
- `docs/b_lane_revised_roadmap_v1.md`
  already encode the Year1-preserve-only / Year2-out-of-scope split

Main current weakness:

- exact output-field coverage is concentrated in the one-way route
- delayed / strong / year1 compare-schema / exported header getter surfaces
  remain only partially guarded or indirectly guarded

## guard-hardening matrix

| guard target | candidate checker home | equality mode | route scope | current coverage | gap | implementation readiness | blocker if any |
| --- | --- | --- | --- | --- | --- | --- | --- |
| strong-route output field exact guard | new dedicated checker adjacent to `scripts/check_coupled_integrators.sh` | exact | `fixed_point_strong`, `monolithic_strong_v1` | one-way checker only forbids strong fields on oneway output; route matrix checks runtime logs only | no exact file-level guard for strong-route `step_columns`, `coupling_metric`, `physical_status_columns`, `step_status_columns` | ready after guard doc closeout | needs canonical strong-route fixture selection |
| delayed-cosim output summary guard | extend `scripts/check_year1_coupled_route_matrix.sh` or add dedicated delayed output checker | exact plus semantic | `delayed_cosim_v1_5` | route matrix checks `step_runner` and `exchange_lag_steps=1`; year2 lagged checker guards semantics indirectly | no exact delayed output-file guard for `delay_semantics_status`, delayed `step_columns`, delayed snapshot metadata | ready after deciding whether to extend or isolate checker | current exact delayed artifact file expectations are not centralized |
| Year1 compare-schema guard | new dedicated compare-schema contract checker | exact plus semantic | `oneway_snapshot`, `monolithic_strong_v1`, `delayed_cosim_v1_5` | oneway exact guard covers only the one-way subset; compare tooling guards semantics downstream | no direct guard for `compare_schema_version`, `compare_step_columns`, `iteration_metric_name` across year1 compare-schema routes | ready after fixture normalization | needs route-specific expected row sets and schema baselines |
| exported CSV header getter guard | new small contract checker under `scripts/` | exact | artifact export getters | no direct dedicated checker found | `coupled_run2d_*header_csv` strings can drift without immediate failure | ready | needs a minimal native probe or existing runtime path that prints headers |
| legacy fallback wording guard | new dedicated legacy stub wording checker | exact semantic wording | non-default no-flex fallback only | wording preserved by staying in place; no direct isolated checker in current matrix | fallback wording can drift silently if the body is ever moved/refactored | later | needs a stable way to trigger fallback intentionally without collateral route changes |
| artifact-writer contract audit gate | docs gate before any future split of `coupled_run2d_write_output(...)` | exact + semantic + path-normalized | all coupled routes using writer | Phase 2E audit fixed the inventory only | no dedicated multi-route artifact writer audit exists yet | planning-ready only | should not be implemented as a partial checker before row-family expectations are frozen |

## minimum guard set before next split

Before moving any Phase 2E `later` helper, the following minimum guard set
should exist:

- exact guard for strong-route summary vocabulary
- exact guard for `step_runner`
- exact guard for `comparison_role`
- exact guard for `step_columns`
- exact guard for `compare_step_columns`
- exact guard for `snapshot_columns`
- semantic guard for `solver_route_class`
- semantic guard for `delay_semantics_status`
- semantic guard for `coupling_metric`
- path-normalized guard for `snapshot_record`
- explicit guard that Year1 monolithic preserve-only wording remains distinct
- explicit guard that Year2 monolithic is not reintroduced as active
  current-goal scope

Recommended ownership split:

- exact one-way and strong/delayed file-output guards:
  dedicated coupled output contract checker family under `scripts/`
- preserve-only route wording and route token guards:
  extend `scripts/check_year1_coupled_route_matrix.sh`
- Year2 scope boundary guards:
  keep using `tools/checkers/year2/check_year2_same_time_mbd_fem_v1.sh`
  plus docs sync
- mixed-bounded and same-time file-exchange fallout:
  continue using existing semantic/indirect route checkers as regression
  backstops, not as the primary exact contract checker

## no-split-yet / later reassessment

Phase 2E reassessment remains in force.

Remain `no-split-yet`:

- `coupled_run2d_write_output(...)`
- summary/report wording helpers
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- visible monolithic preserve-only distinction wording

May remain `later` after guard hardening:

- `coupled_run2d_delay_buffer_scope_from_run`
- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_*`
- `coupled_run2d_step_flex_iteration_column_name`
- exported CSV header getters

Interpretation for Phase 2F:

- Phase 2E `later` does not mean "move next"
- it means "move only after the minimum guard set above exists"

## deferred guards

The following are intentionally deferred rather than required before the next
small split:

- dedicated exact guard for `artifact_route_class` / `artifact_family`
  across every route
- dedicated exact guard for `artifact_preferred_compare_source`
  / `artifact_aux_exports`
- dedicated multi-route path contract checker for every `snapshot_record`
  variant beyond the first canonical one-way baseline
- standalone automation for artifact-writer row-family exhaustiveness

Reason:

- these surfaces are important, but current next-step safety depends more on
  strong/delayed/year1 compare-schema coverage and preserve-only wording
  coverage
- broad artifact-writer contract automation should follow, not precede, the
  minimum guard set

## risks

Main risks are:

- hardening only one-way exact guards and assuming strong/delayed routes are
  equivalently protected
- extending existing checkers too aggressively and mixing route semantics with
  artifact-schema checks in ways that obscure failures
- creating a legacy fallback checker that unintentionally widens fallback use
  beyond its preserve-only purpose
- using Year2 semantic route checkers as a substitute for exact output
  contract guards
- letting monolithic wording checks drift from the Year1-preserve-only /
  Year2-out-of-scope premise

## closeout criteria

Phase 2F is closeout-ready when all of the following are true:

- current checker coverage has been summarized
- guard gaps have been mapped to concrete checker-hardening targets
- the minimum guard set before the next split has been fixed
- `no-split-yet` versus `later` remains explicit after guard hardening
- deferred guards are named so they are not confused with the minimum set
- monolithic remains explicitly Year1-complete, Year2-out-of-scope,
  preserve-only
