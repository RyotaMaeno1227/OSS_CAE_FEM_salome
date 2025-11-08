# Small Matrix Helper

`chrono_small_matrix.h` exposes tiny utilities required by the 3D migration work:

- `chrono_smallmat_invert_{2x2,3x3,4x4}` – Gauss-Jordan with partial pivoting.
- `chrono_smallmat_mul` – dense `n×n` multiplication (n ≤ 4) used by descriptor-level checks.

## Bench snapshot

Command:

```
./chrono-C-all/tests/bench_small_matrix
```

Sample output on x86_64 (Clang 17, `-O3`):

```
Small-matrix inversion throughput (successes/s)
2x2: 6.21e+07
3x3: 4.03e+07
4x4: 2.57e+07
```

- 4×4 inversion is ~2.4× slower than 2×2 but still well below the per-step budget of the Coupled block (took ≈38 ns per solve on this host).
- These helpers are dimension agnostic and will be re-used by the 3D descriptor layer; keeping the code in one place simplifies ABI review and profiling.
