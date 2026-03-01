#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root_dir"

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

pass_log="${tmp_dir}/fem4c_test.pass.log"
fail_log_missing_marker="${tmp_dir}/fem4c_test.fail.missing_marker.log"
missing_log="${tmp_dir}/fem4c_test.missing.log"

cat >"${pass_log}" <<'EOF'
PASS: coupled integrator switch check (newmark_beta + hht_alpha + invalid fallback)
PASS: mbd integrator switch check (default/env/cli + params + out-of-range fallback)
PASS: all MBD checks completed
EOF

if ! bash "FEM4C/scripts/check_fem4c_test_log_markers.sh" "${pass_log}" >"${tmp_dir}/pass.out" 2>&1; then
  echo "FAIL: marker checker should pass for complete marker log" >&2
  cat "${tmp_dir}/pass.out" >&2
  exit 1
fi

cat >"${fail_log_missing_marker}" <<'EOF'
PASS: coupled integrator switch check (newmark_beta + hht_alpha + invalid fallback)
PASS: mbd integrator switch check (default/env/cli + params + out-of-range fallback)
EOF

if bash "FEM4C/scripts/check_fem4c_test_log_markers.sh" "${fail_log_missing_marker}" >"${tmp_dir}/fail_missing_marker.out" 2>&1; then
  echo "FAIL: marker checker should fail when MBD suite marker is missing" >&2
  cat "${tmp_dir}/fail_missing_marker.out" >&2
  exit 1
fi

if ! grep -q "missing marker" "${tmp_dir}/fail_missing_marker.out"; then
  echo "FAIL: missing-marker failure output did not contain expected text" >&2
  cat "${tmp_dir}/fail_missing_marker.out" >&2
  exit 1
fi

if bash "FEM4C/scripts/check_fem4c_test_log_markers.sh" "${missing_log}" >"${tmp_dir}/fail_missing_log.out" 2>&1; then
  echo "FAIL: marker checker should fail when log file does not exist" >&2
  cat "${tmp_dir}/fail_missing_log.out" >&2
  exit 1
fi

if ! grep -q "missing FEM4C test log" "${tmp_dir}/fail_missing_log.out"; then
  echo "FAIL: missing-log failure output did not contain expected text" >&2
  cat "${tmp_dir}/fail_missing_log.out" >&2
  exit 1
fi

echo "PASS: check_fem4c_test_log_markers self-test (pass + missing-marker + missing-log)"
