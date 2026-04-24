# macro-to-patch bridge checker

`tools/checkers/bridges/macro_to_patch/` holds the macro-to-patch bridge
checker entrypoint.

Current policy:

- macro-to-patch bridge checker is support tooling
- checker relocation does not change checker logic
- bridge relocation does not change bridge logic
- solver core remains native implementation
- Python / shell are not solver core
- this phase does not move local-patch generic checkers
- this phase does not move contact-patch smoke checkers
- this phase does not move bridge runners
- legacy `scripts/check_macro_to_patch_bridge.sh` remains a compatibility
  wrapper during Phase 1
