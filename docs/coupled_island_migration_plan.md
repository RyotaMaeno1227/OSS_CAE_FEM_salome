# Coupled & Island Solver Migration Plan

Chrono-C の Coupled 拘束と島ソルバを Chrono C++ (`chrono-main`) と同等の数値基盤へ合わせ込むための差分整理と移植計画をまとめる。

## 1. 現行 Chrono-C (2D) の実装メモ
- **Coupled 拘束**はローカル 4x4（最大）行列を構築し、部分ピボット付きガウス消去で逆行列を得る実装。Pivot 選択と条件数評価は `coupled_constraint_invert_matrix` / `coupled_constraint_condition_bound` で行っている (`chrono-C-all/src/chrono_constraint2d.c:258`, `chrono-C-all/src/chrono_constraint2d.c:351`)。
- 条件数が閾値を超えた場合にはワーニングを立て、最小対角ブロックをドロップする自動回復を備える (`chrono-C-all/src/chrono_constraint2d.c:1546`-`chrono-C-all/src/chrono_constraint2d.c:1604`)。
- 島ソルバは Union-Find でボディ連結成分を抽出し、制約／接触ポインタ配列をワークスペースに集約する単純設計 (`chrono-C-all/src/chrono_island2d.c:48`-`chrono-C-all/src/chrono_island2d.c:135`)。並列化は OpenMP による島単位のループ分割 (`chrono-C-all/src/chrono_island2d.c:390` 近辺)。

## 2. chrono-main の該当機能
- Chrono-main は **システムディスクリプタ** (`ChSystemDescriptor`) を通じて拘束と変数を KKT 形式に組み上げ、グローバルスパース行列を構築 (`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:42`-`third_party/chrono/chrono-main/src/chrono/solver/ChSystemDescriptor.cpp:175`)。
- 拘束は `ChConstraintTwoTuples*` 系クラスで抽象化され、ロール／スピン摩擦まで含む多自由度拘束を提供 (`third_party/chrono/chrono-main/src/chrono/solver/ChConstraintTwoTuplesRollingN.h:37`-`third_party/chrono/chrono-main/src/chrono/solver/ChConstraintTwoTuplesRollingN.h:90`)。
- 反復解法は `ChIterativeSolverVI` ファミリで Warm-start、オーバーリラクゼーション、違反履歴などを扱う (`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:24`-`third_party/chrono/chrono-main/src/chrono/solver/ChIterativeSolverVI.cpp:62`)。SOR/PMINRES/ADMM など複数ソルバを `ChSolver::Type` で切替可能 (`third_party/chrono/chrono-main/src/chrono/physics/ChSystem.cpp:413`-`third_party/chrono/chrono-main/src/chrono/physics/ChSystem.cpp:464`)。
- 接触管理は法線・接線・ローリング成分をクラスごとに保持し、レポート/アセンブリ時にまとめる (`third_party/chrono/chrono-main/src/chrono/physics/ChContactContainerNSC.cpp:748`-`third_party/chrono/chrono-main/src/chrono/physics/ChContactContainerNSC.cpp:776`)。

## 3. 差分サマリ
| 項目 | Chrono-C (現状) | chrono-main | ギャップ |
|------|----------------|-------------|----------|
| 連立解法 | 拘束ごとに 4x4 逆行列 (直接解法) | グローバル KKT を反復解法 | グローバルスパース構築が未実装 |
| Pivot/条件数 | 固定ピボット閾値＋式ドロップ (`chrono_constraint2d.c:1546`) | ソルバが自動で LCP/VI を処理 | Pivot 選択を柔軟化（列交換＋再スケール）が必要 |
| 接触拘束 | 2 点マニフォールド・Coulomb のみ (`chrono_collision2d.h:29`-`chrono_collision2d.h:152`) | 転がり/スピンまで多次元 (`ChConstraintTwoTuplesRollingN`) | 追加自由度と Jacobian 拡張 |
| 島構築 | Union-Find + OpenMP (`chrono_island2d.c:48`-`chrono_island2d.c:395`) | システムディスクリプタが自動再構成 | マルチ DOF 拘束の分配 & 3D 互換 API |

## 4. 移植ロードマップ
1. **共通ディスクリプタ層の導入**  
   - `ChronoConstraint2DBase_C` を KKT ブロックへ射影する構造体を追加し、`ChSystemDescriptor` 相当の集約器を C API で新設。  
   - 2D では従来の 4x4 と並行運用し、`CHRONO_ENABLE_GLOBAL_SOLVER` フラグで切替。
2. **反復ソルバの写経**  
   - `ChIterativeSolverVI` のオメガ/シャープネス制御と収束基準を C 実装へ移植。  
   - カラム再スケールと収束履歴を `ChronoCoupledConstraint2DDiagnostics_C` に拡張。
3. **接触自由度拡張**  
   - Manifold のデータ構造に回転摩擦を追加し、Rolling/Torsional 係数を扱う (`ChConstraintTwoTuplesRollingN` 互換)。  
   - Coupled 拘束との相互作用をテスト (`tests/test_island_contact_constraint.c` を再適用)。
4. **島ワークスペースの 3D 互換化**  
   - `ChronoIsland2DWorkspace_C` をベクトル長指定型へリファクタ、OpenMP セクションをタスクグラフ化。  
   - 3D 導入時は既存ユニットを共通メモリアロケータ経由で再利用。

## 5. 進行管理
- `docs/chrono_3d_abstraction_note.md` の KPI / ガントは拘束・接触・並列化タスクのみを扱うよう更新済み（同ファイル参照）。
- 次アクション: ディスクリプタ試作ブランチで 2D 版 `ChSolverPSOR` を動作させ、条件数とイテレーション数を比較する。

### 5.1 KPI スナップショット（2025-11 更新）
| イニシアチブ | 進捗 | 残タスク | 備考 |
|--------------|------|----------|------|
| Coupled 拘束移行 | 85 % | KKT 共有化の仕上げ、3D ダイアグノスティクス研磨 | Run `local-20251115` でディスクリプタ PoC を再検証。Δκ_s 監視は `docs/reports/kkt_spectral_weekly.*` を参照。 |
| Island ソルバ統合 | 75 % | Contact 併用テスト、OpenMP 最適化 | `bench_island_solver --scheduler tbb` fallback 測定と `data/diagnostics/bench_island_scheduler.csv` 更新を完了。 |
| 3D 抽象化 | 52 % | 共通構造体導入、KKT アダプタ検証 | `docs/chrono_3d_abstraction_note.md` のタスクリストを `sample_diag.json` / Jacobian ログと突き合わせ中。 |

> PoC の結果や KPI 変動が出たらこの表を最優先で更新する。Slack 共有時も同じ指標を引用し、`docs/chrono_3d_abstraction_note.md` のガントと整合させる。

このスナップショットは `docs/pm_status_2024-11-08.md` の KPI 表と共通であり、旧 Appendix B.5.1 の枠を廃止した現行プロセスでは同じ週次レビューで両方を必ず更新する（2025-11-10 更新: Mori）。

## 6. 進捗テンプレート & ガント

### 6.1 バックログトラッカー（週次更新想定）

| タスク | 現状 | 今週の作業 | 次週アクション | Owner | 依存関係 | ステータス | 最終更新 | Evidence (Run ID / Artifact) |
|--------|------|------------|----------------|-------|----------|-----------|------------|----------|
| KKT ディスクリプタ層 PoC (Chrono C) | E2E 検証済み（Run ID: 19582037625） | 共通 API ラッパ経由で `chrono_constraint2d_batch_solve` と並走させ、`docs/logs/kkt_descriptor_poc_e2e.md` に整合ログを保存。CI の `descriptor-e2e` ジョブが `--descriptor-mode actions` を常時実行して pivot CSV (`run-19582037625/`) を残す | chrono-main 側の KKT ログと週次比較し、差分監視を shared diagnostics へ移植。`tools/update_descriptor_run_id.py --run-id 19582037625` を新規 Run で必ず実行 | Cチーム（Mori） | `chrono_constraint2d_batch_solve` | On Track | 2025-11-08 | `docs/logs/kkt_descriptor_poc_e2e.md` |
| KKT ディスクリプタ層 PoC (chrono-main) | E2E 検証済み（Run ID: 19595392293） | chrono-main ビルドで `test_coupled_constraint --descriptor-mode actions` を実行し、`docs/logs/kkt_descriptor_poc_e2e_chrono_main.md` に整合ログを保存。CI の `descriptor-e2e-chrono-main` ジョブが pivot CSV を残す（今回 Artifact は CSV のみ） | Chrono C 側のログと週次で差分比較し、`tools/compare_kkt_logs.py` の出力を `docs/reports/kkt_spectral_weekly_chrono_main.csv` に反映 | Cチーム（Mori） | chrono-main test binaries | On Track | 2025-11-17 | `docs/logs/kkt_descriptor_poc_e2e_chrono_main.md` |
| Iterative Solver 移植 | パラメータ写経済 | `ChIterativeSolverVI` の `omega/sharpness/tolerance` を `ChronoConstraint2DBatchConfig_C.iterative` へ移植し、`tools/update_multi_omega_assets.py` で `bench_coupled_constraint --omega` の結果と README/Hands-on を同時更新 | Coupled ベンチの Δκ 推移と violation history を shared diagnostics へ転送 | 数値解析班（Kobayashi） | ディスクリプタ PoC | In Progress | 2025-11-08 | `docs/reports/kkt_spectral_weekly.md` |
| 接触ヤコビアン 3DOF 化 | API 草稿済み | `chrono_contact2d_build_jacobian_3dof` を公開し、Rolling/Torsional 行を `tests/test_contact_jacobian_3dof` で検証 | Contact + Coupled 組み合わせベンチを `tests/test_coupled_contact_combo` へ拡張。`test_island_parallel_contacts --jacobian-report docs/coupled_contact_test_notes.md` でドキュメントを自動更新 | 物理チーム（Ito） | Math/Geometry ヘルパ | In Progress | 2025-11-08 | `docs/coupled_contact_test_notes.md` / `data/diagnostics/contact_jacobian_log.csv`（`tools/run_contact_jacobian_check.py`） |
| 島ワークスペース 3D 拡張 | メモリ設計完了 | `chrono_island2d_workspace_get_{constraint,contact}_vectors` を追加し、任意ベクトル長のワークスペースを確保できるようにした | OpenMP → TBB タスク実験。`TBB_INCLUDE_DIR=/opt/... TBB_LIBS=-ltbb make bench` の手順を `docs/a_team_handoff.md` / `docs/island_scheduler_poc.md` に反映 | 並列班（Tanaka） | KKT ディスクリプタ | Pending（oneTBB 実測待ち） | 2025-11-08 | `docs/island_scheduler_poc.md` |
| ダイアグノスティクス共通化 | WARN/INFO & Pivot 対応済み | `chrono_constraint_common.h` に共通 `ChronoConstraintDiagnostics_C` を追加し、pivot ログと WARN/INFO レベルを 2D/3D 共有化 | Coupled 3D プロトタイプで検証。`tests/test_constraint_common_abi` を CI に追加し、構造体変更を自動検知 | ログ担当（Suzuki） | Solver 移植 | In Progress | 2025-11-08 | `chrono-C-all/include/chrono_constraint_common.h` |

- KKT PoC のエビデンス: `docs/logs/kkt_descriptor_poc_e2e.md` にバッチソルバとの整合ログ、pivot 列、Δκ_s を記録済み。Slack 週次共有時はこのログへのリンクを貼る。 
- Chrono-C vs chrono-main の条件数差分は `tools/compare_kkt_logs.py` で自動集計し、`docs/reports/kkt_spectral_weekly.md` として毎週更新する。
- 2025-11-15: `python tools/compare_kkt_logs.py --csv-output docs/reports/kkt_spectral_weekly.csv --diag-json data/diagnostics/chrono_c_diagnostics_sample.json` を再実行し、Δκ̂ 最大 1.0e-03 / pivot span 2.5e-02〜2.5e+01 を `docs/reports/kkt_spectral_weekly.md` に反映した。multi-ω CSV/JSON と `data/diagnostics/kkt_backend_stats.json` も同じコミットで更新済み。
- `data/diagnostics/sample_diag.json` を `tools/compare_kkt_logs.py --diag-json data/diagnostics/sample_diag.json` で整形し、Diag テーブル（default / spectral_stress）の最小・最大 pivot、条件数をテンプレ化。
- `data/diagnostics/kkt_backend_stats.json` を bench run（2025-11-15T18:21Z）で再生成し、CI(`descriptor-e2e`) の値と一致することを `git diff` で確認（calls=51480, fallback=1320, hit_rate=92.65%）。
- TBB スケジューラは oneTBB 用の C++ shim を追加済み。CI サンドボックスでは `TBB_LIBS` を渡していないため OpenMP にフォールバックするが、`docs/island_scheduler_poc.md` と `data/diagnostics/bench_island_scheduler.csv` に fallback 測定と enable 手順をまとめた。
- 2025-11-15: `./chrono-C-all/tests/bench_island_solver 64 200 4 0.01 tbb` を実行し、ヘッダ不足による OpenMP fallback を確認。`data/diagnostics/bench_island_scheduler.csv` の `tbb_fallback` 行と `docs/island_scheduler_poc.md` の表を同日付の実測値（平均 0.071 ms/step）に更新した。
- GitHub Actions では `tests/test_coupled_constraint --descriptor-mode actions --pivot-artifact-dir artifacts/descriptor` を追加し、`kkt_descriptor_ci.csv` が `docs/logs/kkt_descriptor_poc_e2e.md` の表と 1:1 で突き合わせられるようになった。失敗時は pivot 履歴が同一ディレクトリに残る。
- `tools/compare_kkt_logs.py` は `data/diagnostics/bench_coupled_constraint_multi.csv` と `data/diagnostics/archive_failure_rate_summary.json` を取り込み、Multi-ω 指標と耐久失敗率を週次レポートに併載するテンプレになった。
- Rank 欠落時（`ChronoConstraintDiagnostics_C.rank=0` や `diagnostics.rank` 未記録） は `data/diagnostics/kkt_backend_stats.json` と `docs/logs/kkt_descriptor_poc_e2e.md` の Run ID を突き合わせたうえで `tools/compare_kkt_logs.py --output` の Δκ テーブルに注記する。さらに `chrono_constraint2d_batch_solve` の WARN ログを `docs/logs/` に保存し、`docs/a_team_handoff.md` §5 のエスカレーション手順に従って PM へ共有する。
- oneTBB backend は `chrono_island2d_tbb.cpp` で `tbb::parallel_for` を呼び出す実装が追加済み。ライブラリが無い環境では自動で OpenMP fallback し、`data/diagnostics/bench_island_scheduler.csv` の `tbb_fallback` 行に測定値を残す。実環境でハードウェア TBB を使う場合は `make TBB_LIBS=-ltbb` などでリンクを有効にする。
- `tests/test_island_parallel_contacts` に 3DOF Jacobian 照合を統合し、`ChronoContactJacobian3DOF_C` の Rolling/Torsional 行が島回帰に含まれるようになった。`docs/coupled_contact_test_notes.md` のチェックリストも Jacobian 判定付きに更新済み。
- `python3 tools/update_multi_omega_assets.py --refresh-report` で `bench_coupled_constraint` の結果と README／Hands-on／`data/coupled_constraint_presets.yaml`／`data/diagnostics/kkt_backend_stats.json` を一括更新できるようになった。
- 「最終更新」列は週次レビューで記録し、Evidence 列から一次資料（ログ／Markdown／CSV）へ飛べるようにする。
- 残りの 20–25% は以下のサブタスクに分解して追う:
  1. KKT descriptor backend の E2E フェーズ（`--use-kkt-descriptor` CI での log diff、ログアーカイブの自動比較）。
  2. 実 TBB 実装（oneTBB のビルド／リンク、CI matrix 追加、`chrono_island2d_solve` の backend 選択）。
  3. Coupled+Contact 3DOF の island 統合テスト（`test_island_parallel_contacts` + Jacobian ログ比較）。

### 6.2 ASCII ガント（四半期スナップショット）
```
2025Q4 | KKT ディスクリプタ PoC    : ████▒▒▒▒▒▒ (PoC)        |====>|
       | Iterative Solver 移植      : ██▒▒▒▒▒▒▒▒ (要件)       |===> |
       | 接触ヤコビアン拡張        : ██▒▒▒▒▒▒▒▒ (設計)       |===> |
2026Q1 | KKT ディスクリプタ PoC    : ███████▒▒▒ (実装)       |====>|
       | Iterative Solver 移植      : ████▒▒▒▒▒▒ (試験)       |====>|
       | 島ワークスペース拡張      : ███▒▒▒▒▒▒▒ (試験)       |====>|
2026Q2 | Coupled 診断共通化        : ███▒▒▒▒▒▒▒ (共通化)     |===> |
       | 接触ヤコビアン拡張        : ████▒▒▒▒▒▒ (実装)       |====>|
```

### 6.3 更新メモ
- 週次レビューでは 6.1 のテーブルを埋めるだけで進捗共有できるようにする。  
- ガント図は四半期の節目に見直し、`docs/chrono_3d_abstraction_note.md` の KPI と整合させる。  
- 進捗報告テンプレートのヘッダ例:
  ```
  - 状態: [On Track / At Risk / Blocked]
  - 今週の成果:
  - 次週の予定:
  - リスク / ブロッカー:
  ```
