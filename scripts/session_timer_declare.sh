#!/usr/bin/env bash
set -euo pipefail

exec "$(dirname "$0")/session_timer.sh" declare "$@"
