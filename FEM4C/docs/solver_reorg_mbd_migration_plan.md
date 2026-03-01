# FEM4C MBD 設計書（Project Chrono 準拠 / C言語 / 2D簡略版）

最終更新: 2026-03-01  
対象: `FEM4C --mode=mbd`

---

## 1. この設計書の目的
- 本プロジェクトの MBD は「Project Chrono をそのまま使う」のではなく、以下の意図で作る。
  1. Chrono（C++/3D）を、C言語で理解しやすい形に再構成する。
  2. 3D ではなく 2D に絞って、計算の中身を学習しやすくする。
- ただし「作り直し」でも、将来 3D や Chrono 本体へ移行しやすい構成にする。
- 参照元は `third_party/chrono/chrono-main` のみとし、`chrono-C-all` は参照しない。

本書では、次を明確化する。
- 何を Chrono から真似るか
- 何を 2D/C 向けに簡略化するか
- どの順序で実装するか

---

## 2. 設計方針（重要）

### 2.1 真似るもの（Chrono準拠）
- データの責務分割:
  - Body（剛体状態）
  - Constraint（拘束式）
  - Descriptor/KKT（拘束付き連立方程式）
  - Integrator（時間積分）
  - System（1ステップ実行の上位制御）
- 実行フロー:
  1. 予測
  2. 拘束線形化（残差/Jacobian）
  3. 連立解法
  4. 補正
  5. 状態更新

### 2.2 簡略化するもの（本PJの制約）
- 言語: C++ 機能は使わず C で実装。
- 次元: 2D のみ（自由度は `x, y, theta`）。
- 連成: `coupled` は仕様確定まで凍結（スタブ維持）。
- 接触/摩擦: リリース1では対象外（後段）。

### 2.3 やりすぎ禁止（今回の反省を反映）
- 回帰基盤は必要最小限にする。
- 実装優先: 「機能追加 > テストラッパー追加」の順で進める。
- 目安: 1サイクル内の工数は「実装 70% / 回帰整備 30%」を上限目安とする。

---

## 3. 現在地点（2026-03-01）

### 3.1 実装済み
- `MBD_BODY / MBD_DISTANCE / MBD_REVOLUTE` の読込と検証
- 距離拘束・回転拘束の残差/Jacobian評価
- 拘束式数に基づく KKT レイアウト算出
- `--mode=mbd` 実行経路、CLI パラメータ受理、出力フォーマット

### 3.2 未実装（本体）
- 質量・慣性行列 `M(q)` の組立
- 重力/外力
- 速度・加速度状態 (`v, a`) の更新
- Newmark-β / HHT-α の「実時間発展」本体

結論: 現状は「拘束チェックツール」段階であり、動力学ソルバー本体はこれから。

---

## 4. Chrono 対応表（設計レベル）

| 役割 | Chrono 側の概念 | FEM4C 側 |
|---|---|---|
| 剛体 | `ChBody` 系 | `mbd_body2d_t`（新設予定） |
| 拘束 | `ChLink...` / `ChConstraint` 系 | `mbd_constraint2d_t`（既存） |
| 線形化 | constraint violation / Jacobian | `mbd_constraint_evaluate`（既存） |
| 方程式器 | descriptor / KKT 構造 | `mbd_kkt_*`（既存、拡張予定） |
| 時間積分 | Newmark/HHT 相当の時間更新 | `mbd_integrator_*`（新設予定） |
| システム実行 | `DoStepDynamics` 相当 | `analysis_run(..., mode=mbd)` で段階実装 |

注記:
- 本対応表は「同じ責務を持たせる」ための表であり、API名を完全一致させることは目的ではない。

---

## 5. 入力仕様（MBD）

### 5.1 既存仕様（維持）
- `MBD_BODY id x y theta`
- `MBD_DISTANCE id body_i body_j ai_x ai_y aj_x aj_y distance`
- `MBD_REVOLUTE id body_i body_j ai_x ai_y aj_x aj_y`

### 5.2 拡張仕様（次段で追加）
- `MBD_BODY_DYN id mass inertia x y theta vx vy omega`
- `MBD_GRAVITY gx gy`
- `MBD_FORCE body_id fx fy mz`

互換方針:
- 既存 `MBD_BODY` は引き続き有効。
- `MBD_BODY_DYN` 未指定時は既定値（設計書で規定）を適用し、警告を出す。

---

## 6. 物理モデルと単位
- 基本単位: SI（m, kg, s, N, rad）。
- 2D剛体は「単位厚みの板」として扱ってよい（厚み `t=1` を既定可）。
- ユーザー要求のような板金形状（穴あり）は、初期段階では次の順で対応する。
  1. まず `mass/inertia` を直接入力して解く。
  2. 次に形状から `mass/inertia` を計算する補助機能を追加する。

---

## 7. 数値解法（リリース1）

### 7.1 方程式
- 拘束付き運動方程式を KKT 形式で解く。
- 位置レベル拘束だけでなく、速度レベル整合も扱う。

### 7.2 積分器
- `Newmark-β`
- `HHT-α`

注意:
- 本PJでは両者を「実行時スイッチで選べる」実装にする。
- 現在はパラメータ受理のみで、時間発展の更新式は未実装。

---

## 8. 実装フェーズ（再定義）

### Phase A: 動力学コア最小実装（最優先）
- `mbd_body2d_t` に `mass, inertia, v, a` を追加
- 重力・外力組立
- 1ステップ更新（Newmark/HHTの最小版）
- 出力に `x,y,theta,vx,vy,omega` を時系列で保存

受入:
- 2リンク振り子（重力 + 初速度）を 10 秒計算できる。

### Phase B: Chrono準拠の構造整理
- Body/Constraint/Integrator/System の責務をヘッダで固定
- Chrono対応表に「参照元式・参照元実装」を追記

受入:
- 各モジュールの責務境界が崩れていない。

### Phase C: 解析品質と学習導線
- 最小回帰セット整備（壊れ検知）
- 初学者向けチュートリアル（2リンクを題材）整備

受入:
- 学習者が入力作成から実行・可視化まで追従できる。

---

## 9. テスト戦略（最小）
- 必須:
  - 単体: 拘束評価、KKT次元、入力異常系
  - 結合: 2リンク時系列（Newmark/HHT両方）
- 抑制:
  - 同じラッパーの派生テストを連鎖的に増やしすぎない。
  - 1つの仕様変更で 1〜2 本の直接回帰に留める。

---

## 10. 3D移行を見据えたルール
- 2D専用ロジックは隔離し、`dof_per_body=3` を定数化して管理する。
- 3D移行時に差し替える層を固定:
  - Body state
  - Constraint Jacobian
  - Inertia/rotation update
- APIの意味は維持し、内部次元のみ差し替え可能にする。

---

## 11. 実装時の必須記録
- 新規機能ごとに、次を `team_status` に記録:
  - 何を Chrono から真似たか（概念名）
  - 何を簡略化したか（2D/C 制約）
  - 受入コマンドと pass/fail

これにより、将来 Chrono本体へ移る際の差分追跡を容易にする。
