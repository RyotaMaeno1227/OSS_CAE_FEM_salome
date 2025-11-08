# Weekly KKT / Spectral Comparison

| Scenario | eq_count | κ̂ (Chrono-C) | κ̂ (chrono-main) | Δκ̂ | κ_s (Chrono-C) | κ_s (chrono-main) | Δκ_s | min pivot Δ | max pivot Δ | pivot₀ Δ | Log levels (C/main) | Status |
|----------|---------:|--------------:|-----------------:|-----:|---------------:|------------------:|------:|-------------:|-------------:|-----------:|---------------------|--------|
| default | 2 | 1.000e+00 | 1.002e+00 | 1.500e-03 | 1.000e+00 | 1.002e+00 | 1.500e-03 | 5.000e-02 | 8.000e-02 | 8.000e-02 | warning/warning | ✅ |
| default | 3 | 1.000e+00 | 1.000e+00 | 2.000e-04 | 1.000e+00 | 1.000e+00 | 2.000e-04 | 2.900e-02 | 7.000e-02 | 7.000e-02 | warning/warning | ✅ |
| default | 4 | 1.000e+00 | 1.000e+00 | 1.000e-04 | 1.000e+00 | 1.000e+00 | 1.000e-04 | 2.600e-02 | 1.500e-01 | 1.500e-01 | warning/warning | ✅ |
| spectral_stress | 4 | 1.000e+26 | 9.990e+25 | 1.000e+23 | 1.000e+26 | 9.981e+25 | 1.900e+23 | 1.000e-04 | 2.500e+01 | 2.500e+01 | info/info | ✅ |

Δ values are absolute differences; ✅ indicates both κ metrics aligned within 5%.
