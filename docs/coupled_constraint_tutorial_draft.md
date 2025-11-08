# Coupled Constraint Tutorial (JP/EN)

Coupled 拘束（距離＋角度の線形結合）を理解し実装へ反映するためのチュートリアルです。  
FEM4C の学習サイクル（Understand → Implement → Inspect → Verify）に倣い、以下の 4 ステップで読み進めてください。

> 学習パス統合（Appendix E）の進捗: W2（Hands-on 側 TODO 整理）は進行中、W3（リンク検証自動化）は本バージョンで着手しました。詳細は `docs/integration/learning_path_map.md` を参照してください。

1. **Theory / 数式** – `docs/coupled_constraint_solver_math.md` で方程式と行列構造を確認。  
2. **Implementation / 実装** – `chrono-C-all/src/chrono_constraint2d.c` を追ってデータフローを把握。  
3. **Hands-on / 体験** – パラメータを変更したりミニスクリプトを動かして挙動を観察。  
4. **Verification / 検証** – 公式テストスイートを実行し、数値特性が維持されていることを確認。

---

## 1. Theory / 数式フェーズ

### 1.1 Equation Skeleton（連立式の骨子 / Skeleton）
```math
\phi_i = r^{(i)}_d C_d + r^{(i)}_\theta C_\theta - b^{(i)}_{\text{target}}
```
- 比率 `r_d`, `r_θ` が距離・角度残差をブレンドする / ratios blend distance and angle residuals.  
- 有効質量 `M_d^{-1}`, `M_θ^{-1}` は `chrono_constraint2d.c:1410-1431` で計算される / effective masses are computed around lines 1410-1431.  
- 対角にはソフトネス `γ_i` を加算して正定性を確保（`docs/coupled_constraint_solver_math.md#1-連立式の構造`）/ add diagonal softness (`γ_i`) per Solver Math §1 to keep the block positive definite.

### 1.2 Decomposition & Condition Numbers
- 4×4 行列を `coupled_constraint_invert_matrix`（`chrono_constraint2d.c:258-348`）で部分ピボット付きガウス消去 / the 4×4 block is factorised via scaled partial pivoting (lines 258-348).  
- 行和ノルムによる `κ̂` とスペクトル推定 `κ_s` を診断に保存（`docs/coupled_constraint_solver_math.md#3-条件数評価と式ドロップ`）/ store both row-sum (`κ̂`) and spectral (`κ_s`) indicators per Solver Math §3.  
- WARN 発生時は最弱式を自動ドロップ（`chrono_constraint2d.c:1556-1605`）/ when the warning policy fires, drop the weakest equation (lines 1556-1605).

#### Numerical sample
```python
import numpy as np
ratio = np.array([[1.0, 0.40],
                  [0.55, -0.25]])
inv_mass = np.diag([0.018, 0.010])
gamma = np.diag([0.014, 0.028])
K = ratio @ inv_mass @ ratio.T + gamma
print(np.linalg.cond(K))
```

> Hands-on: `docs/coupled_constraint_hands_on.md` Chapter 01 ではこの式を使って比率スイープを行う。

---

## 2. Implementation / 実装フェーズ

| 観点 | 関数 / ファイル | 説明 / Description |
|------|----------------|--------------------|
| 初期化 | `chrono_coupled_constraint2d_init` (`chrono_constraint2d.c:968`) | アンカー、軸、比率、バネ・ダンパを登録 / register anchors, axes, ratios, and spring/damper params. |
| 行列構築 | `chrono_constraint2d.c:1410-1460` | 有効質量と距離/角度残差を計算 / build effective masses and residuals. |
| 逆行列計算 | `coupled_constraint_invert_matrix` (`chrono_constraint2d.c:258-348`) | ピボット情報を記録し `inv_mass_matrix` を更新 / store pivot info and update `inv_mass_matrix`. |
| 条件数判定 | `coupled_constraint_condition_bound` (`chrono_constraint2d.c:351-360`) | `κ̂` を算出 / produce the row-sum condition estimate. |
| ソルバ入口 | `chrono_constraint2d.c:1638-1921` | `solve_velocity` / `solve_position` が拘束インパルスを適用 / velocity + position phases apply impulses. |

### 2.1 Code Snippet
```c
ChronoCoupledConstraint2D_C coupled;
chrono_coupled_constraint2d_init(&coupled, body_a, body_b,
                                 anchor_a, anchor_b,
                                 axis_local,
                                 1.0, 0.0,   // rest distance / angle
                                 1.0, 0.40); // ratios

chrono_coupled_constraint2d_set_softness_distance(&coupled, 0.014);
chrono_coupled_constraint2d_set_softness_angle(&coupled, 0.028);

chrono_coupled_constraint2d_prepare(&coupled, dt);
chrono_coupled_constraint2d_apply_warm_start(&coupled);
chrono_coupled_constraint2d_solve_velocity(&coupled);
chrono_coupled_constraint2d_solve_position(&coupled);
```

### 2.2 Reading checklist
1. `chrono_coupled_constraint2d_prepare_impl` 冒頭と末尾で同期されるフィールドをメモ / list the fields synced at the start/end of `prepare_impl`.  
2. `condition_policy` 初期化 (`chrono_constraint2d.c:1009`) と WARN 出力 (`chrono_constraint2d.c:1568`) をトレース / trace from init through the WARN path.  
3. Hands-on Chapter 02 でソフトネス／バネの効果を CSV へ記録 / log the softness vs spring sweep (Hands-on Ch.02).

---

## 3. Verification / テストフェーズ

| テスト | 目的 | コマンド例 |
|--------|------|------------|
| `tests/test_coupled_constraint` | 基本比率・ターゲット切替え | `./chrono-C-all/tests/test_coupled_constraint` |
| `tests/test_coupled_constraint_endurance` | 7 200 ステップ耐久 & 自動ドロップ | `./chrono-C-all/tests/test_coupled_constraint_endurance` |
| `tests/bench_coupled_constraint` | 条件数ベンチ & CSV 出力 | `./chrono-C-all/tests/bench_coupled_constraint --omega 0.85 --omega 1.0 --output data/bench.csv` |
| `tests/test_island_parallel_contacts` | 島分割と並列解決の一致検証 | `./chrono-C-all/tests/test_island_parallel_contacts` |

- `ChronoCoupledConstraint2DDiagnostics_C`（共通ヘッダ `chrono_constraint_common.h`）は `pivot_log[]` と `log_level_{request,actual}` を含み、WARN/INFO 切り替えや pivot 推移を 3D 版と同一形式で収集できる。

### After running
- `tools/plot_coupled_constraint_endurance.py --summary-json out.json --mark-stage 1200:"ratio swap"` で CSV を可視化 / visualise the CSV with stage markers.  
- `diagnostics.rank` とアクティブ式数が一致するか確認 / ensure `diagnostics.rank == active_equations`.  
- Contact との併用評価は `docs/coupled_contact_test_notes.md` を参照 / see Coupled + Contact notes for mixed tests.

---

## 4. Hands-on / 実践ミニ課題

1. **Preset sweep** – `data/coupled_constraint_presets.yaml` に `optic_alignment_trim` を追加し、`tests/test_coupled_constraint` のスイープへ組み込む。  
2. **Pivot tuning** – `CHRONO_COUPLED_PIVOT_EPSILON` を `1e-7` に変更し、`test_coupled_constraint_endurance` の WARN/DROP を比較。  
3. **Island integration** – Hands-on Chapter 03 (`docs/coupled_constraint_hands_on.md`) を進め、Contact + Coupled の同居を確認。

---

## 5. Quick links / クイックリンク

| Topic | リンク | 備考 |
|-------|--------|------|
| Solver math | `docs/coupled_constraint_solver_math.md` | コード・テストへのリンクを節ごとに追加。 |
| Hands-on guide | `docs/coupled_constraint_hands_on.md` | FEM4C 形式のステップバイステップ課題。 |
| Minimal API (JP/EN) | `docs/coupled_contact_api_minimal.md`, `docs/coupled_contact_api_minimal_en.md` | Init / Solve / Diagnostics をフェーズ別に整理。 |
| Coupled + Contact notes | `docs/coupled_contact_test_notes.md` | 島テストの判定基準。 |
| 3D migration | `docs/coupled_island_migration_plan.md`, `docs/chrono_3d_abstraction_note.md` | KPI とガントを確認。 |
| Learning path map | `docs/integration/learning_path_map.md` | Hands-on ↔ Tutorial の統合ロードマップ。 |

> ここで扱った内容は 3D 版 Coupled 拡張の基礎でもあるため、診断・島ソルバ・条件数ログの観点を押さえておくと移行計画（`docs/coupled_island_migration_plan.md`）にもスムーズに参加できます。  
> 2025-11-08 時点で Hands-on / Solver Math / Contact Notes へのリンクを Appendix B.7 のチェックリストで検証済み。次回は同チェックリストに沿って更新してください。
