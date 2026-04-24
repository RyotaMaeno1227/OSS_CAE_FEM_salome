# manifest-bridge bridge/result checkers

`tools/checkers/bridges/manifest_bridge/` holds stricter bridge-oriented
checker entrypoints around the relocated manifest-bridge builder/helper layer.

Current policy:

- these bridge/result checkers are support tooling
- checker relocation does not change checker logic
- tolerance logic remains unchanged
- generated deck semantics remain unchanged
- runtime completion semantics remain unchanged
- result summary semantics remain unchanged
- solver core remains native implementation
- Python / shell are not solver core
- old `scripts/check_mbd_2link_*.sh` and
  `scripts/test_parser_multipart_manifest_to_mbd.sh` paths remain
  compatibility wrappers during Phase 1
- `scripts/analyze_mbd_2link_history.py` remains an old `scripts/`
  dependency and is not moved in this phase

Current entrypoints:

- `check_mbd_2link_ground_revolute_enforcement.sh`
- `check_mbd_2link_rigid_from_bulk_ground0.sh`
- `check_mbd_2link_rigid_result_surface.sh`
- `test_parser_multipart_manifest_to_mbd.sh`
