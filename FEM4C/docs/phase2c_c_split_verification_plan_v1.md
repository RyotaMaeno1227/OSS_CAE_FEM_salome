# Phase 2C C Split Verification Plan v1

## purpose

This document records the Phase 2C verification-planning pass for the first
native split candidate identified in Phase 2A and refined in Phase 2B:
`src/coupled/coupled_run2d.c`.

Phase 2C is verification planning only.

- no C file splits
- no function moves
- no header splits
- no build-system changes
- no include-path changes
- no solver behavior changes
- no parser behavior changes

The goal is to fix the pre-split verification surfaces, go/no-go gates, and
known gaps before any actual native split is attempted in a later phase.

## scope and non-goals

This phase covers only verification planning around the current
`coupled_run2d.c` split proposal from Phase 2B.

Primary in-scope surfaces:

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d.h`
- the Phase 2B proposed future file map:
  - `coupled_run2d_config.c`
  - `coupled_run2d_model_load.c`
  - `coupled_run2d_artifacts.c`
  - `coupled_run2d_snapshots.c`

Read-only related references inspected only to identify verification surfaces:

- immediate step modules under `src/coupled/`
- `src/analysis/runner.c`
- `Makefile`
- existing release-lane route checkers under `scripts/` and `tools/checkers/`

Out of scope:

- creating the future files listed above
- touching solver-core C code
- touching headers
- changing output contracts
- changing route behavior
- redefining monolithic scope
- creating new checkers in this phase

## prerequisites

Phase 2C assumes the following docs exist in the current workspace:

- `docs/phase2a_c_source_split_planning_v1.md`
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md`

The current conclusions carried forward from those phases are:

- `src/coupled/coupled_run2d.c` is the first dry-run split candidate
- `coupled_run2d_dispatch_step_by_scheme` is still a no-split-yet boundary
- `coupled_run2d(...)` remains the exported orchestration front door
- config/string helpers are the best safe-first move candidates
- artifact/report helpers are split candidates only if downstream semantics are
  defended first
- Year1 monolithic behavior is preserve-only in Phase 2 work

## monolithic premise

Monolithic handling is fixed as follows:

- Year1 monolithic is fully implemented
- Year2 monolithic is out of current scope
- existing Year1 monolithic behavior is preserve-only
- Phase 2C plans verification to avoid regression on existing Year1
  monolithic behavior
- Phase 2C does not treat monolithic as Year2 completion work

This means the split-verification plan must distinguish two different rules:

- monolithic branches must not regress
- monolithic must not be reintroduced as a Year2 active development target

## verification categories

| category | target surface | current evidence source | pre-split acceptance evidence |
| --- | --- | --- | --- |
| compile / link surface | object split viability without symbol breakage | `Makefile`, direct includes, exported APIs in `coupled_run2d.h` | future split branch must pass `make -j` without adding non-native glue |
| public API unchanged | `coupled_run2d(...)`, lifecycle helpers, parse/to-string APIs, history storage APIs | `src/coupled/coupled_run2d.h`, `src/analysis/runner.c`, step-module call sites | declarations unchanged, call sites unchanged, no new public API leak |
| private helper visibility | helpers proposed for future `config`, `model_load`, `artifacts`, `snapshots` files | current static/non-static boundaries in `coupled_run2d.c` | moved helpers become private where possible without widening public header surface |
| build graph / object list | future `COUPLED_SRCS` additions | `Makefile` coupled source list | added objects are explicit, ordered, and link-clean |
| one-way route behavior | one-way coupled dispatch and output surfaces | `scripts/check_year2_oneway_mbd_fem_v1.sh` | route exits, summary wording, artifacts remain unchanged |
| lagged route behavior | delayed / lagged coupled dispatch and output surfaces | `scripts/check_year2_lagged_mbd_fem_v1.sh` | delayed semantics and summary surfaces remain unchanged |
| same-time route behavior | strong same-time route selection and file-exchange semantics | `scripts/check_year2_same_time_mbd_fem_v1.sh` | current goal same-time behavior remains unchanged |
| mixed bounded tooling unaffected | coupled changes must not disturb mixed-bounded report/check surfaces | `scripts/check_mixed_macro_local_replay_split_generic_v1.sh` | mixed-bounded canonical checker remains green |
| output / summary / artifact semantics | `coupled_run2d_write_output(...)` and related metadata mapping helpers | route checkers, compare/report tools, Phase 2B artifact cluster analysis | output files and semantic fields remain unchanged |
| legacy fallback wording | legacy no-flex fallback and default-scheme compatibility wording | current error surface in `coupled_run2d.c`, route docs/checkers | failure point, wording class, and exit behavior remain unchanged |
| Year1 monolithic no-regression | `COUPLED_SCHEME_MONOLITHIC_STRONG_V1` dispatch/report/runtime surfaces | `scripts/check_year1_coupled_route_matrix.sh` | monolithic route remains present and preserves current runtime/report surfaces |
| Year2 monolithic out-of-scope | protect against accidental scope revival | current docs and year2 check surfaces | split phase does not add monolithic to Year2 completion or acceptance gates |
| EHL separate-lane unaffected | coupled split must not drift into EHL mainline work | docs and separate-lane checker taxonomy | no EHL mainline integration work introduced |
| no Python / shell in solver core | native-only boundary | current repo architecture docs | split implementation remains C-only inside solver core |

## check matrix

The following matrix fixes the minimum intended verification surface for any
future actual split of `coupled_run2d.c`.

| category | canonical future check surface | role |
| --- | --- | --- |
| compile / link | `make -j` | catch symbol, include, and object-list regressions |
| one-way route | `bash scripts/check_year2_oneway_mbd_fem_v1.sh` | protect one-way route dispatch and summary semantics |
| lagged route | `bash scripts/check_year2_lagged_mbd_fem_v1.sh` | protect delayed / lagged route behavior |
| same-time route | `bash scripts/check_year2_same_time_mbd_fem_v1.sh` | protect current-goal same-time route behavior |
| mixed bounded unaffected | `bash scripts/check_mixed_macro_local_replay_split_generic_v1.sh` | catch cross-lane report/compare drift |
| same-time file exchange | `bash scripts/check_same_time_file_exchange_stub.sh` and `bash scripts/check_same_time_file_exchange_bundle.sh` | protect file-exchange route surfaces and compatibility wording |
| Year1 route matrix | `bash scripts/check_year1_coupled_route_matrix.sh` | current best monolithic preserve-only runtime gate |
| monolithic preserve-only reference | existing Year2 monolithic checks if needed as reference only, not current-goal promotion | optional secondary evidence when touching shared report helpers |

Default interpretation:

- the route matrix above is the minimum evidence set for any first actual split
- if a split touches only config/string helpers, artifact-heavy gates may still
  be required because those helpers influence scheme naming and summary wording
- if a split touches artifact/report helpers, the full matrix is required

## output / artifact contract gates

The following output surfaces are treated as contract gates before any actual
split:

- route class wording emitted through summary/report helpers
- step runner naming derived from scheme mapping helpers
- comparison role / solver route class / coupling metric strings
- delay semantics labels and strong-metric flags
- output writer file layout and metadata column ordering
- history and snapshot gating semantics that downstream tools inspect

Minimum rule:

- no helper may move across files unless the output/summary semantics it feeds
  are covered by an existing checker or explicitly recorded as a remaining gap

Practical implication:

- `coupled_run2d_write_output(...)` stays no-split-yet until downstream
  artifact semantics are guarded more directly than they are today

## no-split-yet verification gates

Phase 2B fixed the following no-split-yet zones. Phase 2C adds the required
gates that must be satisfied before any of them can move.

### scheme dispatch body gate

Applies to:

- `coupled_run2d_dispatch_step_by_scheme`

Required before any movement:

- one-way, lagged, same-time, and preserve-only monolithic routes all have
  current checker coverage in the split branch
- dispatch target function names and route-selection logic remain unchanged

### orchestration body gate

Applies to:

- main body of `coupled_run2d(...)`

Required before any movement:

- compile/link surface passes
- lifecycle cleanup ordering is unchanged
- stdout summary ordering and route-status wording are unchanged

### output writer / summary gate

Applies to:

- `coupled_run2d_write_output(...)`
- scheme-to-report metadata helpers

Required before any movement:

- artifact/report semantics are defended by route checkers and downstream
  compare/report tooling
- no semantic field used by scripts/tools changes name, meaning, or ordering

### legacy fallback wording gate

Applies to:

- `coupled_legacy_no_flex_fallback_error`
- legacy default scheme compatibility logic

Required before any movement:

- failure-point wording remains stable
- fallback behavior remains classified as compatibility behavior, not silently
  redefined

### route-class branch gate

Applies to:

- role / comparison-role / route-class / metric mapping helpers

Required before any movement:

- one-way / lagged / same-time mapping outputs are unchanged
- any monolithic branch remains preserve-only and regression-checked

### header/API exposure gate

Applies to:

- any region whose move would widen `coupled_run2d.h`

Required before any movement:

- no new public declarations unless absolutely required
- private-header introduction, if needed later, is planned separately

### Year1 monolithic preserve-only gate

Applies to:

- all `COUPLED_SCHEME_MONOLITHIC_STRONG_V1` branches

Required before any movement:

- `scripts/check_year1_coupled_route_matrix.sh` or an equivalent stronger gate
  is part of the split verification run
- monolithic remains out of Year2 completion scope
- no branch wording or route classification is changed in the name of cleanup

## pre-split go/no-go checklist

The first actual `coupled_run2d.c` split must not start until all of the
following are true:

- Phase 2A and Phase 2B planning docs still match the current workspace
- the intended move set is still limited to safe-first helpers
- no-split-yet zones are unchanged or backed by new evidence
- `Makefile` touch points for new objects are identified
- public header/API changes are not required for the first move
- route checker matrix is still present and runnable
- Year1 monolithic preserve-only gate is available
- any remaining gap is explicitly accepted as a prerequisite blocker, not
  silently ignored
- split implementation plan keeps solver core native-only

Go decision:

- only proceed if all items above are satisfied

No-go decision:

- if output semantics, monolithic preserve-only coverage, or public-header
  boundaries are unclear, stop before any C edit

## known gaps

Current gaps observed during Phase 2C planning are:

- no dedicated single-purpose Year1 monolithic-only checker was identified;
  the current best preserve-only gate is the broader
  `scripts/check_year1_coupled_route_matrix.sh`
- no isolated exact-equality contract check is dedicated solely to
  `coupled_run2d_write_output(...)`; current protection is indirect through
  route and downstream tooling surfaces
- no compile-only split rehearsal exists yet for hypothetical future object
  additions beyond normal `make -j`
- same-time and mixed-bounded checkers give strong indirect protection, but
  they are not framed as dedicated artifact-writer contract tests

These are not Phase 2C blockers for planning, but they are real pre-split
risks that must be accounted for before the first actual C edit.

## risks

Main risks are:

- output/report semantics drift from apparently harmless helper moves
- dispatch regression from moving helpers across route-selection boundaries
- public-header churn caused by moving lifecycle/history APIs too early
- legacy fallback wording drift that breaks compatibility-oriented checks
- accidental reintroduction of monolithic as Year2 active scope
- overestimating current checker coverage for output-writer semantics

## closeout criteria

Phase 2C is closeout-ready when all of the following are true:

- verification categories are fixed
- the minimum future check matrix is fixed
- no-split-yet verification gates are fixed
- the monolithic premise is written as Year1-complete, Year2 out-of-scope,
  preserve-only
- pre-split go/no-go criteria are fixed
- known gaps are recorded without attempting implementation in this phase

