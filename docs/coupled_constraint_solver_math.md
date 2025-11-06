# Coupled Constraint Solver Notes

本メモは `ChronoCoupledConstraint2D_C` の数値解法を整理し、導出式とピボット選択ロジックを明文化する。実装参照: `chrono-C-all/src/chrono_constraint2d.c`.

## 1. 連立式の構造

Coupled 拘束は距離残差 `C_d` と角度残差 `C_θ` を線形結合した多式拘束で、各式 *i* の残差は

```math
\phi_i = r^{(i)}_d\,C_d + r^{(i)}_\theta\,C_\theta - b^{(i)}_{\text{target}}
```

で表される (式設定: `chrono-C-all/src/chrono_constraint2d.c:1474`-`chrono-C-all/src/chrono_constraint2d.c:1487`)。  
ここで `r^{(i)}_d`, `r^{(i)}_\theta` は比率、`b^{(i)}_{\text{target}}` はターゲットオフセット。

距離・角度の有効質量は

- `M_d^{-1} = Σ(m_a^{-1} + m_b^{-1}) + Σ((r_a × n)^2 I_a^{-1} + (r_b × n)^2 I_b^{-1})` (`chrono-C-all/src/chrono_constraint2d.c:1413`-`chrono-C-all/src/chrono_constraint2d.c:1424`)
- `M_θ^{-1} = Σ I^{-1}` (`chrono-C-all/src/chrono_constraint2d.c:1425`-`chrono-C-all/src/chrono_constraint2d.c:1431`)

で構成され、式 *i, j* 間の KKT ブロックは

```math
K_{ij} = r^{(i)}_d r^{(j)}_d M_d^{-1} + r^{(i)}_\theta r^{(j)}_\theta M_\theta^{-1}
```

として組み立てられる (`chrono-C-all/src/chrono_constraint2d.c:1500`-`chrono-C-all/src/chrono_constraint2d.c:1510`)。

対角にはソフトネス（コンプライアンス）項 `γ_i = r^{(i)2}_d S_d + r^{(i)2}_\theta S_\theta` を加算し正定性を補強する (`chrono-C-all/src/chrono_constraint2d.c:1482`-`chrono-C-all/src/chrono_constraint2d.c:1512`)。

## 2. ガウス消去とピボット選択

最大 4 式のローカル連立を **スケール付き部分ピボット** で解く。実装は `coupled_constraint_invert_matrix` (`chrono-C-all/src/chrono_constraint2d.c:258`-`chrono-C-all/src/chrono_constraint2d.c:348`) にあり、以下を実施する：

1. 各行の最大絶対値をスケールとして保持し、`|a(row,col)| / scale(row)` が最大となる行をピボットに選ぶ（小さい行に引きずられないよう正規化）。
2. Pivot が `pivot_epsilon` 以下ならランク欠損としてスキップ。
3. Pivot row を正規化し、他行から該当列を消去。
4. Pivot の最小・最大値を保存し、後段の条件数評価に利用。

迂回策として、逆行列が得られなかった場合はランクを下げて再試行する。

## 3. 条件数評価と式ドロップ

ガウス消去に成功した後、**行和ノルム**に基づく簡易条件数 `κ̂` と、ヤコビ法による **固有値ベース条件数** `κ_s` を両方算出する (`chrono-C-all/src/chrono_constraint2d.c:351`-`chrono-C-all/src/chrono_constraint2d.c:359`, `chrono-C-all/src/chrono_constraint2d.c:1711` 以降)。  
閾値判定には `max(κ̂, κ_s)` を使用し、閾値 `CHRONO_COUPLED_CONDITION_THRESHOLD` を超えるとワーニングを設定 (`chrono-C-all/src/chrono_constraint2d.c:1553`-`chrono-C-all/src/chrono_constraint2d.c:1577`)。`κ_s` と `κ̂` の差分（condition gap）は診断フィールドに記録され、テストベンチで誤差分析に用いる。

ワーニング発生時は最小対角項をもつ式をドロップし（`coupled_constraint_drop_weak_equation`）、再度分解を試みる (`chrono-C-all/src/chrono_constraint2d.c:1556`-`chrono-C-all/src/chrono_constraint2d.c:1567`)。ドロップ後も改善しなければ、`CHRONO_COUPLED_DIAG_CONDITION_WARNING` フラグのみを残して進行する。

## 4. 反復ステップとの連携

逆行列が確定した後、活性式だけを `inv_mass_matrix` に反映し (`chrono-C-all/src/chrono_constraint2d.c:1579`-`chrono-C-all/src/chrono_constraint2d.c:1595`)、後続の速度／位置ソルバで利用する。`bias_i` は Baumgarte 安定化項として `-β/dt · φ_i` で計算 (`chrono-C-all/src/chrono_constraint2d.c:1489`-`chrono-C-all/src/chrono_constraint2d.c:1495`)。

## 5. chrono-main との対比

- chrono-main では KKT 行列を `ChSystemDescriptor` で組み上げ、グローバル反復ソルバに渡す (`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:98`-`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:182`)。
- 反復ソルバの違反履歴管理やオーバーリラクゼーション制御は `ChIterativeSolverVI` が担う (`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:24`-`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:62`)。

今後はガウス消去から反復解法へ移行するため、ここで整理した式を KKT ブロック構築の基礎として利用する。
