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

# Run all MBD checks in one command
make -C FEM4C mbd_checks

# Fetch CI evidence with strict acceptance:
# threshold: step_present==yes && artifact_present==yes (otherwise non-zero exit)
make -C FEM4C mbd_ci_evidence

# Validate CI contract locally (no run_id required)
make -C FEM4C mbd_ci_contract
```
