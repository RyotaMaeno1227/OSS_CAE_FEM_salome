# Release Support Decision v1

## 目的

これは CT_240b の dry-run packaging で残った undecided items 5 本について、
Proposal A のまま release policy 上の結論を固定する文書である。

今回の前提は次である。

- final release artifact には `.sh` を含めない
- repo には internal tooling を残す
- solver capability claim は増やさない
- generic local proxy solver を final release の user-facing surface として約束しない

## 対象 5 本の判定

| file/path | current role | user-facing flow dependency | can stay internal? | include in final release? | why | migration cost if excluded | risk if included |
|---|---|---|---:|---:|---|---|---|
| `scripts/build_mbd_master_from_manifest.py` | parser-produced manifest から MBD `.dat` を組む bridge | parser/manifest -> generated MBD deck flow を final release に含める場合のみ依存 | yes | no | Proposal A の minimal release は C solver core + minimal docs/examples に寄せる。current release candidate では parser 出力から MBD deck を組む Python bridge まで user-facing に約束しない | medium | medium |
| `scripts/validate_local_patch_generic_request.py` | generic local request contract validator | generic local proxy solver surface を end-user release に残す場合のみ依存 | yes | no | current release policy では generic local proxy tooling は internal-only。validator も internal-only に揃えるのが整合的 | low | low |
| `scripts/validate_local_patch_generic_response.py` | generic local response contract validator | generic local proxy solver surface を end-user release に残す場合のみ依存 | yes | no | request validator と同じ。release artifact には不要 | low | low |
| `scripts/run_local_patch_generic_contract_v1.py` | generic local contract demo / runner helper | local proxy solver demo を final release で user-facing にする場合のみ依存 | yes | no | demo helper であり final runtime surface ではない。Proposal A では internal-only が妥当 | low | low |
| `scripts/local_patch_generic_solver_v1.py` | proxy / MVP local solver 本体 | final release に generic local proxy solver surface を残す場合は依存 | yes | no | current policy では C solver core を final release の中心に据える。proxy / MVP local solver は capability claim を増やさず internal-only に留める | high | medium |

## file-by-file conclusion

### 1. `scripts/build_mbd_master_from_manifest.py`

結論:

- internal-only
- final release には含めない

理由:

- current top-level README では parser -> manifest -> generated MBD deck flow を案内している
- ただし Proposal A の release boundary では、その Python bridge を release support として持つかが唯一の分岐点だった
- current recommendation では、その bridge も internal に落とす
- したがって parser は final release に残しても、parser-generated manifest から MBD `.dat` を組む flow は final release の約束から外れる

### 2. `scripts/validate_local_patch_generic_request.py`

結論:

- internal-only
- final release には含めない

理由:

- generic local request validation は local proxy / MVP workflow の一部
- final release surface が C solver core 中心である以上、validator を user-facing release に残す理由は薄い

### 3. `scripts/validate_local_patch_generic_response.py`

結論:

- internal-only
- final release には含めない

理由:

- request validator と対になる internal QA tooling
- current release candidate に含めると proxy local branch を user-facing に見せるため避ける

### 4. `scripts/run_local_patch_generic_contract_v1.py`

結論:

- internal-only
- final release には含めない

理由:

- contract demo / helper runner であり final runtime surface ではない
- release に入れると local proxy surface を support 対象に見せる

### 5. `scripts/local_patch_generic_solver_v1.py`

結論:

- internal-only
- final release には含めない

理由:

- proxy / MVP local solver であり、current branch でも exact local solver とは主張していない
- final release に含めると user-facing capability claim が過大になりやすい
- C 置換は高コストで、Proposal A の意図にも合わない

## Proposal A の二択

### A1

parser/manifest bridge だけは release support に残す。

- `scripts/build_mbd_master_from_manifest.py` のみ final release に残す
- local validator / local proxy solver 系 4 本は internal-only

長所:

- parser -> manifest -> generated MBD deck の current user flow を維持しやすい

短所:

- final release artifact に Python support script が 1 本残る
- Proposal A の minimal release 境界がやや曖昧になる

### A2

parser/manifest bridge も internal に落とす。

- 対象 5 本をすべて internal-only
- final release artifact は C solver core + build surface + minimal docs/examples に限定

長所:

- release boundary が最も明快
- `.sh` を含めない方針と同じ論理で、research `.py` も final release から外しやすい
- current capability claim を増やさない

短所:

- parser-produced manifest から generated MBD deck を作る flow は final release の外に出る
- top-level release-facing docs は後続 task でその前提に寄せる必要がある

## final recommendation

main recommendation は `A2` とする。

### 推奨理由

- Proposal A の目的は release artifact を最小化し、solver core を C に保ち、tooling は repo に残すことにある
- 5 本のうち 1 本だけ release support に残すと boundary が再び曖昧になる
- current branch は MBD/FEM/mixed の orchestration を多数の `.py` / `.sh` に依存しているため、final release ではむしろ明確に internal-only へ寄せた方が整理しやすい
- generic local validator / solver 系 4 本を internal に落とす判断と、`build_mbd_master_from_manifest.py` を internal に落とす判断は同じ release philosophy で揃う

## remaining open decisions

file-level undecided は今回で解消する。

残る open decision は packaging policy ではなく documentation migration である。

- top-level `README.md` の parser -> manifest -> generated MBD deck 例を、将来の release-facing docs でどう扱うか
- internal-only docs を `docs/` から論理分離するか

## rough migration cost

- current 5 本をすべて internal-only として固定するコスト: `low`
- 将来 A1 に振り直して `build_mbd_master_from_manifest.py` を release support に戻すコスト: `medium`
- local proxy solver 系を final release に含める方向へ振るコスト: `high`
