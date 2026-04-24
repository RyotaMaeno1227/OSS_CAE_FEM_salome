# tools/exporters/golden_examples/fem_macro_local

This directory holds FEM macro-local golden example support tooling relocated from
`scripts/`.

- `export_fem_macro_local_iter_generic_golden_example.sh`

Current policy:

- FEM macro-local golden exporters are support tooling
- export logic is unchanged by relocation
- output contract is unchanged by relocation
- solver core remains native implementation
- Python / shell are not solver core
- runner / plotter dependencies intentionally remain on the old `scripts/`
  compatibility surface
- old `scripts/export_fem_macro_local_iter_generic_golden_example.sh` remains a
  compatibility wrapper
