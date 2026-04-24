# tools/reports/fem_macro_local

This directory holds FEM macro-local plot/report support tooling moved from
`scripts/`.

- `plot_fem_macro_local_iter_generic_v1.py`

Policy:

- the FEM macro-local plot/report utility is support tooling
- plot/report logic is unchanged by relocation
- output PNG / CSV / JSON semantics are unchanged
- stdout PASS semantics are unchanged
- local-patch plotter dependency compatibility is preserved
- old `scripts/plot_fem_macro_local_iter_generic_v1.py` remains a compatibility
  wrapper
