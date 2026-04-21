# solver_core_vs_tooling_boundary_v1

## purpose

この文書は current workspace での solver core と tooling boundary を固定する。

特に mixed family を含む current route 群で、
Python / shell を使っていることと、
solver core に Python を組み込むことを混同しないための memo である。

## solver core

solver core は `src/` 配下の native implementation である。

- native runtime
- native solver behavior
- native trace / output / validation の主経路

solver core の主語は C native implementation に置く。

## tooling layer

Python / shell は current workspace では補助ツール層として扱う。

- runner
- export
- replay
- checker
- report
- handoff / aggregation / compare

これらは support tooling であり、
solver core そのものではない。

Phase 1A では Year2 current-goal runner / checker の実体を `tools/` 側へ寄せるが、
旧 `scripts/` path は compatibility wrapper として残す。
これは tooling taxonomy の整理であり、solver core の移動ではない。

Phase 1B でも同じ原則を mixed bounded family に適用する。
runner / exporter / checker の実体は `tools/` 側へ寄せるが、
それは support tooling の再配置であり、solver core の変更ではない。

Phase 1C でも同じ原則を EHL separate lane に適用する。
package builder / runner の実体は `tools/` 側へ寄せるが、
それは support tooling の再配置であり、EHL mainline integration ではない。

## fixed boundary

current workspace では次を固定する。

- Python runtime を solver core に埋め込まない
- solver から Python callback を要求しない
- Python を solver core execution path の必須要素にしない
- Python を product contract の主語にしない

一方で次は許容する。

- reusable な exporter / checker / replay helper を support layer として残す
- docs / checker を伴う internal tooling を repo に残す
- orchestration / report / hardening を Python / shell で行う

つまり、

- Python を使うこと
- Python を solver core に組み込むこと

は別であり、後者はやらない。

## mixed family application

mixed family でもこの原則を守る。

- `scripts/run_mixed_macro_local_oneway_generic_v1.sh`
- `scripts/export_mixed_local_feedback_reduced_generic.py`
- `scripts/run_mixed_macro_local_replay_split_generic_v1.sh`

Phase 1B 以後は、これらの旧 `scripts/` path は compatibility wrapper として維持され、
実体は `tools/` 側に置く。

は bounded orchestration / export / replay consume の support tooling である。

これらは live route を支える補助層として残してよいが、
solver core そのものとは呼ばない。

mixed bounded route の contract baseline も変えない。

- first-class feedback は `gamma_n` のみ
- `k_contact_eff` / `mu_eff` を route-level return として追加しない
- local contact は post-solve artifact
- friction solver state は internal state

## ehl lane application

EHL separate lane でもこの原則を守る。

- `scripts/build_year2_oneway_local_ehl_case_builder_v1.py`
- `scripts/build_ehl_solver_package1_setup_v1.py`
- `scripts/run_ehl_solver_package2_reynolds_v1.py`
- `scripts/run_ehl_solver_package3_coupled_v1.py`
- `scripts/run_year2_oneway_ehl_package4_batch_v1.py`

Phase 1C 以後は、これらの旧 `scripts/` path は compatibility wrapper として維持され、
実体は `tools/builders/ehl/` と `tools/runners/ehl/` 側に置く。

これらは EHL separate lane を支える support tooling であり、
solver core そのものとは呼ばない。
