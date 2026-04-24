# tools/reports/manifest_bridge

This directory holds manifest-bridge analysis/report support tooling moved from
`scripts/`.

- `analyze_mbd_2link_history.py`

Policy:

- the manifest-bridge analyze/report utility is support tooling
- analysis logic is unchanged by relocation
- summary JSON schema is unchanged
- stdout summary semantics are unchanged
- old `scripts/analyze_mbd_2link_history.py` remains a compatibility wrapper
- manifest-bridge checker dependency compatibility is preserved
