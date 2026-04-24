# tools/exporters/reduced_feedback

`tools/exporters/reduced_feedback/` holds structural reduced-feedback exporter
entrypoints.

- reduced feedback exporters are support tooling
- old `scripts/` paths remain compatibility wrappers
- solver core remains native implementation
- Python is not solver core
- export logic is unchanged
- aggregation semantics are unchanged
- output CSV contracts are unchanged
- these exporters do not introduce route-level `k_contact_eff` / `mu_eff`
- mixed reduced feedback exporter remains separate under
  `tools/exporters/mixed_bounded/`
