# chrono-2d constraints quick note

目的: 2D 教材用に実装した拘束計算の最小仕様と検証ルールをまとめる。

- 有効質量（pivot）  
  拘束行 \(J\) と質量逆行列 \(M^{-1}\) から  
  \(\lambda = 1 / (J M^{-1} J^T)\) を pivot として記録。min/max 比を条件数に使用。

- 条件数  
  `condition_bound = condition_spectral = max_pivot / min_pivot`。pivot が 0/NaN の場合は 1e9 を返し診断しやすくする。

- 拘束の行定義（Body A/B, anchor と軸/法線を使用）  
  - Distance: アンカー間の方向ベクトル  
  - Revolute: X/Y の直交 2 行  
  - Planar/Prismatic: 軸に対する法線 1 行  
  - Gear: 角速度差 1 行 (\(-\omega_a + \omega_b = 0\))  
  - Contact: 法線 1 行 + 接線 1 行（静摩擦判定で stick/slip を切替）

- 摩擦モデル  
  接触点の相対速度から \(v_t\) と \(v_n\) を算出し、\(|v_t| \le \mu_s |v_n| + 10^{-4}\) で stick。  
  Slip 時は接線 pivot を \(\max(0.1, \min(1.0, 0.5\mu_d))\) で減衰し、条件数が悪化することをテストで検知。

- 許容レンジ（`data/constraint_ranges.csv` で外出し）  
  - distance/revolute/planar/prismatic/gear: cond ∈ [0.5, 20]  
  - contact stick: cond ∈ [1, 10]  
  - contact slip: cond ∈ [3, 50]（stick より悪化していること）
- パラメータ感度レンジ（`data/parameter_sensitivity_ranges.csv`）  
  case 単位で cond/pivot の許容レンジを外出しし、テストで範囲判定を行う。
- 初期レンジの根拠（A14）  
  - 低レンジ（例: 0.5–10）: revolute/接触 stick の安定域を優先。  
  - 中レンジ（例: 0.5–60）: 複合/距離系での drift を許容しつつ逸脱を抑制。  
  - 高レンジ（例: 0.5–80）: 複合 prismatic 系の悪化を許容しつつ範囲内か判定。  
- 複合拘束の評価観点（A18）  
  - 対象: `composite_planar_distance`, `composite_distance`, `composite_prismatic_distance`, `composite_prismatic_distance_aux`, `composite_planar_prismatic`, `composite_planar_prismatic_aux`  
  - 判定: 条件数が範囲内（planar/distance=0.5–60、prismatic/distance=0.5–80）、pivot が正値。  
  - 期待: 複合ケース間で cond/pivot が極端に乖離しないこと、determinism チェックで許容誤差内に収まること。

- CSV スキーマ  
  `time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot,vn,vt,mu_s,mu_d,stick` を固定し、テストでヘッダと値域を検証。

- 例題データセット  
  - `data/contact_cases.csv`: stick/slip 判定を外部定義（vn, vt, mu_s, mu_d, expected_stick）。  
  - `data/constraint_ranges.csv`: 拘束タイプ別の cond 許容レンジ。  
  - `data/bench_baseline.csv`: ベンチ許容（1.5x）比較用の基準。

- ベンチ  
  `make bench` で `artifacts/bench_constraints.csv` を生成し、baseline 比 1.5 倍超で失敗。

サンプル出力（descriptor CSV 抜粋）
```
time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot,vn,vt,mu_s,mu_d,stick
5.000000,hydraulic_lift_sync,actions,1.000000e+00,1.000000e+00,1.010664e+00,1.010664e+00,0.500000,0.010000,0.600000,0.400000,1
6.000000,hydraulic_lift_sync_slip,actions,5.000000e+00,5.000000e+00,2.021329e-01,1.010664e+00,0.500000,0.500000,0.600000,0.400000,0
```
