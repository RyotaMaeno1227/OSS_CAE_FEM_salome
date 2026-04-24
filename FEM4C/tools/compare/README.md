# tools/compare

`tools/compare/` holds support-tooling entrypoints that compare generated
artifact surfaces across iterations, routes, or runs.

- compare tools are support tooling
- they do not define solver behavior
- solver core remains native implementation under `src/`
- Python is not solver core

Compatibility policy:

- legacy `scripts/compare_*.py` paths may remain supported
- those legacy paths are thin wrappers that delegate into `tools/compare/`
- release command compatibility is preserved
