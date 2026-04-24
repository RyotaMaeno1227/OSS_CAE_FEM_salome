# tools/checkers/mixed_bounded

This directory holds the canonical mixed / bounded checker tooling.

- `check_mixed_macro_local_oneway_generic_v1.sh`
- `check_mixed_macro_local_feedback_generic_v1.sh`
- `check_mixed_macro_local_replay_split_generic_v1.sh`

These checkers are support tooling, not solver core.

- old `scripts/check_mixed_*` paths remain compatibility wrappers
- release command compatibility is preserved
- first-class feedback remains `gamma_n`
- `k_contact_eff` / `mu_eff` are not introduced as mixed bounded route-level returns
