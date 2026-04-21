# Year2 current goal release

## release title

Year2 current goal release

## scope in

- one-way
- lagged
- same-time

## scope out

- monolithic
- EHL mainline integration

## route status

- one-way: baseline closed
- lagged: accepted main route, freeze-ready
- same-time: freeze-ready

## contract baseline

- route-level reduced return: `k_contact_eff` / `mu_eff`
- local contact: post-solve artifact
- friction solver state: solver internal state

## mixed / bounded note

- canonical bounded route first cut closed
- first-class feedback remains `gamma_n`
- `k_contact_eff` / `mu_eff` are not introduced as mixed bounded route-level returns

## EHL note

EHL remains separate lane.

## tooling boundary

- Python / shell are support tooling
- Python is not embedded into solver core
- solver core remains native implementation

## checks passed

- `make -j`
- one-way checker
- lagged checker
- same-time checker
- mixed bounded checker

## known non-goals

- monolithic
- EHL mainline integration
- solver-core Python integration
- new analytical solver features
