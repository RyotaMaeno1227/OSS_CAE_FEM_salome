# phase2p_post_writer_meta_first_split_audit_v1

## purpose

この文書は、Phase 2O で
`coupled_run2d_step_flex_iteration_column_name` を
`src/coupled/coupled_run2d_writer_meta.c` へ narrow に分離した結果を、
docs-only post-split audit として固定するための audit memo である。

今回の目的は次である。

- Phase 2O actual split result を確認する
- Phase 2N checker-side gate が Phase 2O 後も満たされていることを記録する
- public/private boundary が想定どおりか確認する
- `coupled_run2d_step_flex_iteration_column_name` move を accepted / closed として扱えるか判断する
- next helper candidate の readiness を再評価する
- actual further C edit より checkpoint discipline を優先すべきか結論化する

## scope and non-goals

今回の scope は次である。

- `src/coupled/coupled_run2d.c`
- `src/coupled/coupled_run2d_writer_meta.c`
- `src/coupled/coupled_run2d.h`
- `src/coupled/coupled_run2d_config.c`
- `src/coupled/coupled_run2d_route_meta.c`
- `Makefile`
- `scripts/check_year1_coupled_route_matrix.sh`
- `scripts/check_coupled_integrators.sh`
- related Phase 2 planning / architecture docs

今回の non-goals は次である。

- C source edit
- header edit
- checker/script edit
- build system edit
- function move
- route/output behavior change
- artifact schema change
- `git add`
- `git commit`
- `git push`
- next actual split execution

## prerequisites

前提は次である。

- current HEAD は `5b6918d03944923ba3c265ca7efb6bfe97e98c1e`
- current branch は `release/year1-cleanup`
- docs checkpoint commit `bbf76335724826e875634bd2f0d829cb16079165` が history に存在する
- intentional Phase 2 implementation checkpoint commit
  `5b6918d03944923ba3c265ca7efb6bfe97e98c1e` が history に存在する
- Phase 2N exact guard hardening は current worktree に残っている
- Phase 2O actual split patch は current worktree に残っている
- current repo は still broad cumulative dirty state である
- actual further C edit is not allowed in this phase

## current checkpoint status

current checkpoint status は次である。

- docs checkpoint は history に固定済み
- intentional Phase 2 implementation checkpoint は history に固定済み
- Phase 2N checker hardening は current worktree に残るが未コミットである
- Phase 2O first writer-meta split patch も current worktree に残るが未コミットである
- したがって `coupled_run2d` split lane 自体は coherent だが、next actual C edit 前に
  checkpoint discipline を再度要求する

## monolithic premise

monolithic premise は固定である。

- Year1 monolithic is fully implemented.
- Year2 monolithic is out of current scope.
- existing Year1 monolithic behavior is preserve-only.

Implications:

- visible monolithic preserve-only wording は引き続き no-split-yet に据え置く
- Year2 current-goal scope に monolithic を戻すような helper movement は対象外とする
- checker ownership も Year1 route/preserve-only surface 内に留める

## Phase 2O actual split result

Phase 2O actual split result は次のとおりである。

- `src/coupled/coupled_run2d_writer_meta.c` は存在する
- `Makefile` の `COUPLED_SRCS` には
  `$(SRCDIR)/coupled/coupled_run2d_writer_meta.c` が追加されている
- `coupled_run2d_step_flex_iteration_column_name` の body は
  `src/coupled/coupled_run2d_writer_meta.c` にのみ存在する
- `src/coupled/coupled_run2d.c` 側には compile-preserving declaration と call-site usage だけが残る
- `src/coupled/coupled_run2d.h` はこの helper のために拡張されていない
- `src/coupled/coupled_run2d_config.c` は untouched のままである
- `src/coupled/coupled_run2d_route_meta.c` は untouched のままである
- Phase 2O task で変更した C/build surfaces は
  `src/coupled/coupled_run2d.c`,
  `src/coupled/coupled_run2d_writer_meta.c`,
  `Makefile`
  に限定される

## checker gate status

Phase 2N checker-side gate は Phase 2O 後も満たされている。

- `scripts/check_year1_coupled_route_matrix.sh` は
  `step_flex_counter_columns` exact output token を route-aware に確認する
  - `oneway_snapshot` -> `snapshot_iteration_index`
  - `delayed_cosim_v1_5` -> `communication_iteration_index`
  - `fixed_point_strong` -> `coupling_iteration_index`
  - `monolithic_strong_v1` -> `coupling_iteration_index`
- `scripts/check_coupled_integrators.sh` は oneway exact
  `step_flex_counter_columns` guard を維持している
- Phase 2O verification matrix は green で完了している
- したがって selected helper move の exact acceptance gate は維持されている

補足:

- current worktree で `scripts/check_year1_coupled_route_matrix.sh` と
  `scripts/check_coupled_integrators.sh` は dirty だが、
  これは Phase 2N / pre-existing surfaces であり、Phase 2O task 自体は script edits を行っていない

## public/private boundary status

public/private boundary status は次のとおりである。

- public API remained unchanged: `yes`
- private/internal declaration surface remained minimal: `yes`
- writer body remained in place: `yes`
- writer body semantics unchanged: `yes`

具体的には次である。

- `src/coupled/coupled_run2d.h` の public declaration surface は unchanged
- new helper home は `src/coupled/coupled_run2d_writer_meta.c` のみに閉じる
- `src/coupled/coupled_run2d.c` には local forward declaration を残すだけで、
  writer body や visible wording helper cluster を広く触っていない
- `coupled_run2d_write_output(...)` の format string / emitted field order / route semantics は unchanged

## keep-vs-move table update

| helper / region | current responsibility | current home after Phase 2O | status | guard coverage | reason | risk | verification / next requirement |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `coupled_run2d_step_flex_iteration_column_name` | selects `step_flex_counter_columns` label in writer output | `src/coupled/coupled_run2d_writer_meta.c` | accepted / closed | exact route-aware guard present | first narrow writer-meta split succeeded without public header expansion | uncommitted worktree state only | checkpoint before another C split |
| `coupled_run2d_delay_buffer_scope_from_run` | delayed buffer scope token in writer output | keep in `src/coupled/coupled_run2d.c` | later | partial delayed output coverage only | still writer-adjacent but delayed semantics visible token is more sensitive | delayed scope token drift | dedicated delayed scope/output checker first |
| exported CSV header getters | exported artifact/interface/reaction/observation header constants | keep in `src/coupled/coupled_run2d.c` | later | indirect only | public/exported API surface and exact guard gap remain | public API / exact contract drift | dedicated exact header-getter checker first |
| `coupled_run2d_write_output(...)` | main artifact writer body | keep in `src/coupled/coupled_run2d.c` | no-split-yet | mixed exact/semantic only | still the artifact contract nexus | artifact schema / route output regression | dedicated multi-route writer audit first |
| summary/report wording helpers | visible summary tokens | keep in `src/coupled/coupled_run2d.c` | no-split-yet | selected route exact tokens only | visible wording remains contract-sensitive | wording regression | dedicated wording audit first |
| `coupled_run2d_comparison_role_from_scheme` | visible comparison-role wording | keep in `src/coupled/coupled_run2d.c` | no-split-yet | exact tokens exist | wording remains central to visible output semantics | preserve-only wording drift | dedicated wording audit first |
| `coupled_run2d_solver_route_class_from_run` | route-class wording | keep in `src/coupled/coupled_run2d.c` | no-split-yet | partial exact guard | route topology/body count sensitivity remains high | cross-route wording drift | stronger cross-route exact guard first |
| `coupled_run2d_step_coupling_reason` | monolithic `step_status` wording | keep in `src/coupled/coupled_run2d.c` | no-split-yet | indirect only | exact reason-string guard remains absent | preserve-only wording drift | exact step-status reason checker first |

## next-candidate reassessment

### candidate A

target:

- `coupled_run2d_delay_buffer_scope_from_run`

assessment:

- still narrow in code size
- still touches delayed-only visible token semantics
- still lacks dedicated delayed scope/output exact guard
- does not beat the current provenance risk introduced by stacking another uncommitted C split on top of Phase 2O

status:

- `later`

### candidate B

target:

- exported CSV header getters

assessment:

- helper bodies are small
- but the surface is public/exported via `src/coupled/coupled_run2d.h`
- exact guard coverage remains weaker than for the just-accepted writer-meta helper move
- current repo state does not justify spending another uncommitted C edit on a public surface

status:

- `later`

### candidate C

target:

- no next C split; checkpoint Phase 2N / Phase 2O first

assessment:

- current best option
- Phase 2N and Phase 2O results are technically accepted, but both remain worktree-only
- broad cumulative dirty state still exists outside the `coupled_run2d` lane
- another actual C split before checkpoint discipline would weaken provenance / review / rollback clarity again

status:

- `recommended`

### candidate D

target:

- another docs/checker guard phase before any split

assessment:

- valid as a later preparation lane for candidate A or B
- not the immediate next action because checkpoint discipline is the tighter blocker right now

status:

- `secondary follow-up only after checkpoint planning/commit`

## recommended next action

recommended next action は次である。

- accept Phase 2O split as `accepted / closed`
- do not start another actual C split yet
- prepare checkpoint planning / checkpoint commit work for Phase 2N and Phase 2O surfaces first
- after checkpoint discipline is restored, reassess candidate A
  `coupled_run2d_delay_buffer_scope_from_run` vs any additional guard/doc phase

結論として、next phase は actual C edit ではなく
another docs/checkpoint phase を優先する。

## approval gate for any future actual C edit

固定する approval gate は次である。

- Phase 2P is docs-only
- no C edit is performed in Phase 2P
- no checker/script edit is performed in Phase 2P
- no build system change is performed in Phase 2P
- no commit/staging/push is performed in Phase 2P
- actual further C edit still requires explicit user approval
- current Phase 2N / Phase 2O work remains worktree-only, so further actual C edit should not begin before checkpoint discipline is restored unless the user explicitly approves that additional provenance risk

## risks

主要 risk は次である。

- accepted Phase 2O work を未コミットのまま次 split に重ねると review scope が再び膨らむ
- `coupled_run2d_delay_buffer_scope_from_run` を急いで動かすと delayed-only visible token drift を見逃しやすい
- exported CSV header getters を急いで動かすと public API and exact checker gap を同時に広げる
- current broad dirty state は still repo-wide provenance risk を持つ

## closeout criteria

Phase 2P docs-only audit は次を満たしたら closeout-ready とする。

- Phase 2O actual split result を確認できた
- checker gate remained satisfied を記録できた
- public/private boundary が想定どおりであると確認できた
- `coupled_run2d_step_flex_iteration_column_name` を accepted / closed に更新できた
- next-candidate reassessment を A/B/C/D で明示できた
- recommended next action を checkpoint-first に固定できた
- future approval gate を明記できた
