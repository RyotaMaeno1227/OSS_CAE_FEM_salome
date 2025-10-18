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

Build the programs with `gcc -Wall -Wextra <file.c> -lm` and run the tests embedded in each `main`.
