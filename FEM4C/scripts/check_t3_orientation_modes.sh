#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT_DIR}/bin/fem4c"

if [[ ! -x "${BIN}" ]]; then
    echo "FAIL: fem4c binary not found (${BIN})"
    exit 1
fi

WORK_DIR="$(mktemp -d /tmp/fem4c_t3_orientation.XXXXXX)"
trap 'rm -rf "${WORK_DIR}"' EXIT

CLOCKWISE_INPUT="${WORK_DIR}/t3_clockwise.dat"
cat > "${CLOCKWISE_INPUT}" <<'EOF'
Clockwise T3
3 1
1 0.0 0.0
2 0.0 1.0
3 1.0 0.0
1 1 2 3
2.0e11 0.3
1 1 1 0.0 0.0 0.0
2 1 1 0.0 0.0 0.0
point loads
3 0.0 1000.0 0.0
end
EOF

DEFAULT_LOG="${WORK_DIR}/default.log"
STRICT_LOG="${WORK_DIR}/strict.log"

"${BIN}" "${CLOCKWISE_INPUT}" "${WORK_DIR}/default_out.dat" > "${DEFAULT_LOG}" 2>&1
if ! grep -q "orientation corrected" "${DEFAULT_LOG}"; then
    echo "FAIL: default mode did not report orientation correction"
    exit 1
fi

set +e
"${BIN}" --strict-t3-orientation "${CLOCKWISE_INPUT}" "${WORK_DIR}/strict_out.dat" > "${STRICT_LOG}" 2>&1
STRICT_EXIT=$?
set -e

if [[ ${STRICT_EXIT} -eq 0 ]]; then
    echo "FAIL: strict mode was expected to fail for clockwise T3 input"
    exit 1
fi
if ! grep -q "Analysis failed with error code" "${STRICT_LOG}"; then
    echo "FAIL: strict mode failed without the expected error summary"
    exit 1
fi

echo "PASS: t3 orientation checks (default=pass with correction, strict=expected fail)"
