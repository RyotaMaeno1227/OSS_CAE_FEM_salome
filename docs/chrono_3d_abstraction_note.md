# chrono-C 3D 抽象化メモ（草稿）

## 1. 背景
- 目的: `chrono-C-all` の 2D コア（`ChronoConstraint2DBase_C` / `ChronoConstraint2DOps_C` / `chrono_constraint2d_batch_solve` 等）を再利用しつつ、3D 版へ自然に拡張できる抽象化ポイントと API 互換ポリシーを整理する。
- モチベーション: 近々予定されている 3D 版移植議論の事前資料として、構造変更の影響範囲と段階的な移行計画を可視化する。
- 前提: 既存 2D 実装は C API を維持しながら進化中（`docs/chrono_2d_development_plan.md` 参照）。3D でも C API を維持し、テスト・CI フローを共存させる方針とする。

## 2. 抽象化対象（2D → 共通化）
- **拘束基底構造体**: `chrono-C-all/include/chrono_constraint2d.h` の `ChronoConstraint2DBase_C`（ボディ参照＋ ops テーブル）を `ChronoConstraintCommon_C`（仮称）へリネームし、軸数や積分計算に依存しないメンバだけを残す。2D 固有フィールド（`accumulated_impulse` 等）は派生側へ移譲。
- **関数テーブル**: `ChronoConstraint2DOps_C` をそのまま再利用し、3D 拘束も `prepare` / `apply_warm_start` / `solve_velocity` / `solve_position` の 4 フェーズで統一。演算対象が 3 成分になるだけなので署名変更は不要。
- **ワークスペース**: `ChronoConstraint2DBatchWorkspace_C`（アイランド ID や並列用バッファ）を名前だけ一般化し、内部バッファは void* ではなくテンプレート的に扱えるようメモリ確保関数を共有化する。
- **バッチソルバ設定**: `ChronoConstraint2DBatchConfig_C` の `velocity_iterations` / `position_iterations` / `enable_parallel` はそのまま流用できる。名称だけ `ChronoConstraintBatchConfig_C` に寄せ、2D ラッパを提供して後方互換を維持する。
- **ログ／診断**: Coupled 拘束の診断構造体（`ChronoCoupledConstraint2DDiagnostics_C`）のように、次元に依存しない値は prefixed を落として汎用化。3D 追加時は行列サイズや pivot 情報が増えるが、構造体自体は共有できる。

## 3. API 互換方針
- **名前空間レイヤ**: 既存の `chrono_constraint2d_*` 関数は全てラッパとして残し、内部で dimension-agnostic な実装（例: `chrono_constraint_common_batch_solve`）へ委譲する。新 API は `chrono_constraint_batch_*` のプレフィックスを想定。
- **構造体の二層化**: ヘッダでは `typedef ChronoConstraintCommon_C ChronoConstraint2DBase_C;` のように 2D 別名を提供し、既存コードを変更せずにビルドできる状態を先に確保。その後、3D 専用 typedef (`ChronoConstraint3DBase_C`) を追加する。
- **ビルド切り替え**: 2D/3D を同一ソースツリーでビルドできるよう、`CHRONO_ENABLE_3D` フラグで 3D 向けの追加エントリポイントを有効化。CMake/Makefile では両方のユニットテストを選択的に実行できるようターゲットを分割する。
- **バイナリ互換**: 2D の ABI が変わらないよう、構造体サイズの再配置は避ける。共通化でフィールド位置が動く場合は、`static_assert(sizeof(ChronoConstraint2DBase_C) == ...)` をテストに追加し、互換性を担保する。
- **エラーハンドリング**: 3D 導入時に増える可能性がある `CHRONO_*` エラーコードは、既存の `chrono_constraint2d_*` 関数からも返却されるため、エラー番号の予約領域（例: 2000-2099 を 3D 用に確保）を記録する。

## 4. 実装留意点
- **ボディ表現の差異**: 2D は `ChronoBody2D_C`（位置2成分＋角1成分）で完結しているが、3D ではクォータニオンや 3x3 慣性テンソルが必須。共通ヘッダで `ChronoBodyCommon_C` を導入し、2D/3D それぞれがラッパを提供する構造にすると API の見通しが良い。
- **Jacobian 次元**: 現状のソルバは 1×3 の Jacobian を前提としている箇所がある（距離拘束など）。3D では 1×6（並進3＋回転3）になるため、行列演算ユーティリティを抽象化する必要がある。Gauss-Seidel 実装は pivot 計算を一次元で書いているので、3D 導入前に小さな行列演算ヘルパ（2×2、3×3、最大4×4）を整備する。
- **座標フレーム管理**: 2D 版の `chrono_body2d_local_to_world` は 2×2 ローテーションで済む。3D ではクォータニオンの正規化・浮動小数安定性が課題になるため、共通の math ヘルパ層（`chrono_math.h` 的な位置づけ）を導入し、2D/3D 双方で共有する。
- **ソルバ再利用**: `chrono_constraint2d_batch_solve` のループは拘束 ops へのポインタ呼び出しで完結しているため、3D でもそのまま流用できる。各拘束の `prepare` 内で Jacobian サイズや世界座標計算を調整するだけで済むことを確認している。
- **診断・ログ**: Coupled 拘束の条件数監視や自動式ドロップは、3D でも同様の仕組みが必要。ログ基盤を `chrono_log_warn` に統一する際に、拘束タイプ名や DOF 数をログに含めるフォーマット拡張を行うと、2D/3D の区別が容易になる。

## 5. 移行ステップ案
1. **型エイリアスの導入**: `ChronoConstraintCommon_C` と `ChronoConstraintCommonOps_C`（仮称）を追加し、既存 2D コードは typedef ベースで移行。CI で ABI 変化がないことを確認。
2. **ワークスペース汎用化**: `ChronoConstraint2DBatchWorkspace_C` を共通化し、テスト（`tests/test_constraint_batch_solve` 等）を通しつつリファクタ。OpenMP フラグやアイランドロジックは現状維持。
3. **数学ヘルパ導入**: ベクトル/行列演算の共通ユーティリティを用意し、2D 実装も順次移行。3D 拘束に必要な 3×3、4×4 の補助関数を先に整備する。
4. **3D プロトタイプ作成**: `ChronoRevoluteConstraint3D_C` など、代表的な 1-2 種の拘束を試作し、共通 ops テーブルで動作することを確認。ユニットテストを `tests3d/`（仮）に追加。
5. **API 公開準備**: ドキュメント（`chrono-C-all/README.md` 等）とヘッダコメントを更新し、2D/3D 混在時の使用例を提示。CI では 2D → 3D → 混在の順でテストを追加。

## 6. 想定リスクと対策
- **ABI 変化**: 構造体サイズが変わると既存のバイナリ互換性が崩れるため、段階的に `static_assert` とヘッダガードを導入。必要に応じて `CHRONO_CONSTRAINTS_ENABLE_LEGACY_ABI` フラグを用意し、旧構造体を強制。
- **パフォーマンス劣化**: 共通化による indirection 増加を避けるため、`inline` ヘルパや `restrict` 修飾子の利用を検討。ベンチ（予定している Coupled マイクロベンチ）を 2D/3D で共有して回帰をとる。
- **テスト増加**: 3D 追加でテスト時間が増すため、CI マトリクスでは smoke（2D）、extended（2D+3D）、nightly（耐久＋ベンチ）と段階分けする。

## 7. 参考
- コード参照: `chrono-C-all/include/chrono_constraint2d.h`, `chrono-C-all/src/chrono_constraint2d.c`, `chrono-C-all/src/chrono_island2d.c`
- ドキュメント: `docs/chrono_2d_development_plan.md`, `docs/distance_angle_constraint_spec.md`
- ツール: `tools/plot_coupled_constraint_endurance.py`（診断可視化）、今後追加予定のマイクロベンチスクリプト

## 8. API 互換マトリクス（初版）

| 機能領域 | 2D API/構造体 | 3D 想定（案） | 移行メモ |
|----------|---------------|---------------|----------|
| 基底構造 | `ChronoConstraint2DBase_C` | `ChronoConstraintCommon_C`（新規） + `typedef ChronoConstraintCommon_C ChronoConstraint3DBase_C;` | 2D ヘッダは typedef 維持。 |
| Ops テーブル | `ChronoConstraint2DOps_C` | `ChronoConstraintCommonOps_C`（同型） | フィールド名・関数ポインタは同一。 |
| バッチソルバ | `chrono_constraint2d_batch_solve` | `chrono_constraint_batch_solve`（共通実装）、2D/3D ラッパ | ワークスペース API も共通化。 |
| ワークスペース | `ChronoConstraint2DBatchWorkspace_C` | `ChronoConstraintBatchWorkspace_C` | メモリ確保関数を関数ポインタ化。 |
| ボディ API | `ChronoBody2D_C` | `ChronoBodyCommon_C` + `ChronoBody3D_C` | 2D ラッパで既存フィールドレイアウト保持。 |
| 距離拘束 | `chrono_distance_constraint2d_*` | `chrono_distance_constraint3d_*` ラッパ → 共通実装 | 3D 版は 3 軸 Jacobian を追加。 |
| 回転拘束 | `chrono_revolute_constraint2d_*` | `chrono_revolute_constraint3d_*` | Ops 呼び出しの差分は姿勢計算のみ。 |
| Coupled 拘束 | `chrono_coupled_constraint2d_*` | `chrono_coupled_constraint3d_*` or 汎用化 | 行列サイズが 4×4 まで拡張。 |
| 診断 | `ChronoCoupledConstraint2DDiagnostics_C` | `ChronoConstraintDiagnostics_C`（共通） | pivot 情報配列サイズをパラメータ化。 |
| ログ基盤 | `chrono_log_warn`（予定） | 同一 API | ログレベル enum を共通ヘッダへ移行。 |

## 9. 抽象化レイヤ設計案

```
┌──────────────────────────────────────────────┐
│  アプリケーション層                          │
└───────────────▲─────────────────────────────┘
                │ C API
┌───────────────┴─────────────────────────────┐
│  Constraint Facade (2D/3D ラッパ)            │
│   - chrono_constraint2d_* / 3d_*             │
└───────────────▲─────────────────────────────┘
                │ typedef / inline
┌───────────────┴─────────────────────────────┐
│  Common Core (`ChronoConstraintCommon_*`)     │
│   - ops dispatch                              │
│   - batch solver                              │
│   - diagnostics logging                       │
└───────────────▲─────────────────────────────┘
                │ math helpers
┌───────────────┴─────────────────────────────┐
│  Math/Geometry Layer                          │
│   - Vector/Matrix utils (2×2,3×3,4×4)         │
│   - Frame transforms (2D rot, 3D quat)        │
└──────────────────────────────────────────────┘
```

- **Facade 層**: 現行 2D 関数名を守りつつ、将来的に `chrono_constraint3d_*` を追加。型エイリアスと inline ラッパのみで構成。
- **Common Core**: バッチソルバや診断ロジックを集中させる。ここから下は次元非依存コードとし、ビルドフラグで 2D/3D のみを選択的に有効化可能にする。
- **Math/Geometry 層**: 数学ユーティリティを共通化し、2D 既存コードも順次移行。3D 導入時はここを拡張するだけで Jacobian 計算が可能になる。

## 10. リスク対策チェックリスト

- [ ] `ChronoConstraint2DBase_C` のサイズが共通化後も一致するか `static_assert` で検証した。
- [ ] 2D `chrono_constraint2d_batch_solve` と新共通ソルバでビット単位一致テストを実施した。
- [ ] Math ヘルパ移行で 2D テスト（`make test`）がすべてグリーンになることを確認した。
- [ ] 新規 3D API を追加する際、既存 C ヘッダに `#ifdef CHRONO_ENABLE_3D` ガードを導入した。
- [ ] ログ基盤のレベル切替（WARN/INFO）が 2D/3D 双方で同じ enum 値を使うことを確認した。
- [ ] 3D テスト導入後、CI マトリクス（smoke/extended/nightly）でタイムアウトしないか測定した。
