# Phase 2B Coupled Run2D Split Dry-Run Plan v1

## purpose

This document records the Phase 2B dry-run split plan for
`src/coupled/coupled_run2d.c` after Phase 2A identified it as the first
low-risk native split-planning target.

Phase 2B is planning only.

- no C file splits
- no function moves
- no header splits
- no build-system changes
- no include-path changes
- no solver behavior changes
- no parser behavior changes

The goal is to fix a future split map, keep-vs-move boundaries, and the
required no-regression checks before any native refactor is attempted.

## scope and non-goals

This phase covers only:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d.h`

Minimal related references inspected only as immediate dependencies:

- `src/coupled/coupled_step_common2d.h`
- `src/coupled/coupled_step_oneway2d.c`
- `src/coupled/coupled_step_delayed_cosim2d.c`
- `src/coupled/coupled_step_implicit2d.c`
- `src/coupled/coupled_step_explicit2d.c`
- `src/coupled/coupled_step_monolithic2d.c`
- immediate call site in `src/analysis/runner.c`

Out of scope:

- actual code splitting
- changing exported APIs
- changing runtime route behavior
- changing output/report semantics
- touching Year1 monolithic implementation logic
- treating monolithic as Year2 completion work

## prerequisite check

Phase 2A planning doc exists in the current workspace as
`docs/phase2a_c_source_split_planning_v1.md`.

The current Phase 2A conclusions relevant here are:

- `src/coupled/coupled_run2d.c` is the recommended first dry-run split target
- the remaining bulk is mostly config, report, setup, and orchestration
- the coupled dispatch body itself is a recommended no-split-yet zone until
  helper boundaries are externalized

This Phase 2B document refines that file-specific plan only.

## monolithic premise

Monolithic handling is fixed as follows:

- monolithic is fully implemented in Year1
- monolithic is out of scope for the Year2 current goal
- Phase 2B must preserve existing Year1 monolithic behavior
- Phase 2B must not treat monolithic as a new Year2 feature target
- Phase 2B must not use monolithic as a new completion condition or closeout
  scope for Year2

Therefore, any monolithic-related branch inside `coupled_run2d.c` is
preserve-only in this phase unless a separate no-regression plan exists first.

## current function clusters

Current `coupled_run2d.c` responsibilities cluster cleanly enough for planning:

### lifecycle / init / cleanup

- `coupled_run2d_zero`
- `coupled_run2d_free_dynamic_buffers`
- `coupled_run2d_reserve_flex_model_storage`
- `coupled_step_history2d_free_dynamic_buffers`
- `coupled_step_history2d_reserve_flex_body_storage`
- `coupled_step_history2d_reserve_snapshot_storage`
- `coupled_run2d_free`

These own exported object lifecycle and step-history storage surfaces that are
already used by step modules.

### environment / time-control config

- `coupled_time_control_set_defaults`
- `parse_env_int_or_default`
- `parse_env_double_or_default`
- `coupled_scheme_legacy_default_from_integrator`
- `coupled_time_control_validate_contract`
- `coupled_integrator_from_env`
- `coupled_time_control_from_env`

These are self-contained configuration helpers with small dependency radius.

### scheme / integrator parsing

- `string_equals_ignore_case`
- `coupled_integrator_to_string`
- `coupled_scheme_to_string`
- `coupled_integrator_parse`
- `coupled_scheme_parse`

These are mostly pure helpers and string contracts.

### model loading / FEM-MBD setup

- `coupled_run2d_load_master_input`
- `coupled_run2d_validate_flex_case`
- `coupled_run2d_load_single_flex_model`
- `coupled_run2d_load_flex_models`

These bridge `input_read_data`, `mbd_system2d_load`, `coupled_case2d_clone`,
global FEM state, and flex-model preparation.

### flexible body / snapshot bookkeeping

- `coupled_run2d_capture_step_flex_counters`
- `coupled_run2d_capture_marker_pose`
- `coupled_run2d_write_step_snapshots`
- `coupled_run2d_capture_interface_centers`

These own post-step snapshot bookkeeping, interface-center extraction, and
artifact-path capture.

### artifact path / output metadata

- `coupled_run2d_artifact_metadata_columns_csv`
- `coupled_run2d_interface_centers_header_csv`
- `coupled_run2d_reaction_map_header_csv`
- `coupled_run2d_observation_points_header_csv`
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
- `coupled_run2d_compare_step_iteration_count`
- `coupled_run2d_compare_step_coupling_converged`
- `coupled_run2d_compare_step_exchange_lag_steps`
- `coupled_run2d_compare_step_sample_hold_active`
- `coupled_run2d_compare_step_delayed_snapshot_step`
- `coupled_run2d_step_flex_iteration_column_name`
- `coupled_run2d_step_coupling_reason`
- `coupled_run2d_write_output`

This is the largest “report semantics” cluster and has the strongest downstream
tooling sensitivity.

### per-step orchestration shell

- top-level loop inside `coupled_run2d(...)`
- history allocation and cleanup
- summary stdout surface before and after step execution
- snapshot gating on convergence

### scheme dispatch

- `coupled_run2d_dispatch_step_by_scheme`

This is the central route-selection seam and should stay thin.

### legacy fallback / compatibility behavior

- `coupled_legacy_no_flex_fallback_error`
- legacy-default scheme source handling in `coupled_time_control_from_env`

This preserves a compatibility/error surface that older stub-oriented checks
may still inspect.

### Year1 monolithic preserve-only path / compatibility boundary

- `COUPLED_SCHEME_MONOLITHIC_STRONG_V1` branches inside:
  - `coupled_scheme_to_string`
  - `coupled_scheme_parse`
  - `coupled_time_control_validate_contract`
  - `coupled_run2d_role_from_scheme`
  - `coupled_run2d_comparison_role_from_scheme`
  - `coupled_run2d_solver_route_class_from_run`
  - `coupled_run2d_coupling_metric_from_scheme`
  - `coupled_run2d_scheme_uses_strong_metrics`
  - `coupled_run2d_scheme_uses_year1_compare_schema`
  - `coupled_run2d_dispatch_step_by_scheme`
  - stdout/report branches inside `coupled_run2d(...)` and
    `coupled_run2d_write_output(...)`

This path is already implemented in Year1 and is preserve-only here.

## proposed future file map

This is proposal only. No files are created in Phase 2B.

```text
src/coupled/
  coupled_run2d.c                 # exported run API + high-level orchestration / dispatch body
  coupled_run2d_config.c          # env / time-control / scheme/integrator parse helpers
  coupled_run2d_model_load.c      # coupled case / FEM/MBD model loading helpers
  coupled_run2d_artifacts.c       # output path / metadata / summary / report helpers
  coupled_run2d_snapshots.c       # flex body / snapshot bookkeeping helpers
```

Working interpretation:

- keep `coupled_run2d.c` as the exported front door and dispatch shell
- move pure config/string helpers first
- move model-load helpers after config split is proven safe
- move snapshot bookkeeping before touching final output writer
- defer `coupled_run2d_write_output(...)` until a downstream no-regression plan
  is ready

## keep-vs-move table

| current function / region | current responsibility | proposed future home | move readiness | reason | risk | verification needed |
| --- | --- | --- | --- | --- | --- | --- |
| `coupled_time_control_set_defaults`, `parse_env_*`, `coupled_scheme_legacy_default_from_integrator`, `coupled_time_control_validate_contract` | environment/time-control config | `coupled_run2d_config.c` | safe-first | small dependency radius and mostly local contract logic | env/default wording drift | env parsing and contract error message surface unchanged |
| `string_equals_ignore_case`, `coupled_integrator_to_string`, `coupled_scheme_to_string`, `coupled_integrator_parse`, `coupled_scheme_parse` | scheme/integrator parse + string surface | `coupled_run2d_config.c` | safe-first | pure helpers with low runtime coupling | string contract drift | public parse/to-string API unchanged |
| `coupled_run2d_zero`, `coupled_run2d_free_dynamic_buffers`, `coupled_run2d_reserve_flex_model_storage`, `coupled_run2d_free` | exported lifecycle API | `coupled_run2d.c` | no-split-yet | exported API and object ownership should stay obvious in the core file first | public API churn | header unchanged, direct caller unchanged |
| `coupled_step_history2d_free_dynamic_buffers`, `coupled_step_history2d_reserve_flex_body_storage`, `coupled_step_history2d_reserve_snapshot_storage` | exported history storage API shared with step modules | `coupled_run2d_snapshots.c` | later | move is plausible, but used across step modules and tied to header ownership | header churn and linker surface changes | all step modules compile against unchanged declarations |
| `coupled_run2d_load_master_input`, `coupled_run2d_validate_flex_case`, `coupled_run2d_load_single_flex_model`, `coupled_run2d_load_flex_models` | model loading / FEM-MBD setup | `coupled_run2d_model_load.c` | later | coherent cluster, but depends on globals, parser/input state, and flex preparation | input/setup regression | same input contracts, same flex validation failures, same model counts |
| `coupled_run2d_capture_step_flex_counters`, `coupled_run2d_capture_marker_pose`, `coupled_run2d_capture_interface_centers` | bookkeeping and snapshot support | `coupled_run2d_snapshots.c` | later | mostly helper logic, but downstream artifact semantics depend on them | snapshot metadata drift | snapshot row/path semantics unchanged |
| `coupled_run2d_write_step_snapshots` | post-step snapshot artifact writing | `coupled_run2d_snapshots.c` | later | coherent with snapshot helpers, but output contract is tooling-visible | artifact path/content regression | snapshot CSV, path, iteration index, and gating semantics unchanged |
| metadata/header helpers such as `coupled_run2d_*header_csv`, `*_role_from_scheme`, `*_solver_route_class_from_run`, `*_compare_step_*` | artifact/report metadata contract | `coupled_run2d_artifacts.c` | safe-first | mostly pure mapping helpers | route/summary wording drift | summary/output keys and compare schema lines unchanged |
| `coupled_run2d_write_output` | final output file writer | `coupled_run2d_artifacts.c` | no-split-yet | central output contract inspected by downstream compare/export/check flows | output semantic regression | output file exact/semantic regression checks required first |
| `coupled_run2d_dispatch_step_by_scheme` | scheme dispatch | `coupled_run2d.c` | no-split-yet | central route-selection boundary should stay adjacent to exported run loop | dispatch regression | one-way / delayed / strong / monolithic route selection unchanged |
| `coupled_legacy_no_flex_fallback_error` | legacy fallback / compatibility error surface | `coupled_run2d.c` | no-split-yet | compatibility behavior is intentionally awkward but stable | fallback wording and failure-point drift | legacy fallback stdout/stderr and error code unchanged |
| `coupled_run2d(...)` main body | exported orchestration shell | `coupled_run2d.c` | no-split-yet | main front door should remain thin but local during first dry-run planning | route behavior regression | full route smoke and output semantics unchanged |
| Year1 monolithic-related branches | preserve-only compatibility boundary | `coupled_run2d.c` or later artifact/config helper split only under no-regression proof | no-split-yet | Year1 monolithic is complete and must not become Year2 refactor scope by accident | monolithic regression or scope drift | explicit Year1 monolithic no-regression checklist |

## no-split-yet zones

The following are fixed no-split-yet zones for the first actual dry-run split:

- `coupled_run2d_dispatch_step_by_scheme`
- the main orchestration body inside `coupled_run2d(...)`
- `coupled_legacy_no_flex_fallback_error`
- `coupled_run2d_write_output(...)`
- any region whose move would require public header/API redesign
- all Year1 monolithic preserve-only branches unless a separate no-regression
  plan exists first

Additional rule:

- monolithic-related code must not be removed, rewritten, or reframed as Year2
  completion work in this phase

## header / build touch points

If a later implementation phase performs the split, the likely touch points are:

### public header touch points

- `src/coupled/coupled_run2d.h` currently declares:
  - parse/to-string APIs
  - lifecycle APIs
  - history storage APIs
  - exported `coupled_run2d(...)`
- the first split should prefer keeping this public header stable
- if new private helper translation units are added, prefer a private internal
  header before changing the public header

### compile / link touch points

- `Makefile` or equivalent object-list wiring will need new
  `coupled_run2d_*.o` entries in a future implementation phase
- include dependency propagation for:
  - `../io/input.h`
  - step module headers
  - `flex_body2d.h`
  - `flex_snapshot2d.h`
  - `flex_solver2d.h`
- internal helper prototypes must not leak into public headers unless required

### direct caller touch points

- `src/analysis/runner.c` currently calls `coupled_run2d(...)`
- step modules already include `coupled_run2d.h` and consume history storage
  APIs
- this means lifecycle/history exports are more sensitive than pure config
  helpers

## verification checklist

Any future actual split should be blocked on the following checklist.

### compile surface

- new translation units compile cleanly
- no missing prototypes or duplicate symbols
- public `coupled_run2d.h` API remains link-compatible

### public API unchanged

- `coupled_run2d(...)` signature unchanged
- parse/to-string API signatures unchanged
- lifecycle and history storage API signatures unchanged

### route behavior unchanged

- one-way route behavior unchanged
- lagged/delayed co-simulation route behavior unchanged
- fixed-point strong route behavior unchanged
- Year1 monolithic route behavior unchanged

### output / summary semantics unchanged

- stdout summary lines unchanged or semantically unchanged where path-bearing
  text exists
- output file keys and CSV header surfaces unchanged
- route-class / comparison-role / artifact metadata semantics unchanged
- snapshot gating on non-converged strong steps unchanged

### route smoke coverage

- one-way route smoke checklist
- lagged/delayed route smoke checklist
- same-step strong route smoke checklist
- Year1 monolithic no-regression checklist

### downstream tooling

- downstream compare/check/report tooling that inspects coupled output remains
  unaffected
- mixed bounded / report tooling remains unaffected if it consumes shared
  output/report semantics

### boundary confirmation

- Year2 monolithic remains out of scope confirmation
- no Python / shell in solver core confirmation

## risks

- exported lifecycle/history helpers are already shared across step modules, so
  a naive first split can create avoidable header churn
- `coupled_run2d_write_output(...)` encodes output/report semantics that
  downstream compare/export/check tooling may inspect
- legacy fallback behavior is intentionally compatibility-shaped and easy to
  perturb by “cleanup”
- scheme dispatch is small but central; moving it early increases route
  regression risk without reducing much file weight
- monolithic branches are easy to misread as “unfinished experimental code”
  even though Year1 behavior is already implemented and must be preserved
- there is specific risk of accidentally reintroducing monolithic as Year2
  active scope if the docs do not keep the preserve-only premise explicit

## closeout criteria

Phase 2B dry-run planning is closeout-ready when all of the following are
true:

- Phase 2A planning exists and was used as the prerequisite
- `coupled_run2d.c` current clusters are documented
- a future file map is fixed
- keep-vs-move and no-split-yet boundaries are fixed
- monolithic premise is written as Year1 implemented / Year2 out of scope /
  preserve-only
- header/build touch points are enumerated
- verification checklists are recorded
- no C files, headers, build rules, or runtime behaviors were changed
