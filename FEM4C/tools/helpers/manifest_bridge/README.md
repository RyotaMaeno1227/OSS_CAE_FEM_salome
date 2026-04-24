# tools/helpers/manifest_bridge

`tools/helpers/manifest_bridge/` holds manifest-bridge helper entrypoints.

Current entries:

- `run_mbd_2link_rigid_from_bulk.sh`

Notes:

- manifest bridge helpers are support tooling
- old `scripts/run_mbd_2link_rigid_from_bulk.sh` remains a compatibility
  wrapper
- helper logic is unchanged
- generated deck / runtime summary / result summary semantics are unchanged
- downstream checker callers intentionally keep the old `scripts/`
  compatibility surface where needed
