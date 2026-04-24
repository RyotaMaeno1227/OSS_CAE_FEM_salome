# tools/compare/fem_macro_local

This directory holds FEM macro-local iteration compare tooling moved from
`scripts/`.

- `compare_fem_macro_local_iterations.py`

Policy:

- compare logic and output schema are unchanged by relocation
- old `scripts/compare_fem_macro_local_iterations.py` remains a compatibility
  wrapper
- iteration checkers remain checker-owned and are not moved in this phase
