# contact-patch smoke checkers

`tools/checkers/smoke/contact_patch/` holds the contact-patch smoke / fixture
checker pair.

Current policy:

- contact-patch smoke checkers are support tooling
- checker relocation does not change checker logic
- solver core remains native implementation
- Python / shell are not solver core
- this phase does not move local-patch generic checkers
- this phase does not move macro-to-patch bridge checker
- legacy `scripts/check_contact_patch_*.sh` paths remain compatibility
  wrappers during Phase 1
