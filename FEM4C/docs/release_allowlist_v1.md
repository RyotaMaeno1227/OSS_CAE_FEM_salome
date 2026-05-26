# Release Allowlist v1

## 目的

これは Proposal A を packaging に落とすための allowlist である。

方針は固定する。

- final release artifact には `.sh` を含めない
- repo には internal tooling として `.sh` / research `.py` を残す
- CT_240c で undecided 5 本は解消した

この文書は [release_manifest_draft_v1.md](release_manifest_draft_v1.md)
および [release_support_decision_v1.md](release_support_decision_v1.md)
と整合する allowlist 正本である。

## included in final release

### solver core

- `src/**/*.c`
- `src/**/*.h`
- `src/fem4c.c`
- `parser/parser.c`

### build / executable surface

- `Makefile`
- `README.md`

### user-facing docs

- `docs/README.md`
- `docs/release_user_guide_v1.md`
- `docs/fem_macro_penalty_generic_contract_v1.md`
- `docs/mbd_macro_penalty_generic_contract_v1.md`
- `docs/local_patch_generic_contract_v1.md`
- `docs/fem_macro_penalty_generic_solver_v1.md`
- `docs/mbd_macro_penalty_generic_solver_v1.md`
- `docs/release_surface_policy_v1.md`
- `docs/release_manifest_draft_v1.md`
- `docs/release_allowlist_v1.md`

### minimal examples

- `examples/t3_cantilever_beam.dat`
- `examples/t6_cantilever_beam.dat`
- `examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat`
- `examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat`
- `examples/local_patch_generic_contract_v1/request_penetration.json`
- `examples/local_patch_generic_contract_v1/response_example.json`

current release-facing contact example は
`examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat`
とする。
その direct C-run に必要な surface CSV も final release に含める。

- `examples/mbd_macro_penalty_generic_contract_v1/surface_body0_active.csv`
- `examples/mbd_macro_penalty_generic_contract_v1/surface_body1_active.csv`

## excluded from final release

### shell tooling

- all `*.sh`
- all `scripts/check_*`
- all `scripts/export_*`
- all `scripts/run_*`
- all bundle / golden / fallback shell entrypoints

### internal research python

- plotting / compare / aggregation / feedback / mixed orchestration `.py`
- local proxy orchestration `.py`
- same-time / replay / fallback / bundle helpers

### handoff and continuity

- all `*handoff*.md`
- branch continuity memos
- next-chat refresh docs

### internal docs

- `docs/internal_tooling_index_v1.md`
- `docs/release_support_decision_v1.md`
- mixed branch hardening docs
- iterative hardening docs
- fallback / replay branch research notes

## internal-only tooling

### acceptance / regression

- `scripts/check_*.sh`

### orchestration / export / bundle

- `scripts/export_*.sh`
- `scripts/run_*.sh`
- `scripts/plot_*.py`
- `scripts/export_*_generic.py`
- `scripts/build_*_from_*_trace.py`
- `scripts/build_*_from_*_manifest.py`
- `scripts/compare_*`

### resolved release-support decisions

- `scripts/build_mbd_master_from_manifest.py`
- `scripts/validate_local_patch_generic_request.py`
- `scripts/validate_local_patch_generic_response.py`
- `scripts/run_local_patch_generic_contract_v1.py`
- `scripts/local_patch_generic_solver_v1.py`

current v1 policy では、上の 5 本はすべて internal-only であり、
final release artifact には含めない。

### reproducibility

- golden example exporters
- fallback scenario exporters
- mixed bundle scripts
- review bundle scripts

## packaging rules

dry-run exporter は少なくとも次を満たす。

- release tree は allowlist-based で作る。repo root の broad copy はしない。
- exported release tree に `.sh` が 0 件
- exported release tree に handoff `*.md` が入らない
- exported release tree に `check_*` が入らない
- exported release tree に internal `export_*` が入らない
- exported release tree に bundle / fallback / golden internal artifact generator が入らない
- `src/**/*.c`, `src/**/*.h`, `parser/parser.c`, `Makefile`, minimal docs/examples は入る
- internal-only に確定した release-support Python も exported tree から除外する

## current recommendation

Proposal A のまま進める。

- final release artifact: solver core + build/runtime support + minimal docs/examples
- repo retained internal tooling: `.sh`, research `.py`, handoff, bundle, fallback, golden export, parser/manifest bridge, proxy local validator/runner/solver

`.sh` を final release に含めない方針は current repo で possible であり、
current recommendation の移行コストは `low` である。

release support の二択では `A2` を採用する。

- `build_mbd_master_from_manifest.py` も internal-only
- generic local validator / runner / proxy solver 系も internal-only
