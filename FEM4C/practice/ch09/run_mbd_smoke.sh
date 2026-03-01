#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

tmp_input="/tmp/fem4c_mbd_smoke_input_$$.dat"
tmp_builtin_out="/tmp/fem4c_mbd_smoke_builtin_$$.dat"
tmp_input_out="/tmp/fem4c_mbd_smoke_input_$$.dat.out"
tmp_builtin_log="/tmp/fem4c_mbd_smoke_builtin_$$.log"
tmp_input_log="/tmp/fem4c_mbd_smoke_input_$$.log"

cleanup() {
    unlink "$tmp_input" 2>/dev/null || true
    unlink "$tmp_builtin_out" 2>/dev/null || true
    unlink "$tmp_input_out" 2>/dev/null || true
    unlink "$tmp_builtin_log" 2>/dev/null || true
    unlink "$tmp_input_log" 2>/dev/null || true
}
trap cleanup EXIT

assert_contains() {
    local file="$1"
    local pattern="$2"
    if ! grep -q "$pattern" "$file"; then
        echo "[ERROR] pattern not found: '$pattern' in $file" >&2
        echo "[ERROR] tail of $file:" >&2
        tail -n 40 "$file" >&2 || true
        exit 1
    fi
}

make -C "$repo_root" >/dev/null

"$repo_root/bin/fem4c" --mode=mbd "$repo_root/examples/t6_cantilever_beam.dat" "$tmp_builtin_out" >"$tmp_builtin_log" 2>&1

cat > "$tmp_input" <<'EOF'
MBD_BODY 0 0.0 0.0 0.0
MBD_BODY 1 1.0 0.0 0.0
MBD_DISTANCE 1 0 1 0.0 0.0 0.0 0.0 1.0
MBD_REVOLUTE 2 0 1 0.5 0.0 -0.5 0.0
EOF

"$repo_root/bin/fem4c" --mode=mbd "$tmp_input" "$tmp_input_out" >"$tmp_input_log" 2>&1

assert_contains "$tmp_builtin_log" "Analysis mode: mbd"
assert_contains "$tmp_builtin_log" "mbd_source: builtin_fallback"
assert_contains "$tmp_builtin_log" "constraint_equations:"
assert_contains "$tmp_builtin_log" "residual_l2:"
assert_contains "$tmp_input_log" "mbd_source: input_case"
assert_contains "$tmp_input_log" "constraint_equations:"
assert_contains "$tmp_input_log" "residual_l2:"
assert_contains "$tmp_builtin_out" "source,builtin"
assert_contains "$tmp_input_out" "source,input"

echo "A-3 smoke: PASS (builtin_fallback/input_case + constraint_equations/residual_l2)"
