# tools/checkers/generic/local_patch

`tools/checkers/generic/local_patch/` holds the generic local-patch contract
checker subset in the support-tooling layer.

Current policy:

- local-patch generic checkers are support tooling
- checker logic is unchanged by relocation
- solver core remains native implementation
- Python / shell are not solver core
- validators / runners / plotters / exporters are not moved in this phase
- legacy `scripts/check_local_patch_generic_*.sh` paths remain compatibility wrappers
