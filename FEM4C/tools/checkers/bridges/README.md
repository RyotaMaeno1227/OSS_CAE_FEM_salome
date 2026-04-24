# bridges checkers

`tools/checkers/bridges/` holds checker entrypoints for bridge-oriented support
tooling.

Current policy:

- bridge checkers are support tooling
- checker relocation does not change checker logic
- bridge relocation does not change bridge logic
- solver core remains native implementation
- Python / shell are not solver core
- legacy `scripts/check_*.sh` paths and selected `scripts/test_*.sh` paths
  remain compatibility wrappers during Phase 1

Current bridge subsets include:

- `tools/checkers/bridges/macro_to_patch/`
- `tools/checkers/bridges/manifest_bridge/`
