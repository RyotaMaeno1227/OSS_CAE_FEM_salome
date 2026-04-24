# tools/exporters/golden_examples/mixed_macro_local

This directory holds mixed macro-local golden example support tooling relocated
from `scripts/`.

- `export_mixed_macro_local_golden_example.sh`
- `export_mixed_macro_local_fallback_golden_example.sh`
- `export_mixed_macro_local_dual_engine_matrix_golden_example.sh`

Current policy:

- mixed macro-local golden exporters are support tooling
- export logic is unchanged by relocation
- output contract is unchanged by relocation
- solver core remains native implementation
- Python / shell are not solver core
- dependency checker calls intentionally remain on the old `scripts/`
  compatibility surface
- canonical mixed bounded first-class feedback remains `gamma_n`
- `k_contact_eff` / `mu_eff` are not introduced as mixed bounded route-level
  returns
- old `scripts/export_mixed_macro_local_*.sh` paths remain compatibility
  wrappers
