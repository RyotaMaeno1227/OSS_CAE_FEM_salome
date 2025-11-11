from __future__ import annotations

import shutil
import tempfile
import textwrap
import unittest
from pathlib import Path

from tools import update_multi_omega_assets as updater


class UpdateCommandBlockTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmpdir = Path(tempfile.mkdtemp())

    def tearDown(self) -> None:
        shutil.rmtree(self.tmpdir)

    def _rewrite(self, content: str, omegas: list[float]) -> str:
        path = self.tmpdir / "doc.md"
        path.write_text(content, encoding="utf-8")
        updater.update_command_block(path, omegas)
        return path.read_text(encoding="utf-8")

    def test_rewrites_indented_block(self) -> None:
        source = textwrap.dedent(
            """\
            - Example:
              ```bash
              ./chrono-C-all/tests/bench_coupled_constraint \\
                --omega 0.9 \\
                --omega 1.1 \\
                --output data/out.csv
              ```
            """
        )
        result = self._rewrite(source, [0.8, 1.0, 1.2])
        expected = textwrap.dedent(
            """\
            - Example:
              ```bash
              ./chrono-C-all/tests/bench_coupled_constraint \\
                --omega 0.8 \\
                --omega 1 \\
                --omega 1.2 \\
                --output data/out.csv
              ```
            """
        )
        self.assertEqual(result, expected)

    def test_handles_unindented_block(self) -> None:
        source = textwrap.dedent(
            """\
            ```bash
            ./chrono-C-all/tests/bench_coupled_constraint \\
              --omega 0.85 \\
              --output data/out.csv
            ```
            """
        )
        result = self._rewrite(source, [0.75])
        expected = textwrap.dedent(
            """\
            ```bash
            ./chrono-C-all/tests/bench_coupled_constraint \\
              --omega 0.75 \\
              --output data/out.csv
            ```
            """
        )
        self.assertEqual(result, expected)

    def test_handles_additional_markdown_sections(self) -> None:
        source = textwrap.dedent(
            """\
            ## Appendix
            Refer to the profiling command below:
            ```bash
            ./chrono-C-all/tests/bench_coupled_constraint \\
              --omega 0.9 \\
              --output data/tmp.csv
            ```
            Continue with analysis.
            """
        )
        result = self._rewrite(source, [0.8, 1.1])
        expected = textwrap.dedent(
            """\
            ## Appendix
            Refer to the profiling command below:
            ```bash
            ./chrono-C-all/tests/bench_coupled_constraint \\
              --omega 0.8 \\
              --omega 1.1 \\
              --output data/tmp.csv
            ```
            Continue with analysis.
            """
        )
        self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
