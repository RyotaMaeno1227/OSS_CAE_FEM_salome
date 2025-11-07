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

| 機能領域 | 2D API/構造体 | 3D 想定（案） | 移行メモ | 初期評価コメント | 想定工数 (人日) | リスクスコア (1–5) |
|----------|---------------|---------------|----------|------------------|----------------|--------------------|
| 基底構造 | `ChronoConstraint2DBase_C` | `ChronoConstraintCommon_C`（新規） + `typedef ChronoConstraintCommon_C ChronoConstraint3DBase_C;` | 2D ヘッダは typedef 維持。 | 構造体サイズを据え置ける見込み。ABI テスト必須。 | 12 | 2 |
| Ops テーブル | `ChronoConstraint2DOps_C` | `ChronoConstraintCommonOps_C`（同型） | フィールド名・関数ポインタは同一。 | 追加作業ほぼ不要。既存コードをそのまま流用可能。 | 4 | 1 |
| バッチソルバ | `chrono_constraint2d_batch_solve` | `chrono_constraint_batch_solve`（共通実装）、2D/3D ラッパ | ワークスペース API も共通化。 | ループ構造は再利用可。Jacobian 尺度差で精度回帰テストが必要。 | 20 | 4 |
| ワークスペース | `ChronoConstraint2DBatchWorkspace_C` | `ChronoConstraintBatchWorkspace_C` | メモリ確保関数を関数ポインタ化。 | バッファ種別が増える想定。再割当テストを追加したい。 | 10 | 3 |
| ボディ API | `ChronoBody2D_C` | `ChronoBodyCommon_C` + `ChronoBody3D_C` | 2D ラッパで既存フィールドレイアウト保持。 | クォータニオン管理が未整備。math 層の先行実装が前提。 | 18 | 4 |
| 距離拘束 | `chrono_distance_constraint2d_*` | `chrono_distance_constraint3d_*` ラッパ → 共通実装 | 3D 版は 3 軸 Jacobian を追加。 | 係数行列が 1×3→1×6 に拡張。専用ユニットテストが必要。 | 15 | 3 |
| 回転拘束 | `chrono_revolute_constraint2d_*` | `chrono_revolute_constraint3d_*` | Ops 呼び出しの差分は姿勢計算のみ。 | 角度表現をクォータニオンへ置換。制御ゲインの再検証が必要。 | 14 | 4 |
| Coupled 拘束 | `chrono_coupled_constraint2d_*` | `chrono_coupled_constraint3d_*` or 汎用化 | 行列サイズが 4×4 まで拡張。 | 方程式数が増えると条件数が悪化しやすい。ベンチを先行実装。 | 22 | 5 |
| 診断 | `ChronoCoupledConstraint2DDiagnostics_C` | `ChronoConstraintDiagnostics_C`（共通） | pivot 情報配列サイズをパラメータ化。 | 3D 用に pivot ベクトル長を可変化する必要あり。 | 8 | 3 |
| ログ基盤 | `chrono_log_warn`（予定） | 同一 API | ログレベル enum を共通ヘッダへ移行。 | API 追加コスト小。CI で WARN 粒度を共通化する。 | 6 | 2 |

### 8.1 Visual Progress Templates

To surface progress in status reports, embed either Markdown-style progress bars or a lightweight Gantt table.

**Progress bar example (GitHub-flavored Markdown)**
```markdown
| Component | Progress |
|-----------|----------|
| Common Constraint Base | ![70%](https://progress-bar.dev/70/?title=70%25) |
| Batch Solver | ![35%](https://progress-bar.dev/35/?title=35%25) |
| Coupled 3D Extension | ![10%](https://progress-bar.dev/10/?title=10%25) |
```
- The `progress-bar.dev` badge renders in GitHub issues/PRs. For offline docs, replace with inline SVG hosted under `docs/media/badges/`.

**ASCII Gantt snippet (monthly granularity)**
```markdown
| Component              | 2025-10 | 2025-11 | 2025-12 | 2026-01 |
|------------------------|---------|---------|---------|---------|
| Common Constraint Base | ████▌   | ██████  |         |         |
| Batch Solver           | ██      | ███     | ████    | █████   |
| Coupled 3D Extension   |         |         | ██      | ███     |
```
- Use full block `█` for planned work, `▌` for partially completed weeks, and leave cells blank for idle months.
- When embedding in Confluence, apply a monospaced font to preserve alignment (`{code}` macro).

Both templates should reference the same KPI values (工数 / リスク). Update them during your monthly review alongside the table above.

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

### 10.1 月次進捗レポートテンプレ

```markdown
## 3D Constraint Migration Status (YYYY-MM)

| 達成項目 | ステータス | 進捗バー | 想定工数 (人日) | 消化工数 (人日) | リスクスコア | 今月の進捗 | 次月アクション | メモ |
|-----------|------------|----------|-----------------|------------------|--------------|------------|-----------------|------|
| 拘束ディスクリプタ統合 | 設計草稿済み | █████░░░░░ (45%) | 15 | 7 | 3 | KKT ブロック PoC を共通 API へラップ済み | バッチソルバと統合し E2E テスト | 共通 API ラッパ |
| 接触ヤコビアン拡張 | 要件定義中 | ███░░░░░░ (25%) | 18 | 4.5 | 4 | Rolling/Torsional 係数の仕様整理完了 | 3 点マニフォールド試験実装 | Math/Geometry ヘルパ |
| 並列バッチスケジューラ | 初期実装中 | ████░░░░░ (35%) | 20 | 7 | 3 | OpenMP island ランナーに計測フック追加 | TBB ベースのタスク実験 | Island ワークスペース ABI 固定 |
| Coupled 診断整備 | 設計着手 | ██▌░░░░░░ (28%) | 12 | 3.5 | 2 | 2D/3D 共通ロガーの設計メモ共有 | Coupled 3D サンプルで検証 | Solver 移植タスク |

### ブロッカー
- [ ] Math ヘルパ 4×4 実装レビュー待ち（担当: Kobayashi、期限: 11/15）
- [ ] ログ API 仕様確定待ち（担当: Suzuki、期限: 11/08）

### 共有事項
- Coupled 3D ベンチ仕様案は 11/01 技術定例でレビュー予定（担当: Mori）。
- 進捗テンプレートは `docs/coupled_island_migration_plan.md` のセクション 6 と連携する。
```

上記テンプレートは `docs/chrono_3d_progress_template.md` として共有予定。KPI はテーブルの数値と一致するよう週次で見直してください。

### 10.2 簡易 ASCII ガント（四半期スナップショット）

```
2025Q4 | 拘束ディスクリプタ統合 : ████▒▒▒▒▒▒ (PoC)        |====>|
       | 接触ヤコビアン拡張     : ██▒▒▒▒▒▒▒▒ (要件)       |===> |
       | 並列バッチスケジューラ : ███▒▒▒▒▒▒▒ (計測)       |====>|
2026Q1 | 拘束ディスクリプタ統合 : ███████▒▒▒ (実装)       |====>|
       | 接触ヤコビアン拡張     : ████▒▒▒▒▒▒ (試験)       |====>|
2026Q2 | Coupled 診断整備       : ███▒▒▒▒▒▒▒ (共通化)     |===> |
```

- `█` は確定済みの進捗、`▒` は予定中、`=` はレビュー期間、`>` は移行完了を示すマーカーとして使用します。
- 表に合わせて四半期単位で更新し、PJ レビューではガントと KPI テーブルをセットで提示すると議論が進めやすくなります。

### 10.3 月次サマリ提出メモ

- レポート送付時は Slack `#chrono-3d-migration` に KPI テーブル（拘束・接触・並列のみ）と ASCII ガントを貼り付ける。
- 想定工数と消化工数の差分が 30% を超えた項目は、翌月アクション欄に数値的リスク緩和策（例: 追加計測、ソルバ切替評価）を記入する。
- リスクスコアは 1（低）～5（高）で統一し、変更があれば根拠（条件数悪化、性能退行など）をコメント欄へ明記する。

## 11. 技術移行ステータス表（ドラフト）

| コンポーネント | ステータス | 想定期間 | 主な依存関係 | 担当候補 | コメント |
|----------------|------------|----------|--------------|----------|-----------|
| 共通拘束ベース（`ChronoConstraintCommon_C`） | 設計草稿済み | 2026-Q1 | ABI テスト、ヘッダ整備 | Cチーム コア（Sato）、アーキ WG | typedef 化のみで移行可能。ABI チェックリストを作成中。 |
| バッチソルバ共通化 | 要件定義中 | 2026-Q2 | 共通ベース導入、math ヘルパ 3×3 | 物理チーム（Ito）、Cチーム 並列班 | OpenMP スケジューラ検証が必要。性能ベンチに依存。 |
| Math/Geometry ヘルパ | プロトタイプあり | 2025-Q4 | ベクトル/行列ユーティリティ、単体テスト | 数値解析班（Kobayashi） | 2×2/3×3 汎用関数はレビュー済み。4×4 とクォータニオン補助が未着手。 |
| ボディ共通層（`ChronoBodyCommon_C`） | 構想段階 | 2026-Q2 | Math ヘルパ、構造体 ABI | ランタイム班（Yamada） | 慣性テンソル管理がボトルネック。2D 側の既存 API 影響を調査中。 |
| 3D 拘束ラッパ（距離・レボリュート） | 調査中 | 2026-Q3 | 共通ベース、ボディ共通層 | Cチーム ジョイント班（Tanaka） | 既存 C++ 実装との対応表作成が先行タスク。 |
| Coupled 拡張（3D 対応） | 未着手 | 2026-Q4 | 共通ベース完了、数学ヘルパ拡張 | Cチーム Coupled 班（Mori） | 条件数解析とベンチ拡張が依存。2D 耐久ベースラインを流用予定。 |
| 診断／ログ共通化 | 要件定義中 | 2025-Q4 | ログ基盤（`chrono_log_*`）、CI パイプライン | DevOps（Suzuki）、Cチーム ログ担当 | WARN/INFO 切替 API が 2D 側に先行投入済み。3D 追加の形式検討中。 |
| 文書／サンプル整備 | 進行中 | 継続（四半期レビュー） | すべて（特に API 更新） | ドキュメント班（Nakajima） | 3D 構成案のドラフトを wiki に同期予定。 |
