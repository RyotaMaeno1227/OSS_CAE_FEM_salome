# Review Spec Priority Plan

最終更新: 2026-03-08
正本レビュー仕様: `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md`

## 1. この文書の目的

この文書は、`FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md` を
**今後の最優先方針**として採用したうえで、Codex / 各チームが次の 3 run をどう進めるかを固定する。

以後、root 側の queue で older task が `In Progress` のまま残っていても、
**M1 rigid closure までの優先順位はこの文書が上位**とする。

## 2. 採用する判断

以下は review spec に同意し、そのまま採用する。

1. build は green だが warning hygiene が未閉塞である。
2. `M1 rigid 2-link` completion を最優先に閉じる。
3. 新しい wrapper / resilience pack の増設は一時停止する。
4. `src/mbd/system2d.c` / `src/io/input.c` / `src/coupled/coupled_run2d.c` の monolith 問題は認識するが、全面分割は後回しにする。
5. 1-link flexible の meaningful case を先に作り、2-link flexible compare 強化の前に reaction/mapping を見えるようにする。
6. 学習 docs は後回しではなく、最小 2-3 本の骨格だけ先に作る。

## 3. 差分認識

review spec と current repo を照合したときの差分は次。

1. E-team は source tree / queue が formal accepted を先行している。
   - formal accepted は `E-13` までとみなす。
   - `E-14` 以降は provisional とみなす。
2. D-team は formal accepted が `D-23` まで確認できる。
3. A/B の current open task は queue 上にあるが、review spec 優先期間中はそのまま default 進行に使わない。

## 4. review-spec 採用中の凍結事項

`PM-05` が閉じるまで、次を凍結する。

- 新規 large wrapper / resilience pack 増設
- contact / friction / 3D
- COM / inertia update coupling
- large monolith full split
- 2-link flex external compare 演出の先行

## 5. 次の 3 run

## Run 1: P0 hygiene + priority reset

### PM-R1
- Goal: review spec 採用を queue / handoff / runbook に固定する。
- Scope:
  - `docs/fem4c_team_next_queue.md`
  - `docs/abc_team_chat_handoff.md`
  - `docs/team_runbook.md`
  - `docs/10_review_spec_priority_plan.md`
- Acceptance:
  - review spec が最優先であることが docs から読める。
  - older open task より Run 1 task が優先であることが明記される。

### A-R1
- Goal: implicit logging / history label の stale `newmark_*` 命名を中立化し、M1 compare 用の出力語彙を揃える。
- Scope:
  - `FEM4C/src/mbd/output2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/practice/ch09/*`
- Acceptance:
  - HHT 実行でも stale な `newmark_*` ラベルが主要 summary に残らない。
  - `implicit_result` 系の中立ラベルで compare / history を読める。

### Run 1 current A-team self-test entrypoint
- PM が A-team の再開点と focused self-test entrypoint を docs だけで追跡する時は、まず `docs/fem4c_team_next_queue.md` の current A-task を見てから次の 3 コマンドを辿る。
- `make -C FEM4C mbd_system2d_history_contract_smoke`
  history-only current command surface。generalized-force history の probe + CLI/system summary contract だけを bundle として確認する。
- `make -C FEM4C mbd_a_team_foundation_smoke`
  full foundation current command surface。history contract 再利用 bundle を含む rigid MBD foundation 全体を確認する。
- `make -C FEM4C mbd_run1_surface_docs_sync_test`
  focused self-test entrypoint。Run 1 review spec / runbook / acceptance matrix の A-team surface が上の 2 コマンドと矛盾していないことを確認する。
- `bash FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh --print-a-team-surface-summary`
  focused inspection surface。A-team の history/foundation/self-test entrypoint と `review-plan / runbook / acceptance / handoff / queue` を 1 コマンドで機械可読に取得する。

### B-R1
- Goal: `src/mbd/system2d.c` の `fclose` 起因 warning を局所修正で解消する。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - `-Wuse-after-free` warning が消える。
  - runtime behavior は変えない。

### C-R1
- Goal: Q4/T3 の stiffness function pointer warning を adapter または整合 wrapper で解消する。
- Scope:
  - `FEM4C/src/elements/q4/q4_element.c`
  - `FEM4C/src/elements/t3/t3_element.c`
  - 必要時のみ `FEM4C/src/elements/t6/t6_element.c`
- Acceptance:
  - `make clean && make -j2` で Q4/T3 の incompatible pointer warning が消える。

### D-R1
- Goal: 1-link flexible meaningful case の最小骨格を作る。
- Scope:
  - `FEM4C/examples/`
  - `FEM4C/src/coupled/flex_snapshot2d.*`
  - 必要時のみ `FEM4C/scripts/compare_2link_flex_reference.py`
- Acceptance:
  - nonzero load / nonzero reaction を観測できる 1-link case の input / observation point が定義される。

### E-R1
- Goal: default acceptance path を M1/M2 に必要な最小コアへ絞り、extra wrapper を non-default 扱いにする。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/README.md`
  - `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - default path に残す target と non-default target が明文化される。
  - gate / resilience pack の default 実行を一時停止する方針が docs で読める。

## Run 2: M1 rigid closure

### PM-R2
- Goal: M1 formal acceptance 条件を 1 本に凍結する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/04_2d_coupled_scope.md`
- Acceptance:
  - rigid main case
  - rigid analytic compare
  - top-level rigid acceptance target
  の 3 点が formal route として固定される。

### A-R2
- Goal: non-trivial rigid case で必要な output/history field を compare-ready に揃える。
- Scope:
  - `FEM4C/src/mbd/output2d.*`
  - `FEM4C/examples/mbd_2link_rigid_dyn.dat`
- Acceptance:
  - rigid analytic compare に必要な field が stable に出る。

### B-R2
- Goal: HHT-alpha step を non-trivial rigid 2-link で formal closure する。
- Scope:
  - `FEM4C/src/mbd/integrator_hht2d.c`
  - `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - rigid 2-link の HHT run が 1 本の formal acceptance へ接続できる。

### E-R2
- Goal: rigid analytic compare を含む top-level rigid acceptance target を 1 本に縮退する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - 必要時のみ `FEM4C/scripts/run_mbd_regression.sh`
- Acceptance:
  - M1 の default acceptance route が 1 本で説明できる。

## Run 3: M2 meaningful pass + debt cleanup

### PM-R3
- Goal: 学習 docs の最小初手 3 本を凍結する。
- Scope:
  - `docs/MBD_00_learning_map.md`
  - `docs/MBD_01_rigid_body_2d_basics.md`
  - `docs/MBD_02_constraints_and_jacobians.md`
- Acceptance:
  - 学習順と 1問1答の出題範囲が docs から読める。

### C-R2
- Goal: `coupled_step_common2d.{c,h}` の helper 抽出を開始し、explicit/implicit 共通部の重複を 1 つ減らす。
- Scope:
  - `FEM4C/src/coupled/coupled_step_common2d.h`
  - `FEM4C/src/coupled/coupled_step_common2d.c`
  - `FEM4C/src/coupled/coupled_step_explicit2d.c`
  - `FEM4C/src/coupled/coupled_step_implicit2d.c`
- Acceptance:
  - node set build / pose capture / reaction apply のうち少なくとも 1 系統が共通 helper 化される。
  - behavior change を伴わない。

### D-R2
- Goal: 1-link meaningful case を reaction mapping artifact まで閉じる。
- Scope:
  - `FEM4C/examples/`
  - `FEM4C/src/coupled/flex_reaction2d.*`
  - 必要時のみ `FEM4C/scripts/compare_2link_flex_reference.py`
- Acceptance:
  - nonzero reaction が artifact で確認できる。
  - 1-link case を M2 の main debug case として再利用できる。

### E-R3
- Goal: stub legacy を default path から外し、actual coupled path 優先の docs / target surface へ整理する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/src/coupled/coupled_run2d.c`
  - `FEM4C/README.md`
- Acceptance:
  - default path で stub contract を前提にしない。
  - legacy stub の位置付けが限定される。

## 6. default path に残すもの / 外すもの

### 残すもの
- top-level rigid acceptance 1 本
- rigid analytic compare 1 本
- flex compare 1 本
- stale-binary guard 1 本
- lightweight smoke 1 本

### default path から外すもの
- stub contract 系
- bundle-of-bundle wrapper
- docs/queue/manifest の meta-contract 群
- resilience pack の追加増殖

## 7. 学習 docs の最小初手

最初に作るのは次の 3 本に限定する。

1. `docs/MBD_00_learning_map.md`
2. `docs/MBD_01_rigid_body_2d_basics.md`
3. `docs/MBD_02_constraints_and_jacobians.md`

ここでは次だけを定義する。
- 学習順
- 1問1答の出題範囲
- 理論 -> code file 対応
- Project Chrono 概念への橋渡し

## 8. チームへの運用指示

- PM から「作業してください」だけ来た場合でも、この文書を最優先で適用する。
- review-spec 採用期間中は、older open task より `Run 1 -> Run 2 -> Run 3` の順を優先する。
- 1 run = 1 merge reason を守る。
- core change と wrapper change を同じ run に混ぜない。
