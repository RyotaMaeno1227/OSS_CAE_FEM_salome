# Coupled Endurance Failure History

This report mirrors `docs/reports/kkt_spectral_weekly.md` but focuses on the archive-and-summarize job in `coupled_endurance.yml`. The GitHub Actions job now runs `tools/report_archive_failure_rate.py` on every weekly invocation and publishes the following artifacts:

- `data/endurance_archive/archive_failure_rate.png` – PNG bar/line chart of failure rate vs run count.
- `data/endurance_archive/archive_failure_rate.md` – Markdown table containing each ISO week, counts, and associated run IDs.
- `data/endurance_archive/archive_failure_rate.json` – Machine-readable payload (also consumed by Slack digests).

To refresh this page:

1. Download the latest artifact (`coupled-endurance-<run-id>`) and copy the generated Markdown table into a new section below.
2. Update the chart link if a new PNG should be embedded.

> Note: interactive generation of these files requires GitHub REST access. Locally you can run `python tools/report_archive_failure_rate.py --repo <owner>/<repo> --workflow coupled_endurance.yml --job-name archive-and-summarize --weeks 8 --output-chart ... --output-markdown ... --skip-chart --dry-run` when `api.github.com` is reachable, so you can preview the Markdown without regenerating PNGs.

## Latest Snapshot (fill after artifact download)

_Pending update – run `workflow_dispatch` and paste the contents of `archive_failure_rate.md` here along with the PNG link._

Placeholder structure (replace per Run ID):

```
### Run <ID> (<date>)

![Archive failure rate](../data/endurance_archive/archive_failure_rate.png)

<paste archive_failure_rate.md table here>
```
