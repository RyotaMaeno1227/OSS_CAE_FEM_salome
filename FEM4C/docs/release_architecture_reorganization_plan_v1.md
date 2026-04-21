# release_architecture_reorganization_plan_v1

## purpose

この文書は、Year2 current goal の release-ready 状態を維持したまま、
release 後の repo 構成見直し計画を固定するための plan です。

今回の目的は次です。

- 可読性を上げる
- 変更影響範囲を module ごとに切り離しやすくする
- solver core と tooling を明確に分ける
- 将来の実移動を docs 先行で安全に準備する

今回は docs only であり、
ファイル移動、rename、include path 修正、build system 修正、solver behavior 変更は行いません。

## current pain points

current workspace で確認できる pain point は次です。

- `scripts/` に runner / exporter / replay / checker / report が混在している
- Year2 route script と mixed bounded script と EHL package script が同じ flat 層に並んでいる
- `src/mbd/system2d.c` が巨大で、MBD core / coupling / output 周辺の責務が密集している
- `src/io/input.c` が巨大で、parser bridge / deck loading / route option 解釈が集中している
- `src/analysis/static.c` が大きく、解析本体と route-level validation / orchestration 寄りの責務が近接している
- `src/` 配下は native module としては分かれているが、solver-level に FEM / MBD / local_contact / EHL / coupling を並列に読む layout にはなっていない
- `parser/` と `src/io/` と `src/mbd/` の責務境界が release 観点では読み取りづらい
- `docs/` と `examples/` も route / contract / closeout / release が flat に増えており、参照導線が長い

## current module inventory

### top-level directories

current top-level では少なくとも次が live inventory です。

- `src/`
- `scripts/`
- `docs/`
- `examples/`
- `parser/`
- `tools/`
- `build/`
- `bin/`
- `archive/`

### native source inventory

current `src/` は次の module 群に分かれています。

- `src/common`
- `src/analysis`
- `src/io`
- `src/elements`
- `src/mbd`
- `src/solver`
- `src/coupled`

現時点で特に大きい C source は次です。

- `src/mbd/system2d.c`
- `src/io/input.c`
- `src/analysis/static.c`
- `src/coupled/coupled_run2d.c`
- `src/solver/assembly.c`

これは release 後に分割設計の優先対象として扱う。

### script inventory

current `scripts/` は flat 配置で、少なくとも次の群が同居しています。

- Year2 route runners
  - `scripts/year2_oneway_mbd_fem_v1.py`
  - `scripts/year2_lagged_mbd_fem_v1.py`
  - `scripts/year2_same_time_mbd_fem_v1.py`
- Year2 route checkers
  - `scripts/check_year2_oneway_mbd_fem_v1.sh`
  - `scripts/check_year2_lagged_mbd_fem_v1.sh`
  - `scripts/check_year2_same_time_mbd_fem_v1.sh`
- EHL package scripts
  - `scripts/build_ehl_solver_package1_setup_v1.py`
  - `scripts/run_ehl_solver_package2_reynolds_v1.py`
  - `scripts/run_ehl_solver_package3_coupled_v1.py`
  - `scripts/run_year2_oneway_ehl_package4_batch_v1.py`
- mixed / bounded scripts
  - `scripts/run_mixed_macro_local_oneway_generic_v1.sh`
  - `scripts/export_mixed_local_feedback_reduced_generic.py`
  - `scripts/run_mixed_macro_local_replay_split_generic_v1.sh`
  - mixed dedicated checkers
- compare / report / migration / matrix tooling

サイズ面では次が特に大きい層です。

- `scripts/year2_lagged_mbd_fem_v1.py`
- `scripts/year2_oneway_mbd_fem_v1.py`
- `scripts/year2_same_time_mbd_fem_v1.py`
- `scripts/run_year2_oneway_ehl_package4_batch_v1.py`
- `scripts/run_ehl_solver_package3_coupled_v1.py`
- `scripts/run_mixed_macro_local_replay_split_generic_v1.sh`

### docs inventory

current `docs/` は少なくとも次の群を含みます。

- route docs
- contract docs
- closeout / release memos
- architecture / policy docs
- EHL lane docs
- mixed bounded docs

flat 配置のため、route / contract / closeout / architecture を directory taxonomy で整理する余地があります。

### examples inventory

current `examples/` は少なくとも次の群を含みます。

- `examples/year2_*`
- `examples/ehl_*`
- `examples/mixed_*`
- generic FEM / MBD / local patch contract examples

### tools inventory

current `tools/` directory は存在し、Phase 1A / 1B / 1C で
Year2、mixed bounded、EHL separate lane の support tooling 実体を段階的に受ける層として使う。
したがって Phase 1 では `tools/` 側 taxonomy を導入し、`scripts/` flat 層を整理対象とする。

Phase 1A では、Year2 current-goal runner / checker の実体を `tools/runners/year2/` と
`tools/checkers/year2/` に寄せ、旧 `scripts/` path は compatibility wrapper として残す。
Phase 1B では、canonical mixed bounded family の runner / exporter / checker 実体を
`tools/runners/mixed_bounded/`、`tools/exporters/mixed_bounded/`、
`tools/checkers/mixed_bounded/` に寄せ、旧 `scripts/` path は compatibility wrapper として残す。
Phase 1C では、EHL separate lane の package builder / runner 実体を
`tools/builders/ehl/` と `tools/runners/ehl/` に寄せ、旧 `scripts/` path は
compatibility wrapper として残す。EHL mainline integration は行わない。
Phase 1F-a では、contact-circle / gear-pin proxy / involute-gear の
route-specific live checkers を `tools/checkers/routes/` に寄せ、旧
`scripts/` path は compatibility wrapper として残す。
Phase 1F-b では、mixed macro-local の route-specific live checkers を
`tools/checkers/routes/mixed_macro_local/` に寄せ、旧 `scripts/` path は
compatibility wrapper として残す。
Phase 1F-c では、same-time file-exchange の route-specific checker と
smoke/bundle companion を `tools/checkers/routes/same_time_file_exchange/`
に寄せ、旧 `scripts/` path は compatibility wrapper として残す。
Phase 1F-d では、same-time local-patch route-specific checker を
`tools/checkers/routes/same_time_local_patch/` に寄せ、旧 `scripts/` path は
compatibility wrapper として残す。generic local-patch contract checker は
この phase では移さない。
Phase 1G-a では、generic local-patch contract / solver / visualization
checkers を `tools/checkers/generic/local_patch/` に寄せ、旧 `scripts/`
path は compatibility wrapper として残す。validator / runner / plotter /
exporter 実体はこの phase では移さない。
Phase 1G-b では、contact-patch fixture / receiver smoke checker を
`tools/checkers/smoke/contact_patch/` に寄せ、旧 `scripts/` path は
compatibility wrapper として残す。local-patch generic checker と
macro-to-patch bridge checker はこの phase では移さない。
Phase 1G-c では、macro-to-patch bridge checker を
`tools/checkers/bridges/macro_to_patch/` に寄せ、旧 `scripts/` path は
compatibility wrapper として残す。bridge runner 実体はこの phase では
移さない。
Phase 1H-a では、local-patch plotter と local-patch golden exporter を
`tools/reports/local_patch/` と
`tools/exporters/golden_examples/local_patch/` に寄せ、旧 `scripts/` path
は compatibility wrapper として残す。FEM macro-local plotter はこの phase
では移さない。
Phase 1H-b では、FEM/MBD macro-local iteration compare tool を
`tools/compare/fem_macro_local/` と
`tools/compare/mbd_macro_local/` に寄せ、旧 `scripts/compare_*.py` path は
compatibility wrapper として残す。iteration checkers はこの phase では
移さない。
Phase 1H-c では、contact review bundle を
`tools/reports/contact_review/` に寄せ、旧
`scripts/fem4c_contact_review_bundle.sh` path は compatibility wrapper とし
て残す。internal dependency call は意図的に `scripts/check_*.sh`
compatibility surface のまま維持し、bundle logic と output bundle
contract は変えない。
Phase 1H-d-a では、FEM macro-local golden exporter を
`tools/exporters/golden_examples/fem_macro_local/` に寄せ、旧
`scripts/export_fem_macro_local_iter_generic_golden_example.sh` path は
compatibility wrapper として残す。runner / plotter dependency call は
意図的に `scripts/run_fem_macro_local_iter_generic_v1.sh` と
`scripts/plot_fem_macro_local_iter_generic_v1.py` の compatibility surface
のまま維持し、export logic と output contract は変えない。
Phase 1H-d-b では、mixed macro-local golden exporter family を
`tools/exporters/golden_examples/mixed_macro_local/` に寄せ、旧
`scripts/export_mixed_macro_local_*.sh` paths は compatibility wrapper と
して残す。dependency checker call は意図的に
`scripts/check_mixed_macro_local_*.sh` の compatibility surface のまま
維持し、canonical mixed bounded route の first-class feedback は引き続き
`gamma_n` のみで、`k_contact_eff` / `mu_eff` は route-level return に
導入しない。export logic と output contract は変えない。
Phase 1H-d-c では、same-time golden exporter を
`tools/exporters/golden_examples/same_time/` に寄せ、旧
`scripts/export_same_time_golden_example.sh` path は compatibility wrapper
として残す。dependency call は意図的に
`scripts/run_same_time_file_exchange.sh`,
`scripts/run_external_stub_local_once.sh`,
`scripts/run_external_stub_ehl_once.sh`,
`scripts/validate_same_time_request_csv.py`,
`scripts/validate_same_time_response_csv.py` の compatibility surface のまま
維持し、same-time file-exchange protocol と validator behavior と output
contract は変えない。
Phase 1I-a では、FEM/MBD reduced-feedback structural exporters を
`tools/exporters/reduced_feedback/` に寄せ、旧
`scripts/export_*local_feedback_reduced_from_local_oneway_generic.py` paths は
compatibility wrapper として残す。export logic、aggregation semantics、
output CSV contract は変えず、これらの structural exporters は route-level
`k_contact_eff` / `mu_eff` return を導入しない。mixed reduced feedback
exporter は引き続き `tools/exporters/mixed_bounded/` に残す。
Phase 1I-b では、Year2 material/properties exporter を
`tools/exporters/year2/` に寄せ、旧
`scripts/export_year2_oneway_fem_material_properties_v1.py` path は
compatibility wrapper として残す。export logic、material/property parsing
semantics、output artifact contract は変えず、EHL mainline integration は
行わない。
Phase 1J-a では、same-time EHL stub helper family を `tools/stubs/ehl/` に
寄せ、旧
`scripts/build_same_time_stub_ehl_responses.py`,
`scripts/run_external_stub_ehl_once.sh`,
`scripts/stub_ehl_solver_from_request.py`,
`scripts/stub_ehl_solver_from_request_only.py` paths は compatibility wrapper
として残す。stub logic、same-time protocol、output CSV / marker / origin
JSON contract は変えず、shell helper の dependency call は意図的に old
`scripts/stub_ehl_solver_*` compatibility surface のまま維持する。EHL
mainline integration は行わない。
Phase 1J-b では、Year2 EHL `mu_eff` bridge / consumer helper family を
`tools/bridges/ehl/` に寄せ、旧
`scripts/run_year2_ehl_to_mainline_mu_eff_bridge_v1.py` と
`scripts/run_year2_ehl_to_mainline_mu_eff_consumer_v1.py` paths は
compatibility wrapper として残す。bridge logic、consumer logic、output
artifact contract は変えず、bridge は separate lane に留め、
`gamma_n` は first bridge scope 外、`k_contact_eff` は
keep-current-mainline policy のままとする。EHL mainline integration は
行わない。
Phase 1K-a では、same-time request / response validator pair を
`tools/validators/same_time/` に寄せ、旧
`scripts/validate_same_time_request_csv.py` と
`scripts/validate_same_time_response_csv.py` paths は compatibility wrapper
として残す。validator logic、`same_time_file_exchange_v1` semantics、
request / response CSV contracts、output text surface は変えない。
Phase 1K-b では、same-time runner/helper cluster を
`tools/runners/same_time/` と `tools/helpers/same_time/` に寄せ、旧
`scripts/run_same_time_file_exchange.sh`,
`scripts/build_same_time_stub_responses.py`,
`scripts/build_same_time_stub_local_responses.py`,
`scripts/stub_local_solver_from_request.py`,
`scripts/stub_local_solver_from_request_only.py`,
`scripts/build_same_time_local_patch_mvp_from_request.py`,
`scripts/run_external_stub_local_once.sh` paths は compatibility surface と
して残す。`scripts/build_same_time_stub_responses.py` は
import-compatible shim のまま維持し、runner logic、helper logic、
`same_time_file_exchange_v1` semantics、request / response CSV contracts、
marker contracts は変えない。
Phase 1L-a では、manifest bridge builder / helper pair を
`tools/builders/manifests/` と `tools/helpers/manifest_bridge/` に寄せ、旧
`scripts/build_mbd_master_from_manifest.py` と
`scripts/run_mbd_2link_rigid_from_bulk.sh` paths は compatibility wrapper と
して残す。builder logic、helper logic、generated deck semantics、runtime
summary semantics、result summary semantics は変えない。
`pack_clean_repo.sh` と `test_build_mbd_master_from_manifest.sh` はこの phase
では移さない。

Phase 1L-b では、repo packaging helper `scripts/pack_clean_repo.sh` を
`tools/helpers/repo_ops/pack_clean_repo.sh` に寄せ、旧
`scripts/pack_clean_repo.sh` path は compatibility wrapper として残す。
packaging / cleanup behavior、exclusion roots、backup filtering semantics、
generated archive semantics は変えない。
`test_build_mbd_master_from_manifest.sh` はこの phase でも移さない。

Phase 1M-a では、manifest-bridge smoke/check companion pair を
`tools/checkers/smoke/manifest_bridge/` に寄せ、旧
`scripts/test_build_mbd_master_from_manifest.sh` と
`scripts/check_mbd_2link_from_bulk_smoke.sh` paths は compatibility wrapper
として残す。checker logic、generated deck semantics、runtime completion
semantics は変えない。stricter bridge/result checkers、
parser-to-manifest bridge checker、`scripts/analyze_mbd_2link_history.py` は
この phase では移さない。

Phase 1M-b では、stricter manifest-bridge bridge/result checker cluster を
`tools/checkers/bridges/manifest_bridge/` に寄せ、旧
`scripts/check_mbd_2link_ground_revolute_enforcement.sh`,
`scripts/check_mbd_2link_rigid_from_bulk_ground0.sh`,
`scripts/check_mbd_2link_rigid_result_surface.sh`,
`scripts/test_parser_multipart_manifest_to_mbd.sh` paths は compatibility
wrapper として残す。checker logic、tolerance logic、generated deck
semantics、runtime completion semantics、result summary semantics は変えない。
`scripts/analyze_mbd_2link_history.py` はこの phase でも移さない。

Phase 1N-a では、manifest-bridge analyze/report utility
`scripts/analyze_mbd_2link_history.py` を
`tools/reports/manifest_bridge/analyze_mbd_2link_history.py` に寄せ、旧
`scripts/analyze_mbd_2link_history.py` path は compatibility wrapper として
残す。analysis logic、summary JSON schema、stdout summary semantics は変えない。
`tools/checkers/bridges/manifest_bridge/check_mbd_2link_ground_revolute_enforcement.sh`
は引き続き old `scripts/analyze_mbd_2link_history.py` compatibility surface
を dependency として使ってよい。
Phase 1N-b では、FEM macro-local plot/report utility
`scripts/plot_fem_macro_local_iter_generic_v1.py` を
`tools/reports/fem_macro_local/plot_fem_macro_local_iter_generic_v1.py` に寄せ、
旧 `scripts/plot_fem_macro_local_iter_generic_v1.py` path は compatibility
wrapper として残す。plot/report logic、output PNG / CSV / JSON semantics、
stdout PASS semantics は変えない。local-patch plotter dependency
compatibility は preserved され、already-moved
local-patch/contact-review/compare wrapper surfaces は再移動しない。
Phase 1O then closes out the moved Phase 1 support-tooling families in
`docs/phase1o_tools_taxonomy_closeout_v1.md`. Older phase taxonomy docs remain
historical planning notes, while the closeout summary plus the current
`tools/` tree become the current-truth source for moved release-lane tooling.

## proposed target layout

release 後に目指す target layout は次です。

```text
solver/
  common/
  fem/
    core/
    elements/
    io/
    contact/
  mbd/
    core/
    constraints/
    output/
  local_contact/
    patch/
    roughness/
    postprocess/
  ehl/
    case_builder/
    reynolds/
    coupled/
    report/
  coupling/
    oneway/
    lagged/
    same_time/
    mixed_bounded/
  apps/
    fem4c_cli/

tools/
  builders/
  stubs/
  bridges/
  runners/
  exporters/
  replays/
  checkers/
  reports/
  migration/

docs/
  architecture/
  routes/
  contracts/
  closeout/

examples/
  year2/
  ehl/
  mixed_bounded/

tests/
  smoke/
  regression/
  contracts/
```

この layout の意図は次です。

- FEM / MBD / local_contact / EHL を solver-level の並列 module として読む
- one-way / lagged / same_time / mixed_bounded は coupling module として分離する
- Python / shell の支援層は `tools/` 側に寄せる
- docs / examples / tests も route / contract / closeout / lane ごとの導線で整理する
- Phase 1 では release command compatibility を壊さないため、旧 `scripts/` path を段階的 wrapper として維持する

## solver core vs tooling boundary

solver core は native implementation として維持する。

- solver core の主語は `src/` 配下の native source
- runtime / analysis / core trace は native path を基準にする
- Python / shell は support tooling に置く

release 後も boundary は次で固定する。

- Python runtime を solver core に埋め込まない
- solver から Python callback を要求しない
- Python を product contract の主語にしない
- support tooling は orchestration / export / replay / checker / report に限定する

この boundary は `solver_core_vs_tooling_boundary_v1.md` と整合させたまま維持する。

## Python policy

Python policy は次です。

- Python / shell は support tooling
- solver core は native implementation
- Python は reusable support layer として残してよい
- ただし solver core execution path の必須要素にしない
- release branch では Python を solver core に近づける変更をしない

これは Year2 one-way / lagged / same-time、mixed bounded、EHL separate lane の全てで共通の方針とする。

## migration phases

### Phase 0

- release branch は現状維持
- docs で boundary を固定
- 実ファイル移動なし

### Phase 1

- tools taxonomy を導入
- `scripts/` を runner / exporter / replay / checker / report に分類
- compatibility wrapper を残す
- existing commands を壊さない
- EHL separate lane の package builder / runner も `tools/` taxonomy に寄せる

### Phase 2

- native source の module split 設計
- `src/mbd/system2d.c`
- `src/io/input.c`
- `src/analysis/static.c`
  など巨大 source の分割方針を設計
- parser / input / system の責務分離案を作る

Phase 2A は planning only とする。

- solver core は native implementation のまま扱う
- Python / shell は support tooling のままとする
- C file split はまだ実行しない
- function move はまだ実行しない
- header split はまだ実行しない
- build system change はまだ実行しない

Phase 2B では `src/coupled/coupled_run2d.c` を first C split dry-run target
として docs-only で扱う。

- Phase 2B も planning only
- C file split / function move / header split / build system change はまだ実行しない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2C では `src/coupled/coupled_run2d.c` split proposal に対する
verification planning を docs-only で固定する。

- Phase 2C も planning only
- C file split / function move / header split / build system change はまだ実行しない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2E では Phase 2D 後の `coupled_run2d` output / summary / artifact
contract と downstream checker dependency を read-only / docs-only で監査する。

- Phase 2E も read-only / docs-only
- additional C split / function move / header split / build system change は行わない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2F では `coupled_run2d` artifact-sensitive surfaces に対する
checker / guard hardening plan を docs-only で固定する。

- Phase 2F も docs-only
- additional C split / function move / header split / build system change は行わない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2G では `coupled_run2d` output/report surface に対する
first checker-hardening subset を scripts/checkers 側へ最小実装する。

- Phase 2G では checker/script edit のみを扱う
- additional C split / function move / header split / build system change は行わない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2H では Phase 2F plan と Phase 2G actual checker coverage を
read-only / docs-only で照合し、next split planning の readiness を再固定する。

- Phase 2H も read-only / docs-only
- C split / function move / checker/script edit / header split / build system change は行わない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う

Phase 2I では `coupled_run2d` route-metric / compare-schema helper cluster を
next safe split planning target として docs-only で固定する。

- Phase 2I も docs-only
- C edit / function move / checker/script edit / header split / build system change は行わない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う
- future C edit は explicit user approval を必須とする

Phase 2J では `coupled_run2d` route-metric / compare-schema helper cluster の
narrow actual split を `src/coupled/coupled_run2d_route_meta.c` へ適用した。

- Phase 2J では move-first helper set だけを移動した
- public API 拡張は行わない
- private internal header 追加も行わない
- writer / wording / fallback / dispatch / orchestration は動かさない
- solver core は native implementation のまま維持する
- Python / shell は solver core に組み込まない
- monolithic は Year1 では実装済み
- monolithic は Year2 current goal では scope 外
- 既存 Year1 monolithic behavior は preserve-only として扱う
- Phase 2J route-meta split は required verification matrix が通った場合にのみ受理する

Phase 2K では Phase 2J route-meta split を read-only / docs-only で監査し、
remaining `later` / `no-split-yet` boundary を再固定する。

- Phase 2K は read-only / docs-only
- no additional C edit is performed
- no function move is performed
- no checker/script edit is performed
- no header split is performed
- no build system change is performed
- solver core remains native implementation
- Python / shell are not solver core
- Year1 monolithic behavior is preserve-only
- Year2 monolithic remains out of scope
- Phase 2J route-meta split is accepted only because required verification matrix passed

Phase 2L に進む前に、git provenance audit の結果に基づく checkpoint grouping
freeze を先に行う。

- Phase 2L is blocked until checkpoint grouping is resolved
- current git state is cumulative, not phase-local
- checkpoint grouping must precede further C splitting
- implementation candidate-only checkpoint was not feasible
- further C splitting must wait for intentional implementation checkpoint resolution
- no commit is performed in this phase
- no staging is performed in this phase

docs checkpoint と intentional Phase 2 implementation checkpoint 完了後、
Phase 2L は docs-only planning としてのみ再開可能になった。

- Phase 2L is docs-only
- no C edit is performed
- no function move is performed
- no checker/script edit is performed
- no build system change is performed
- current dirty state still blocks actual C edit without explicit approval and checkpoint discipline
- Phase 2M is docs-only and fixes the future exact guard path for `step_flex_counter_columns`
- Phase 2N hardens route-aware exact `step_flex_counter_columns` guards in the Year1 route matrix checker
- Phase 2O performs the first actual writer-meta helper move for `coupled_run2d_step_flex_iteration_column_name`
- Phase 2P is docs-only and accepts the Phase 2O split only if the earlier full verification matrix stayed green
- Phase 2Q is docs-only and fixes the checkpoint plan for the accepted Phase 2N / 2O / 2P worktree-only family
- further actual C edit should prefer checkpoint discipline for the Phase 2N / 2O worktree surfaces before another split is attempted
- further actual C edit should wait for an explicit-path checkpoint commit of that family unless the user explicitly accepts the added provenance risk
- future checker/script edit requires explicit user approval
- future C edit requires explicit user approval
- solver core remains native implementation
- Python / shell are not solver core
- Year1 monolithic behavior is preserve-only
- Year2 monolithic remains out of scope

### Phase 3

- target layout への実移動
- build system / include path / tests 更新
- compatibility wrapper を段階的に廃止

## release branch policy

release branch policy は次で固定する。

- current release-ready branch では layout change を実行しない
- docs 先行で migration plan を固定する
- 実移動は専用 phase / branch で行う
- wrapper 互換と route contract を壊す変更は release branch に載せない
- one-way / lagged / same-time の release-ready baselines を基準線として守る
- mixed bounded canonical route first cut と EHL separate lane も現状 contract のまま扱う

## risks

主な risk は次です。

- directory taxonomy 導入時に wrapper 互換が壊れる
- `scripts/` から `tools/` への移行で既存 command path が崩れる
- `src/mbd/system2d.c` / `src/io/input.c` / `src/analysis/static.c` 分割時に責務境界の切り方を誤る
- parser / input / system 分離で入出力 contract の実質変更を混入させる
- docs 再配置で route / contract / closeout の参照導線が一時的に悪化する
- release-ready branch と migration branch の差分管理が曖昧になる

したがって Phase 0 と Phase 1 では、構造改善より互換維持を優先する。

## non-goals

今回の non-goals は次です。

- 今回はファイル移動しない
- 今回は directory rename しない
- 今回は build system を触らない
- 今回は parser 実装を分割しない
- 今回は solver / C source の挙動変更をしない
- Python を solver core に組み込まない
- EHL mainline integration を今回進めない
- monolithic を current goal に戻さない
- new analytical solver features を追加しない
