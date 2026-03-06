# FEM4C Codex Team Prompt Pack


最終更新: 2026-03-06

この文書は、`fem4c_2link_flexible_detailed_todo.md` を **そのまま Codex に投げやすい形**へ変換したプロンプト集である。  
対象は **2D / 2-link / 両リンク flexible / full mesh 再アセンブル / MBD explicit + Newmark-beta + HHT-alpha** に固定する。

## 0. 使い方

1. まず対象チームの **起動プロンプト** を 1 回送る。  
2. 次に、そのチームへ **タスク実行プロンプト** を 1 件ずつ送る。  
3. 1 セッションで扱うのは **1 タスクだけ** とし、範囲外の先取り実装は禁止する。

## 1. 全チーム共通ベースプロンプト

以下を、新しい Codex セッションの最初に送ること。

```text
あなたは FEM4C リポジトリの実装担当です。今回の対象は 2D 技術検証ソルバーで、ゴールは次に固定します。

- 対象モデル: 2-link planar mechanism
- 柔軟体: 両リンク flexible
- FEM: 線形静解析 snapshot を毎 step / 毎 coupling iteration で解く
- full mesh 再アセンブル: 必須
- MBD: explicit と implicit の両方
- implicit: Newmark-beta と HHT-alpha の両方
- 言語: C
- 参照思想: Project Chrono の System / Body / Constraint / Timestepper の責務分割
- 比較対象:
  - rigid 2-link は解析解比較
  - flexible 2-link は RecurDyn / AdamsFlex 比較

厳守ルール:
1. C 言語のみを使う。C++ 化しない。
2. Project Chrono は責務分割の参考にとどめ、コードの転載や依存追加はしない。
3. 指定されたタスク以外の設計変更をしない。
4. 指定されたファイル以外は、ビルド修正や include 整理に必須な最小限だけ触る。
5. public API を壊さない。名前変更はタスクで明示された場合だけ行う。
6. 未来のタスクを先回りして実装しない。
7. 実装後は可能な範囲で build / 実行確認を行う。docs only タスクなら不要。
8. 報告は簡潔に行う:
   - touched files
   - 実装した関数 / 構造体
   - build / run 結果
   - 未解決事項
9. 迷ったら「今回の 2D PJ のコア文」を優先する:
   両リンク flexible、MBD は explicit / Newmark-beta / HHT-alpha を持ち、各 coupling iteration で各 flexible link の full FE mesh を再アセンブルして static snapshot を解き、その reaction を MBD へ返す。
10. 範囲外の TODO を勝手に解消しない。必要ならコメントで残すだけにする。
```

## 2. チーム起動プロンプト

### PM Team 起動プロンプト

```text
あなたは PM チームです。担当はスコープ定義、受入条件、入力仕様、比較 schema、マージ順の固定です。
今回のセッションでは、コード実装ではなく docs / schema / acceptance の固定を行います。
担当外:
- C コード本体のロジック実装
- 数値積分器の変更
- FEM kernel の改造
docs は Codex が迷わない粒度で具体化してください。曖昧な言葉を避け、directive 名、列名、許容誤差、対象ファイル名を明記してください。
```

### Team A (Body / Forces / Explicit / Kinematics) 起動プロンプト

```text
あなたは Team A です。担当は Body / Forces / Explicit / Kinematics です。
今回のセッションでは A 系 task だけを実装してください。
主担当:
- body2d
- forces2d
- kinematics2d
- integrator_explicit2d
- output2d
- system2d への explicit 側接続
担当外:
- constraint Jacobian の本実装
- Newmark / HHT の本実装
- FEM full reassembly
- coupled orchestration 全体
```

### Team B (Constraint / KKT / Newmark / HHT) 起動プロンプト

```text
あなたは Team B です。担当は Constraint / KKT / Newmark / HHT です。
今回のセッションでは B 系 task だけを実装してください。
主担当:
- system2d
- assembler2d
- linear_solver_dense
- constraint2d の acceleration-level RHS
- integrator_newmark2d
- integrator_hht2d
- projection2d
担当外:
- body2d の構造体設計の大変更
- FEM model 管理
- coupled case の top-level orchestration
```

### Team C (FEM API / full reassembly / nodeset / output) 起動プロンプト

```text
あなたは Team C です。担当は FEM API / full reassembly / nodeset / output です。
今回のセッションでは C 系 task だけを実装してください。
主担当:
- build recovery
- fem_model2d deep copy
- flex_solver2d
- flex_bc2d
- flex_nodeset
- full mesh 再アセンブル
- snapshot 出力
担当外:
- MBD integrator
- constraint formulation
- coupled iteration 制御
```

### Team D (Flexible body wrapper / reaction / 2-link flex) 起動プロンプト

```text
あなたは Team D です。担当は Flexible body wrapper / reaction / 2-link flexible 化です。
今回のセッションでは D 系 task だけを実装してください。
主担当:
- flex_body2d
- rigid interpolation の BC 展開
- snapshot solve ラッパ
- FE reaction の generalized force 化
- 1-link flexible から 2-link flexible への拡張
- coupling residual と snapshot 接続
担当外:
- MBD の数値積分器そのもの
- parser の詳細実装
- 比較スクリプト
```

### Team E (System orchestration / parser / regression / compare) 起動プロンプト

```text
あなたは Team E です。担当は System orchestration / parser / regression / compare です。
今回のセッションでは E 系 task だけを実装してください。
主担当:
- runner.c の縮退
- coupled input directive
- benchmark input
- coupled_step_explicit2d
- coupled_step_implicit2d
- compare scripts
- acceptance script
担当外:
- FEM kernel の改造
- body2d / constraint2d の基礎データ構造の再設計
```


## 3. タスク実行プロンプト


### PM Team


#### PM-01 実行プロンプト

```text
このセッションでは **PM-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-01
想定作業時間: 60-75 min
目的: 2D PJ の必須要件を凍結する

対象ファイル:
- `docs/04_2d_coupled_scope.md`

実装要件:
1. 文書タイトルを `2D coupled scope for 2-link flexible validation` にする
2. 以下を明記する
   - 両リンク flexible
   - full mesh 再アセンブル必須
   - MBD explicit / Newmark / HHT 必須
   - rigid case は解析解比較
   - flexible case は RecurDyn / AdamsFlex 比較
3. 今回やらないものを明記する
   - 接触
   - 摩擦
   - 非線形材料
   - 3D

完了条件:
- 上記 5 項目が 1 ページで読める
- `README.md` からリンクが貼られている

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### PM-02 実行プロンプト

```text
このセッションでは **PM-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-02
想定作業時間: 60-75 min
目的: モジュール責務を固定する

対象ファイル:
- `docs/05_module_ownership_2d.md`

実装要件:
1. `mbd/`, `coupled/`, `analysis/`, `solver/` の責務を表で書く
2. `runner.c` から追い出す責務を列挙する
3. `runner.c` に残す責務を列挙する
4. 各チームの担当ファイル一覧を末尾に書く

完了条件:
- `runner.c` の役割が「入口と分岐」へ縮退する方針になっている

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### PM-03 実行プロンプト

```text
このセッションでは **PM-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-03
想定作業時間: 60-90 min
目的: 受入条件の数値指標を固定する

対象ファイル:
- `docs/06_acceptance_matrix_2d.md`

実装要件:
1. rigid explicit / Newmark / HHT の 3 行を作る
2. flexible 1-link / flexible 2-link の 2 行を追加する
3. 各行に以下を列挙する
   - 入力ファイル
   - 比較対象
   - 許容誤差
   - 出力 CSV 名
4. `constraint residual`, `joint angle`, `tip displacement` を必須列にする

完了条件:
- 5 ケース以上の受入表が完成している

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### PM-04 実行プロンプト

```text
このセッションでは **PM-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-04
想定作業時間: 60-75 min
目的: 入力仕様を固定する

対象ファイル:
- `docs/07_input_spec_coupled_2d.md`

実装要件:
1. 以下の新規 directive を定義する
   - `MBD_BODY_DYN`
   - `MBD_GRAVITY`
   - `MBD_FORCE`
   - `COUPLED_FLEX_BODY`
   - `COUPLED_FLEX_ROOT_SET`
   - `COUPLED_FLEX_TIP_SET`
2. 各 directive の引数順を 1 行例付きで書く
3. body id と fem file path の対応を明記する

完了条件:
- Codex が parser 実装時に迷わない粒度になっている

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### PM-05 実行プロンプト

```text
このセッションでは **PM-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-05
想定作業時間: 60-75 min
目的: マージ順を固定する

対象ファイル:
- `docs/08_merge_order_2d.md`

実装要件:
1. `phase-1 build` から `phase-6 validation` まで章立てする
2. 各 phase に task ID を並べる
3. 同時マージしてよい task と、依存で待つ task を分ける

完了条件:
- 依存順が一本道で見える

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### PM-06 実行プロンプト

```text
このセッションでは **PM-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: PM Team
タスクID: PM-06
想定作業時間: 60-90 min
目的: 比較データの持ち方を固定する

対象ファイル:
- `docs/09_compare_schema_2d.md`

実装要件:
1. RecurDyn / AdamsFlex / FEM4C の比較 CSV schema を定義する
2. 列名を固定する
   - `time`
   - `theta1`
   - `theta2`
   - `omega1`
   - `omega2`
   - `tip1_x`
   - `tip1_y`
   - `tip2_x`
   - `tip2_y`
   - `root_reaction_x`
   - `root_reaction_y`
3. 単位も明記する

完了条件:
- 比較スクリプトがこの schema だけ読めばよい形になっている

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


### Team A (Body / Forces / Explicit / Kinematics)


#### A-01 実行プロンプト

```text
このセッションでは **A-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-01
想定作業時間: 60-75 min
目的: 剛体 body の実体を作る

対象ファイル:
- `src/mbd/body2d.h`
- `src/mbd/body2d.c`

実装要件:
1. `mbd_body2d_t` を新設する
2. 保持項目:
   - `id`
   - `mass`
   - `inertia`
   - `q[3]`
   - `v[3]`
   - `a[3]`
   - `force[3]`
   - `is_ground`
3. `mbd_body2d_zero()`
4. `mbd_body2d_init_dyn()`
5. `mbd_body2d_clear_force()`

完了条件:
- 単体で body 初期化できる
- `runner.c` から切り出して使える

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-02 実行プロンプト

```text
このセッションでは **A-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-02
想定作業時間: 60-90 min
目的: 動力学 input を parse できるようにする

対象ファイル:
- `src/io/input.c`
- `src/mbd/body2d.h`

実装要件:
1. `MBD_BODY_DYN id mass inertia x y theta vx vy omega` を読み込む
2. `MBD_GRAVITY gx gy` を読み込む
3. `MBD_FORCE body_id fx fy mz` を読み込む
4. 未指定時の既定値を入れる
5. 既存 `MBD_BODY` との後方互換を保つ

完了条件:
- 新旧両 directive で parse が通る
- 異常入力時に明確なエラーメッセージが出る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-03 実行プロンプト

```text
このセッションでは **A-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-03
想定作業時間: 60-75 min
目的: 外力の assemble API を作る

対象ファイル:
- `src/mbd/forces2d.h`
- `src/mbd/forces2d.c`

実装要件:
1. `mbd_forces2d_apply_gravity()`
2. `mbd_forces2d_apply_user_loads()`
3. `mbd_forces2d_add_generalized_force()`
4. `mbd_forces2d_build_rhs_vector()`
5. `force[3]` を body 配列から RHS へ詰める

完了条件:
- gravity と body force を同じ経路で足し込める

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-04 実行プロンプト

```text
このセッションでは **A-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-04
想定作業時間: 60-75 min
目的: marker / interface の幾何変換を作る

対象ファイル:
- `src/mbd/kinematics2d.h`
- `src/mbd/kinematics2d.c`

実装要件:
1. local point → world point 変換
2. world point の `∂x/∂q` Jacobian
3. local vector の回転変換
4. `theta` に対する微分
5. unit test 相当の簡単な self-check 関数

完了条件:
- revolute joint の anchor 計算と flex interface 計算で再利用できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-05 実行プロンプト

```text
このセッションでは **A-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-05
想定作業時間: 60-90 min
目的: explicit integrator の器を作る

対象ファイル:
- `src/mbd/integrator_explicit2d.h`
- `src/mbd/integrator_explicit2d.c`

実装要件:
1. 初版 explicit を `semi_implicit_euler` で実装する
2. 関数:
   - `mbd_explicit2d_predict()`
   - `mbd_explicit2d_update_velocity()`
   - `mbd_explicit2d_update_position()`
3. 位置更新は `q_{n+1}=q_n + dt*v_{n+1}` を採用
4. 回転 DOF も同様に更新する

完了条件:
- unconstrained single body の重力落下が動く

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-06 実行プロンプト

```text
このセッションでは **A-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-06
想定作業時間: 60-90 min
目的: explicit path に body state 更新を接続する

対象ファイル:
- `src/mbd/system2d.c`
- `src/mbd/integrator_explicit2d.c`

実装要件:
1. `mbd_system2d_do_explicit_step()` を作る
2. 流れ:
   - clear force
   - gravity / user load
   - acceleration solve 呼び出し
   - velocity update
   - position update
3. 現時点では flexible reaction はフックだけ置く

完了条件:
- rigid 2-link explicit の step 関数が呼べる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-07 実行プロンプト

```text
このセッションでは **A-07 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-07
想定作業時間: 60-75 min
目的: 出力を整える

対象ファイル:
- `src/mbd/output2d.h`
- `src/mbd/output2d.c`

実装要件:
1. 時系列 CSV writer を作る
2. 1 行に以下を書く
   - step
   - time
   - body_id
   - x
   - y
   - theta
   - vx
   - vy
   - omega
   - ax
   - ay
   - alpha
3. header を固定する

完了条件:
- rigid case の履歴が CSV に出る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-08 実行プロンプト

```text
このセッションでは **A-08 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-08
想定作業時間: 60-90 min
目的: flexible reaction を MBD に戻す口を作る

対象ファイル:
- `src/mbd/forces2d.c`
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`

実装要件:
1. `mbd_system2d_add_flexible_generalized_force(body_id, qflex[3])`
2. 1 step の RHS 組立時に user force に加算する
3. step 開始時に flexible force を clear する

完了条件:
- coupling 側から 1 body あたり 3DOF の generalized force を戻せる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### A-09 実行プロンプト

```text
このセッションでは **A-09 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team A (Body / Forces / Explicit / Kinematics)
タスクID: A-09
想定作業時間: 60-75 min
目的: link-local reference frame を保持する

対象ファイル:
- `src/mbd/body2d.h`
- `src/mbd/body2d.c`

実装要件:
1. 各 body に local frame 原点と初期姿勢を保持する項目を追加
2. `mbd_body2d_set_reference_frame()` を作る
3. flexible body 側が link local frame を参照できる accessor を作る

完了条件:
- FE 側が current rigid pose を body から取得できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


### Team B (Constraint / KKT / Newmark / HHT)


#### B-01 実行プロンプト

```text
このセッションでは **B-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-01
想定作業時間: 60-75 min
目的: constraint data を system 側へ寄せる

対象ファイル:
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`
- `src/mbd/constraint2d.h`

実装要件:
1. `mbd_system2d_t` を新設する
2. 保持項目:
   - body array
   - constraint array
   - gravity
   - time control
3. `runner.c` が body/constraint を system へ渡す経路を作る

完了条件:
- runtime state が `runner.c` ローカル配列に残らない

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-02 実行プロンプト

```text
このセッションでは **B-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-02
想定作業時間: 60-90 min
目的: KKT assembler を実装する

対象ファイル:
- `src/mbd/assembler2d.h`
- `src/mbd/assembler2d.c`

実装要件:
1. `[M G^T; G 0]` の dense KKT を組む
2. body ごとの mass / inertia を対角に詰める
3. constraint Jacobian を body index ごとに詰める
4. RHS ベクトルも同じ順序で作る

完了条件:
- rigid 2-link の KKT 行列と RHS が数値で出せる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-03 実行プロンプト

```text
このセッションでは **B-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-03
想定作業時間: 60-75 min
目的: 小規模 dense solver を用意する

対象ファイル:
- `src/mbd/linear_solver_dense.h`
- `src/mbd/linear_solver_dense.c`

実装要件:
1. 部分 pivot 付き Gaussian elimination を実装
2. 入力:
   - dense matrix
   - rhs
3. 出力:
   - solution
4. singular 近傍ならエラーを返す

完了条件:
- KKT の小規模系を単体で解ける

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-04 実行プロンプト

```text
このセッションでは **B-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-04
想定作業時間: 60-90 min
目的: acceleration-level constraint RHS を作る

対象ファイル:
- `src/mbd/constraint2d.c`
- `src/mbd/assembler2d.c`

実装要件:
1. `Phi(q)` residual を計算
2. `G(q)` を計算
3. 初版は Baumgarte stabilization を入れる
4. `gamma_c = -2*alpha*Phi_dot - beta^2*Phi` 型の RHS を作る
5. パラメータは固定値でよい

完了条件:
- explicit / implicit 共通で constraint RHS を使える

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-05 実行プロンプト

```text
このセッションでは **B-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-05
想定作業時間: 60-90 min
目的: Newmark-beta の器を作る

対象ファイル:
- `src/mbd/integrator_newmark2d.h`
- `src/mbd/integrator_newmark2d.c`

実装要件:
1. predictor 関数を作る
2. effective acceleration/velocity 更新式を実装する
3. `beta`, `gamma` は引数または system から読む
4. state update 関数を 1 つにまとめる

完了条件:
- unconstrained single body で Newmark 更新が動く

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-06 実行プロンプト

```text
このセッションでは **B-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-06
想定作業時間: 60-90 min
目的: Newmark-beta implicit step を完成させる

対象ファイル:
- `src/mbd/system2d.c`
- `src/mbd/integrator_newmark2d.c`

実装要件:
1. `mbd_system2d_do_newmark_step()` を作る
2. predictor
3. residual assemble
4. KKT solve
5. corrector
6. residual check
7. 最大反復回数超過時のエラー

完了条件:
- rigid 2-link の Newmark 計算が 1 run 完了する

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-07 実行プロンプト

```text
このセッションでは **B-07 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-07
想定作業時間: 60-90 min
目的: HHT-alpha の係数計算を実装する

対象ファイル:
- `src/mbd/integrator_hht2d.h`
- `src/mbd/integrator_hht2d.c`

実装要件:
1. `alpha` を入力で受ける
2. HHT の effective residual 側に `alpha` を反映する
3. `alpha ∈ [-1/3,0]` をチェックする
4. modified Newton 用の関数分離をする

完了条件:
- HHT 単独モジュールが compile する

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-08 実行プロンプト

```text
このセッションでは **B-08 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-08
想定作業時間: 60-90 min
目的: HHT-alpha step を完成させる

対象ファイル:
- `src/mbd/system2d.c`
- `src/mbd/integrator_hht2d.c`

実装要件:
1. `mbd_system2d_do_hht_step()` を作る
2. predictor
3. HHT residual assemble
4. modified Newton loop
5. convergence 判定
6. state update

完了条件:
- rigid 2-link の HHT 計算が 1 run 完了する

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### B-09 実行プロンプト

```text
このセッションでは **B-09 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team B (Constraint / KKT / Newmark / HHT)
タスクID: B-09
想定作業時間: 60-75 min
目的: constraint projection を入れる

対象ファイル:
- `src/mbd/projection2d.h`
- `src/mbd/projection2d.c`
- `src/mbd/system2d.c`

実装要件:
1. step 後に位置レベル residual を計算
2. 小さい correction KKT を解いて `q` を補正
3. explicit / implicit 共通で呼べるようにする

完了条件:
- 長時間積分で constraint drift が減る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


### Team C (FEM API / full reassembly / nodeset / output)


#### C-01 実行プロンプト

```text
このセッションでは **C-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-01
想定作業時間: 60-75 min
目的: build を復旧する

対象ファイル:
- `src/elements/t6/t6_element.c`
- 必要なら `src/elements/element_base.h`

実装要件:
1. `t6_register()` の `stiffness` 関数ポインタ型不一致を解消する
2. `make -j` が最後まで通るようにする
3. warning は増やさない

完了条件:
- `make -j2` 成功

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-02 実行プロンプト

```text
このセッションでは **C-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-02
想定作業時間: 60-90 min
目的: globals ベースの FE model を深いコピー可能にする

対象ファイル:
- `src/coupled/fem_model_copy.h`
- `src/coupled/fem_model_copy.c`

実装要件:
1. `fem_model2d_t` を新設する
2. 保持項目:
   - nodes
   - elements
   - materials
   - boundary conditions
   - loads
   - analysis metadata
3. `fem_model2d_from_globals()`
4. `fem_model2d_free()`

完了条件:
- 1 つの FE input を globals から独立保持できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-03 実行プロンプト

```text
このセッションでは **C-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-03
想定作業時間: 60-90 min
目的: model-centric assembly API を作る

対象ファイル:
- `src/coupled/fem_model_copy.h`
- `src/coupled/flex_solver2d.h`
- `src/coupled/flex_solver2d.c`

実装要件:
1. 既存 `assembly_*` を直接書き換えず、ラッパを作る
2. `flex_solver2d_prepare_model(fem_model2d_t*)`
3. `flex_solver2d_assemble_full_mesh(fem_model2d_t*)`
4. globals に model を一時注入して existing FEM kernel を呼ぶ最小経路でもよい

完了条件:
- 1 model 単体で full assembly が再利用できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-04 実行プロンプト

```text
このセッションでは **C-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-04
想定作業時間: 60-90 min
目的: Dirichlet BC を runtime で差し替えられるようにする

対象ファイル:
- `src/coupled/flex_bc2d.h`
- `src/coupled/flex_bc2d.c`

実装要件:
1. node id / dof / value を保持する runtime BC 配列を作る
2. 既存境界条件に追加上書きできるようにする
3. root / tip nodeset に与える BC をここで表現する

完了条件:
- step ごとに BC を差し替えて FE solve できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-05 実行プロンプト

```text
このセッションでは **C-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-05
想定作業時間: 60-90 min
目的: full mesh 再アセンブルを明示化する

対象ファイル:
- `src/coupled/flex_solver2d.c`

実装要件:
1. `flex_solver2d_reassemble_and_solve()` を作る
2. 処理順:
   - clear previous system arrays
   - assemble full stiffness
   - assemble full force
   - apply runtime BC
   - solve
3. `full_reassembly_count++` を入れる

完了条件:
- 1 呼び出しで必ず full reassembly が走る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-06 実行プロンプト

```text
このセッションでは **C-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-06
想定作業時間: 60-75 min
目的: full reassembly のログを出す

対象ファイル:
- `src/coupled/flex_solver2d.c`
- `src/io/output.c` または新規 `src/coupled/flex_log2d.c`

実装要件:
1. link ごとの reassembly count
2. step ごとの solve count
3. coupling iteration index
4. 結果を CSV または log に書く

完了条件:
- 実行後に「各 link が何回再アセンブルされたか」が見える

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-07 実行プロンプト

```text
このセッションでは **C-07 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-07
想定作業時間: 60-90 min
目的: nodeset データを扱えるようにする

対象ファイル:
- `src/coupled/flex_nodeset.h`
- `src/coupled/flex_nodeset.c`

実装要件:
1. `node_set_t` を作る
2. root / tip node id 配列を保持する
3. `node_set_contains()`
4. `node_set_center()`
5. `node_set_local_coordinates()` を作る

完了条件:
- root/tip interface を node set で管理できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-08 実行プロンプト

```text
このセッションでは **C-08 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-08
想定作業時間: 60-90 min
目的: inertial equivalent load の受け口を作る

対象ファイル:
- `src/coupled/flex_solver2d.c`
- `src/solver/assembly.c` も必要なら更新

実装要件:
1. translational acceleration による body force の入力口を作る
2. 角加速度・遠心項は初版では TODO でもよいが、関数シグネチャは切っておく
3. `flex_solver2d_set_inertial_loads()` を作る

完了条件:
- coupled 側から link ごとの擬似静的 body force を渡せる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### C-09 実行プロンプト

```text
このセッションでは **C-09 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team C (FEM API / full reassembly / nodeset / output)
タスクID: C-09
想定作業時間: 60-90 min
目的: 変形形状の snapshot 出力を作る

対象ファイル:
- `src/coupled/flex_snapshot2d.h`
- `src/coupled/flex_snapshot2d.c`

実装要件:
1. local FE displacement を world 座標へ写す
2. deformed node 座標を CSV または VTK へ出す
3. body_id, step, iteration をファイル名に入れる

完了条件:
- 各 link の変形形状を時系列で見られる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


### Team D (Flexible body wrapper / reaction / 2-link flex)


#### D-01 実行プロンプト

```text
このセッションでは **D-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-01
想定作業時間: 60-75 min
目的: flexible body wrapper を作る

対象ファイル:
- `src/coupled/flex_body2d.h`
- `src/coupled/flex_body2d.c`

実装要件:
1. `flex_body2d_t` を定義する
2. 保持項目:
   - `body_id`
   - `fem_model2d_t model`
   - `node_set_t root_set`
   - `node_set_t tip_set`
   - `u_local`
   - `reaction_root[3]`
   - `reaction_tip[3]`
3. init / free を作る

完了条件:
- 1 flexible link を 1 struct で保持できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-02 実行プロンプト

```text
このセッションでは **D-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-02
想定作業時間: 60-90 min
目的: interface の rigid interpolation を実装する

対象ファイル:
- `src/coupled/flex_bc2d.c`
- `src/coupled/flex_body2d.c`

実装要件:
1. node set 内の各 node について local 座標 `(xk, yk)` を持つ
2. `[ux, uy, theta]` から各 node の `ux_k, uy_k` を計算する
3. root / tip それぞれに適用する関数を作る

完了条件:
- interface marker 変位を node BC に展開できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-03 実行プロンプト

```text
このセッションでは **D-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-03
想定作業時間: 60-90 min
目的: 1 flexible link の static solve ラッパを作る

対象ファイル:
- `src/coupled/flex_body2d.c`
- `src/coupled/flex_solver2d.c`

実装要件:
1. `flex_body2d_solve_snapshot()` を作る
2. 入力:
   - root marker displacement
   - tip marker displacement
   - inertial load
3. 処理:
   - runtime BC 作成
   - full mesh 再アセンブル
   - solve
4. 出力:
   - nodal displacement
   - root/tip reaction

完了条件:
- 1 link 単体で snapshot solve できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-04 実行プロンプト

```text
このセッションでは **D-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-04
想定作業時間: 60-90 min
目的: FE reaction を generalized force に変換する

対象ファイル:
- `src/coupled/flex_reaction2d.h`
- `src/coupled/flex_reaction2d.c`

実装要件:
1. root/tip node reaction から合力・合モーメントを計算する
2. body generalized force `[Fx, Fy, Mz]` に変換する
3. virtual work 一致で符号を整理する
4. root body / tip body のどちらへ返すかを明確化する

完了条件:
- MBD 側へ body generalized force を返せる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-05 実行プロンプト

```text
このセッションでは **D-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-05
想定作業時間: 60-90 min
目的: 1-link flexible coupling を成立させる

対象ファイル:
- `src/coupled/coupled_step_explicit2d.c`
- `src/coupled/coupled_step_implicit2d.c`

実装要件:
1. まず link1 だけ flexible に固定
2. MBD body pose → root/tip marker displacement を作る
3. FE snapshot solve
4. generalized reaction を MBD に返す
5. 1 step だけ動かす

完了条件:
- 1-link flexible の step trace が出る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-06 実行プロンプト

```text
このセッションでは **D-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-06
想定作業時間: 60-90 min
目的: 2-link flexible に拡張する

対象ファイル:
- `src/coupled/case2d.h`
- `src/coupled/case2d.c`
- `src/coupled/coupled_run2d.c`

実装要件:
1. `flex_body[2]` を保持できる coupled case を作る
2. link1 / link2 を別 mesh で保持できるようにする
3. 両方を solve し、reaction を合算する

完了条件:
- 両リンク flexible の coupled case が初期化できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-07 実行プロンプト

```text
このセッションでは **D-07 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-07
想定作業時間: 60-90 min
目的: coupling residual と iteration 管理を入れる

対象ファイル:
- `src/coupled/coupled_run2d.c`

実装要件:
1. residual を
   - `||Qflex_new - Qflex_old||`
   - `||u_tip_new - u_tip_old||`
   のいずれかで定義する
2. step 内反復回数上限を設ける
3. 収束 / 非収束ログを出す

完了条件:
- same-step iteration が明示的に回る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-08 実行プロンプト

```text
このセッションでは **D-08 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-08
想定作業時間: 60-75 min
目的: snapshot 出力を coupled に接続する

対象ファイル:
- `src/coupled/coupled_run2d.c`
- `src/coupled/flex_snapshot2d.c`

実装要件:
1. step accept 後だけ snapshot を出す
2. link1 / link2 で別ファイルにする
3. step と time をファイル名へ入れる

完了条件:
- 両リンクの変形形状が保存される

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### D-09 実行プロンプト

```text
このセッションでは **D-09 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team D (Flexible body wrapper / reaction / 2-link flex)
タスクID: D-09
想定作業時間: 60-75 min
目的: 高剛性 limit test を作る

対象ファイル:
- `examples/coupled_2link_flex_rigid_limit_link1.dat`
- `examples/coupled_2link_flex_rigid_limit_link2.dat`
- 必要なら script

実装要件:
1. `E` を十分大きくした flexible case を作る
2. rigid 2-link と比較する差分 CSV を出す
3. `theta1, theta2, tip2_x, tip2_y` を比較する

完了条件:
- flexible → rigid limit が確認できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


### Team E (System orchestration / parser / regression / compare)


#### E-01 実行プロンプト

```text
このセッションでは **E-01 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-01
想定作業時間: 60-90 min
目的: `runner.c` から system 実行を切り出す

対象ファイル:
- `src/mbd/system2d.h`
- `src/mbd/system2d.c`
- `src/analysis/runner.c`

実装要件:
1. `mbd_analysis_minimal()` の中身を `mbd_system2d_run()` へ寄せる
2. `runner.c` は
   - parse
   - mode 分岐
   - run 呼び出し
   のみ残す
3. coupled も同様に `coupled_run2d()` へ寄せる準備をする

完了条件:
- `runner.c` が入口に近い役割だけになる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-02 実行プロンプト

```text
このセッションでは **E-02 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-02
想定作業時間: 60-90 min
目的: coupled input directive を実装する

対象ファイル:
- `src/io/input.c`
- `src/coupled/case2d.h`
- `src/coupled/case2d.c`

実装要件:
1. 以下を parse する
   - `COUPLED_FLEX_BODY body_id fem_input_path`
   - `COUPLED_FLEX_ROOT_SET body_id n id1 id2 ...`
   - `COUPLED_FLEX_TIP_SET body_id n id1 id2 ...`
2. 2つの flexible body を保持できる
3. path 文字列の保持と検証を行う

完了条件:
- coupled case の input が読める

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-03 実行プロンプト

```text
このセッションでは **E-03 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-03
想定作業時間: 60-90 min
目的: rigid 2-link benchmark input を作る

対象ファイル:
- `examples/mbd_2link_rigid_dyn.dat`

実装要件:
1. `MBD_BODY_DYN`
2. `MBD_GRAVITY`
3. revolute constraints
4. 初期姿勢 / 初期速度
を含む rigid 2-link 入力を作る

完了条件:
- explicit / Newmark / HHT の3系統で同じ input を回せる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-04 実行プロンプト

```text
このセッションでは **E-04 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-04
想定作業時間: 60-90 min
目的: explicit coupled run を作る

対象ファイル:
- `src/coupled/coupled_step_explicit2d.h`
- `src/coupled/coupled_step_explicit2d.c`
- `src/coupled/coupled_run2d.c`

実装要件:
1. explicit step の orchestration を実装する
2. 2-link flexible の順序は
   - explicit MBD step
   - flex1 solve
   - flex2 solve
   - reaction map
   - optional fixed-point correction
3. step history を保存する

完了条件:
- explicit coupled run が最後まで走る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-05 実行プロンプト

```text
このセッションでは **E-05 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-05
想定作業時間: 60-90 min
目的: implicit coupled run (Newmark) を作る

対象ファイル:
- `src/coupled/coupled_step_implicit2d.h`
- `src/coupled/coupled_step_implicit2d.c`
- `src/coupled/coupled_run2d.c`

実装要件:
1. Newmark path の same-step iteration を実装する
2. 反復ごとに flex1 / flex2 を再 solve する
3. residual が収束したら accept する

完了条件:
- Newmark coupled run が最後まで走る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-06 実行プロンプト

```text
このセッションでは **E-06 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-06
想定作業時間: 60-90 min
目的: implicit coupled run (HHT) を作る

対象ファイル:
- `src/coupled/coupled_step_implicit2d.c`
- `src/coupled/coupled_run2d.c`

実装要件:
1. HHT path の same-step iteration を実装する
2. HHT residual と flex reaction を同じ loop に入れる
3. `alpha` をログ出力する

完了条件:
- HHT coupled run が最後まで走る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-07 実行プロンプト

```text
このセッションでは **E-07 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-07
想定作業時間: 60-90 min
目的: 2-link flexible input を作る

対象ファイル:
- `examples/coupled_2link_flex_master.dat`
- `examples/flex_link1_q4.dat`
- `examples/flex_link2_q4.dat`

実装要件:
1. master input に MBD と coupled directive を書く
2. link1 FE input を作る
3. link2 FE input を作る
4. root / tip nodeset の例を書く

完了条件:
- 両リンク flexible の最小入力一式がそろう

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-08 実行プロンプト

```text
このセッションでは **E-08 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-08
想定作業時間: 60-90 min
目的: 比較スクリプトを作る

対象ファイル:
- `scripts/compare_2link_rigid_analytic.py`
- `scripts/compare_2link_flex_reference.py`

実装要件:
1. rigid case:
   - FEM4C CSV と解析参照を重ねる
2. flexible case:
   - FEM4C CSV と RecurDyn/AdamsFlex CSV を重ねる
3. RMS 誤差と最大誤差を出す
4. PNG を保存する

完了条件:
- 比較結果が数値と図で出る

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```


#### E-09 実行プロンプト

```text
このセッションでは **E-09 だけ** を実装してください。範囲外の変更は禁止です。

担当チーム: Team E (System orchestration / parser / regression / compare)
タスクID: E-09
想定作業時間: 60-90 min
目的: end-to-end acceptance script を作る

対象ファイル:
- `scripts/run_2d_coupled_acceptance.sh`

実装要件:
1. build
2. rigid explicit / Newmark / HHT
3. flexible explicit / Newmark / HHT
4. compare scripts
5. pass/fail summary
を 1 本で回す

完了条件:
- 1 コマンドで 2D acceptance が実行できる

作業ルール:
- 既存コードのスタイルと命名をできるだけ維持すること。
- 指定ファイル以外を触る場合は、compile / include 修正に必須な最小限にとどめること。
- この task に必要ない将来機能は入れないこと。
- docs task でない限り、最後に可能な範囲で build または最小実行確認を行うこと。
- 失敗したら黙って設計変更せず、未解決事項として報告すること。

最後の報告フォーマット:
1. touched files
2. 追加 / 変更した関数・構造体
3. build / run 結果
4. 未解決事項
```
