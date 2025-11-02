# chrono-C 2D 機能拡張ロードマップ

本ドキュメントは `chrono-C-all` における 2D 機能を Chrono C++ 本家のカバレッジへ近づけるための設計指針と進捗管理をまとめたものです。以降、作業が進むたびにこのファイルを更新します。

## 1. ゴール定義
- 2D 剛体・拘束・接触機能を Chrono-main と同等レベルまで移植し、最終的に 2D シミュレーションを `chrono-C-all` 単体で完結できるようにする。
- テスト（単体・回帰）を充実させ、将来の改修によるリグレッションを防止する。
- 最小限のインフラ（Makefile、CI スクリプト）で自動テストを回せる体制を整え、開発効率を確保する。

## 2. 現状サマリ（2025-10-19 時点）
| 領域 | ステータス | 備考 |
|------|------------|------|
| 剛体基盤 (`chrono_body2d`) | ✅ 初期機能あり | 明示的オイラー積分、ワールド/ローカル変換、円形形状、材料パラメータ（反発・摩擦）に加え、凸ポリゴン形状＋質量特性自動計算 (`chrono_body2d_set_polygon_shape_with_density`) に対応。スリープや多形状未対応。 |
| 距離制約 (`chrono_constraint2d`) | ✅ 安定化 solver あり | Baumgarte + Warm-start に加えて、線形／角度ソフトネスを個別設定できる API を導入し、`test_distance_constraint_multi` で複数拘束同時解決と角速度連成を回帰。`chrono_distance_constraint2d_set_spring` でバネ・ダンパも設定可能（`last_spring_force` をログ化）。 |
| 接触 (`chrono_collision2d`) | ✅ 円/凸ポリゴン/カプセル/エッジ対応（反発＋静/動摩擦＋2点マニフォールド） | GJK(EPA) ベースの汎用検出ルートを追加し、新プリミティブ（カプセル・エッジ）にも対応。`test_contact_manager_longrun`、`test_polygon_collision` 等で回帰。 |
| アイランド統合 (`chrono_island2d`) | ✅ 初期版 | 拘束・接触をまとめる `ChronoIsland2D_C` とワークスペース整備、OpenMP 対応ラッパソルバを実装。 |
| テスト | ✅ 主要カバレッジ | 距離拘束・円衝突各種・連続接触に加え、アイランド並列テスト、マニフォールド長期回帰、島ビルダ単体テストを追加済み。 |
| ドキュメント | ⚠️ 一部整備 | README に単体テストの手順あり。本ドキュメントに島ソルバ・ベンチ情報を追記。 |

## 3. 機能拡張ロードマップ
### 3.1 剛体
1. **質量特性拡張**  
   - 慣性テンソル更新ロジック（2D 版）を追加し、任意形状に対応できるようにする。✅ `chrono_body2d_set_polygon_shape_with_density` で凸ポリゴンの質量・慣性を自動設定済み。  
   - `chrono_body2d_set_mass_properties` の実装とテスト（任意分布の対応は今後の課題）。
2. **積分器拡張**  
   - セミインプリシット・サブステップなどを導入。`chrono_body2d_integrate_semi_implicit` 等の API を検討。
3. **スリープ/ウェイク管理**  
   - 速度閾値と時間閾値を導入し、アイドルなボディを休止状態へ移行。

### 3.2 拘束
1. **抽象インターフェース化**  
   - `ChronoConstraint2DInterface`（関数テーブル）を追加し、複数拘束を扱えるよう統合管理。  
   - **設計メモ（2025-10-19 更新）**  
     - 共通構造体 `ChronoConstraint2DOps_C` に `prepare`, `warm_start`, `solve_velocity`, `solve_position` の関数ポインタを保持。  
     - ボディ参照やアンカーポイントなどの共有データを `ChronoConstraint2DBase_C` にまとめ、各拘束型はこれを先頭メンバとして含む。  
     - 初期化関数で ops を設定し、既存 API (`chrono_distance_constraint2d_prepare` 等) は ops 呼び出しに委譲する薄いラッパーへ移行して後方互換を維持。  
     - 拘束配列を処理する `chrono_constraint2d_batch_solve(...)` を追加し、Sequential Impulse 風に反復できるよう設計。  
     - 将来的な並列化を見据え、バッチ構造には拘束グループ ID（アイランド）を保持できる拡張余地を残す。
2. **新規拘束タイプ**  
   - ✅ 回転ジョイント（ピン）: `ChronoRevoluteConstraint2D_C` を追加し、Sequential Impulse ベースのバッチソルバに統合。`tests/test_revolute_constraint` で回帰確認。  
   - ✅ プリズマティック（スライダ）: `ChronoPrismaticConstraint2D_C` に法線拘束＋ウォームスタートを実装し、ストローク制限とモータ駆動（リミット＆モータ）を追加。位置制御モード（PID ベース）とソフトリミット（ばね＋ダンパ）を導入し、`tests/test_prismatic_constraint` でリミット／モータ動作と位置収束を確認。  
   - ✅ リボルート＋ギア: `ChronoRevoluteConstraint2D_C` に速度／位置モータを追加し、`ChronoGearConstraint2D_C` でギア比拘束を実装。`tests/test_revolute_constraint` と `tests/test_gear_constraint` を追加。  
   - ✅ プラナー（2軸スライダ）: `ChronoPlanarConstraint2D_C` を追加し、軸毎のモータ／ソフトリミット／位置制御をサポート。Baumgarte 0.15／PID 周波数 3.5 Hz・減衰 1.2／ソフトリミットばね 55 N/m・ダンパ 8 N·s/m を推奨初期値とし、`tests/test_planar_constraint_longrun`（6000 ステップ）でモータ目標切替え・リミット衝突の安定性を回帰確認。`tests/test_planar_constraint` で基本挙動をチェックできる。  
   - ✅ スプリングダンパ: `ChronoSpringConstraint2D_C` を追加し、フック＋粘性力をインパルスとして適用。`tests/test_spring_constraint` で収束挙動を確認。  
   - ✅ 距離＋角度複合拘束: `ChronoDistanceAngleConstraint2D_C` を実装し、`test_distance_angle_constraint` / `test_distance_angle_endurance` で収束と耐久性を回帰。  
   - ✅ 線形結合拘束: 距離+角度の一次結合を扱う `ChronoCoupledConstraint2D_C` を実装し、距離/角度ソフトネスの個別設定、バネ/ダンパ、`last_distance_force` / `last_angle_force` ログを追加。複数式（最大 4 本）の同時拘束に対応し、式別ログと `chrono_coupled_constraint2d_get_diagnostics` によるランク欠損・条件数の検出を実装。条件数が `CHRONO_COUPLED_DIAG_CONDITION_WARNING` 閾値を超えた場合に標準エラーへ警告を出しつつ、オプトイン設定で最小対角の式を自動ドロップできるよう `chrono_coupled_constraint2d_set_condition_warning_policy` を追加。`tests/test_coupled_constraint` / `tests/test_coupled_constraint_endurance` でパラメータ切替と長時間挙動を回帰。  
   - 今後: スライダのリミット／モータ、他拘束タイプ（スライダ2軸、ギア、距離減衰など）を段階的に追加。
3. **ソルバー**  
   - Sequential Impulse / Gauss-Seidel による複合拘束解決。  
   - Baumgarte 以外の安定化手法（位置校正、XPBD 的アプローチ）の検討。

### 3.3 接触
1. **接線摩擦・接線方向インパルス**  
   - クーロン摩擦モデルの実装と回帰テスト。  
   - スリープ解除や粘着処理フラグの検討。
   - ボディ単位での摩擦・反発係数設定APIを提供し、接触毎に組み合わせた係数を利用。
2. **継続接触管理**  
   - マンifold（複数接触点）の保持、ウォームスタート用蓄積インパルス管理。
3. **形状拡張**  
   - 凸ポリゴン対応：SAT ベースの検出・解決を追加済み（円 vs ポリゴン、ポリゴン vs ポリゴン）。  
   - カプセル・エッジ形状を追加し、2D GJK/EPA による汎用検出パスを実装。今後はGJKの多接触抽出・GJK/EPAの精度向上やGJK活用範囲拡大を検討。
4. **アイランドソルバーの導入**  
   - 拘束・接触をまとめたアイランド分割と並列実行への布石。  
   - `chrono_island2d_build`・`chrono_island2d_solve` を実装済み（OpenMP 対応）。今後は形状拡張／新拘束追加後の統合稼働を想定。

### 3.4 テスト & ツール
1. **単体テスト整備**  
   - 各拘束・接触ケースの数値回帰テストを `tests/` 以下に追加。  
   - `test_island_parallel_contacts`・`test_island_builder` でアイランド分割の正当性と並列経路を検証。  
   - `test_polygon_collision` で凸ポリゴン／円の接触検出と解決を回帰し、`test_polygon_mass_properties` で質量・慣性の数値精度を確認。  
   - `test_polygon_slope_friction`・`test_polygon_spin_collision` で斜面摩擦・回転ポリゴン衝突シナリオを網羅、`test_capsule_edge_collision` でカプセル・エッジの組合せを検証。  
   - `test_island_polygon_longrun` でポリゴン同士の接触と距離拘束を同一アイランド内で長時間駆動し、ウォームスタート統合を回帰。  
   - `test_contact_manager_longrun` でマニフォールドの再利用・ウォームスタート安定性を長時間回帰。  
   - `make test` で全テストが実行されるよう連携。
2. **ベンチマーク/デモ**  
   - パフォーマンス検証用のベンチスイートを整備し、改善効果を把握。  
   - `tests/bench_island_solver` を追加（呼び出し例: `make bench` または `./tests/bench_island_solver 128 400 8`）。スレッド数スイープ結果をログに出力。  
   - `examples/planar_constraint_demo` を追加し、`docs/planar_constraint_visualization.m` でモータ／リミット挙動を教材用に可視化できるようにした。  
   - Coupled 拘束用のマイクロベンチ（多数の式を抱える拘束をアイランド内に並べ、条件数／反復回数／解法時間を計測）を次スプリントで追加。結果を CSV 化し、CI の nightly ベンチに組み込む計画。
3. **CI / 可視化連携**  
   - GitHub Actions 等でテスト自動実行（ローカルでの再現性重視、後日CI環境を検討）。  
   - `tools/plot_coupled_constraint_endurance.py` に Markdown/HTML/JSON サマリ出力と `--skip-plot`/しきい値判定オプションを追加し、ヘッドレス CI から `--no-show` と組み合わせてレポート生成および自動ゲートが可能。  
   - `tools/archive_coupled_constraint_endurance.py` で耐久 CSV を SHA-256 ハッシュで重複判定しつつ `data/endurance_archive/` に時刻付きで保存、`latest.*`（CSV/Markdown/HTML/JSON）と manifest を更新、`--prune-duplicates`/`--max-entries`/`--max-age-days`/`--max-file-size-mb`/`--plan-markdown` で履歴や容量を整理。保護したいアーカイブは `--exclude-config`（YAML/JSON）で保留リストに登録可能。  
   - 週次（月曜 03:15 UTC）ワークフロー `.github/workflows/coupled_endurance.yml` を追加し、アーカイブ＋可視化＋アーティファクト化＋重複整理まで自動実行（`coupled_endurance-${{github.run_id}}` として `latest.csv/.summary.*` と manifest を収集）。通知は `tools/compose_endurance_notification.py` で Slack/メール共通テンプレートを生成し、失敗時には `tools/fetch_endurance_artifact.py` が自動的に再現コマンドとコメントを生成するフォールバックを追加。CI 失敗時の調査手順は `docs/coupled_endurance_ci_troubleshooting.md` を参照。

## 4. マイルストンとタスク一覧
| フェーズ | 想定期間 | 主タスク | 成果物 | 進捗 |
|----------|----------|----------|----------|------|
| Phase 1: テスト基盤強化 | 1-2 週間 | 既存テスト拡張、`make test` 整備、リグレッションテスト追加 | テストスイート、ログ | ✅ （正面/斜め/静止/摩擦の接触テストを整備済み） |
| Phase 2: 拘束インターフェース化 | 2-3 週間 | 距離拘束の共通化、ピンジョイント追加 | `chrono_constraint2d` 拡張、テスト | ⏳ （ops テーブル導入済み。複数拘束処理と新拘束追加が未実装） |
| Phase 3: 接触摩擦対応 | 2-3 週間 | 接触摩擦、接線方向処理、接触継続管理 | `chrono_collision2d` 拡張、回帰テスト | ⏳ |
| Phase 4: 形状/ソルバー強化 | 3-4 週間 | 凸形状衝突、Sequential Impulse、アイランド処理 | 拘束/接触統合ソルバー | ⏳ |
| Phase 5: 最終統合・ドキュメント | 1-2 週間 | ドキュメント整備、ベンチ、CI 準備 | README 更新、設計ドキュメント | ⏳ |

## 5. 進捗トラッキング
- **最新更新日**: 2025-10-19
- **完了済みハイライト**
- 明示的距離拘束テスト (`test_distance_constraint_stabilization`) 自動化。
- 距離拘束のソフト化 (`chrono_distance_constraint2d_set_spring`) を追加し、`test_distance_constraint_soft` でばね・ダンパ挙動を回帰できるようにした。
- 距離拘束に線形／角度ソフトネス設定を追加し、`test_distance_constraint_multi` で複数拘束同時解決と角速度連成の回帰テストを整備。
- 円衝突回帰テスト (`test_circle_collision_regression`) 追加。
- 円衝突斜め回帰テスト (`test_circle_collision_oblique`) 追加し、摩擦ゼロ時の接線速度保存を検証。
- 円衝突静止回帰テスト (`test_circle_collision_resting`) 追加し、ゼロ相対速度での安定性を確認。
- 円衝突摩擦回帰テスト (`test_circle_collision_friction`) を追加し、静摩擦保持と動摩擦減速、マンifoldウォームスタートを確認。
- 円衝突摩擦回帰テスト (`test_circle_collision_friction`) を追加し、静摩擦保持と動摩擦減速、マネージャ経由でのマニフォールドウォームスタートを確認。
- マテリアル組み合わせテスト (`test_circle_collision_materials`) を追加し、ボディごとの係数設定が期待どおり合成されることを解析解と比較して確認。
- 連続接触シミュレーションテスト (`test_circle_collision_continuous`) を追加し、接触マネージャを用いた複数フレームの滑り/反発処理を確認。
- Coupled 拘束の長時間挙動を確認する `test_coupled_constraint_endurance` を追加し、比率切替・ターゲット更新時のログを `data/coupled_constraint_endurance.csv` に書き出して診断フラグと条件数を回帰。
- 拘束フレームワークに `ChronoConstraint2DOps_C`/`ChronoConstraint2DBase_C` を導入し、距離拘束を新インターフェースへ移行。
- 距離拘束バッチソルバ (`chrono_constraint2d_batch_solve`) と OpenMP ベースの SMP 対応を実装し、`test_constraint_batch_solve` で並列／直列の一致を検証。
  - アイランド分割を Union-Find＋ハッシュマップベースに刷新し、大規模拘束でも高速にグルーピング可能にした。
- **次のアクション候補**
  1. 摩擦付き接触での複数接触点・マンifold拡張（複数点、永続性）を検討し、継続接触の安定化を図る。
  2. ボディ/素材ごとの摩擦係数設定APIを検討し、テストを拡張（複数組み合わせ）。

## 9. バッチソルバ最適化メモ（2025-10-19）
- 目的: `chrono_constraint2d_batch_solve` 内で繰り返し確保している中間配列（アイランドID、サイズ、オフセット、並び替えバッファなど）を再利用可能なワークスペースに移し、ヒープ割り当てコストと断片化を低減する。
- 方針:
  - `ChronoConstraint2DBatchWorkspace_C` を導入し、必要バッファと現在容量を保持。
  - API: `chrono_constraint2d_workspace_init`, `chrono_constraint2d_workspace_reserve`, `chrono_constraint2d_workspace_reset`, `chrono_constraint2d_workspace_free` を提供。
  - バッチソルバはワークスペースが指定された場合それを利用し、NULL の場合は従来どおり内部で一時バッファを確保するフォールバックを用意。
  - テストではワークスペース有無双方で挙動が変わらないことを検証する。
- 利用例（シミュレーションループ）:
  ```c
  ChronoConstraint2DBatchWorkspace_C workspace;
  chrono_constraint2d_workspace_init(&workspace);

  while (running) {
      chrono_constraint2d_workspace_reset(&workspace);
      chrono_constraint2d_batch_solve(constraints,
                                      constraint_count,
                                      dt,
                                      &batch_cfg,
                                      &workspace);
      // その他のステップ処理...
  }

  chrono_constraint2d_workspace_free(&workspace);
  ```

## 10. 摩擦導入テスト設計（初版）
- 目的: 円同士の接触に静摩擦・動摩擦を導入する際の挙動を検証し、将来の接線方向処理に備える。
- 想定シナリオ
  | ケース | 内容 | 期待挙動 | 備考 |
  |-------|------|---------|------|
  | 静止接触保持 | 接触面で相対速度 0、摩擦係数高 | 接線方向速度が発生せず静止維持 | 静摩擦閾値テスト |
  | 静摩擦崩壊 | 接線方向力が静摩擦を超える | 滑り始め、動摩擦係数に基づく減速 | 閾値付近での分岐確認 |
  | 斜め衝突滑り | 初速度に接線成分あり | 反発＋接線減速（動摩擦） | 既存斜め衝突テスト拡張 |
  | 継続接触 | 連続タイムステップで接触継続 | 摩擦インパルスの蓄積・解放を検証 | アイランド分割の影響確認 |
- テスト実装メモ:
  - 既存 `test_circle_collision_*` シリーズを拡張し、摩擦係数と接線方向の目標値を追加。
  - 静摩擦 vs 動摩擦の閾値比較用にサンプル力（もしくは接線速度）を設定。
  - 将来的なウォームスタートや接触継続処理と相性を確認するため、連続ステップのログ/チェックを導入。
  - `ChronoContactManifold2D_C` を用意し、接触ごとの法線・接線インパルスを蓄積してウォームスタートできるようにした。
  - `ChronoContactManager2D_C` でボディペアごとのマニフォールドを管理し、最大2接触点の継続・ウォームスタートを扱えるようにした。

## 6. 参考資料・メモ
- Chrono-main (C++) の関連ソース：`ChBody`, `ChLink`, `ChContactContainer` 等
- 既存の 3D 実装を参考にする際は、データ構造と API レイヤに差異があるため抽象化設計を先に行うこと。
- 接触摩擦導入時は数値発散を避けるため、テストを細分化しベータ版段階での回帰テストを徹底する。

## 7. SMP 対応方針（初版）
- 目標: 拘束・接触処理のうちデータ依存性の低い部分を OpenMP を用いた並列化で高速化。
- 当面の対象: 距離拘束のバッチ解法 (`chrono_constraint2d_batch_solve`) をマルチスレッド化し、複数拘束を独立に処理できるケースで並列実行する。
- 設計メモ:
  - 依存関係のある拘束同士は同一スレッド／シリアル処理とし、将来的なアイランド分割アルゴリズム導入で並列性を向上させる。
  - OpenMP セクションを利用し、`#pragma omp parallel for` で拘束配列を処理。
  - テストでは OpenMP を有効にしたビルドで結果がシリアル実装と一致することを確認。
  - Makefile に `-fopenmp` フラグを追加し、環境変数 `OMP_NUM_THREADS`で制御可能とする。

## 8. アイランド分割計画（新規）
- 目的: ボディが共通する拘束同士を同じ「アイランド」としてグループ化し、並列ソルバ実行時のデータ競合を避ける。
- 初期実装案:
  - 入力: `ChronoConstraint2DBase_C*` 配列と件数。
  - Union-Find または BFS を用いて、共有ボディを介して連結な拘束を同じアイランドへ割り当てる。
  - 出力: アイランド配列（開始インデックス＋サイズ）または各拘束のアイランドID。
  - 既存バッチソルバはアイランド単位で逐次実行し、`enable_parallel` が真の場合はアイランド間を並列化する。
- テスト計画:
  - 3 本の拘束でアイランド数が 2 になるケースを用意し、ビルドしたアイランド情報が期待通りか確認。
  - 並列実行でアイランド単位にスケジュールされるかどうかをログまたは結果の一致で検証。

---
※ 本ドキュメントは進捗に応じて逐次更新します。更新時は「最新更新日」と該当セクションの内容を明確に変更してください。

## 11. 接触マネージャ運用ガイド（初版）
- 推奨ライフサイクル（1ステップ）
  1. `chrono_contact_manager2d_begin_step(manager)` を呼び、既存マニフォールド内の接触点を一時的に非アクティブ化する。
  2. 座標更新後、ペアごとに `chrono_collision2d_detect_*` → `chrono_contact_manager2d_update_*` → `chrono_collision2d_resolve_*` の順で処理し、必要な回数だけ解決する。
  3. 処理完了後 `chrono_contact_manager2d_end_step(manager)` を呼び、未使用の接触点や空マニフォールドを自動で削除する。
- 境界ケース方針
  - 接触が消失したペア: `end_step` で `num_points == 0` のマニフォールドは削除されるため、別途クリーンアップは不要。
  - 接触点の逸脱: `chrono_contact_manifold2d_add_or_update` は位置差 < 0.01m かつ法線内積 > 0.95 を満たす場合に既存点を再利用。大きく離れた点は新規登録され、旧点は `end_step` で自然に除去される。
  - 複数点の解決順: 現状は最大2点まで保持し、先に登録された点から順に解決。2点目は最小貫入点を上書きする簡易ルールであり、将来必要に応じ優先戦略を見直す。
  - アイランドとの親和性: マネージャはボディペア単位でありアイランド情報は保持しない。アイランドソルバに組み込む際は、各アイランドの解法前後に `begin_step`/`end_step` を呼び、並列実行する場合はマネージャアクセスの同期が必要。

## 12. 拘束・接触アイランド統合設計（初版）
- 目的: 接触マネージャと拘束バッチソルバを共通のアイランドフレームワークで扱い、並列実行時の競合やスリープ判定を簡潔にする。
- 基本方針:
  - 剛体ノードをキーに Union-Find を構築し、拘束 (`ChronoConstraint2DBase_C`) と接触ペア (`ChronoContactPair2D_C`) の両方から辺を貼る。
  - 各アイランドには「拘束リスト」と「接触マニフォールドリスト」を保持し、ソルバはアイランド単位で `begin_step` → 拘束バッチ → 接触処理 → `end_step` の順に実行する。
  - 接触マネージャはアイランドごとに分割されたビューを持ち、`chrono_contact_manager2d_begin_step/end_step` はアイランド処理の外側で一括またはアイランド内で個別に呼ぶ。並列処理時はアイランド内でのみマネージャを触る。
- 境界ケース:
  1. **拘束のみ/接触のみのアイランド**: どちらか片方だけ存在する場合でもフレームワークが成立するよう、空リストを許容する。
  2. **多接触点の揺れ**: 同一点で高速に接触/離脱を繰り返す場合、アイランド再構築のコストが問題になる可能性があるため、将来的に接触安定化（キャッシュ）やスリープ判定の導入を検討。
  3. **並列処理時の競合**: アイランドごとにローカルなマネージャ・拘束リストを用意することで競合を回避。ただし、接触マネージャのグローバルリストを共有する場合はロックが必要になるため、基本はアイランド単位のハッシュマップに分割。
- 今後のタスク候補:
  - `ChronoConstraint2DBase_C` と `ChronoContactPair2D_C` を併せて扱う共通アイランド構造（例: `ChronoIsland2D_C`）を定義済み。
  - アイランドごとのソルバ呼び出しラッパを実装し、OpenMP などで並列化するパイプラインを試作済み（`chrono_island2d_solve`）。
  - テスト: 拘束＋接触が混在する小さなセットアップを用いた `test_island_contact_constraint`、複数アイランド並列検証の `test_island_parallel_contacts`、ビルドロジック単体検証の `test_island_builder` を整備。
- 連続接触シミュレーションテスト (`test_circle_collision_continuous`) を追加し、接触マネージャを用いた複数フレームの滑り/反発処理を確認。
- 拘束＋接触を合わせた並列テスト (`test_island_contact_constraint`) を追加し、アイランド統合後の競合がないことを確認。
- 長時間接触回帰 (`test_contact_manager_longrun`) を追加し、マニフォールド再利用の持続性を検証。

## 13. Coupled 拘束ユースケース／チューニング事例（2025-10-21）

- 対象: 距離＋角度を線形結合し、複数式（`CHRONO_COUPLED_MAX_EQ`=4）を同時に解く `ChronoCoupledConstraint2D_C`
- 目的: 既存 2D シーンのパターン把握と、テスト／ロギング／チューニング時の指針を確立する
- 主要プリセットは `data/coupled_constraint_presets.yaml` に記録し、スクリプトや CI で共有できるようにしている。

### 13.1 代表ユースケース
- **テレスコピック＋ヨー制御**: ブーム先端の距離制御と旋回角を組み合わせ、`ratio_distance=1.0` × `ratio_angle=0.4` で目標姿勢を連動。距離ソフトネス 0.012-0.018、角度ソフトネス 0.025-0.04 が安定域。
- **カム機構の追従補正**: 従動節の距離誤差と角度エラーを一次結合し、`target_offset` で位相調整。追加式で `ratio_distance` を 0.5 未満に抑えると、角速度スパイクを緩和できる。
- **カウンターバランス梁**: 主拘束を距離主導（`ratio_distance` > 0.8）にし、補助式に `ratio_angle=-0.3` を持たせてモーメントの釣り合いを保持。角度スプリング 15-22 N·m/rad を加えるとドリフト抑制が容易。
- **ドッキングガイド**: 位置×姿勢の連動でズレ吸収する用途。`tests/test_coupled_constraint` の `extra` 式（距離 0.55 / 角度 -0.25）をテンプレートに、接線方向の制御を別拘束で補完する。

### 13.2 パラメータチューニング指針
- **Baumgarte/Slop**: 0.35-0.4 と 5e-4-7e-4 の組み合わせが耐久テストで最も安定。大きな目標切り替えがある場合は `max_correction` を 0.08-0.1 に引き上げる。
- **ソフトネス**: グローバル値（`chrono_coupled_constraint2d_set_softness_*`）は 0.015 前後を基準に、式ごとの上書きで急峻な制御点を局所調整する。角度側は距離側の 1.5-2 倍が目安。
- **スプリング／ダンパ**: 距離 30-45 N/m、角度 15-25 N·m/rad が標準。急峻な比率変更を扱う場合は式別に 20 N/m / 10 N·m/rad を追加し、`tests/test_coupled_constraint_endurance` と同条件でドリフトを確認。
- **追加式の管理**: `chrono_coupled_constraint2d_add_equation` で 2 本目以降を登録したら、切り替え時に `chrono_coupled_constraint2d_set_equation` を介し差分更新しないとウォームスタート値が途切れる。大きな比率変更前に `target_offset` をゼロへ戻すと過渡振動が抑えられる。
- **ログ観測**: `constraint.last_distance_force_eq[i]` / `last_angle_force_eq[i]` を CSV へ落とし込み、`tools/plot_coupled_constraint_endurance.py` でピーク検査や条件数の推移を確認。`tests/test_coupled_constraint_endurance` の CSV にはドロップ数・対象式 index・再解ステップ数を追記済み。週次ジョブでは `tools/run_coupled_benchmark.py`（閾値は `config/coupled_benchmark_thresholds.yaml` で共通化）を用いて `data/coupled_benchmark_metrics.csv` と GitHub Actions Warning を自動収集し、`tools/summarize_coupled_benchmark_history.py` で Markdown/HTML レポートと簡易グラフ（条件数・pending 推移）を生成、さらに `.github/workflows/coupled_benchmark.yml` が GitHub Pages へ自動デプロイする（ローカル手順は `docs/coupled_benchmark_setup.md` を参照）。

### 13.3 診断・警告運用
- **診断フィールド**: `ChronoCoupledConstraint2DDiagnostics_C` は `flags`（`CHRONO_COUPLED_DIAG_*`）、`rank`、`condition_number`、pivot 最小/最大を提供。閾値超過時に `CHRONO_COUPLED_DIAG_CONDITION_WARNING` が立つため、CI ではこのビットの回数を集計する。
- **ポリシー設定**: `ChronoCoupledConditionWarningPolicy_C` で `enable_logging`（デフォルト WARN 出力）、`log_cooldown`（秒換算タイマー）、`enable_auto_recover`（最小対角式のドロップ）、`max_drop` を制御。ドロップが発生した場合は `diagnostics.rank` が能動式数と一致することを確認する。
- **標準ログ基盤との統合**: `chrono_log` のハンドラ差し替え手順は `docs/chrono_logging_integration.md` に集約済み。CI や長時間テストでは WARN→INFO へダウングレードし、stderr 汚染を抑えつつ `diagnostics.condition_number` の閾値監視を継続する。

### 13.4 テスト＆検証フロー
- **単体テスト**: `tests/test_coupled_constraint` で式追加・条件数警告・自動式ドロップを網羅。新しい比率やソフトネスを導入する場合はここへケース追加。
- **耐久テスト**: `tests/test_coupled_constraint_endurance` は 7200 ステップで複数段階のターゲット切り替えを実施。CSV を `tools/plot_coupled_constraint_endurance.py` でプロットし、最大誤差・最大力・条件数をサマリへ抽出。
- **ベンチ指標**: マイクロベンチ（近日実装）では「式数」「diagnostics.condition_number」「Gauss-Jordan の反復/ドロップ回数」「solve 時間(ns)」を計測する。目安は条件数 1e6 以内でドロップ 0、1e10 クラスで 1-2 式ドロップ、1 ステップ 200 µs 未満（デスクトップ CPU）。
- **ログレビュー**: 耐久テスト中の WARN は仕様。CI へ取り込む際はポリシーで WARN→INFO へ切り替えつつ、`accumulated_flags` と CSV のドロップ統計を閾値評価する（`tools/run_coupled_benchmark.py` は自動 Warning を出力）。

### 13.5 診断ログを使ったデバッグワークフロー
1. **収集**: `chrono_coupled_constraint2d_get_diagnostics` の結果（`flags`, `rank`, `condition_number`, pivot 値）と、式別反力 `last_distance_force_eq[]` / `last_angle_force_eq[]` を CSV に記録。`tests/test_coupled_constraint_endurance` が雛形。
2. **可視化**: `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --output out.png` を実行し、条件数ピークと WARN ログのタイムスタンプを照合。必要に応じ `phase_id` 列を追加してステージ境界を描画。
3. **分析**: 条件数が長時間高止まりする場合は、該当ステップの `equation_active[]` と `diagnostics.rank` を比較し、自動ドロップが働いたか判断。補助式の `ratio_*` を調整するか、`softness_*` を増やして安定化を図る。
4. **再検証**: パラメータ更新後は `tests/test_coupled_constraint` とマイクロベンチ（実装予定）を再実行し、条件数・反力ピークが改善されたか確認する。CI 取り込み時は WARN→INFO フラグを設定した上で、CSV の最大値を自動チェックするスクリプトを追加予定。
