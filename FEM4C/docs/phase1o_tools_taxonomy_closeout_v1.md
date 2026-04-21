# Phase 1O Tools Taxonomy Closeout v1

## purpose

This document closes out the Phase 1 tooling taxonomy work after Phase 1A
through 1N-c relocated the main release-lane support-tooling families into
`tools/` and verified the corresponding compatibility wrappers.

Phase 1O is docs only.

- no file moves
- no wrapper creation
- no script logic changes
- no solver behavior changes

The goal is to make the current source of truth explicit:

- moved release-lane support-tooling implementations live under `tools/`
- old `scripts/` entrypoints remain compatibility wrappers where applicable
- older phase taxonomy docs remain useful as planning history, but not as the
  final closeout summary

## scope and non-goals

This closeout covers the support-tooling families intentionally relocated in
Phase 1A through 1N-c for the current release lane.

It does not claim that every remaining file under `scripts/` has been moved.

Out of scope for this closeout:

- native solver-core refactoring
- parser refactoring
- build-system changes
- include-path changes
- EHL mainline integration
- monolithic route revival
- archival / research / internal-only script cleanup

## current-truth principles

- current release-lane support tooling lives under `tools/`
- old `scripts/` paths remain compatibility wrappers where applicable
- solver core remains native implementation under `src/`
- Python / shell are support tooling, not solver core
- Python / shell are not embedded into solver core
- subtree `README.md` files under `tools/` describe the live implementation
  families
- this document is the closeout summary for the Phase 1 tools taxonomy
- older phase taxonomy docs remain historical planning notes
- EHL separate lane remains separate
- monolithic remains out of scope

## relocated families summary

The current-truth moved implementation families are:

| family | current-truth `tools/` home | phase closeout note |
| --- | --- | --- |
| Year2 route runners / checkers | `tools/runners/year2/`, `tools/checkers/year2/` | old `scripts/year2_*` and `scripts/check_year2_*` entrypoints remain compatibility wrappers |
| canonical mixed / bounded route tooling | `tools/runners/mixed_bounded/`, `tools/checkers/mixed_bounded/`, `tools/exporters/mixed_bounded/` | canonical runner/checker/exporter implementations live under `tools/` |
| EHL separate-lane package tooling | `tools/builders/ehl/`, `tools/runners/ehl/` | remains a separate lane; old `scripts/` entrypoints remain wrappers |
| route checker families | `tools/checkers/routes/` | contact-circle, gear-pin proxy, involute-gear, mixed macro-local, same-time file-exchange, same-time local-patch |
| generic / smoke / bridge checker families | `tools/checkers/generic/local_patch/`, `tools/checkers/smoke/contact_patch/`, `tools/checkers/smoke/manifest_bridge/`, `tools/checkers/bridges/macro_to_patch/`, `tools/checkers/bridges/manifest_bridge/` | includes Phase 1M-a / 1M-b manifest-bridge checker moves |
| report / review tooling | `tools/reports/local_patch/`, `tools/reports/contact_review/`, `tools/reports/manifest_bridge/`, `tools/reports/fem_macro_local/` | report/analyze live implementations now live under `tools/reports/` |
| compare tooling | `tools/compare/fem_macro_local/`, `tools/compare/mbd_macro_local/` | old `scripts/compare_*.py` paths remain wrappers |
| golden / structural exporters | `tools/exporters/golden_examples/`, `tools/exporters/reduced_feedback/`, `tools/exporters/year2/` | local-patch, FEM macro-local, mixed macro-local, same-time, reduced-feedback, and Year2 exporter families are under `tools/exporters/` |
| same-time validators / helpers / runners | `tools/validators/same_time/`, `tools/helpers/same_time/`, `tools/runners/same_time/` | old request/response validator and helper paths remain compatibility surfaces |
| manifest-bridge builders / helpers / repo ops | `tools/builders/manifests/`, `tools/helpers/manifest_bridge/`, `tools/helpers/repo_ops/` | builder/helper/package live implementations are under `tools/` |
| EHL separate-lane bridge / stub tooling | `tools/bridges/ehl/`, `tools/stubs/ehl/` | still separate lane support tooling, not mainline solver-core work |

Implication:

- for the relocated Phase 1 families, `tools/` is the live implementation tree
- `scripts/` is no longer the source of truth for those moved families

## compatibility wrapper inventory summary

Current wrapper-only release-lane surfaces are grouped as follows.

| wrapper family | representative old `scripts/` surfaces kept stable | live implementation home |
| --- | --- | --- |
| Year2 route wrappers | `scripts/year2_oneway_mbd_fem_v1.py`, `scripts/year2_lagged_mbd_fem_v1.py`, `scripts/year2_same_time_mbd_fem_v1.py`, `scripts/check_year2_*` | `tools/runners/year2/`, `tools/checkers/year2/` |
| mixed / bounded canonical wrappers | `scripts/run_mixed_macro_local_oneway_generic_v1.sh`, `scripts/run_mixed_macro_local_replay_split_generic_v1.sh`, `scripts/check_mixed_macro_local_*_generic_v1.sh`, `scripts/export_mixed_local_feedback_reduced_generic.py` | `tools/runners/mixed_bounded/`, `tools/checkers/mixed_bounded/`, `tools/exporters/mixed_bounded/` |
| EHL package wrappers | `scripts/build_ehl_solver_package1_setup_v1.py`, `scripts/run_ehl_solver_package2_reynolds_v1.py`, `scripts/run_ehl_solver_package3_coupled_v1.py`, `scripts/run_year2_oneway_ehl_package4_batch_v1.py` | `tools/builders/ehl/`, `tools/runners/ehl/` |
| route checker wrappers | `scripts/check_contact_circle_*`, `scripts/check_gear_pin_proxy_*`, `scripts/check_involute_gear_local_patch_oneway.sh`, `scripts/check_same_time_file_exchange_*`, `scripts/check_same_time_local_patch_mvp_circle.sh`, route-level `scripts/check_mixed_macro_local_*` subset | `tools/checkers/routes/` |
| generic / smoke / bridge checker wrappers | `scripts/check_local_patch_generic_*`, `scripts/check_contact_patch_*`, `scripts/check_macro_to_patch_bridge.sh`, `scripts/test_build_mbd_master_from_manifest.sh`, `scripts/check_mbd_2link_from_bulk_smoke.sh`, `scripts/check_mbd_2link_ground_revolute_enforcement.sh`, `scripts/check_mbd_2link_rigid_from_bulk_ground0.sh`, `scripts/check_mbd_2link_rigid_result_surface.sh`, `scripts/test_parser_multipart_manifest_to_mbd.sh` | `tools/checkers/generic/`, `tools/checkers/smoke/`, `tools/checkers/bridges/` |
| report / review wrappers | `scripts/plot_local_patch_generic_solver_v1.py`, `scripts/fem4c_contact_review_bundle.sh`, `scripts/analyze_mbd_2link_history.py`, `scripts/plot_fem_macro_local_iter_generic_v1.py` | `tools/reports/` |
| compare wrappers | `scripts/compare_fem_macro_local_iterations.py`, `scripts/compare_mbd_macro_local_same_time_iterations.py` | `tools/compare/` |
| exporter wrappers | `scripts/export_local_patch_generic_golden_example.sh`, `scripts/export_fem_macro_local_iter_generic_golden_example.sh`, `scripts/export_mixed_macro_local_*`, `scripts/export_same_time_golden_example.sh`, `scripts/export_*local_feedback_reduced_from_local_oneway_generic.py`, `scripts/export_year2_oneway_fem_material_properties_v1.py` | `tools/exporters/` |
| same-time validator / helper / runner wrappers | `scripts/validate_same_time_request_csv.py`, `scripts/validate_same_time_response_csv.py`, `scripts/build_same_time_stub_responses.py`, `scripts/build_same_time_stub_local_responses.py`, `scripts/build_same_time_local_patch_mvp_from_request.py`, `scripts/stub_local_solver_from_request.py`, `scripts/stub_local_solver_from_request_only.py`, `scripts/run_external_stub_local_once.sh`, `scripts/run_same_time_file_exchange.sh` | `tools/validators/same_time/`, `tools/helpers/same_time/`, `tools/runners/same_time/` |
| manifest-bridge helper wrappers | `scripts/build_mbd_master_from_manifest.py`, `scripts/run_mbd_2link_rigid_from_bulk.sh`, `scripts/pack_clean_repo.sh` | `tools/builders/manifests/`, `tools/helpers/manifest_bridge/`, `tools/helpers/repo_ops/` |
| EHL separate-lane bridge / stub wrappers | `scripts/build_same_time_stub_ehl_responses.py`, `scripts/run_external_stub_ehl_once.sh`, `scripts/stub_ehl_solver_from_request.py`, `scripts/stub_ehl_solver_from_request_only.py`, `scripts/run_year2_ehl_to_mainline_mu_eff_bridge_v1.py`, `scripts/run_year2_ehl_to_mainline_mu_eff_consumer_v1.py` | `tools/stubs/ehl/`, `tools/bridges/ehl/` |

Current-truth interpretation:

- for the families above, the old `scripts/` paths are compatibility surfaces
- command compatibility is intentionally preserved
- implementation changes should land in `tools/`, not in the wrapper bodies,
  unless the wrapper contract itself is broken

## wrapper integrity phases summary

Wrapper integrity evidence in Phase 1 was established in two ways:

- each relocation phase required direct old/new smoke or equivalent downstream
  compatibility checks before acceptance
- dedicated wrapper-integrity sweeps later verified grouped families after the
  moves landed

Dedicated grouped sweeps completed in the current workspace:

| phase | verified family |
| --- | --- |
| Phase 1M-c | manifest-bridge smoke/check and bridge/result checker wrapper integrity |
| Phase 1N-c | report/analyze, compare, and contact-review wrapper integrity |

Closeout reading rule:

- if a family appears in the relocated families summary above and survived its
  move-phase smoke plus any dedicated integrity sweep, treat the `tools/`
  target as the source of truth and the old `scripts/` path as a wrapper-only
  surface

## remaining out-of-scope lanes

This closeout does not reclassify every remaining real implementation under
`scripts/`.

Still out of scope or intentionally separate:

- native solver-core source under `src/`
- parser source under `parser/`
- EHL separate lane as a lane distinct from current mainline release scope
- monolithic Year2 lane
- Year1 / archival / research / acceptance-only script clusters
- PM / handoff / branch-management docs

Important distinction:

- some `scripts/` files are still real implementations
- that fact does not invalidate the current-truth rule for the families that
  were intentionally moved under Phase 1A through 1N-c
- this closeout only states that the moved release-lane support-tooling
  families now live under `tools/`

## stale/legacy taxonomy notes guidance

Older phase taxonomy docs remain useful as planning history and rationale, but
they should be read as historical notes once this closeout exists.

Use older phase docs for:

- why a family was grouped a certain way
- what was intentionally left out of a move
- move sequencing rationale

Do not use older phase docs as the final current-truth inventory when they
conflict with the current workspace.

In particular:

- `docs/phase1d_remaining_scripts_inventory_v1.md` remains a historical
  inventory / planning memo
- `docs/phase1h_report_compare_tools_taxonomy_v1.md`,
  `docs/phase1m_smoke_check_companions_taxonomy_v1.md`, and
  `docs/phase1n_report_analyze_utilities_taxonomy_v1.md` remain phase-level
  planning / move records
- this document becomes the closeout summary for the moved Phase 1
  support-tooling families

## recommended source-of-truth docs

For current release-lane tooling layout, use these docs first:

1. `docs/phase1o_tools_taxonomy_closeout_v1.md`
2. `tools/README.md`
3. subtree `README.md` files under the relevant `tools/` family
4. `docs/internal_tooling_index_v1.md`
5. `docs/release_architecture_reorganization_plan_v1.md`

Use older phase taxonomy docs as historical planning notes after consulting the
closeout summary above.

## risks

- the flat `scripts/` tree still contains both wrapper-only surfaces and
  unrelated real implementations, so reading `scripts/` without this closeout
  can produce false inventory conclusions
- old wrapper paths can be mistaken for the implementation source of truth if
  contributors edit wrappers first
- EHL separate-lane tooling exists under `tools/`, but that does not imply EHL
  mainline integration
- monolithic and archival lanes still exist in the repo and should not be
  reintroduced as current-goal scope through taxonomy drift

## closeout status

Phase 1A through 1N-c closes out the moved main release-lane support-tooling
families as follows:

- live implementations for those moved families are under `tools/`
- old `scripts/` paths remain compatibility wrappers where applicable
- solver core remains native implementation under `src/`
- Python / shell remain support tooling, not solver core
- EHL separate lane remains separate
- monolithic remains out of scope

Current-truth closeout judgment:

- for the moved Phase 1 families, this closeout is ready to serve as the
  canonical summary
- remaining real implementation work outside those families belongs to future
  out-of-scope or post-closeout planning, not to reopening the moved wrapper
  inventory
