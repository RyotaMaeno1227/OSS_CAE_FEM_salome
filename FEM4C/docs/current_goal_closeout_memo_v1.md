# Current Goal Closeout Memo

## Completion Declaration

current goal は完了扱いとして閉じてよい。

この closeout memo は、current goal の completion declaration を 1 本に固定するための status memo である。  
long-term target spec や将来 lane の設計拡張をここで再定義しない。

## Scope In

- one-way
- lagged
- same-time

## Scope Out

- monolithic
- EHL mainline integration

## Contract Baseline

- route-level reduced return は `k_contact_eff` / `mu_eff` のまま固定する
- local contact は post-solve artifact のまま固定する
- friction solver state は solver internal state のまま固定する
- EHL は separate lane として扱う

## Route Status

- one-way: baseline closed。internal kernel / 基準線として扱う
- lagged: accepted main route。freeze-ready
- same-time: freeze-ready
- monolithic: out of scope

## Current Goal Reading

current goal は、one-way を基準線として閉じ、lagged を accepted main route として凍結し、same-time を freeze-ready baseline として閉じた段階として読む。  
monolithic は current goal の完成条件に含めない。

## Next Phase Candidates

- 候補A: mixed / bounded route の quality / failure visibility parity
- 候補B: EHL lane の standalone closeout を mainline integration とは切り分けて進める
