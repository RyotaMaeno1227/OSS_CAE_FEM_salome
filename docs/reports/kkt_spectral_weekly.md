# Weekly KKT / Spectral Comparison

| Scenario | eq_count | κ̂ (Chrono-C) | κ̂ (chrono-main) | Δκ̂ | κ_s (Chrono-C) | κ_s (chrono-main) | Δκ_s | min pivot Δ | max pivot Δ | pivot₀ Δ | Log levels (C/main) | Status |
|----------|---------:|--------------:|-----------------:|-----:|---------------:|------------------:|------:|-------------:|-------------:|-----------:|---------------------|--------|
| default | 2 | 1.000e+00 | 1.002e+00 | 1.500e-03 | 1.000e+00 | 1.002e+00 | 1.500e-03 | 5.000e-02 | 8.000e-02 | 8.000e-02 | warning/warning | ✅ |
| default | 3 | 1.000e+00 | 1.000e+00 | 2.000e-04 | 1.000e+00 | 1.000e+00 | 2.000e-04 | 2.900e-02 | 7.000e-02 | 7.000e-02 | warning/warning | ✅ |
| default | 4 | 1.000e+00 | 1.000e+00 | 1.000e-04 | 1.000e+00 | 1.000e+00 | 1.000e-04 | 2.600e-02 | 1.500e-01 | 1.500e-01 | warning/warning | ✅ |
| spectral_stress | 4 | 1.000e+26 | 9.990e+25 | 1.000e+23 | 1.000e+26 | 9.981e+25 | 1.900e+23 | 1.000e-04 | 2.500e+01 | 2.500e+01 | info/info | ✅ |

Δ values are absolute differences; ✅ indicates both κ metrics aligned within 5%.

## Multi-ω Bench Status

| Scenario | ω | eq_countₘₐₓ | κ̂ₘₐₓ | κ_sₘₐₓ | Δκ | pivot span | drop events | avg solve (µs) | Status |
|----------|---:|-----------:|-------:|--------:|-----:|-----------:|-----------:|---------------:|--------|
| default | 0.850 | 4 | 1.000e+00 | 1.000e+00 | 0.000e+00 | 8.130e-04 | 3 | 5.871 | ✅ |
| default | 1.000 | 4 | 1.000e+00 | 1.000e+00 | 0.000e+00 | 8.130e-04 | 3 | 5.654 | ✅ |
| default | 1.150 | 4 | 1.000e+00 | 1.000e+00 | 0.000e+00 | 8.130e-04 | 3 | 5.161 | ✅ |
| spectral_stress | 0.850 | 4 | 1.000e+26 | 1.000e+26 | 0.000e+00 | 2.000e+06 | 1 | 5.327 | ✅ |
| spectral_stress | 1.000 | 4 | 1.000e+26 | 1.000e+26 | 0.000e+00 | 2.000e+06 | 1 | 5.183 | ✅ |
| spectral_stress | 1.150 | 4 | 1.000e+26 | 1.000e+26 | 0.000e+00 | 2.000e+06 | 1 | 5.215 | ✅ |

## Archive Failure Rate

| Week start | Runs | Failures | Failure rate |
|------------|-----:|---------:|-------------:|
| 2025-10-20 | 5 | 0 | 0.0% |
| 2025-10-27 | 6 | 1 | 16.7% |
| 2025-11-03 | 4 | 1 | 25.0% |

## KKT Backend Cache Metrics

| Calls | Fallback | Cache hits | Cache misses | Cache checks | Hit rate |
|------:|---------:|-----------:|-------------:|-------------:|---------:|
| 51480 | 1320 | 17121 | 1359 | 18480 | 92.65% |

Histogram (eq_count=0…4): [0, 17160, 15840, 11880, 6600]
