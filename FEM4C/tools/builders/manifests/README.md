# tools/builders/manifests

`tools/builders/manifests/` holds manifest-driven builder entrypoints.

Current entries:

- `build_mbd_master_from_manifest.py`

Notes:

- manifest bridge builders are support tooling
- old `scripts/build_mbd_master_from_manifest.py` remains a compatibility
  wrapper
- builder logic is unchanged
- generated deck semantics are unchanged
- solver core remains native implementation
- Python is not embedded into solver core
