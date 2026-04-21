# canonical_bounded_route_closeout_memo_v1

## completion declaration

canonical bounded route first cut is closed.

current workspace では、mixed family を canonical bounded route として読む。
独立の `bounded_*` runner を live route の正本とはしない。

## scope in

current closeout の scope in は次の 3 本である。

- mixed one-way local execution
- mixed reduced feedback export
- mixed split replay consume

current live mapping は次で固定する。

- `scripts/run_mixed_macro_local_oneway_generic_v1.sh`
- `scripts/export_mixed_local_feedback_reduced_generic.py`
- `scripts/run_mixed_macro_local_replay_split_generic_v1.sh`

tooling taxonomy の観点では、これらの旧 `scripts/` path は compatibility wrapper として維持してよい。
実体を `tools/runners/mixed_bounded/`、`tools/exporters/mixed_bounded/`、
`tools/checkers/mixed_bounded/` に寄せても canonical bounded route の contract は変わらない。

canonical bounded route は `mixed_macro_local_replay_split_generic_v1` である。
理由は、mixed one-way local execution と reduced feedback export を upstream に含み、
bounded consume semantics を reviewer-facing に一番よく代表するからである。

## scope out

current closeout の scope out は次である。

- independent `bounded_*` runner の新設
- `k_contact_eff` / `mu_eff` の mixed bounded route-level return 化
- monolithic
- EHL mainline integration

## contract baseline

mixed bounded route の current contract baseline は次で固定する。

- first-class feedback は `gamma_n` のみ
- `k_contact_eff` / `mu_eff` は mixed bounded route の route-level return ではない
- local contact は post-solve artifact
- friction solver state は internal state

したがって、mixed bounded route の parity/hardening は
return contract を増やすことではなく、
bounded orchestration / export / replay consume の見え方を揃えることを意味する。

## route-by-route status

### mixed one-way local execution

- status: closeout 済み
- role: bounded local execution の upstream source
- no-feedback route
- machine-readable 主役:
  - `mixed_oneway_summary.json`
  - `mixed_oneway_summary.md`
  - `mixed_bundle/provenance_manifest.json`

### mixed reduced feedback export

- status: closeout 済み
- role: replay-ready reduced feedback surface の export
- export-only route
- machine-readable 主役:
  - `mixed_feedback_reduced_summary.json`
  - `mixed_feedback_reduced_manifest.json`

### mixed split replay consume

- status: closeout 済み
- role: canonical bounded route
- consume semantics を reviewer-facing に固定する current representative route
- machine-readable 主役:
  - `mixed_split_replay_summary.json`
  - `mixed_split_replay_summary.md`
  - `split_replay_handoff_manifest.json`

## route meaning

この family は time-coupled co-sim route ではない。
bounded reduced-key orchestration / export / replay consume route として読む。

したがって top-level semantics は

- 何を export したか
- 何を consume したか
- fallback がどこで起きたか
- 最後に available だった reduced row / applied された reduced row がどれか

を中心に固定する。

## next phase candidates

- 候補A: mixed bounded route の upper-doc status sync を追加し、current workspace 全体の route map に統合する
- 候補B: mixed bounded route の reviewer-facing target-key memo を独立 closeout doc に圧縮する
