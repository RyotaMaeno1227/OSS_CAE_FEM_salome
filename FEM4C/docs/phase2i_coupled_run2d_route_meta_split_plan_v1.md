# Phase 2I Coupled Run2D Route Meta Split Plan v1

## purpose

Phase 2H で `safe-next-planning` と再固定した route-metric /
compare-schema helper cluster だけを対象に、next safe C split plan を
docs-only で固定する。

Phase 2I では actual C split はまだ行わない。目的は、future split を
`coupled_run2d_route_meta.c` proposal に限定したうえで、move-first
helper、keep-in-place helper、future approval gate、minimum verification
matrix を事前に固定することにある。

## scope and non-goals

This phase covers only:

- `src/coupled/coupled_run2d.c`
- route-metric / compare-schema helper cluster
  - `coupled_run2d_coupling_metric_from_scheme`
  - `coupled_run2d_delay_semantics_status_from_scheme`
  - `coupled_run2d_scheme_uses_strong_metrics`
  - `coupled_run2d_scheme_uses_year1_compare_schema`
  - `coupled_run2d_compare_step_*`
- related but not first-target helpers
  - `coupled_run2d_delay_buffer_scope_from_run`
  - `coupled_run2d_step_flex_iteration_column_name`
  - exported CSV header getters

This phase does not:

- edit C source or headers
- move functions
- edit scripts/checkers
- change the build graph
- move `coupled_run2d_write_output(...)`
- move summary/report wording helpers
- widen public API

## prerequisites

Current workspace prerequisites confirmed for this plan:

- `docs/phase2a_c_source_split_planning_v1.md`
- `docs/phase2b_coupled_run2d_split_dry_run_plan_v1.md`
- `docs/phase2c_c_split_verification_plan_v1.md`
- `docs/phase2e_coupled_run2d_output_contract_audit_v1.md`
- `docs/phase2f_coupled_run2d_guard_hardening_plan_v1.md`
- `docs/phase2h_post_guard_audit_before_next_split_v1.md`
- `src/coupled/coupled_run2d_config.c` exists
- `Makefile` includes `src/coupled/coupled_run2d_config.c`
- `scripts/check_year1_coupled_route_matrix.sh` contains the Phase 2G exact
  route/output guards

Carry-forward conclusions:

- Phase 2H kept writer / wording / fallback surfaces as `no-split-yet`
- Phase 2H promoted the route-metric / compare-schema helper cluster to
  `safe-next-planning`
- future C edits still require explicit user approval

## monolithic premise

The monolithic premise remains fixed exactly as follows:

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Implications for Phase 2I:

- move-first helpers must not emit or redefine monolithic preserve-only wording
- move-first helpers must not create pressure to widen Year2 scope
- any helper tied to visible monolithic distinction wording remains out of scope

## target helper cluster

The target cluster is intentionally narrow.

Move-first planning target:

- `coupled_run2d_coupling_metric_from_scheme`
- `coupled_run2d_delay_semantics_status_from_scheme`
- `coupled_run2d_scheme_uses_strong_metrics`
- `coupled_run2d_scheme_uses_year1_compare_schema`
- `coupled_run2d_compare_step_iteration_count`
- `coupled_run2d_compare_step_coupling_converged`
- `coupled_run2d_compare_step_exchange_lag_steps`
- `coupled_run2d_compare_step_sample_hold_active`
- `coupled_run2d_compare_step_delayed_snapshot_step`

Related but not first-target:

- `coupled_run2d_delay_buffer_scope_from_run`
- `coupled_run2d_step_flex_iteration_column_name`
- `coupled_run2d_artifact_metadata_columns_csv`
- `coupled_run2d_interface_centers_header_csv`
- `coupled_run2d_reaction_map_header_csv`
- `coupled_run2d_observation_points_header_csv`

## function dependency review

| helper | current signature | visibility | called by region | calls helper(s) | touches output/report wording directly | touches monolithic preserve-only wording directly | generates route/output semantics directly | current checker coverage |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `coupled_run2d_coupling_metric_from_scheme` | `static const char *...(coupled_scheme_t scheme)` | `static` | writer header emission | none | no freeform wording, only route-metric token | no | yes, emits `coupling_metric` value | exact strong-route output coverage via Phase 2G |
| `coupled_run2d_delay_semantics_status_from_scheme` | `static const char *...(coupled_scheme_t scheme)` | `static` | runtime summary + writer header emission | none | no freeform wording, only route-status token | no | yes, emits `delay_semantics_status` value | exact oneway/delayed runtime/output coverage |
| `coupled_run2d_scheme_uses_strong_metrics` | `static int ...(coupled_scheme_t scheme)` | `static` | writer branch selection + main loop warning branch | none | no | no | indirectly selects strong metric fields and convergence warnings | exact strong-route field coverage |
| `coupled_run2d_scheme_uses_year1_compare_schema` | `static int ...(coupled_scheme_t scheme)` | `static` | writer compare-schema branch selection | none | no | no | indirectly selects year1 compare-schema rows/headers | exact year1 compare-schema coverage |
| `coupled_run2d_compare_step_iteration_count` | `static int ...(const coupled_step_history2d_t *history)` | `static` | writer `compare_step` row emission | none | no | no | yes, compare-step numeric field | exact `compare_step_columns` contract coverage |
| `coupled_run2d_compare_step_coupling_converged` | `static int ...(coupled_scheme_t scheme, const coupled_step_history2d_t *history)` | `static` | writer `compare_step` row emission | none | no | no | yes, compare-step numeric field | exact `compare_step_columns` contract coverage |
| `coupled_run2d_compare_step_exchange_lag_steps` | `static int ...(coupled_scheme_t scheme, const coupled_step_history2d_t *history)` | `static` | writer `compare_step` row emission | none | no | no | yes, delayed compare-step numeric field | exact delayed compare-schema coverage |
| `coupled_run2d_compare_step_sample_hold_active` | `static int ...(coupled_scheme_t scheme, const coupled_step_history2d_t *history)` | `static` | writer `compare_step` row emission | none | no | no | yes, delayed compare-step numeric field | exact delayed compare-schema coverage |
| `coupled_run2d_compare_step_delayed_snapshot_step` | `static int ...(coupled_scheme_t scheme, const coupled_step_history2d_t *history)` | `static` | writer `compare_step` row emission | none | no | no | yes, delayed compare-step numeric field | exact delayed compare-schema coverage |

Dependency interpretation:

- all first-target helpers are file-local today
- all first-target helpers are called only from `coupled_run2d.c`
- none require public API exposure for a future split
- none own the writer body itself
- two helpers (`delay_semantics_status_from_scheme`, `scheme_uses_strong_metrics`)
  are used outside the writer, so a future split needs a private internal
  declaration surface rather than public header expansion

## proposed future file map

The future file map is still proposal only.

```text
src/coupled/
  coupled_run2d.c                 # exported run API + orchestration / dispatch / writer remain
  coupled_run2d_config.c          # already split config/string/env helpers
  coupled_run2d_route_meta.c      # candidate for route-metric / compare-schema helpers
  coupled_run2d_route_meta_internal.h   # optional private internal declarations only, not public API
```

Interpretation:

- `coupled_run2d.c` keeps exported entrypoints, main orchestration, dispatch,
  runtime summary block, writer body, fallback body
- `coupled_run2d_route_meta.c` would receive only the selected safe-next helper
  set
- if declarations are needed, prefer a private internal header over expanding
  `src/coupled/coupled_run2d.h`

## keep-vs-move table

| current helper | current responsibility | proposed future home | move readiness | guard coverage | reason | risk | verification needed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `coupled_run2d_coupling_metric_from_scheme` | strong/monolithic metric token mapping | `coupled_run2d_route_meta.c` | safe-next-actual-split-candidate | exact strong-route output coverage | pure mapping helper with no public API need | accidental writer coupling if call-site logic grows | year1 route matrix + one-way canonical matrix |
| `coupled_run2d_delay_semantics_status_from_scheme` | route delay-status token mapping | `coupled_run2d_route_meta.c` | safe-next-actual-split-candidate | exact oneway/delayed runtime+output coverage | pure mapping helper reused by summary and writer | same-time semantic guard still indirect | year1 route matrix + year2 lagged/same-time matrix |
| `coupled_run2d_scheme_uses_strong_metrics` | branch predicate for strong metric routes | `coupled_run2d_route_meta.c` | safe-next-actual-split-candidate | exact strong-route field coverage | pure predicate with narrow dependencies | accidental widening into orchestration logic | year1 route matrix + same-time matrix |
| `coupled_run2d_scheme_uses_year1_compare_schema` | branch predicate for year1 compare-schema emission | `coupled_run2d_route_meta.c` | safe-next-actual-split-candidate | exact compare-schema coverage | pure predicate and central to compare-schema cluster | compare-schema drift if moved with unrelated logic | year1 route matrix + mixed-bounded replay checker |
| `coupled_run2d_compare_step_*` | compare-step row field derivation | `coupled_run2d_route_meta.c` | safe-next-actual-split-candidate | exact `compare_step_columns` coverage | self-contained numeric derivation helpers | easy to over-scope into writer row emission | year1 route matrix + compare-schema consumer checks |
| `coupled_run2d_delay_buffer_scope_from_run` | delayed snapshot scope label | keep in `coupled_run2d.c` for now | later | partial delayed output coverage | still route-topology sensitive | can drift with body/interface count semantics | add dedicated delayed output checker first |
| `coupled_run2d_step_flex_iteration_column_name` | step-flex counter column label | keep in `coupled_run2d.c` for now | later | indirect only | no exact field guard yet | header/column drift | add exact `step_flex_counter_columns` checker |
| exported CSV header getters | artifact header constants | keep in `coupled_run2d.c` for now | later | indirect only | no direct exact header checker | silent schema drift | add dedicated header-getter checker |
| `coupled_run2d_write_output(...)` | main artifact writer | remain in `coupled_run2d.c` | no-split-yet | partial exact + semantic | writer body still too contract-dense | multi-route artifact drift | dedicated writer audit gate first |
| `coupled_run2d_comparison_role_from_scheme` | visible comparison-role wording | remain in `coupled_run2d.c` | no-split-yet | exact visible token coverage but wording-sensitive | visible summary/output wording | preserve-only wording drift | dedicated wording audit first |
| `coupled_run2d_solver_route_class_from_run` | visible route-class wording | remain in `coupled_run2d.c` | no-split-yet | some exact runtime/output coverage | depends on route topology/body count | route contract drift | stronger cross-route exact guard first |
| `coupled_run2d_step_coupling_reason` | monolithic step-status wording | remain in `coupled_run2d.c` | no-split-yet | indirect via `step_status_columns` | user-visible wording helper | monolithic wording drift | exact reason-string checker first |
| legacy fallback body | preserve-only no-flex stub wording/error path | remain in `coupled_run2d.c` | no-split-yet | indirect only | preserve-only stub path | fallback wording drift | dedicated fallback wording checker first |
| visible monolithic preserve-only distinction wording | runtime preserve-only distinction lines | remain in `coupled_run2d.c` | no-split-yet | exact preserve-only token coverage | coupled to summary block | accidental Year2 scope confusion | keep outside move-first set |

## no-split-yet confirmation

Phase 2I keeps the following surfaces explicitly out of the next actual split.

- writer body
- summary/report wording helpers
- `coupled_run2d_summary_title_from_scheme`
- `coupled_run2d_step_runner_name_from_scheme`
- `coupled_run2d_path_class_from_scheme`
- `coupled_run2d_role_from_scheme`
- `coupled_run2d_comparison_role_from_scheme`
- `coupled_run2d_solver_route_class_from_run`
- `coupled_run2d_step_coupling_reason`
- legacy fallback body
- visible monolithic preserve-only wording
- scheme dispatch body
- main orchestration body

## future actual split patch plan

If a later phase proceeds to actual C edits, the patch plan should be constrained
as follows.

1. explicit user approval is required before any C edits begin
2. create `src/coupled/coupled_run2d_route_meta.c`
3. move only the selected safe-next helper set
4. keep `coupled_run2d_write_output(...)`, summary wording, fallback, dispatch,
   orchestration in `coupled_run2d.c`
5. avoid public API expansion unless private declarations prove impossible
6. prefer an internal private header, e.g.
   `src/coupled/coupled_run2d_route_meta_internal.h`
7. update `Makefile` minimally by adding the new source to `COUPLED_SRCS`
8. run the minimum checker matrix before accepting the patch
9. keep monolithic preserve-only no-regression as an explicit acceptance gate

## minimum verification matrix

Any future actual split of this cluster should use at least the following matrix.

- `make -j`
- `bash scripts/check_coupled_integrators.sh`
- `bash scripts/check_year1_coupled_route_matrix.sh`
- `bash scripts/check_year2_oneway_mbd_fem_v1.sh`
- `bash scripts/check_year2_lagged_mbd_fem_v1.sh`
- `bash scripts/check_year2_same_time_mbd_fem_v1.sh`
- `bash scripts/check_same_time_file_exchange_stub.sh`
- `bash scripts/check_same_time_file_exchange_bundle.sh`
- `bash scripts/check_mixed_macro_local_replay_split_generic_v1.sh`

Optional / conditional additions:

- any additional exact route-output checker introduced before Phase 2J
- any later Year1 monolithic-only checker if one is added
- any artifact-writer audit checker if writer-adjacent helpers are touched later

## risks

- moving a helper that looks pure but still controls writer branch shape can
  accidentally widen the actual edit scope
- using a public header instead of a private internal declaration surface would
  widen API scope unnecessarily
- moving compare-step helpers together with writer row emission would turn a
  narrow split into a writer split
- underestimating delayed/same-time semantic coupling around
  `delay_semantics_status_from_scheme` could weaken route guarantees
- any leak of monolithic preserve-only wording into the move-first set risks
  confusing Year1 preserve-only with Year2 active scope

## closeout criteria

Phase 2I is closeout-ready when all of the following are true.

- target helper cluster is fixed
- current dependency and visibility constraints are recorded
- future file map is fixed as proposal only
- move-first vs keep-in-place boundary is explicit
- no-split-yet surfaces remain explicit
- future actual split requires explicit user approval
- minimum verification matrix is fixed
