# ChronoCoupledConstraint2D チュートリアル

`ChronoCoupledConstraint2D_C` を使って距離と角度を連動させる典型的なユースケースを、パラメータセットと ASCII 図入りでまとめたチュートリアルです。  
各シナリオは **距離比（ratio_distance）**、**角度比（ratio_angle）**、**柔構造（softness / spring）** の組み合わせを段階的に設定できるようにしています。

> テストや可視化スクリプトは `tests/test_coupled_constraint*.c`、`tools/plot_coupled_constraint_endurance.py` を参照してください。
> 各ユースケースのパラメータは `data/coupled_constraint_presets.yaml` にも整理してあり、スクリプトからそのまま読み込めます。

---

## 1. テレスコピック＋ヨー制御

```
 [Base]───(Boom)─────────●  <-- Load hook
            |            ↖ 角度θ
            └─軸ベクトル→
```

| 項目 | 値 | メモ |
|------|----|------|
| `ratio_distance` | 1.0 | ブーム延伸が主。 |
| `ratio_angle` | 0.4 | ヨー角を 40% 混ぜる。 |
| `softness_distance` | 0.014 | 中庸なコンプライアンス。 |
| `softness_angle` | 0.028 | 距離側の約 2 倍で回転揺れを吸収。 |
| `distance_spring` (`k`, `c`) | 38.0 N/m, 3.0 N·s/m | 急な目標変更でも伸長をフォロー。 |
| `angle_spring` (`k`, `c`) | 18.0 N·m/rad, 0.85 N·m·s/rad | 角度の追従性を確保。 |

**設定手順**
1. アンカーと可動ボディのローカルアンカーを `chrono_coupled_constraint2d_init` で登録。
2. Baumgarte と Slop を `0.38`, `6e-4` に設定して位置ドリフトを抑制。
3. 上記テーブルの柔構造とスプリング係数を適用。
4. 90° 回り込みなど大きな角度変更を行う場合は `max_correction` を `0.09` へ引き上げる。
5. 条件数警告を監視するには `chrono_coupled_constraint2d_get_diagnostics` の `condition_number` をログへ記録し、`tools/plot_coupled_constraint_endurance.py` で推移を確認。

---

## 2. カム機構の追従補正

```
 Cam follower
     │
     ▼
 [カムプロフィール]───●───→ 距離
                       ↘ 角度オフセット
```

| 項目 | 値 | メモ |
|------|----|------|
| `ratio_distance` | 0.48 | 距離成分を半分以下に抑える。 |
| `ratio_angle` | -0.32 | 逆位相で角度エラーを補償。 |
| `target_offset` | 0.012 | カム位相遅れを調整。 |
| `softness_distance` | 0.018 | 局所的に柔らかく。 |
| `softness_angle` | 0.024 | 距離よりやや硬め。 |
| `distance_spring` (`k`, `c`) | 24.0 N/m, 2.5 N·s/m | プロフィール変化に追従。 |
| `angle_spring` (`k`, `c`) | 12.0 N·m/rad, 0.75 N·m·s/rad | 角速度ピークを緩和。 |

**設定手順**
1. 初期式（index 0）で上記パラメータを設定。
2. プロフィールが急変する箇所で `ChronoCoupledConstraint2DEquationDesc_C` を更新し、`chrono_coupled_constraint2d_set_equation` で差し替える。
3. 目標が急激に変わるタイミングでは事前に `target_offset` を 0 へ戻し、ステップ内で段階的に新値を適用。
4. ログでは `last_distance_force_eq[0]` と `last_angle_force_eq[0]` を CSV 化。角度側のピークが距離側の 1.5 倍を超える場合は `softness_angle` を追加で 0.004 程度増やす。

---

## 3. カウンターバランス梁

```
 トルクアーム      荷重
     ↓             ↓
 [Fulcrum]───┬───────●
             │
             └─── カウンタウェイト
```

| 項目 | 値 | メモ |
|------|----|------|
| `ratio_distance` | 0.85 | 主拘束を距離優先に。 |
| `ratio_angle` | -0.30 | モーメントを逆位相で調整。 |
| `softness_distance` | 0.013 | 剛性寄りに設定。 |
| `softness_angle` | 0.022 | 振動抑制用。 |
| `distance_spring` (`k`, `c`) | 42.0 N/m, 3.2 N·s/m | 支点距離を強く保持。 |
| `angle_spring` (`k`, `c`) | 20.0 N·m/rad, 0.9 N·m·s/rad | カウンタウェイトの遅れを抑制。 |

**設定手順**
1. `chrono_coupled_constraint2d_add_equation` で補助式（index 1）を追加し、`ratio_distance = 0.35`, `ratio_angle = 0.65`（角度主導）を登録。ドロップ時の保険として機能。
2. 耐久テスト相当のシナリオでは `chrono_coupled_constraint2d_set_condition_warning_policy` で `enable_auto_recover=1`, `max_drop=1` を設定し、極端な条件数で補助式のみをドロップ可能にする。
3. ログはメイン式と補助式で分け、補助式の力が 0 近傍で安定しているか確認。過剰なら `softness_distance_eq[1]` を 0.02 へ引き上げる。

---

## 4. ドッキングガイド（位置＋姿勢合わせ）

```
   受け側プレート ┌───┐
                   │   │
   ----------------┘ ● └---------------- 移動側プレート
         距離Δ           角度Δ
```

| 項目 | 値 | メモ |
|------|----|------|
| `ratio_distance` | 0.72 | 位置合わせを優先。 |
| `ratio_angle` | -0.25 | 角度のズレを徐々に矯正。 |
| `target_offset` | 0.0 → 0.02 | 挿入開始後にバイアス追加。 |
| `softness_distance` | 0.02 | 接触前の柔らかさを確保。 |
| `softness_angle` | 0.034 | 角度はやや硬めに。 |
| `distance_spring` (`k`, `c`) | 30.0 N/m, 2.6 N·s/m | |
| `angle_spring` (`k`, `c`) | 14.0 N·m/rad, 0.8 N·m·s/rad | |

**設定手順**
1. 初期ステージでは `target_offset=0.0` とし、キャプチャ後（例: ステップ 1200）に `0.02` へ切り替える。
2. `ratio_distance` を `0.55`、`ratio_angle` を `-0.35` に変化させるステージを挟むと、最終合わせ込みが安定。
3. 接触と併用する場合は摩擦や反力が増えるため、ソルバ反復を `velocity_iterations=24`, `position_iterations=6` に引き上げておく。
4. CI での回帰時は `diagnostics.flags & CHRONO_COUPLED_DIAG_CONDITION_WARNING` が立つ頻度をチェックし、閾値超過が多い場合は `softness_distance` を 0.024 まで上げる。

---

## 5. ステップ実装テンプレート

```c
ChronoCoupledConstraint2D_C coupled;
chrono_coupled_constraint2d_init(&coupled,
                                 anchor,
                                 body,
                                 local_anchor,
                                 local_anchor,
                                 axis_local,
                                 initial_distance,
                                 initial_angle,
                                 ratio_distance,
                                 ratio_angle,
                                 target_offset);

// 1. 必須パラメータ
chrono_coupled_constraint2d_set_baumgarte(&coupled, 0.38);
chrono_coupled_constraint2d_set_slop(&coupled, 6e-4);
chrono_coupled_constraint2d_set_max_correction(&coupled, 0.09);

// 2. 柔構造とスプリング
chrono_coupled_constraint2d_set_softness_distance(&coupled, softness_distance);
chrono_coupled_constraint2d_set_softness_angle(&coupled, softness_angle);
chrono_coupled_constraint2d_set_distance_spring(&coupled, distance_k, distance_c);
chrono_coupled_constraint2d_set_angle_spring(&coupled, angle_k, angle_c);

// 3. 追加式
ChronoCoupledConstraint2DEquationDesc_C extra = {0};
extra.ratio_distance = 0.55;
extra.ratio_angle = -0.25;
extra.target_offset = -0.012;
extra.softness_distance = 0.02;
extra.softness_angle = 0.03;
chrono_coupled_constraint2d_add_equation(&coupled, &extra);

// 4. 警告ポリシー
ChronoCoupledConditionWarningPolicy_C policy;
chrono_coupled_constraint2d_get_condition_warning_policy(&coupled, &policy);
policy.enable_logging = 1;
policy.log_cooldown = 0.25;     // 0.25s ごとに WARN まで抑制
policy.enable_auto_recover = 1;
policy.max_drop = 1;
chrono_coupled_constraint2d_set_condition_warning_policy(&coupled, &policy);
```

---

## 6. ログと可視化ワークフローの併用

1. テストまたはアプリで `chrono_coupled_constraint2d_get_diagnostics` の値を CSV へ書き出す（例: `tests/test_coupled_constraint_endurance.c`）。
2. `tools/plot_coupled_constraint_endurance.py` を実行し、`--output figure.png` でグラフ化。  
   条件数のピークと `last_*_force_eq` の推移を重ねると、比率切替時の揺れが把握しやすい。
3. 目標値をステージごとに切り替える場合は CSV に「ステージ ID」列を追加しておくと、可視化で境界が明確になる。
4. WARN ログを INFO へ落とす際は、ポリシーの `enable_logging=0` ではなく、今後追加予定のログレベルフラグを利用する。
5. 可視化後は最大誤差や条件数を README のワークフローに沿って分析し、マイクロベンチで再現性を確認する。

---

## 7. 実行検証ログ（2025-10-21）

- **テスト実行**  
  - コマンド: `./tests/test_coupled_constraint`  
  - 結果: `Coupled constraint test passed.`（条件数警告は発生せず、診断ランクが能動式数と一致）
- **耐久 CSV の生成**  
  - コマンド: `./tests/test_coupled_constraint_endurance`  
  - 出力: `data/coupled_constraint_endurance.csv`（7200 サンプル、drop/recovery フィールド付き）  
  - WARN ログは `log_cooldown=0` 設定で連続発火するため、後続の CI ではポリシーでレベル抑制を推奨。
- **可視化サマリ**  
  - コマンド: `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot`  
  - 主な指標: `max condition number = 1.088e+01`、`condition warnings = 7200 (100%)`、`rank deficient = 0`  
    - 式別ピーク: `eq0 force_distance = 1.63e+01`, `eq0 force_angle = 3.35e+01`, `eq2 force_distance = 7.36e+00`, `eq2 force_angle = 1.33e+01`  
  - `matplotlib` が無い環境でも `--skip-plot` でテキストサマリを取得可能。プロットが必要な場合は `pip install matplotlib` を実行し、`--output` で画像を保存する。
