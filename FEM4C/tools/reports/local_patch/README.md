# tools/reports/local_patch

This directory holds the local-patch visualization support tooling moved from
`scripts/`.

- `plot_local_patch_generic_solver_v1.py`

Policy:

- local-patch plot/report tooling is support tooling
- plot/report logic is unchanged by relocation
- old `scripts/plot_local_patch_generic_solver_v1.py` remains a compatibility
  wrapper
- solver core remains native implementation
