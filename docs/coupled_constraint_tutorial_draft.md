# Coupled Constraint Tutorial (Draft)

Coupled 拘束（距離＋角度の線形結合）を理解し実装へ反映するためのドラフト教材です。  
FEM4C チュートリアルの学習サイクルに倣い、次の 4 ステップを繰り返しながら読み進めてください。

1. **数式を整理する** – `docs/coupled_constraint_solver_math.md` をベースに、拘束方程式と行列構造を理解する。  
2. **実装を読む** – `chrono-C-all/src/chrono_constraint2d.c` の該当関数を追い、データフローを確認する。  
3. **自分で確かめる** – 小さなスニペットやパラメータ変更で挙動を体験する。  
4. **テストで検証する** – 公式テストスイートを走らせ、数値特性が維持されていることを確認する。

---

## 1. 数式フェーズ（Theory）

### 1.1 連立式の骨子
- Coupled 拘束の各式は  
  ```math
  \phi_i = r^{(i)}_d C_d + r^{(i)}_\theta C_\theta - b^{(i)}_{\text{target}}
  ```  
  で定義される（`docs/coupled_constraint_solver_math.md` 参照）。
- 距離・角度の有効質量は `M_d^{-1}`・`M_θ^{-1}` へ分解でき、比率 `r_d`, `r_θ` を組み合わせることで式間の相互作用を表現する。
- 対角成分にはソフトネス `γ_i` を加算し、正定性と安定性を確保する。

### 1.2 行列分解と条件数
- `coupled_constraint_invert_matrix` では最大 4×4 のガウス消去を行い、部分ピボットと Pivot ε によってランク欠損を検知する。
- `coupled_constraint_condition_bound` の行和ノルムで簡易条件数 `κ̂` を求め、閾値超過時は式の自動ドロップ（最小対角）を試みる。

#### 演習（Notebook / MATLAB）
1. `docs/coupled_constraint_solver_math.md` の式 (1) を写経し、任意の比率・ソフトネスで行列 `K` を組み立てる。
2. Python または MATLAB で `numpy.linalg.cond` を使い、`κ̂` と本来の条件数を比較する。
3. `constraint->ratio_distance = 1.0`, `ratio_angle = 0.4` のケースで、`γ` を 0 と 0.02 に変えたときの `K` を計算し、ソフトネスの効果を可視化する。

---

## 2. 実装フェーズ（Implementation）

| 観点 | 関数 / ファイル | 説明 |
|------|----------------|------|
| 初期化 | `chrono_C-all/src/chrono_constraint2d.c:968` `chrono_coupled_constraint2d_init` | アンカー、軸、初期比率、バネ・ダンパを登録し、Equation バッファを整える。 |
| 行列構築 | `chrono_constraint2d.c:1410`-`1460` | 距離・角度の有効質量、距離誤差 `C_d`、角度誤差 `C_θ` を算出。 |
| 逆行列計算 | `chrono_constraint2d.c:258`-`348` `coupled_constraint_invert_matrix` | ガウス消去で `system_matrix` を反転し、Pivot 情報をダイアグノスティクスへ記録。 |
| 条件数判定 | `chrono_constraint2d.c:351`-`360` `coupled_constraint_condition_bound` | 行和ノルムで `κ̂` を推定。 |
| 自動ドロップ | `chrono_constraint2d.c:1546`-`1605` | `condition_policy` に従い式を間引く処理。 |
| ソルバ入口 | `chrono_constraint2d.c:1638`-`1921` | `solve_velocity` / `solve_position` が `inv_mass_matrix` を使って拘束インパルスを計算。 |

### コードリーディング課題
1. `chrono_coupled_constraint2d_prepare_impl` の最初と最後を読んで、どのフィールドが同期されるのかメモする。  
2. `condition_policy` の初期化 (`chrono_constraint2d.c:1009`) とログ出力 (`chrono_constraint2d.c:1568`) をたどり、WARN が出る条件を整理する。  
3. `chrono_constraint2d.c:1678` 以降の速度ソルバで、`lambda` 更新式が距離／角度の残差にどう寄与するかコメントを追加してみる（ローカル環境推奨）。

---

## 3. テストフェーズ（Verification）

| テスト | 目的 | コマンド例 |
|--------|------|------------|
| `tests/test_coupled_constraint` | 基本比率・ターゲット切替えの回帰 | `./chrono-C-all/tests/test_coupled_constraint` |
| `tests/test_coupled_constraint_endurance` | 7200 ステップ耐久と式ドロップ挙動 | `./chrono-C-all/tests/test_coupled_constraint_endurance` |
| `tests/bench_coupled_constraint` | 条件数・反復回数のマイクロベンチ | `./chrono-C-all/tests/bench_coupled_constraint --output coupled_bench.csv` |
| `tests/test_island_parallel_contacts` | アイランド分割と並列解決の一貫性 | `./chrono-C-all/tests/test_island_parallel_contacts` |

### テスト後チェック
- CSV/ログを `tools/plot_coupled_constraint_endurance.py --summary-json` で解析し、`condition_warning` フラグと `rank` が期待通りか確かめる。
- 自動ドロップが発生したときは `diagnostics.rank` とアクティブ式数が一致しているか確認。

---

## 4. 実践ミニ課題

1. **パラメータ探索**  
   - `chrono_coupled_constraint2d_set_ratios` を呼び出すヘルパスクリプトを書き、比率 (1.0, 0.4) → (0.48, -0.32) へ 3 ステップで遷移させる。  
   - 各ステップで `diagnostics.condition_number` を収集し、棒グラフ化する。

2. **Pivot 観察**  
   - `CHRONO_COUPLED_PIVOT_EPSILON` を 1e-9 → 1e-7 に変更し、`test_coupled_constraint_endurance` を実行。  
   - WARN 発生回数・ドロップ数の差分を記録し、Pivot 閾値が挙動へ与える影響を考察。

3. **島ソルバ連携**  
   - `chrono_island2d_build` で生成された島のうち Coupled 拘束を含むものについて、`constraint_count` をログする。  
   - 並列実行 (`CHRONO_ENABLE_OPENMP`) を ON/OFF して実行時間と結果差分を比較。

---

## 5. クイックリンク（学習用マップ）

| トピック | リンク / ファイル | メモ |
|----------|------------------|------|
| 理論ノート | `docs/coupled_constraint_solver_math.md` | 行列式、Pivot 方針、条件数評価を整理。 |
| 実装コード | `chrono-C-all/src/chrono_constraint2d.c` | `prepare_impl` / `solve_velocity` / `solve_position` / `condition_policy`。 |
| 公開 API | `docs/coupled_contact_api_minimal.md` | Init / Solve / Diagnostics に分けて参照。 |
| テストスイート | `chrono-C-all/tests/test_coupled_constraint*.c` | Endurance・ベンチ含む公式テスト。 |
| 可視化ツール | `tools/plot_coupled_constraint_endurance.py` | CSV → Summary/Plot。 |
| アイランド連携 | `chrono-C-all/src/chrono_island2d.c` | Coupled を含む島のスケジューリング確認。 |
| 追加リーディング | `docs/coupled_island_migration_plan.md` | 3D 互換化へのロードマップ。 |

---

> ここで扱った内容は、今後予定されている 3D 版 Coupled 拘束（`docs/chrono_3d_abstraction_note.md`）の基礎にもなるため、ログ・条件数・島ソルバの観点を押さえておくと移行作業の理解がスムーズになります。

