# Release Surface Policy v1

## 目的

この文書は、FEM4C の最終リリース版に何を含めるかを決めるための
release boundary specification である。

今回は planning-only であり、実装変更は行わない。
ここで固定するのは次である。

- final release に含める面
- internal tooling として repo に残す面
- `.sh` / `.py` を end-user release artifact から切り離せるかどうか
- そのために必要な移行コストの概算
- release artifact を allowlist-based に作り、repo root の broad copy を避けること

## 前提

current repo には C solver core の周囲に次が混在している。

- solver core 本体
- parser / manifest 生成
- run/check/export bundle
- golden example export
- handoff / next-chat 文書
- research reproducibility scripts

current capability claim は増やさない。
特に mixed / FEM / MBD の current MVP scope は据え置きとする。

## 分類ポリシー

### A. solver core

final release 必須。

含むもの:

- `src/**/*.c`
- `src/**/*.h`
- `src/fem4c.c`
- `parser/parser.c`
- runtime に直接必要な input/output 実装

定義:

- end-user が solver を実行するために不可欠
- solver physics / parser semantics / runtime I/O の正本

### B. runtime support

final release 候補。

含むもの:

- `Makefile`
- `bin/fem4c` を作る build instructions
- user-facing examples のうち最小の canonical examples
- user-facing docs のうち contract / usage / quickstart

定義:

- solver core を end-user がビルド / 起動 /理解するために必要
- C 本体ではないが、最終 artifact に同梱する合理性がある

### C. dev/test tooling

internal only。

含むもの:

- `scripts/check_*.sh`
- `scripts/run_*_golden_example*.sh`
- `scripts/export_*_golden_example*.sh`
- validator / bundle / packaging scripts
- orchestration 用 `.py`

定義:

- 開発時の acceptance、review、回帰確認、artifact 収集に必要
- end-user の日常利用には不要

### D. research reproducibility artifacts

internal only。

含むもの:

- fallback scenario bundle
- mixed branch handoff 文書
- `docs/*handoff*`
- golden example README 群
- iteration / replay / fallback 再現用 export

定義:

- 研究レビュー、次チャット引継ぎ、検証再現には有用
- 製品 release の最小 surface からは外せる

## current file classification

代表例を path 単位で整理する。
これは repo 全件 listing ではなく、release boundary 判断に必要な代表 inventory である。

| file/path | current role | needed for end-user release? | can remain internal? | replacement needed in C? | estimated migration cost |
|---|---|---:|---:|---:|---|
| `src/fem4c.c` | solver executable entrypoint | yes | no | no | low |
| `src/io/input.c` | parser/runtime input implementation | yes | no | no | low |
| `src/io/output.c` | solver output implementation | yes | no | no | low |
| `src/analysis/static.c` | FEM static analysis core | yes | no | no | low |
| `src/mbd/system/system2d.c` | MBD runtime core | yes | no | no | low |
| `src/common/globals.h` | runtime state definitions | yes | no | no | low |
| `Makefile` | canonical build entry | yes | no | no | low |
| `README.md` | top-level usage surface | yes | no | no | low |
| `docs/fem_macro_penalty_generic_contract_v1.md` | user-facing contract reference | yes | no | no | low |
| `docs/mbd_macro_penalty_generic_contract_v1.md` | user-facing contract reference | yes | no | no | low |
| `docs/local_patch_generic_contract_v1.md` | local request/response contract | yes | no | no | low |
| `examples/t3_cantilever_beam.dat` | minimal native FEM example | yes | no | no | low |
| `examples/mbd_macro_penalty_generic_contract_v1/minimal_generic_contact_active.dat` | MBD generic contact example | maybe | yes | no | low |
| `examples/fem_macro_penalty_generic_contract_v1/minimal_fem_generic_contact_active.dat` | FEM generic contact example | maybe | yes | no | low |
| `scripts/build_mbd_master_from_manifest.py` | parser manifest -> MBD deck bridge | maybe | yes | maybe | medium |
| `scripts/run_local_patch_generic_contract_v1.py` | local contract runner helper | no | yes | no | low |
| `scripts/local_patch_generic_solver_v1.py` | proxy local solver MVP | no for final solver-only release | yes | maybe | high |
| `scripts/check_local_patch_generic_contract_v1.sh` | acceptance check | no | yes | no | low |
| `scripts/check_local_patch_generic_solver_v1.sh` | acceptance check | no | yes | no | low |
| `scripts/check_mbd_macro_penalty_generic_trace_v1.sh` | dev regression check | no | yes | no | low |
| `scripts/check_fem_macro_penalty_generic_trace_v1.sh` | dev regression check | no | yes | no | low |
| `scripts/check_fem_macro_local_iter_generic_v1.sh` | dev regression check | no | yes | no | low |
| `scripts/plot_fem_macro_local_iter_generic_v1.py` | review visualization | no | yes | no | low |
| `scripts/export_fem_macro_local_iter_generic_golden_example.sh` | golden export | no | yes | no | low |
| `scripts/check_mixed_macro_local_bundle_v1.sh` | mixed branch bundle | no | yes | no | low |
| `scripts/export_mixed_macro_local_golden_example.sh` | mixed happy-path golden export | no | yes | no | low |
| `scripts/export_mixed_macro_local_fallback_golden_example.sh` | mixed fallback golden export | no | yes | no | low |
| `scripts/run_mixed_macro_local_oneway_generic_v1.sh` | mixed orchestration runner | no for minimal end-user release | yes | maybe | medium |
| `scripts/run_mixed_macro_local_replay_split_generic_v1.sh` | mixed split replay orchestration | no | yes | maybe | medium |
| `scripts/export_mixed_local_feedback_reduced_generic.py` | reduced feedback exporter | no | yes | maybe | medium |
| `scripts/build_local_patch_generic_requests_from_mixed_macro_manifest.py` | mixed manifest -> local request builder | no | yes | maybe | medium |
| `docs/mixed_macro_local_generic_contract_v1.md` | mixed interface spec | maybe | yes | no | low |
| `docs/mixed_macro_local_feedback_generic_v1.md` | mixed reduced feedback surface spec | maybe | yes | no | low |
| `docs/mixed_macro_local_replay_split_generic_v1.md` | mixed split replay consume spec | maybe | yes | no | low |
| `examples/mixed_macro_local_generic_contract_v1/README.md` | mixed example note | no | yes | no | low |
| `fem4c_next_chat_handoff_2026-04-04_mixed_generic_contact.md` | next-chat handoff | no | yes | no | low |

## category-level inventory

### `src/**/*.c`, `src/**/*.h`

- current role: solver core / runtime implementation
- final release: yes
- internal-only possible: no
- replacement in C needed: no
- note: すでに C 正本

### `Makefile` / build

- current role: canonical build surface
- final release: yes
- internal-only possible: no
- replacement in C needed: no
- note: build system を変えない前提ならそのまま含める

### `scripts/*.sh`

- current role: acceptance / regression / export / orchestration
- final release: 原則 no
- internal-only possible: yes
- replacement in C needed: case-by-case
- note: 多くは end-user ではなく開発者向け

### `scripts/*.py`

- current role: validators, builders, exporters, visualizers, local proxy solver, mixed tooling
- final release: 原則 no。ただし manifest bridge を end-user workflow に残すなら一部 B に昇格する余地あり
- internal-only possible: yes
- replacement in C needed: workflow による

### `docs/*`

- current role: contract / usage / design / handoff / release notes
- final release: 分離が必要
- final release に含める候補:
  - contract docs
  - usage docs
  - quickstart docs
- internal-only:
  - handoff
  - team / branch hardening
  - bundle / golden example 説明

### `examples/*`

- current role: example decks, fixtures, research reproducibility inputs
- final release: 一部 yes
- internal-only possible: 多くは yes
- note: minimal canonical examples は残す。fallback / golden / aggregation 専用 fixtures は internal に落とせる

### `handoff *.md`

- current role: next-chat continuity
- final release: no
- internal-only possible: yes
- replacement in C needed: no

## Proposal A

### 定義

final release artifact から `.sh` と research-oriented `.py` を除外する。
ただし repo には internal tooling として残す。

### included

- C solver core
- Makefile
- minimal release docs
- minimal examples

### excluded from final artifact

- `scripts/check_*.sh`
- `scripts/export_*_golden_example*.sh`
- `scripts/plot_*.py`
- mixed / fallback / handoff / bundle orchestration
- next-chat handoff 文書

### pros

- 最小工数で `.sh` を release artifact から外せる
- solver core は C で維持できる
- dev/test reproducibility は repo 内で保持できる
- capability claim を増やさず packaging だけ整理できる

### cons

- repo と release artifact の内容差が大きくなる
- end-user は bundled check/export を使えない
- 一部 Python bridge を release からも外すと、manifest workflow は開発者向け扱いになる

### risk

- low

### migration cost

- low

### expected impact

- release packaging のみ整理
- 現行 branch の開発速度を落としにくい

## Proposal B

### 定義

end-user entrypoint だけを C 側に寄せる。
check/export/handoff は internal tooling のまま残す。
manifest bridge のうち end-user に必要なものだけ release support として残す。

### included

- Proposal A の included
- 必要に応じて user-facing bridge 用の限定 `.py`
  - 例: `scripts/build_mbd_master_from_manifest.py`

### excluded from final artifact

- `check_*.sh`
- golden export
- handoff
- bundle
- visualization

### pros

- end-user workflow を壊しにくい
- `.sh` を release artifact から外しやすい
- Python surface を必要最小限に限定できる

### cons

- release support と internal tooling の境界線管理が必要
- 一部 `.py` は release artifact に残る可能性がある
- 「最終 artifact にスクリプトを含めたくない」という希望には完全一致しない

### risk

- medium

### migration cost

- medium

### expected impact

- user-facing workflow の現実性を保ったまま整理できる
- 中期的に C 置換の優先順位も付けやすい

## Proposal C

### 定義

`.sh` / `.py` を最終的に全面廃止し、runtime / validation / export の主要 surface を C 側に寄せる。

### included

- ほぼ C / header / build / minimal docs のみ

### pros

- release artifact は最も単純になる
- 「solver の本体は C 言語で持ちたい」という希望に最も強く一致する
- packaging / distribution policy が明快

### cons

- builder / validator / exporter / local proxy orchestration を C に再実装する必要がある
- mixed branch の tooling 面の置換コストが大きい
- regression / golden / review workflow の再構築が必要

### risk

- high

### migration cost

- high

### expected impact

- 長期的には一貫性が高い
- ただし短中期では研究速度とレビュー性を落とす可能性が高い

## recommendation

main recommendation は Proposal A とし、release support の二択では A2 を採用する。
A2 では parser/manifest bridge と generic local proxy validator / runner / solver を
internal-only として扱い、final release artifact には含めない。

### 理由

- 今回の repo は C solver core の周囲に研究用 / hardening 用 / handoff 用 orchestration が厚く載っている
- これらは end-user release artifact から外しても current capability claim を損なわない
- `.sh` を final release に含めない方針は、Proposal A なら low cost で可能
- C への全面移行は現時点では高コストで、release boundary を先に決める目的に対して過剰

### `.sh` を final release に含めない方針は possible か

possible である。

- Proposal A なら `low`
- Proposal B なら `medium`
- Proposal C なら `high`

### 最小工数の次アクション

実装に進むなら次の順で着手するのが最小工数である。

1. release artifact に含める docs / examples / binaries の allowlist を文書で固定する
2. `scripts/check_*.sh`, `scripts/export_*`, handoff を internal-only として packaging 対象から外す
3. internal tooling、handoff logs、generated bundles、golden / fallback artifacts、
   PM continuity docs を release-facing docs/examples と混ぜない

## resolved by A2 / remaining documentation work

release support の file-level decision は A2 で解消した。
残る作業は release-facing docs と internal-only docs の整理である。

- `scripts/build_mbd_master_from_manifest.py` は internal-only とする
- local proxy solver `.py` は end-user release から除外する
- examples を minimal set にどこまで絞るか
- `docs/README.md` から internal-only docs をどう隠すか

## non-goals

この文書は次を行わない。

- ファイル移動
- rename
- 削除
- build system 変更
- solver physics 変更
- mixed / FEM / MBD capability claim の増加
