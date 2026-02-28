#!/usr/bin/env python3
"""Compare Gauss-Jordan scaling strategies against a high-precision reference."""

from __future__ import annotations

import json
import math
import subprocess
from decimal import Decimal, getcontext
from pathlib import Path
from typing import List

REPO_ROOT = Path(__file__).resolve().parents[1]
ANALYZER = REPO_ROOT / "tools" / "compare_coupled_gaussjordan"


def invert_matrix_decimal(matrix: List[List[float]]) -> List[List[Decimal]]:
    n = len(matrix)
    getcontext().prec = 100
    a = [[Decimal(x) for x in row] for row in matrix]
    inv = [[Decimal(1) if i == j else Decimal(0) for j in range(n)] for i in range(n)]

    for col in range(n):
        pivot_row = max(range(col, n), key=lambda r: abs(a[r][col]))
        pivot = a[pivot_row][col]
        if pivot == 0:
            raise RuntimeError("Matrix is singular during Decimal inversion.")
        if pivot_row != col:
            a[col], a[pivot_row] = a[pivot_row], a[col]
            inv[col], inv[pivot_row] = inv[pivot_row], inv[col]
        inv_pivot = Decimal(1) / pivot
        for k in range(n):
            a[col][k] *= inv_pivot
            inv[col][k] *= inv_pivot
        for row in range(n):
            if row == col:
                continue
            factor = a[row][col]
            if factor == 0:
                continue
            for k in range(n):
                a[row][k] -= factor * a[col][k]
                inv[row][k] -= factor * inv[col][k]

    return inv


def compute_residual(matrix: List[List[float]], inverse: List[List[float]]) -> float:
    n = len(matrix)
    max_err = 0.0
    for i in range(n):
        for j in range(n):
            accum = 0.0
            for k in range(n):
                accum += matrix[i][k] * inverse[k][j]
            target = 1.0 if i == j else 0.0
            max_err = max(max_err, abs(accum - target))
    return max_err


def compute_residual_decimal(matrix: List[List[float]], inverse: List[List[Decimal]]) -> Decimal:
    getcontext().prec = 100
    n = len(matrix)
    max_err = Decimal(0)
    matrix_decimal = [[Decimal(x) for x in row] for row in matrix]
    for i in range(n):
        for j in range(n):
            accum = Decimal(0)
            for k in range(n):
                accum += matrix_decimal[i][k] * inverse[k][j]
            target = Decimal(1) if i == j else Decimal(0)
            err = abs(accum - target)
            if err > max_err:
                max_err = err
    return max_err


def main() -> int:
    if not ANALYZER.exists():
        raise FileNotFoundError(f"Analyzer binary not found: {ANALYZER}")

    result = subprocess.run([str(ANALYZER)], capture_output=True, text=True, check=True)
    lines = [line for line in result.stdout.splitlines() if line.strip()]

    records = [json.loads(line) for line in lines]

    print("| scenario | n | residual (unscaled) | residual (scaled) | residual (decimal) | scaled / decimal |")
    print("|----------|---:|---------------------:|-------------------:|-------------------:|-----------------:|")
    for record in records:
        matrix = record["matrix"]
        residual_unscaled = record["residual_unscaled"]
        residual_scaled = record["residual_scaled"]
        inverse_decimal = invert_matrix_decimal(matrix)
        residual_decimal = float(compute_residual_decimal(matrix, inverse_decimal))
        ratio = residual_scaled / residual_decimal if residual_decimal > 0 else math.inf
        print(
            f"| {record['scenario']} | {record['n']} | {residual_unscaled:.3e} | "
            f"{residual_scaled:.3e} | {residual_decimal:.3e} | {ratio:.3e} |"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
