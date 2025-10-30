# chrono-C 2D 複合拘束仕様草案

この文書は、`chrono-C-all` に距離＋角度複合拘束や線形結合（カップリング）拘束を導入するための設計案と API 草案をまとめたものです。距離＋角度拘束（`ChronoDistanceAngleConstraint2D_C`）は既に初期実装済みで、ここでは後続の線形結合拘束 `ChronoCoupledConstraint2D_C` に焦点を当てます。既存の `ChronoConstraint2DOps_C` / `ChronoConstraint2DBase_C` フレームワークを前提とし、最小限の互換性を保ちながら機能拡張を行います。

## 1. 目標と適用範囲

- 距離拘束に角度管理を加えた「Distance + Angle」拘束、及び複数拘束の線形結合（ギア比・距離比）を扱える `ChronoCoupledConstraint2D_C` を追加。
- 2D 平面内での典型的な機構（スライダークランク、差動、リンク機構）を再現可能にする。
- 既存の距離・プラナー・プリズマティック拘束と API 体系を揃え、`chrono_constraint2d_batch_solve` で一括処理できるようにする。

## 2. 新構造体の概要

既に実装された `ChronoDistanceAngleConstraint2D_C` では距離・角度の独立ソフトネス、バネ／ダンパ、最新インパルスのログが利用できます。本節では未実装の `ChronoCoupledConstraint2D_C` に焦点を当てます。

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

### 2.2 `ChronoCoupledConstraint2D_C`（計画中）

複数の既存拘束（距離、角度、スライダ等）の線形結合を維持する汎用拘束。初期フェーズでは下記の簡易版を想定。

- 2 body + 2 拘束値 (距離・角度) の線形結合 `a * distance + b * angle = c` を保持。
- 追加の `constraint_ids` を保持し、既存拘束と組み合わせる場合のフックとして利用。
- API: `chrono_coupled_constraint2d_init(distance_constraint, angle_constraint, ratio_distance, ratio_angle, offset)`。
- 将来的には複数ボディ・複数拘束の連成へ拡張する余地を残す。

#### テスト計画
- `tests/test_coupled_constraint.c` を追加し、スライダークランクや距離比を保つギア同等の挙動を確認。
- 連成によって系が過拘束にならないか、エラーログを出す仕組みを導入。

## 3. 実装ステップ

1. `ChronoDistanceAngleConstraint2D_C` の基礎を実装。距離拘束＋角度拘束を同時に処理する Gauss-Seidel ステップを実装する。
2. 回帰テスト (`test_distance_angle_constraint.c`) を作成し、距離のみ・角度のみ・両方のケースを検証。
3. `ChronoCoupledConstraint2D_C` の初期版を実装。距離＋角度の線形結合 (Slider-Crank) をサポート。
4. 汎用的な線形結合 API の設計と、既存拘束を参照する仕組みを整備。
5. README / ロードマップを更新し、想定する用途とサンプルコードを追加。

## 4. 未決定事項と検討

- `ChronoCoupledConstraint2D_C` が他拘束のインスタンスを直接参照するのか、内部に独立パラメータを持つのか。
- 複合拘束に対するソフトネスやスプリング設定をどこまで提供するか。
- 2D 限定の仕様にとどめるか、将来的な 3D 展開を想定した抽象化を行うか。

---

本仕様草案をベースに、次の開発サイクルで実装・テストに着手する。
