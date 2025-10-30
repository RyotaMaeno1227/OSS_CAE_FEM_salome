# chrono-C 2D 複合拘束仕様草案

この文書は、`chrono-C-all` に距離＋角度複合拘束や線形結合（カップリング）拘束を導入するための設計案と API 草案をまとめたものです。距離＋角度拘束（`ChronoDistanceAngleConstraint2D_C`）は既に初期実装済みで、ここでは後続の線形結合拘束 `ChronoCoupledConstraint2D_C` に焦点を当てます。既存の `ChronoConstraint2DOps_C` / `ChronoConstraint2DBase_C` フレームワークを前提とし、最小限の互換性を保ちながら機能拡張を行います。

## 1. 目標と適用範囲

- 距離拘束に角度管理を加えた「Distance + Angle」拘束、及び複数拘束の線形結合（ギア比・距離比）を扱える `ChronoCoupledConstraint2D_C` を追加。
- 2D 平面内での典型的な機構（スライダークランク、差動、リンク機構）を再現可能にする。
- 既存の距離・プラナー・プリズマティック拘束と API 体系を揃え、`chrono_constraint2d_batch_solve` で一括処理できるようにする。

## 2. 新構造体の概要

既に実装された `ChronoDistanceAngleConstraint2D_C` では距離・角度の独立ソフトネス、バネ / ダンパ、最新インパルスのログが利用できます。本節では初期版が実装された `ChronoCoupledConstraint2D_C` の拡張余地とチューニング指針に焦点を当てます。

### 2.1 `ChronoDistanceAngleConstraint2D_C`（実装済み）

| フィールド | 説明 |
|------------|------|
| `ChronoConstraint2DBase_C base` | 共通ヘッダ |
| `double local_anchor_a[2], local_anchor_b[2]` | 各ボディのローカルアンカー |
| `double rest_distance` | 目標距離 |
| `double rest_angle` | 目標角度差（`body_b.angle - body_a.angle`） |
| `double axis_local[2]` | 拘束方向基準（任意） |
| `double distance_softness_linear/angle_softness` | 距離・角度コンプライアンス |
| `double distance_spring_stiffness/damping` | 距離スプリングパラメータ |
| `double angle_spring_stiffness/damping` | 角度スプリングパラメータ |
| `double last_distance_impulse/last_angle_impulse` | 直近インパルスログ |
| `double cached_dt` | 前回タイムステップ |

#### 主な API

```c
void chrono_distance_angle_constraint2d_init(ChronoDistanceAngleConstraint2D_C*, ...);
void chrono_distance_angle_constraint2d_set_rest_distance(...);
void chrono_distance_angle_constraint2d_set_rest_angle(...);
void chrono_distance_angle_constraint2d_set_distance_softness(...);
void chrono_distance_angle_constraint2d_set_angle_softness(...);
void chrono_distance_angle_constraint2d_set_distance_spring(...);
void chrono_distance_angle_constraint2d_set_angle_spring(...);
```

#### テスト計画
- `tests/test_distance_angle_constraint.c` を追加し、距離＋角度拘束が単独および複数混在で安定するか検証。
- 角度のみを変化させたケース（距離固定）と距離のみ変化させたケースを用意。

### 2.2 `ChronoCoupledConstraint2D_C`（複数式対応）

距離と相対角の一次結合 `\sum_i (ratio_distance_i * (d - d0) + ratio_angle_i * (θ - θ0)) = offset_i` を維持する拘束。最大 4 本の式を同時に扱い、式ごとにソフトネスやスプリング、ログを個別に保持できます。

| フィールド | 説明 |
|------------|------|
| `equation_count` / `equation_active[]` | 有効な線形式の本数と解法で使用中かどうかのフラグ。動的に式を追加・無効化できる。 |
| `ratio_distance_eq[]` / `ratio_angle_eq[]` | 各線形式に対する距離・角度係数。距離のみ/角度のみの拘束にも利用可能。 |
| `target_offset_eq[]` | 各式の目標値。ステージ切替や比率変更時に個別更新可能。 |
| `softness_distance_eq[]` / `softness_angle_eq[]` | 距離（並進）側と角度（回転）側のコンプライアンスを式ごとに独立設定。0 で剛体拘束。 |
| `spring_distance_*_eq[]` / `spring_angle_*_eq[]` | 任意のバネ剛性 / 減衰係数を各式に付与。ターゲット切替時のスムージング用途。 |
| `last_impulse_eq[]` / `last_distance_impulse_eq[]` / `last_angle_impulse_eq[]` | 直近の式別インパルス（N*s / N*m*s）。 |
| `last_distance_force_eq[]` / `last_angle_force_eq[]` | `solve_velocity` 射影結果（インパルス / dt）とスプリング反力の合算。CSV や可視化に利用可能。 |
| `inv_mass_matrix[][]` | 有効な線形式に対する 1〜4 次の有効質量行列（SPD）。Gauss-Jordan による小規模解でホットキャッシュ化。 |
| `diagnostics` | `flags`（ランク欠損・条件数超過）、`rank`、`condition_number`、`min/max_pivot` を格納。`chrono_coupled_constraint2d_get_diagnostics` で取得可能。 |
| `condition_policy` | 条件数閾値超過時のロギングクールダウン、最小ピボット式の自動ドロップ許可数などを保持するポリシー。`chrono_coupled_constraint2d_set_condition_warning_policy` で設定。 |

`ChronoCoupledConditionWarningPolicy_C` は `enable_logging`（初期値 1）、`log_cooldown`（初期値 0.25 s）、`enable_auto_recover`（初期値 0）、`max_drop`（初期値 1）を持つシンプルな構造体。デフォルトでは標準エラーへ診断を出力するのみで、`enable_auto_recover` を 1 にすると条件数閾値を超えた際に最小対角要素を持つ式を最大 `max_drop` 本まで自動で無効化し、現行ステップの解を継続させる。

#### 追加 API

```c
void chrono_coupled_constraint2d_clear_equations(ChronoCoupledConstraint2D_C*);
int  chrono_coupled_constraint2d_add_equation(ChronoCoupledConstraint2D_C*, const ChronoCoupledConstraint2DEquationDesc_C*);
int  chrono_coupled_constraint2d_set_equation(ChronoCoupledConstraint2D_C*, int index, const ChronoCoupledConstraint2DEquationDesc_C*);
int  chrono_coupled_constraint2d_get_equation_count(const ChronoCoupledConstraint2D_C*);
const ChronoCoupledConstraint2DDiagnostics_C *chrono_coupled_constraint2d_get_diagnostics(const ChronoCoupledConstraint2D_C*);
void chrono_coupled_constraint2d_get_condition_warning_policy(const ChronoCoupledConstraint2D_C*,
                                                              ChronoCoupledConditionWarningPolicy_C *out_policy);
void chrono_coupled_constraint2d_set_condition_warning_policy(ChronoCoupledConstraint2D_C*,
                                                              const ChronoCoupledConditionWarningPolicy_C *policy);

void chrono_coupled_constraint2d_set_softness_distance(ChronoCoupledConstraint2D_C*, double softness);
void chrono_coupled_constraint2d_set_softness_angle(ChronoCoupledConstraint2D_C*, double softness);
void chrono_coupled_constraint2d_set_distance_spring(ChronoCoupledConstraint2D_C*, double stiffness, double damping);
void chrono_coupled_constraint2d_set_angle_spring(ChronoCoupledConstraint2D_C*, double stiffness, double damping);
```

`chrono_coupled_constraint2d_set_softness` は後方互換のため残しつつ、式 0 への設定を内部で委譲する形へ更新済みです。

#### テスト
- `tests/test_coupled_constraint.c` で距離ターゲットと角度ターゲットをステージ別に切り替え、複数式のログ出力と診断フラグが安定することを確認。条件数が閾値を超えた際に `chrono_coupled_constraint2d_set_condition_warning_policy` を通じて自動式ドロップが発火し、アクティブ式数と `diagnostics.rank` が同期することも検証します。
- `tests/test_coupled_constraint_endurance.c` は 7,200 ステップの耐久テストを実行し、`data/coupled_constraint_endurance.csv` に式別反力・インパルス・診断フラグを記録して推奨パラメータを評価。生成 CSV は `tools/plot_coupled_constraint_endurance.py` で即時可視化できる。

#### 次のステップ
- 条件数警告時にユーザーレベルのコールバックやログストリームへルーティングする仕組み（現在は標準エラー出力／最小対角式のドロップのみ）。
- 耐久テストを CI に接続する際の運用設計と、生成 CSV の要約を自動チェックする仕組み。Python 可視化 (`tools/plot_coupled_constraint_endurance.py`) をレポート生成フローに組み込む。
- 比率切替と目標値補間の API 化（ステージ遷移時の連続化サポート）。

## 3. 実装ステップ

1. [done] `ChronoDistanceAngleConstraint2D_C` の基礎を実装。距離拘束+角度拘束を同時に処理する Gauss-Seidel ステップを実装。
2. [done] 回帰テスト (`test_distance_angle_constraint.c`) を作成し、距離のみ・角度のみ・両方のケースを検証。
3. [done] `ChronoCoupledConstraint2D_C` の初期版を実装。距離+角度の線形結合 (Slider-Crank) をサポートし、個別ソフトネス/スプリング/ログを追加。
4. [done] 汎用的な線形結合 API の設計と、既存拘束を参照する仕組みを整備。
5. [done] README / ロードマップを更新し、想定する用途とサンプルコードを追加。

## 4. 未決定事項と検討

- `ChronoCoupledConstraint2D_C` が他拘束のインスタンスを直接参照するのか、内部に独立パラメータを持つのか。
- 複合拘束に対するソフトネスやスプリング設定をどこまで提供するか。
- 2D 限定の仕様にとどめるか、将来的な 3D 展開を想定した抽象化を行うか。

## 5. チューニング指針メモ

- 基本設定: `chrono_coupled_constraint2d_set_baumgarte(constraint, 0.3-0.4)`、`set_slop(5e-4)`、`set_max_correction(0.08)` を起点にすると距離と角度の両方で位置誤差が収束しやすい。
- ソフトネス: 距離側は 0.01-0.02 程度、角度側は 0.02-0.04 程度から調整。角度を厳密に合わせたい場合は距離側よりも大きな値を与えて角速度の振れを抑える。
- スプリング: ステージ切替えなどでターゲットが急変する場合は距離スプリングを 30-45 N/m、角度スプリングを 15-25 N*m/rad に設定し、減衰 0.6-1.0 を目安にする。スプリングを無効化するには剛性または減衰を 0 に設定する。
- ログ: `last_distance_force` / `last_angle_force` はインパルス由来の反力とスプリング反力を合算した値を返す。各ステップの `constraint->cached_dt` で除算しているため、固定ステップであればそのまま力・トルクの推移をグラフ化できる。

---

本仕様草案をベースに、次の開発サイクルで実装・テストに着手する。
