# tools/runners/mixed_bounded

This directory holds the canonical mixed / bounded runner tooling.

- `run_mixed_macro_local_oneway_generic_v1.sh`
- `run_mixed_macro_local_replay_split_generic_v1.sh`

These files are support tooling, not solver core.

- old `scripts/run_mixed_*` paths remain compatibility wrappers
- release command compatibility is preserved
- first-class feedback remains `gamma_n`
- `k_contact_eff` / `mu_eff` are not introduced as mixed bounded route-level returns
