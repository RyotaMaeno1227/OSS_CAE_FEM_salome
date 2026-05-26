# Release Manifest Draft v1

## 目的

これは final release boundary の draft manifest である。
実装ではなく packaging policy の下書きであり、
current repo から何を release に含め、何を internal-only に残すかを列挙する。

main recommendation は `Proposal A` である。
すなわち、`.sh` と研究用 `.py` は final release artifact から除外し、
repo には internal tooling として残す。
release artifact は allowlist-based で作り、repo root の broad copy では作らない。

CT_240c では CT_240b の undecided 5 本を解消し、
current v1 draft は candidate-level decision を持つ。
release support の二択では `A2` を採用し、parser/manifest bridge も
internal-only とする。

## included in final release

### solver core

- `src/**/*.c`
- `src/**/*.h`
- `src/fem4c.c`
- `parser/parser.c`

### build/runtime support

- `Makefile`
- top-level `README.md`
- user-facing build / usage docs
  - `docs/README.md`
  - `docs/release_user_guide_v1.md`
  - `docs/fem_macro_penalty_generic_contract_v1.md`
  - `docs/mbd_macro_penalty_generic_contract_v1.md`
  - `docs/local_patch_generic_contract_v1.md`
  - `docs/fem_macro_penalty_generic_solver_v1.md`
  - `docs/mbd_macro_penalty_generic_solver_v1.md`

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

### shell-based dev/test tooling

- `scripts/check_*.sh`
- `scripts/run_*golden*`
- `scripts/export_*golden*`
- `scripts/check_mixed_macro_local_bundle_v1.sh`
- `scripts/export_mixed_macro_local_golden_example.sh`
- `scripts/export_mixed_macro_local_fallback_golden_example.sh`
- `scripts/fem4c_contact_review_bundle.sh`

### research orchestration python

- `scripts/plot_fem_macro_local_iter_generic_v1.py`
- `scripts/export_mixed_local_feedback_reduced_generic.py`
- `scripts/build_local_patch_generic_requests_from_mixed_macro_manifest.py`
- `scripts/build_local_patch_generic_requests_from_fem_trace.py`
- `scripts/build_local_patch_generic_requests_from_mbd_trace.py`
- `scripts/compare_fem_macro_local_iterations.py`
- `scripts/compare_mbd_macro_local_same_time_iterations.py`

### handoff / continuity docs

- `fem4c_next_chat_handoff_2026-04-03_mixed_generic_contact.md`
- `fem4c_next_chat_handoff_2026-04-04_mixed_generic_contact.md`
- any future `*handoff*.md`

### research reproducibility outputs

- mixed fallback scenarios
- bundle-only summary artifacts
- golden example export trees under `/tmp` or generated packaging locations

## internal-only tooling

### acceptance / regression

- all `scripts/check_*.sh`
- `scripts/check_mixed_macro_local_generic_contract_v1.sh`
- `scripts/check_mixed_macro_local_oneway_generic_v1.sh`
- `scripts/check_mixed_macro_local_feedback_generic_v1.sh`
- `scripts/check_mixed_macro_local_replay_split_generic_v1.sh`
- `scripts/check_mixed_macro_local_feedback_aggregation_generic_v1.sh`
- `scripts/check_mixed_macro_local_feedback_fallback_generic_v1.sh`
- `scripts/check_mixed_macro_local_bundle_v1.sh`

### research bridge / exporter / builder

- `scripts/build_mbd_macro_local_same_time_generic_requests.py`
- `scripts/assemble_mbd_macro_local_same_time_generic_response.py`
- `scripts/export_fem_local_feedback_reduced_from_local_oneway_generic.py`
- `scripts/export_local_feedback_reduced_from_local_oneway_generic.py`
- `scripts/export_mixed_local_feedback_reduced_generic.py`
- `scripts/build_local_patch_generic_requests_from_mixed_macro_manifest.py`
- `scripts/build_mbd_master_from_manifest.py`
- `scripts/validate_local_patch_generic_request.py`
- `scripts/validate_local_patch_generic_response.py`
- `scripts/run_local_patch_generic_contract_v1.py`
- `scripts/local_patch_generic_solver_v1.py`

### golden example / bundle / visualization

- `scripts/plot_local_patch_generic_solver_v1.py`
- `scripts/plot_fem_macro_local_iter_generic_v1.py`
- `scripts/export_local_patch_generic_golden_example.sh`
- `scripts/export_fem_macro_local_iter_generic_golden_example.sh`
- `scripts/export_mixed_macro_local_golden_example.sh`
- `scripts/export_mixed_macro_local_fallback_golden_example.sh`

### branch-hardening docs

- `docs/fem_macro_local_iter_generic_v1.md`
- `docs/fem_macro_local_replay_pipeline_v1.md`
- `docs/mixed_macro_local_oneway_generic_v1.md`
- `docs/mixed_macro_local_feedback_generic_v1.md`
- `docs/mixed_macro_local_replay_split_generic_v1.md`
- `docs/mixed_macro_local_generic_contract_v1.md`
- `docs/internal_tooling_index_v1.md`
- `docs/release_support_decision_v1.md`

## resolved support decisions

CT_240c で次を確定する。

- `scripts/build_mbd_master_from_manifest.py`: internal-only
- `scripts/validate_local_patch_generic_request.py`: internal-only
- `scripts/validate_local_patch_generic_response.py`: internal-only
- `scripts/run_local_patch_generic_contract_v1.py`: internal-only
- `scripts/local_patch_generic_solver_v1.py`: internal-only

つまり、release support の二択では `A2` を採用する。

- A1: parser/manifest bridge を release support に残す
- A2: parser/manifest bridge も internal に落とす

current recommendation は `A2` である。

## still open

file-level undecided は current v1 では残さない。

still open は documentation packaging のみである。

- mixed/FEM/MBD branch-hardening docs を将来 `docs/internal/` 相当に分離するか
- release-facing docs と internal-only docs を将来物理分離するか

## rationale

この draft は次を優先する。

- end-user release artifact から `.sh` を外す
- solver core の正本を C に保つ
- repo には internal tooling を残し、研究再現性と review bundle を失わない
- parser/manifest bridge と local proxy validator/runner/solver も internal-only に揃えて release boundary を明快にする

## explicit non-claims

この draft は次を主張しない。

- mixed capability の増加
- solver physics の変更
- FEM / MBD / mixed branch の current MVP を超える完成
- `.py` 全廃の即時実行
