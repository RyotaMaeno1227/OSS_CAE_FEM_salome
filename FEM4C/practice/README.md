# Practice Exercises

The tutorial manual refers to these scaffolding files so that each chapter can be tried directly in C.

- `ch01/hello.c`: minimal CLI program and starter for command line parsing.
- `ch02/penalty.c`: 1D two-spring penalty example with small consistency checks.
- `ch03/t3_shape.c`: shape functions, Jacobian helpers, and a simple numerical regression.
- `ch04/t3_stiffness.c`: B, D, and element stiffness computation for a unit T3 element.
- `ch05/assembly.c`: dense-assembly demo with Dirichlet handling.
- `ch06/cg.c`: conjugate-gradient solver for a small SPD system.
- `ch07/t6_shape.c`: quadratic triangle shape functions and gradients.
- `ch08/t6_body_force.c`: body-force load integration with 3-point Gauss.
- `ch09/native_probe.c`: native input quick scan (header and counts).
- `ch09/nastran_probe.c`: Nastran BULK quick scan (card counts).
- `ch09/mbd_constraint_probe.c`: MBD distance/revolute residual and Jacobian finite-difference check (`eps=1e-7`, `|analytic-fd| <= 1e-6`, 2 states).
- `ch09/check_mbd_mode_equations.sh`: compare probe equation count with `--mode=mbd` runtime log (`constraint_equations`).

Build the programs with `gcc -Wall -Wextra <file.c> -lm` and run the tests embedded in each `main`.

Example logs:
```
./native_probe examples/t3_cantilever_beam.dat
Title: T3 Cantilever Beam
Declared nodes: 297 (read 297)
Declared elements: 512 (read 512)

./nastran_probe NastranBalkFile/3Dtria_example.dat
GRID:   451
CTRIA3: 800
MAT1:   1
SPC:    11
FORCE:  1

# MBD constraint API verification (single command)
gcc -Wall -Wextra -std=c99 -Isrc practice/ch09/mbd_constraint_probe.c src/mbd/constraint2d.c src/mbd/kkt2d.c src/common/error.c -lm -o bin/mbd_constraint_probe && ./bin/mbd_constraint_probe

# Make target wrapper for probe
make -C FEM4C mbd_probe

# MBD runtime/log equation consistency check
cd FEM4C && ./practice/ch09/check_mbd_mode_equations.sh

# One-command MBD mode regression (builtin fallback + input case + negative diagnostics with stable error codes)
make -C FEM4C mbd_regression

# Equation-count consistency check (runtime log vs probe)
make -C FEM4C mbd_consistency

# Invalid-input diagnostics check (negative cases)
make -C FEM4C mbd_negative
# expected errors include stable diagnostic codes, e.g. MBD_INPUT_ERROR[E_DUP_BODY]
# script summary prints DIAG_CODES_SEEN=<comma-separated-codes>
# includes range/incomplete codes (e.g. E_BODY_RANGE, E_REVOLUTE_RANGE, E_INCOMPLETE_INPUT)

# Run all MBD checks in one command
make -C FEM4C mbd_checks

# Verify standalone MBD integrator/parameter contract in --mode=mbd
# checks default/env/cli source labels, range validation, and fallback
# includes time-control boundaries:
#   - env invalid/out-of-range: warning + default fallback
#   - CLI invalid/out-of-range: non-zero exit with explicit error
#   - env values with leading whitespace are treated as invalid
# runtime log includes:
#   - integrator_fallback:
#       newmark_beta=<default|cli|env|env_invalid_fallback|env_out_of_range_fallback>
#       newmark_gamma=<...>
#       hht_alpha=<...>
#   - time_control: dt=<...> steps=<...>
#   - time_fallback: dt=default|cli|env|env_invalid_fallback|env_out_of_range_fallback
#   - step trace: mbd_step=<k>/<N> ... + steps_trace: requested=<N> executed=<N>
# output `.dat` includes source status keys:
#   - newmark_beta_source_status/newmark_gamma_source_status/hht_alpha_source_status
#   - dt_source_status/steps_source_status
#   - steps_requested/steps_executed
# CLI options: --mbd-integrator, --mbd-newmark-beta, --mbd-newmark-gamma, --mbd-hht-alpha, --mbd-dt, --mbd-steps
# env vars: FEM4C_MBD_INTEGRATOR, FEM4C_MBD_NEWMARK_BETA, FEM4C_MBD_NEWMARK_GAMMA, FEM4C_MBD_HHT_ALPHA, FEM4C_MBD_DT, FEM4C_MBD_STEPS
# optional test-only override: FEM4C_MBD_BIN=<path-to-fem4c-binary>
#   - if path is missing/non-executable, checker fails fast with non-zero + explicit preflight error
# mbd-prefixed CLI options update only FEM4C_MBD_* keys (coupled env keys are not mutated)
make -C FEM4C mbd_integrator_checks
# `make -C FEM4C mbd_checks` and `make -C FEM4C test` also run mbd_integrator_checks

# Parser compatibility regression (legacy parser package + Nastran parser path)
make -C FEM4C parser_compat
# default old-parser input: FEM4C_OLD_PARSER_PKG -> /tmp/parser_pkg_old -> built-in fallback fixture
# force built-in fallback fixture: make -C FEM4C parser_compat_fallback
# parser_compat is serialized with a lock; concurrent runs fail fast to avoid run_out/part_0001 races
# `make -C FEM4C test` also runs parser_compat

# Fetch CI evidence with strict acceptance:
# threshold: step_present==yes && artifact_present==yes (otherwise non-zero exit)
make -C FEM4C mbd_ci_evidence
# Optional: inspect one known run directly to reduce API calls
make -C FEM4C mbd_ci_evidence RUN_ID=21772351026
# Optional: reduce API consumption under low quota
make -C FEM4C mbd_ci_evidence SCAN_RUNS=5
# rate-limit / network errors emit `CI_EVIDENCE_ERROR` with structured fields

# Validate CI contract locally (no run_id required)
make -C FEM4C mbd_ci_contract
# also checks wiring contract: `make -C FEM4C test` calls `mbd_checks`,
# and `mbd_checks` includes `mbd_integrator_checks`
# also checks CI log gate markers:
#   PASS: mbd integrator switch check
#   PASS: all MBD checks completed
# also checks MBD time-control boundary coverage markers in `check_mbd_integrators.sh`:
#   run_env_time_fallback_case / run_env_time_whitespace_case
#   run_cli_invalid_dt_case / run_cli_invalid_steps_case
#   time_fallback: dt=env_invalid_fallback steps=env_out_of_range_fallback
#   time_fallback: dt=env_invalid_fallback steps=env_invalid_fallback
# also checks source-status/fallback markers:
#   integrator_fallback: newmark_beta=cli newmark_gamma=cli hht_alpha=cli
#   newmark_beta_source_status,cli
#   dt_source_status,env_invalid_fallback
# also checks mbd integrator binary preflight markers:
#   FEM4C_BIN_DEFAULT/FEM4C_MBD_BIN override + preflight error message
#   mbd_step=1/3
#   steps_trace: requested=3 executed=3
#   steps_requested,3
#   steps_executed,3
#   run_env_time_compact_trace_case / mbd_step=... (14 steps omitted for compact trace)
# checker self-test (CI contract + log-marker checker, pass + expected fail cases)
make -C FEM4C mbd_ci_contract_test
# one-command A-21 regression (time/source static contract + self-tests + runtime checks)
make -C FEM4C mbd_a21_regression
# self-test for A-21 regression wrapper (pass + expected fail path)
make -C FEM4C mbd_a21_regression_test
# one-command A-24 regression (step-trace runtime/static contract + self-tests)
make -C FEM4C mbd_a24_regression
# run emits one machine-readable summary line:
#   A24_REGRESSION_SUMMARY contract_test=<0|1> lock=<not_used|acquired|acquired_stale_recovered|held|skipped> integrator_attempts=<n> ci_contract_attempts=<n> ci_contract_test_attempts=<n> overall=<pass|fail> failed_step=<...> failed_cmd=<...>
# optional: skip nested ci-contract self-test (useful for wrapper-focused smoke/debug)
A24_RUN_CONTRACT_TEST=0 make -C FEM4C mbd_a24_regression
# optional: skip regression lock acquisition (for nested wrapper orchestration/debug)
A24_REGRESSION_SKIP_LOCK=1 make -C FEM4C mbd_a24_regression
# optional: override regression lock path (default: /tmp/fem4c_a24_regression.lock)
A24_REGRESSION_LOCK_DIR=/tmp/custom_a24_regression.lock make -C FEM4C mbd_a24_regression
# optional: persist the same summary line to a file for report reuse
A24_REGRESSION_SUMMARY_OUT=/tmp/a24_regression_summary.log make -C FEM4C mbd_a24_regression
# self-test for A-24 regression wrapper (pass + expected fail path)
make -C FEM4C mbd_a24_regression_test
# one-command A-24 full regression from clean rebuild
make -C FEM4C mbd_a24_regression_full
# full run emits one machine-readable summary line:
#   A24_FULL_SUMMARY lock=<...> retry_on_137=<0|1> retry_used=<0|1> clean=<...> clean_attempts=<...> build=<...> build_attempts=<...> regression=<...> regression_attempts=<...> overall=<pass|fail> failed_step=<...> failed_cmd=<...>
# if nested `mbd_a24_regression` fails with its own summary marker, full wrapper propagates:
#   failed_step=regression_<nested_failed_step> / failed_cmd=<nested_failed_cmd>
# optional: retry rc=137 once per step (default `A24_FULL_RETRY_ON_137=1`, set `0` to disable)
A24_FULL_RETRY_ON_137=0 make -C FEM4C mbd_a24_regression_full
# optional: persist the same summary line to a file for report reuse
A24_FULL_SUMMARY_OUT=/tmp/a24_full_summary.log make -C FEM4C mbd_a24_regression_full
# self-test for A-24 full regression wrapper (pass + expected fail path)
make -C FEM4C mbd_a24_regression_full_test
# one-command A-24 serial batch (regression + tests)
make -C FEM4C mbd_a24_batch
# A-24 full/batch wrappers force `MAKEFLAGS=-j1` internally to avoid nested clean/build races.
# both wrappers also hold a lock under `/tmp/fem4c_a24_batch.lock` by default (override:
#   full: A24_FULL_LOCK_DIR, batch: A24_BATCH_LOCK_DIR, shared default via A24_SERIAL_LOCK_DIR)
# batch run emits one machine-readable summary line:
#   A24_BATCH_SUMMARY lock=<...> retry_on_137=<0|1> retry_used=<0|1> regression=<...> regression_attempts=<...> regression_test=<...> regression_test_attempts=<...> regression_full_test=<...> regression_full_test_attempts=<...> overall=<pass|fail> failed_step=<...> failed_cmd=<...>
# if nested `mbd_a24_regression` fails with its own summary marker, batch wrapper propagates:
#   failed_step=regression_<nested_failed_step> / failed_cmd=<nested_failed_cmd>
# optional: retry rc=137 once per sub-step (default `A24_BATCH_RETRY_ON_137=1`, set `0` to disable)
A24_BATCH_RETRY_ON_137=0 make -C FEM4C mbd_a24_batch
# optional: persist the same summary line to a file for report reuse
A24_BATCH_SUMMARY_OUT=/tmp/a24_batch_summary.log make -C FEM4C mbd_a24_batch
# self-test for A-24 batch wrapper (pass + expected fail path)
make -C FEM4C mbd_a24_batch_test
# one-command serial acceptance for A-24 wrappers (full_test -> batch_test -> ci_contract_test)
make -C FEM4C mbd_a24_acceptance_serial
# serial acceptance emits one machine-readable summary line:
#   A24_ACCEPT_SERIAL_SUMMARY lock=<...> retry_on_137=<0|1> fake_137_step=<none|full_test|batch_test|ci_contract_test> step_log_dir=<path|none> full_test=<pass|fail|skip> full_test_attempts=<n> full_test_retry_used=<0|1> batch_test=<pass|fail|skip> batch_test_attempts=<n> batch_test_retry_used=<0|1> ci_contract_test=<pass|fail|skip> ci_contract_test_attempts=<n> ci_contract_test_retry_used=<0|1> overall=<pass|fail> failed_step=<...> failed_cmd=<...> failed_rc=<n> failed_log=<path|none>
# optional: disable rc=137 retry
A24_ACCEPT_SERIAL_RETRY_ON_137=0 make -C FEM4C mbd_a24_acceptance_serial
# optional: inject deterministic rc=137 on first attempt of one step (for wrapper self-check)
A24_ACCEPT_SERIAL_FAKE_137_STEP=batch_test make -C FEM4C mbd_a24_acceptance_serial
# optional: persist the same summary line to a file for report reuse
# (summary path must be a writable file path under an existing directory)
A24_ACCEPT_SERIAL_SUMMARY_OUT=/tmp/a24_acceptance_serial_summary.log make -C FEM4C mbd_a24_acceptance_serial
# optional: capture per-step logs and include failed_log path in summary
# (step-log path must be a writable directory; file path/non-writable dir fails fast)
A24_ACCEPT_SERIAL_STEP_LOG_DIR=/tmp/a24_acceptance_serial_logs make -C FEM4C mbd_a24_acceptance_serial
# self-test for serial acceptance wrapper (pass + expected fail path)
make -C FEM4C mbd_a24_acceptance_serial_test
# one-command B-14 regression (contract + self-tests + local test entry)
make -C FEM4C mbd_b14_regression
# standalone gate check for an existing fem4c_test.log
make -C FEM4C fem4c_test_log_markers LOG_FILE=fem4c_test.log
# self-test for marker checker (pass + expected fail paths)
make -C FEM4C fem4c_test_log_markers_test

# B-8 daily guard (run_id日次共有不要運用):
# always runs static contract + local regression; spot evidence is optional
make -C FEM4C mbd_b8_guard
# optional: include B-14 one-command regression in the same guard run
make -C FEM4C mbd_b8_guard RUN_B14_REGRESSION=1
# one-command B-8 regression (contract + self-tests + guard with B-14 chain)
make -C FEM4C mbd_b8_regression
# override B-14 target while keeping local path lightweight
make -C FEM4C mbd_b8_regression B8_B14_TARGET=mbd_ci_contract_test B8_LOCAL_TARGET=mbd_ci_contract
# disable B-14 chain from make entrypoint
make -C FEM4C mbd_b8_regression B8_RUN_B14_REGRESSION=0
# optional: skip lock check in wrapper-focused runs (default lock scope: repo)
make -C FEM4C mbd_b8_regression B8_REGRESSION_SKIP_LOCK=1
# optional: force global lock scope (/tmp/fem4c_b8_regression.lock)
make -C FEM4C mbd_b8_regression B8_REGRESSION_LOCK_SCOPE=global
# optional: fail-fast timeout for wrapper-internal make calls (seconds)
make -C FEM4C mbd_b8_regression B8_MAKE_TIMEOUT_SEC=120
# self-test for the B-8 regression wrapper (pass + expected fail path)
make -C FEM4C mbd_b8_regression_test
# full path from clean rebuild
make -C FEM4C mbd_b8_regression_full
# disable B-14 chain from full wrapper entrypoint
make -C FEM4C mbd_b8_regression_full B8_RUN_B14_REGRESSION=0
# optional: force global lock scope from full wrapper entrypoint
make -C FEM4C mbd_b8_regression_full B8_REGRESSION_LOCK_SCOPE=global
# optional: pass custom lock path to full wrapper -> mbd_b8_regression path
make -C FEM4C mbd_b8_regression_full B8_REGRESSION_SKIP_LOCK=1 B8_REGRESSION_LOCK_DIR=/tmp/fem4c_b8_regression_custom.lock
# optional: fail-fast timeout for clean/all/test + nested mbd_b8_regression (seconds)
make -C FEM4C mbd_b8_regression_full B8_MAKE_TIMEOUT_SEC=180
# self-test for the full wrapper (pass + expected fail path)
make -C FEM4C mbd_b8_regression_full_test
# knob matrix test (0/1 + invalid knob/make-command fail-fast)
make -C FEM4C mbd_b8_knob_matrix_test
# smoke入口（full再構築ケースをスキップ）
make -C FEM4C mbd_b8_knob_matrix_smoke_test
# matrix coverage also validates full-wrapper path:
#   mbd_b8_regression_full with B8_RUN_B14_REGRESSION=0/1
#   invalid B8_RUN_B14_REGRESSION=2 and B8_MAKE_CMD=__missing_make__ fail-fast
# optional override:
#   B8_KNOB_MATRIX_SKIP_FULL=1 make -C FEM4C mbd_b8_knob_matrix_test
# optional spot evidence (non-strict: overall pass even if spot fails)
make -C FEM4C mbd_b8_guard RUN_SPOT=1 RUN_ID=21773820916
# optional: run both B-14 regression and spot evidence together
make -C FEM4C mbd_b8_guard RUN_B14_REGRESSION=1 RUN_SPOT=1 SPOT_SCAN_RUNS=5
# optional: reduce spot scan width to avoid API rate-limit spikes
make -C FEM4C mbd_b8_guard RUN_SPOT=1 SPOT_SCAN_RUNS=5
# strict spot mode (fails when spot evidence fails)
make -C FEM4C mbd_b8_guard RUN_SPOT=1 RUN_ID=21773820916 SPOT_STRICT=1
# spot失敗時は `spot_failure_reason` と `spot_error_type` / `spot_retry_after_sec` を出力
# spot実行時は `spot_scan_runs` を出力
# B-14連結時は `b14_regression_requested` / `b14_regression_result` を出力
# advanced override knobs (for local self-test/custom wrapper):
#   B8_MAKE_CMD=<command> (default: make)
#   invalid B8_MAKE_CMD values fail fast with non-zero exit
#   B8_RUN_B14_REGRESSION=0|1 (default: 1, used by mbd_b8_regression/_full wrappers)
#   invalid B8_RUN_B14_REGRESSION values fail fast with non-zero exit
#   B8_CONTRACT_TARGET=<target> (default: mbd_ci_contract)
#   B8_LOCAL_TARGET=<target> (default: mbd_checks)
#   B8_B14_TARGET=<target> (default: mbd_b14_regression for mbd_b8_guard*)
#   mbd_b8_regression/_full wrappers use B8_B14_TARGET=mbd_ci_contract as default
#   B8_REGRESSION_SKIP_LOCK=0|1 (default: 0 for mbd_b8_regression/_full wrappers)
#   invalid B8_REGRESSION_SKIP_LOCK values fail fast with non-zero exit
#   B8_REGRESSION_LOCK_SCOPE=repo|global (default: repo)
#   invalid B8_REGRESSION_LOCK_SCOPE values fail fast with non-zero exit
#   B8_REGRESSION_LOCK_DIR=<path> (optional override; repo scope default: /tmp/fem4c_b8_regression.<repo_hash>.lock)
#   B8_MAKE_TIMEOUT_SEC=<seconds> (default: 0 = disabled; non-numeric values fail fast)
#   wrapper summary trace: lock_dir=<path>, lock_dir_source=env|scope_repo_default|scope_global_default
#   full wrapper summary trace: b8_lock_dir_source=env|scope_repo_default|scope_global_default
#   B8_SPOT_TARGET=<target> (default: mbd_ci_evidence)
#   B8_TEST_TMP_COPY_DIR=<dir> (default: ${root_dir}/FEM4C/scripts in B-8 self-tests)
# stability contract:
#   mbd_b8_guard / mbd_b8_guard_contract / run_b8_regression sanitize parent make env via
#   `env -u MAKEFLAGS -u MFLAGS` before recursive make calls.
#   this keeps `mbd_b8_regression_test` stable even when called from nested wrappers.
#   run_b8_regression/_full also isolate `B8_LOCAL_TARGET` and B14 knobs
#   (`B8_B14_TARGET`, `B8_RUN_B14_REGRESSION`) from nested self-tests, and pass them only
#   to the final guard execution path.
#   run_b8_regression_full also isolates lock knobs from clean/all/test and forwards
#   `B8_REGRESSION_SKIP_LOCK` / `B8_REGRESSION_LOCK_SCOPE` / `B8_REGRESSION_LOCK_DIR`
#   only to final `mbd_b8_regression`.
# CI static checks:
#   make -C FEM4C mbd_ci_contract
#   (checks `mbd_b8_local_target_default`, `b8_guard_makeflags_isolation`,
#    `b8_regression_b14_target_default`, `b8_regression_makeflags_isolation`,
#    `b8_regression_b14_target_isolation`, `b8_regression_b14_knob_isolation`,
#    `b8_regression_local_target_isolation`, `b8_regression_local_target_pass_through`,
#    `b8_full_regression_local_target_isolation`, `b8_full_regression_local_target_pass_through`,
#    `b8_regression_test_temp_copy_marker`, `b8_full_test_temp_copy_marker`,
#    `b8_guard_contract_test_temp_copy_marker`,
#    `b8_regression_test_b14_target_override_case_marker`,
#    `b8_*_temp_copy_dir_knob_marker`, `b8_*_temp_copy_dir_validate_marker`,
#    `b8_*_temp_copy_dir_writable_marker`)
# self-test for guard behavior (B-14 chaining + strict/non-strict spot)
make -C FEM4C mbd_b8_guard_test
# validate guard output contract keys + summary consistency
make -C FEM4C mbd_b8_guard_contract RUN_B14_REGRESSION=1
# self-test for guard-contract wrapper (pass + expected fail path)
make -C FEM4C mbd_b8_guard_contract_test
# syntax-only check for B-8 script family
make -C FEM4C mbd_b8_syntax
# self-test for B-8 guard output checker (pass + expected fail path)
make -C FEM4C mbd_b8_guard_output_test

# Verify T3 clockwise orientation behavior in one command
# expected: default mode passes with correction warning, strict mode fails
make -C FEM4C t3_orientation_checks

# Verify coupled-mode stub failure path keeps contract snapshot logs
# checks stub snapshot path + invalid-input boundaries
#   E_BODY_PARSE/E_BODY_RANGE/E_DISTANCE_RANGE/E_REVOLUTE_RANGE/E_UNDEFINED_BODY_REF/E_INCOMPLETE_INPUT/E_UNSUPPORTED_DIRECTIVE
# coupled integrator setting: FEM4C_COUPLED_INTEGRATOR=newmark_beta|hht_alpha
# invalid value falls back to newmark_beta with warning
# CLI override is also available: --coupled-integrator=newmark_beta|hht_alpha
# parameter env overrides (logged in coupled stub contract):
#   FEM4C_NEWMARK_BETA (default: 0.25)
#   FEM4C_NEWMARK_GAMMA (default: 0.5)
#   FEM4C_HHT_ALPHA     (default: -0.05)
# parameter CLI overrides are also available:
#   --newmark-beta=<value> --newmark-gamma=<value> --hht-alpha=<value>
# ranges:
#   newmark_beta: 1e-12..1.0, newmark_gamma: 1e-12..1.5, hht_alpha: -1/3..0
# precedence in coupled mode: CLI option > environment variable > built-in default
# out-of-range environment values emit warning and fall back to defaults
# coupled startup log shows source labels:
#   Coupled integrator source: cli|env|default
#   Coupled parameter source: newmark_beta=<...> newmark_gamma=<...> hht_alpha=<...>
# one-command check validates both integrators in serial
# includes base FEM input + FEM input with appended MBD_* lines (+ legacy parser pkg if available)
make -C FEM4C coupled_stub_check
# `make -C FEM4C test` also runs coupled_stub_check

# Verify coupled integrator switching (B-12 lightweight regression)
# checks newmark_beta / hht_alpha and invalid fallback->newmark_beta
make -C FEM4C integrator_checks
# `make -C FEM4C test` also runs integrator_checks

```
