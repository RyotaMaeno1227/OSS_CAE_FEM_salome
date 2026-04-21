# Phase 2A C Source Split Planning v1

## purpose

This document records the Phase 2A native solver-core inventory and
split-planning pass after Phase 1O closed out the release-lane support-tooling
taxonomy work.

Phase 2A is planning only.

- no C file moves
- no function moves
- no header splits
- no build-system changes
- no include-path changes
- no solver behavior changes
- no parser behavior changes

The goal is to inspect the current workspace as the source of truth, inventory
the largest native source files, and fix safe future split boundaries without
performing the split itself.

## scope and non-goals

This phase covers only the following large native source files:

- `src/mbd/system2d.c`
- `src/io/input.c`
- `src/analysis/static.c`
- `src/coupled/coupled_run2d.c`
- `src/solver/assembly.c`

Out of scope:

- actual source splitting
- moving functions between files
- changing exported APIs
- changing headers or include paths
- changing parser behavior
- changing solver behavior
- changing route contracts
- changing build scripts or tests
- EHL mainline integration
- monolithic route revival

## current-truth principles

Phase 1O closeout exists in the current workspace as
`docs/phase1o_tools_taxonomy_closeout_v1.md` and remains the prerequisite
closeout summary for moved support-tooling families.

Phase 2A keeps the tooling/native boundary fixed:

- solver core remains native implementation under `src/`
- Python / shell remain support tooling, not solver core
- Phase 2A is documentation and planning only
- no C file splitting is performed in Phase 2A
- no function move is performed in Phase 2A
- no header split is performed in Phase 2A
- no build-system change is performed in Phase 2A

## target file inventory

| file | line count | dominant current responsibilities |
| --- | ---: | --- |
| `src/mbd/system2d.c` | 10953 | MBD system lifecycle, contact registration, implicit/explicit stepping, output/trace writing, manifest/input bridge helpers, same-time reduced feedback, monolithic-local-patch and monolithic-proper support |
| `src/io/input.c` | 6727 | native/Nastran/parser-package import, MBD directive parsing, coupled-flex directives, FEM generic-contact directives, token/path helpers, side-effect-heavy global population |
| `src/analysis/static.c` | 2590 | static analysis workflow entrypoints, assembly/solve bridge, explicit/implicit one-way helpers, FEM generic-contact state building, local-feedback handling, trace/replay CSV output |
| `src/coupled/coupled_run2d.c` | 1760 | coupled route orchestration, environment/time parsing, flex-model loading, scheme dispatch, artifact/report metadata, step snapshot/output management |
| `src/solver/assembly.c` | 1508 | global assembly setup, skyline/profile handling, stiffness accumulation, lumped-mass helpers, body/traction/pressure load assembly |

## function inventory method

Function inventory was built with multiple passes rather than a single regex.

Method used:

1. rough `rg` scan for likely definition candidates
2. `ctags -x --c-kinds=f` probe when available
3. fallback custom Python multiline signature scan
4. header/API review to separate public entrypoints from file-local helpers
5. representative source-range reads to confirm responsibility clusters

Important notes from the current workspace:

- single-line `rg` patterns were not sufficient because many definitions use
  multiline signatures
- `ctags` did not provide a usable inventory in this environment
- the fallback Python scan was constrained to top-level brace depth to reduce
  false positives
- the resulting inventory was then manually filtered using the corresponding
  headers and representative body reads

This phase therefore treats the function inventory as a planning aid for split
seams, not as an ABI contract dump.

## function cluster table

| file | major entrypoints | internal helper clusters | obvious dependency edges |
| --- | --- | --- | --- |
| `src/mbd/system2d.c` | `mbd_system2d_init`, `mbd_system2d_free`, `mbd_system2d_clone`, `mbd_system2d_load`, `mbd_system2d_do_newmark_step`, `mbd_system2d_do_hht_step`, `mbd_system2d_run`, contact/body registration APIs | storage sizing and reserve helpers, output file open/write/close helpers, generic-contact trace helpers, same-time reduced lookup helpers, dense KKT/projection helpers, env parsing, line token parsing, local-feedback loading, monolithic-local-patch helpers, monolithic-proper file/template/mapping helpers | integrators, dense solver/projection/output modules, `src/io/input.h`, `src/analysis/static.h`, coupled local-patch bridge headers |
| `src/io/input.c` | `input_read_data`, `input_read_mbd_body_directives`, `input_read_coupled_directives`, `input_read_parser_package`, Nastran parse entrypoints, input open/close/detect APIs | token/path trimming, Nastran normalization and card helpers, MBD directive line parsers, coupled-flex directive parsers, FEM generic-contact directive parsers, parser-package mesh/material/boundary/pressure readers | writes into global FEM state, writes into `mbd_system2d_t`, depends on coupled case definitions and parser-package file layout |
| `src/analysis/static.c` | `static_analysis`, phase entrypoints, `static_assemble_system`, `static_solve_equations`, `static_calculate_stresses`, `static_write_results`, `static_validate_input` | predictor/history/corrector helpers, explicit/implicit one-way helpers, prescribed-displacement helpers, FEM generic-contact trace/state builders, feedback CSV readers, replay/trace writers | depends heavily on `assembly.*`, output paths, FEM globals, input configuration, generic-contact/local-feedback runtime data |
| `src/coupled/coupled_run2d.c` | `coupled_run2d`, `coupled_time_control_from_env`, parse/to-string APIs, buffer reserve/free APIs | env parsing, scheme/integrator parsing, master/flex model loading, output writing, step summary metadata helpers, scheme dispatch, fallback/no-flex guards | depends on split coupled-step modules, flex-model copy/load helpers, `input_read_data`, `mbd_system2d_*`, coupled case state |
| `src/solver/assembly.c` | `assembly_global_stiffness_matrix`, `assembly_global_force_vector`, `assembly_parallel_stiffness_matrix`, lumped-mass APIs, DOF-map APIs, boundary-condition APIs | element topology/DOF helpers, profile/storage setup, matrix add/get/set helpers, area/thickness helpers, body-force helpers, traction helpers, pressure helpers | depends on FEM globals, skyline storage, T3/Q4/T6 element definitions, load/boundary state populated elsewhere |

## candidate split seams

### `src/mbd/system2d.c`

Likely future seams:

- storage / snapshot / lifecycle helpers
- contact geometry registration and polyline loading
- output / trace / compare-file writing helpers
- same-time reduced/local-feedback source loading and lookup
- input-bridge parsing helpers for embedded MBD directives
- monolithic-local-patch helper block
- monolithic-proper helper block

Safe-first guidance:

- move storage/snapshot helpers before touching stepping logic
- move output/trace helpers before touching dense solve or constraint loops
- treat input-bridge helpers as a separate seam from runtime stepping

### `src/io/input.c`

Likely future seams:

- token/path/common parsing helpers
- Nastran import readers
- parser-package import readers
- MBD directive parsing
- coupled-flex directive parsing
- FEM generic-contact directive parsing and registration

Safe-first guidance:

- split pure token/path helpers first
- then isolate parser-package readers and Nastran readers
- leave side-effect-heavy directive registration close to existing ownership
  until helper boundaries are stable

### `src/analysis/static.c`

Likely future seams:

- static workflow/orchestration entrypoints
- explicit/implicit one-way stepping helpers
- FEM generic-contact trace/feedback/state helpers

Safe-first guidance:

- separate trace/feedback support before touching solve sequencing
- keep the public workflow surface thin and stable

### `src/coupled/coupled_run2d.c`

Likely future seams:

- environment/time parsing
- flex/master model load/setup
- artifact/report metadata and CSV-header helpers
- scheme dispatch / orchestration shell around already split step modules

Safe-first guidance:

- extract reporting/config helpers before touching dispatch ownership
- keep scheme dispatch thin and adjacent to `coupled_run2d(...)`

### `src/solver/assembly.c`

Likely future seams:

- profile/storage preparation helpers
- load assembly helpers
- element geometry/thickness helpers
- stiffness kernel entrypoints

Safe-first guidance:

- split body/traction/pressure load helpers before numerical kernel code
- keep matrix/profile invariants close to global stiffness entrypoints until
  helper interfaces are proven stable

## high-risk coupling notes

### `src/mbd/system2d.c`

High-risk regions:

- constrained implicit solve path and dense KKT/projection retry logic
- ground-lock enforcement intertwined with dense workspace ownership
- same-time reduced contact replay/use state coupled to runtime trace output
- monolithic-proper explicit loop with FEM input generation and feedback
  re-ingest
- top-level run loop that currently owns both runtime behavior and many output
  side effects

### `src/io/input.c`

High-risk regions:

- directive readers that both parse and immediately mutate global/runtime state
- MBD and coupled input handlers that must preserve exact contract wording and
  defaults
- generic-contact registration functions that validate cross references while
  mutating analysis state

### `src/analysis/static.c`

High-risk regions:

- FEM generic-contact enable/feedback/state path
- implicit one-way predictor/history/corrector path
- load-scale bridge behavior and trace/replay wording
- places where orchestration and behavior-sensitive contact logic currently
  share locals and temporary buffers

### `src/coupled/coupled_run2d.c`

High-risk regions:

- scheme dispatch boundary around one-way / delayed-cosim / strong-coupling
  execution
- legacy fallback handling for no-flex cases
- artifact/report semantics that downstream compare/export flows may assume

### `src/solver/assembly.c`

High-risk regions:

- skyline/profile preparation and matrix indexing invariants
- element stiffness accumulation ordering
- lumped-mass and load-assembly interactions with boundary-condition handling
- numerical helper regions where a seemingly cosmetic split could perturb
  accumulation order or storage assumptions

## recommended no-split-yet zones

The following zones should not be the first split targets:

- `src/mbd/system2d.c`
  - dense KKT solve / projection / constrained implicit execution
  - monolithic-proper explicit loop and FEM feedback round-trip
- `src/io/input.c`
  - high-side-effect directive readers that mutate global and MBD runtime
    state while validating tokens
- `src/analysis/static.c`
  - FEM generic-contact runtime state build and solver-coupled feedback path
- `src/coupled/coupled_run2d.c`
  - scheme dispatch body itself until reporting/config helpers are already
    externalized
- `src/solver/assembly.c`
  - core global stiffness assembly and skyline storage mutation path

## recommended split order

Recommended split order for later implementation phases:

1. `src/coupled/coupled_run2d.c`
   - lowest-risk planning target because step kernels are already in separate
     files and the remaining bulk is mostly config/report/setup/orchestration
2. `src/solver/assembly.c`
   - split non-kernel load/profile helpers before touching stiffness kernels
3. `src/io/input.c`
   - split pure helper and import-family blocks while keeping side-effect-heavy
     directive registration stable
4. `src/analysis/static.c`
   - split contact-generic support helpers from workflow shell
5. `src/mbd/system2d.c`
   - defer the hardest split until other seams and helper ownership patterns
     are proven on smaller files first

## out-of-scope lanes

Still out of scope for Phase 2A:

- actual C source splitting
- header/API redesign
- parser refactoring outside planning notes
- EHL mainline integration
- monolithic route revival
- Python / shell integration into native solver-core execution
- support-tooling taxonomy reopening from Phase 1O

## risks

- large files mix orchestration, IO/trace, and numerical behavior, so a naive
  split boundary can silently change runtime semantics
- some helpers that look isolated are coupled through global state or implicit
  output contracts
- `src/mbd/system2d.c` and `src/io/input.c` both bridge runtime logic and
  parsing/config concerns, making clean ownership boundaries harder than line
  count alone suggests
- function inventory generated by lightweight parsing still requires manual
  review before any future mechanical split

## closeout criteria

Phase 2A planning is closeout-ready when all of the following are true:

- Phase 1O closeout exists in the current workspace and was used as the
  prerequisite current-truth closeout summary
- each target file has a documented responsibility inventory
- candidate split seams are named
- high-risk coupling and no-split-yet zones are recorded
- a recommended split order is fixed
- no native source files, headers, build rules, or runtime behaviors were
  changed
