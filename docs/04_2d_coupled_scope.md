# 2D coupled scope for 2-link flexible validation

最終更新: 2026-03-06

## 1. 目的
- FEM4C を 2D 技術検証ソルバーとして前進させる。
- 対象は 2-link planar mechanism に固定する。
- 今回は「両リンク flexible の 2D validation solver」を完成対象とする。

## 2. 固定要件
- 両リンク flexible。
- FEM は線形静解析 snapshot を毎 step / 毎 coupling iteration で解く。
- full mesh 再アセンブルは必須。
- MBD は explicit と implicit の両方を持つ。
- implicit は Newmark-beta と HHT-alpha の両方を持つ。
- rigid 2-link は解析解または高精度 ODE 参照解と比較する。
- flexible 2-link は RecurDyn / AdamsFlex と比較する。
- 言語は C に固定する。
- 責務分割は Project Chrono の System / Body / Constraint / Timestepper を参考にする。

## 3. 今回やらないもの
- 接触
- 摩擦
- 非線形材料
- 3D
- FEM-MBD の一般化連成機能
- 制御連成

## 4. 2D の前提
- FEM は小ひずみ線形弾性とする。
- 大きい剛体回転は MBD 側が保持する。
- FE はリンク局所座標系で解く。
- joint 周辺 node set の変位は `[ux, uy, theta]` の rigid interpolation で与える。
- 各 flexible link は毎回 full mesh を再アセンブルして static snapshot を解く。

## 5. 比較データの扱い
- rigid 比較は内部で完結するため、M1 から受入対象に入れる。
- flexible 比較は外部参照値が必要だが、2026-03-06 時点では RecurDyn / AdamsFlex の実データは未投入。
- したがって、M0-M3 では「比較 CSV schema の固定」までを必須とし、実データ比較は M4 の受入で必須化する。
- 実データ未取得は、M0-M3 の blocker にはしない。

## 6. 参照ルール
- Project Chrono の参照元は `third_party/chrono/chrono-main` のみとする。
- `third_party/chrono/chrono-C-all` は参照禁止とする。
- コード転載や依存追加は行わず、責務分割と解法設計の参考に限定する。
