# FEM4C Solver Reorg and MBD Migration Plan

## Goal
- Reorganize the FEM solver into stable, testable modules.
- Migrate a minimal 2D MBD solver path from Chrono-style concepts to C.
- Enable three execution modes with reproducible results:
  - FEM-only
  - MBD-only (2D constraints)
  - FEM+MBD coupled (incremental)

## Scope
- In scope:
  - 2D linear static FEM baseline stabilization
  - 2D MBD constraints (distance/revolute first)
  - Shared linear solver and diagnostics path
- Out of scope (for this phase):
  - 3D MBD
  - Full nonlinear/contact-complete parity
  - Multi-physics extensions

## Target Architecture
- `src/io/`
  - Input adapters (native, parser package, optional Nastran pre-step)
- `src/elements/` + `src/solver/`
  - FEM core (assembly, boundary, linear solve)
- `src/mbd/` (new in migration steps)
  - Constraint descriptors, Jacobians, residual assembly
- `src/coupling/` (new in migration steps)
  - FEM-MBD DOF mapping and coupling equations
- `src/analysis/`
  - Mode orchestration and top-level execution flow

## Phase Plan
1. Phase 0: Stabilize Build and Execution Entry
- Ensure `make` builds both solver and parser across POSIX/Windows.
- Fix parser/runtime path mismatch and C99 compatibility issues.
- Verify:
  - Native example run passes
  - Parser one-shot run passes with canonical sample

2. Phase 1: FEM Solver Reorganization
- Split noisy debug output from default runtime.
- Normalize assembly interfaces (`prepare`, `assemble`, `solve`, `export`).
- Add deterministic baseline checks for one native and one parser case.
- Deliverable:
  - Stable FEM-only mode with documented run commands

3. Phase 2: MBD Core Migration (2D Minimal Set)
- Add C structs for constraints/state/Jacobian blocks.
- Implement first constraints:
  - Distance
  - Revolute
- Assemble KKT for minimal MBD cases and solve with existing linear path.
- Deliverable:
  - MBD-only minimal tests with expected residual thresholds

4. Phase 3: FEM-MBD Coupling
- Introduce coupling layer to map FEM DOFs and MBD constraints.
- Add one coupled reference case and convergence checks.
- Deliverable:
  - Coupled run path with reproducible output and diagnostics

5. Phase 4: Validation and Performance Baseline
- Add regression matrix:
  - native FEM
  - parser FEM
  - MBD minimal
  - coupled minimal
- Record runtime and residual baselines.
- Deliverable:
  - "ready for extension" baseline and handoff docs

## Team Split
- A Team:
  - Core implementation in FEM/MBD/coupling
  - Numerical validation and solver behavior checks
- B Team:
  - C <-> C++ mapping tables and migration notes
  - Minimal success/failure sample inputs for A Team
- C Team:
  - Canonical docs synchronization, command recipes, and glossary
  - Keep docs entry points clean and non-duplicated

## Definition of Done (per phase)
- Build:
  - `make -C FEM4C` succeeds on target environment
- Execution:
  - Required sample commands run and produce outputs
- Validation:
  - Residual/iteration thresholds are reported and within expected bounds
- Documentation:
  - `FEM4C/docs/README.md` points to updated canonical docs
  - No duplicate "draft/final" docs for the same topic

## Immediate Next Actions
1. Keep Phase 0 green while reducing default log noise.
2. Add Phase 1 API boundary notes to `src/solver` and `src/analysis`.
3. Create `src/mbd/` with placeholder interfaces and first constraint test skeleton.

## Current Baseline (2026-02-06)
- Build path stabilized for parser binary naming:
  - POSIX: `parser/parser`
  - Windows: `parser/parser.exe`
- Native FEM run and parser one-shot run (3Dtria sample) are passing.
- `src/mbd/` initial files added:
  - `constraint2d.h/.c`
  - `kkt2d.h/.c`
