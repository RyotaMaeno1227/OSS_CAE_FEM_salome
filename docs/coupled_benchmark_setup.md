# Coupled Benchmark Automation Setup

This note collects the steps needed to reproduce the CI benchmark pipeline on a local machine, including the optional YAML-based configuration.

## 1. Install prerequisites

```bash
python3 -m pip install --upgrade pip
python3 -m pip install pyyaml
```

PyYAML is required because the benchmark script loads shared thresholds from `config/coupled_benchmark_thresholds.yaml`.  
If PyYAML is not available, you can either install it as above or pass a JSON configuration file to `tools/run_coupled_benchmark.py --config`.

## 2. Run the benchmark locally

```bash
python3 tools/run_coupled_benchmark.py \
  --config config/coupled_benchmark_thresholds.yaml \
  --output data/coupled_benchmark_metrics.csv
```

The command emits the same warnings and hard failures as the CI workflow.

## 3. Generate the static site bundle

```bash
python3 tools/build_coupled_benchmark_site.py \
  --output-dir site \
  --copy-data \
  --latest 12 \
  data/coupled_benchmark_metrics.csv
```

This mirrors the GitHub Pages routine: it writes `site/index.html` with Chart.js visualisations and copies recent CSVs into `site/data/`.

## 4. Continuous deployment (CI)

The workflow `.github/workflows/coupled_benchmark.yml` now:

1. Runs the benchmark with the shared thresholds.
2. Uploads the raw CSV as a build artifact.
3. Builds the static bundle via `tools/build_coupled_benchmark_site.py`.
4. Publishes the contents of `site/` to GitHub Pages using `actions/deploy-pages`.

The published URL is shown in the workflow summary under the `deploy` job.
