#!/usr/bin/env bash

# Shared helpers for deriving stable review-command reason codes.

c_team_normalize_reason_code() {
  local raw="${1:-}"
  local normalized
  normalized="$(
    printf '%s' "${raw}" \
      | tr '[:upper:]' '[:lower:]' \
      | sed -E 's/[^a-z0-9]+/_/g; s/^_+//; s/_+$//'
  )"
  printf '%s' "${normalized}"
}

c_team_build_prefixed_reason_codes() {
  local prefix="${1:-review_command_}"
  local reason_codes_raw="${2:-}"
  local reasons_raw="${3:-}"
  local prefixed=""
  local fallback=""

  if [[ -n "${reason_codes_raw}" ]]; then
    prefixed="$(
      printf '%s' "${reason_codes_raw}" \
        | awk -v prefix="${prefix}" '{
            raw=$0
            gsub(/[;,]/, "\n", raw)
            n=split(raw, parts, /\n/)
            for (i = 1; i <= n; i++) {
              code=parts[i]
              gsub(/^ +| +$/, "", code)
              if (code != "" && code != "-") {
                if (out != "") out = out ","
                out = out prefix code
              }
            }
          } END {print out}'
    )"
  fi

  if [[ -n "${prefixed}" ]]; then
    printf '%s' "${prefixed}"
    return 0
  fi

  fallback="$(c_team_normalize_reason_code "${reasons_raw}")"
  if [[ -z "${fallback}" ]]; then
    fallback="unknown_review_command_reason"
  fi
  printf '%s%s' "${prefix}" "${fallback}"
}

c_team_collect_missing_log_review_pattern() {
  printf '%s' "collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source"
}

c_team_resolve_binary_toggle() {
  local primary_env="${1:-}"
  local fallback_env="${2:-}"
  local default_value="${3:-}"
  local primary_value=""
  local fallback_value=""

  if [[ -n "${primary_env}" ]]; then
    primary_value="${!primary_env-}"
  fi
  if [[ -n "${fallback_env}" ]]; then
    fallback_value="${!fallback_env-}"
  fi

  if [[ -n "${primary_value}" ]]; then
    printf '%s' "${primary_value}"
    return 0
  fi
  if [[ -n "${fallback_value}" ]]; then
    printf '%s' "${fallback_value}"
    return 0
  fi
  printf '%s' "${default_value}"
}
