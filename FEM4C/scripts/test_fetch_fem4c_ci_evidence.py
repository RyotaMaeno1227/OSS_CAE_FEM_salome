#!/usr/bin/env python3
"""Unit tests for fetch_fem4c_ci_evidence helpers."""

import unittest
from contextlib import redirect_stdout
from importlib import util as importlib_util
from io import StringIO
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parent / "fetch_fem4c_ci_evidence.py"
SPEC = importlib_util.spec_from_file_location("fetch_fem4c_ci_evidence", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
ci = importlib_util.module_from_spec(SPEC)
SPEC.loader.exec_module(ci)


class FetchCiEvidenceHelpersTest(unittest.TestCase):
    def test_parse_repo_variants(self) -> None:
        self.assertEqual(ci.parse_repo("owner/repo"), "owner/repo")
        self.assertEqual(ci.parse_repo("https://github.com/owner/repo"), "owner/repo")
        self.assertEqual(ci.parse_repo("https://github.com/owner/repo.git"), "owner/repo")
        self.assertEqual(ci.parse_repo(" https://github.com/owner/repo/ "), "owner/repo")

    def test_find_step_outcome_present(self) -> None:
        jobs_payload = {
            "jobs": [
                {
                    "steps": [
                        {"name": "Checkout", "conclusion": "success"},
                        {"name": "Run FEM4C regression entrypoint", "conclusion": "failure"},
                    ]
                }
            ]
        }
        self.assertEqual(ci.find_step_outcome(jobs_payload, "Run FEM4C regression entrypoint"), "failure")

    def test_find_step_outcome_missing(self) -> None:
        jobs_payload = {"jobs": [{"steps": [{"name": "Checkout", "conclusion": "success"}]}]}
        self.assertEqual(ci.find_step_outcome(jobs_payload, "Run FEM4C regression entrypoint"), "missing")

    def test_has_artifact(self) -> None:
        artifacts_payload = {"artifacts": [{"name": "chrono-tests"}, {"name": "other"}]}
        self.assertTrue(ci.has_artifact(artifacts_payload, "chrono-tests"))
        self.assertFalse(ci.has_artifact(artifacts_payload, "fem4c-only"))

    def test_safe_int(self) -> None:
        self.assertEqual(ci.safe_int("123"), 123)
        self.assertEqual(ci.safe_int("-1"), -1)
        self.assertIsNone(ci.safe_int(None))
        self.assertIsNone(ci.safe_int("abc"))

    def test_format_rate_limit_reset(self) -> None:
        self.assertEqual(ci.format_rate_limit_reset(None), "unknown")
        self.assertEqual(ci.format_rate_limit_reset(0), "1970-01-01T00:00:00Z")

    def test_emit_structured_error(self) -> None:
        capture = StringIO()
        with redirect_stdout(capture):
            ci.emit_structured_error("rate_limit", retry_after_sec=12, remaining=0)
        lines = capture.getvalue().strip().splitlines()
        self.assertEqual(
            lines,
            [
                "CI_EVIDENCE_ERROR",
                "error_type=rate_limit",
                "retry_after_sec=12",
                "remaining=0",
            ],
        )


if __name__ == "__main__":
    unittest.main()
