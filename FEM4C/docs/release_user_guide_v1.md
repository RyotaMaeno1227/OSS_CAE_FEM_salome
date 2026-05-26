# Release User Guide v1

## 目的

この文書は final release artifact の user-facing surface を説明する。

current release policy は Proposal A / A2 である。

- final release artifact には `.sh` を含めない
- parser/manifest bridge は final release に含めない
- generic local proxy solver は final release capability ではない

## final release に含まれるもの

### executable / build surface

- `Makefile`
- `src/**/*.c`
- `src/**/*.h`
- `parser/parser.c`

### release-facing docs

- `README.md`
- `docs/README.md`
- `docs/release_user_guide_v1.md`
- `docs/fem_macro_penalty_generic_contract_v1.md`
- `docs/mbd_macro_penalty_generic_contract_v1.md`
- `docs/local_patch_generic_contract_v1.md`
- `docs/fem_macro_penalty_generic_solver_v1.md`
- `docs/mbd_macro_penalty_generic_solver_v1.md`
- release surface / manifest docs

### included examples

- `examples/t3_cantilever_beam.dat`
- `examples/t6_cantilever_beam.dat`
- `examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat`
- `examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat`
- `examples/local_patch_generic_contract_v1/request_penetration.json`
- `examples/local_patch_generic_contract_v1/response_example.json`

current release-facing contact example は次の 1 本に固定する。

- `examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat`

この contact example は同じ directory の surface CSV 2 本も含めて release tree に入る。

- `examples/mbd_macro_penalty_generic_contract_v1/surface_body0_active.csv`
- `examples/mbd_macro_penalty_generic_contract_v1/surface_body1_active.csv`

## minimal build / run

Build:

```bash
make -j
```

Structural direct C-run example:

```bash
./bin/fem4c examples/t6_cantilever_beam.dat /tmp/t6_out.dat
```

Contact direct C-run example:

```bash
./bin/fem4c --mode=mbd --mbd-integrator=newmark_beta \
  examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat \
  /tmp/mbd_generic_contact.out
```

この 2 本が current final release tree 上の main path である。

## final release に含まれないもの

以下は repo には残るが、final release artifact の同梱対象外である。

- all `.sh`
- `scripts/check_*`
- `scripts/run_*`
- `scripts/export_*`
- bundle / golden / fallback artifact generator
- handoff documents
- parser/manifest -> generated deck bridge
- generic local proxy validator / runner / solver

## current capability boundary

この user guide は current branch の研究 hardening route 全体を release promise にしない。

release-facing promise に含めないもの:

- FEM static macro+local iterative hardening route
- mixed reduced feedback surface hardening route
- mixed split replay consume hardening route
- local proxy / mixed split replay / fallback tooling

また、次も final release claim ではない。

- joint mixed replay
- mixed solve
- live co-sim
- true lagged mixed time-step co-sim
- same-time mixed co-sim

## internal-only への導線

repo 内には internal tooling が残る。
ただしそれは final release artifact とは切り離す。
release artifact は allowlist-based で作り、repo root の broad copy では作らない。

一覧は [docs/internal_tooling_index_v1.md](internal_tooling_index_v1.md) に置く。
