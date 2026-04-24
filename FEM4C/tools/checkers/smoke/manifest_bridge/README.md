# manifest-bridge smoke checkers

`tools/checkers/smoke/manifest_bridge/` holds smoke-oriented checker
entrypoints around the relocated manifest-bridge builder/helper layer.

Current policy:

- these checkers are support tooling
- checker relocation does not change checker logic
- generated deck semantics remain unchanged
- runtime completion semantics remain unchanged
- solver core remains native implementation
- Python / shell are not solver core
- old `scripts/test_build_mbd_master_from_manifest.sh` and
  `scripts/check_mbd_2link_from_bulk_smoke.sh` paths remain compatibility
  wrappers during Phase 1

Current entrypoints:

- `test_build_mbd_master_from_manifest.sh`
- `check_mbd_2link_from_bulk_smoke.sh`
