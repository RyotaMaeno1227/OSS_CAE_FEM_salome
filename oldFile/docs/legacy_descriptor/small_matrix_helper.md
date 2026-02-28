# Small Matrix Helper

`chrono_small_matrix.h` exposes tiny utilities required by the 3D migration work:

- `chrono_smallmat_invert_{2x2,3x3,4x4}` – Gauss-Jordan with partial pivoting.
- `chrono_smallmat_mul` – dense `n×n` multiplication (n ≤ 4) used by descriptor-level checks.

## Bench snapshot

Command:

```
./chrono-C-all/tests/bench_small_matrix
```

Sample output on x86_64 (Clang 17, `-O3`):

```
Small-matrix inversion throughput (successes/s)
2x2: 6.21e+07
3x3: 4.03e+07
4x4: 2.57e+07
```

- 4×4 inversion is ~2.4× slower than 2×2 but still well below the per-step budget of the Coupled block (took ≈38 ns per solve on this host).
- These helpers are dimension agnostic and will be re-used by the 3D descriptor layer; keeping the code in one place simplifies ABI review and profiling.

## Integration notes

- `src/chrono_constraint_kkt_backend.c` now delegates to `chrono_smallmat_invert_with_history(...)`, so the KKT backend and descriptor layer share the same pivot history/min/max logic.
- Future 3D descriptors only need to extend `CHRONO_SMALL_MAT_MAX_N`; the backend automatically mirrors `pivot_history[]` into `ChronoConstraintDiagnostics_C`.
- `ChronoKKTBackendStats_C` exposes `cache_hits`/`cache_misses`/`cache_checks` に加えてサイズごとのヒストグラムを保持するようになった。`chrono-C-all/tests/bench_coupled_constraint --stats-json data/diagnostics/kkt_backend_stats.json` を実行すると値が JSON に落ち、`tools/compare_kkt_logs.py --kkt-stats …` が週次レポートへヒット率を追記する。

### kkt_backend_stats.json の読み方

`data/diagnostics/kkt_backend_stats.json` は以下のフィールドを持つシンプルな JSON オブジェクトです:

| フィールド | 意味 |
|-----------|------|
| `calls` / `fallback_calls` | KKT backend への呼び出し総数と SmallMatrix で処理できずフォールバックした回数。 |
| `cache_hits` / `cache_misses` / `cache_checks` | `chrono_kkt_backend_cache_*` の統計。`cache_hit_rate` は `cache_hits / cache_checks` を表す。 |
| `size_histogram` | 添字 `n` が `n` 式を持つブロックのリクエスト件数に相当する（0 は未使用）。 |

`jq '.cache_hit_rate' data/diagnostics/kkt_backend_stats.json` のように単純なツールで参照できるため、Multi-ω 計測や descriptor PoC のレビューに添付しておくと便利です。

### DEBUG_KKT ログ例

`make tests CFLAGS='-DDEBUG_KKT'` のようにビルドすると KKT backend が行番号＋ラベル付きで pivot 履歴を stderr に出力します。`chrono_constraint2d` 側で constraint ポインタと式カウントを埋め込んでおり、問題の島を特定しやすくなります。

```
[kkt-debug:chrono_constraint_kkt_backend.c:94] label=constraint=0x5562df2fdc10 eq=3 n=3 min_pivot=1.472e+02 max_pivot=3.884e+02 rank=3 cache_hits=12
```

- `label` には `chrono_constraint2d` がセットした constraint アドレス／式数が入る。
- `n` / `min_pivot` / `max_pivot` / `rank` は `ChronoKKTBackendResult_C` の値と一致するので、`docs/logs/kkt_descriptor_poc_e2e.md` や `docs/reports/kkt_spectral_weekly.md` の Δκ_s と合わせて調査できる。
- 解析手順の例:
  1. `make tests CFLAGS='-DDEBUG_KKT'` でビルド。
  2. 対象テストを実行し、stderr を `tee debug.log` で保存。
  3. `rg --no-heading --line-number "\\[kkt-debug" debug.log` で問題行を抽出し、constraint ポインタごとに `sort`。
  4. `jq '.[] | select(.scenario==\"tele_yaw_control\")' data/diagnostics/chrono_c_diagnostics_sample.json` のように diag JSON と突き合わせ、ピボット差分や WARN フラグを確認する。

### KKT JSON → CSV 変換案

`tools/compare_kkt_logs.py --kkt-stats` で使用している JSON を Aチームが簡易 CSV へ変換できるよう、以下の Python スニペット案を残す。

```bash
python - <<'PY'
import csv, json
from pathlib import Path

payload = json.loads(Path("data/diagnostics/kkt_backend_stats.json").read_text())
rows = [
    {
        "label": entry["label"],
        "min_pivot": entry["min_pivot"],
        "max_pivot": entry["max_pivot"],
        "condition_number": entry["condition_number"],
        "condition_number_spectral": entry["condition_number_spectral"],
    }
    for entry in payload.get("diagnostics", [])
]
with open("data/diagnostics/kkt_backend_stats_flat.csv", "w", newline="", encoding="utf-8") as handle:
    writer = csv.DictWriter(handle, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
PY
```

この CSV を `tools/compare_kkt_logs.py --diag-json data/diagnostics/sample_diag.json` の結果と突き合わせれば、A/B 両チームで条件数の異常値を共有しやすくなる。
