# CI 設計案（chrono-main向けドラフト・実装前の提案）

目的: chrono-main 向けに通知／アーカイブ無しの最小 CI を設計する。現状は chrono-C-all 用 CI のみ存在し、chrono-main 用ビルド・テスト構成は未確認。ここではジョブ構成・依存・スキップ条件・Artifacts 方針を矩形項目でまとめ、実装前のレビュー素材とする。

前提:
- 変更しない: `.github/workflows/*` は現時点で編集しない。
- ビルド系: chrono-main の CMake/Makefile は未調査。OpenMP fallback を前提とし、oneTBB はオプション扱い（あれば matrix で軸追加）。
- 外部通知: Slack / メール等は追加しない。Artifacts は最小限。
- サブモジュール: `third_party/chrono/chrono-main` をチェックアウトして利用。

## 提案ジョブ構成（最小セット）
1) build-and-test (chrono-main)
- 目的: chrono-main の最小ビルドと単体/回帰テストを実行。通知なし。
- 手順案:
  - actions/checkout（`submodules: true` で chrono-main 取得）
  - 依存インストール: `build-essential`, `cmake`, `python3` (必要なら `python3-pip`)
  - コンフィグ: `cmake -S third_party/chrono/chrono-main -B build -DCMAKE_BUILD_TYPE=Release -DUSE_OMP=ON -DUSE_TBB=OFF`
  - ビルド: `cmake --build build -j$(nproc)`
  - テスト: `ctest --test-dir build --output-on-failure`（OpenMP fallback 前提）
  - Artifacts: `build/Testing/Temporary/LastTest.log` と `build/CTestTestfile.cmake` のみ
- スキップ条件: `if: contains(github.event.head_commit.message, '[skip-chrono-main]')` 等で任意スキップ可能に。
- 実行時間目安: chrono-C-all CI と同等〜やや長め（ビルド依存で変動）。初回は 10–20 分程度を想定。

2) descriptor-e2e (chrono-main)
- 目的: chrono-main 側で descriptor-e2e を実行し、Run ID 付き CSV/pivot を artifacts 化。
- 手順案:
  - 依存: build-and-test に依存（`needs`）。
  - 実行: `tests/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions --descriptor-log artifacts/descriptor/kkt_descriptor_actions_${{ github.run_id }}.csv --pivot-artifact-dir artifacts/descriptor/run-${{ github.run_id }}`
  - Artifacts: `kkt_descriptor_actions_<run>.csv` と `run-<run>/pivot_*.csv` のみ。
  - 追加チェック: 列数/必須カラムを python ワンライナーで検証し、欠落時は fail。
  - スキップ条件: `if: contains(github.event.head_commit.message, '[skip-descriptor]')` など。

3) ABI チェック (chrono-main)
- 目的: `tests/test_constraint_common_abi` 相当を chrono-main で実行し、ABI ずれを早期検知。
- 手順案:
  - ビルド後に `tests/test_constraint_common_abi` を実行。
  - 失敗時ログ: フィールド offset/size 差分を `artifacts/abi/abi_diff.log` に出力。
  - Artifacts: `abi_diff.log` のみ。

4) 簡易 Lint（preset/doc links）
- 目的: chrono-main 用の最低限 Lint を追加。
- 手順案:
  - `python scripts/check_preset_links.py` 相当（対象: README / Hands-on / preset YAML を想定）。
  - `python scripts/check_doc_links.py <対象md>` の chrono-main 対応版を用意（存在しない場合はテンプレだけ作成）。
  - 失敗時はジョブ fail。Artifacts は `artifacts/ci/doc_link_report.md` 程度。

5) endurance 関連（閾値緩和・OpenMP/TBB 差分）
- 目的: coupled/prismatic/planar 系の耐久テストを chrono-main で実行するための閾値設計。
- 方針:
  - C 版より緩い閾値を初期設定（例: κmax 12.5, warn_ratio 100% でも fail させない）。
  - TBB 軸はオプション: matrix で `tbb: [off, on]`。`tbb=on` かつ oneTBB 不在なら自動 skip。
  - Artifacts: `data/coupled_constraint_endurance_chrono_main.csv`（新設・Run ID とメトリクスを追記）、`data/latest.endurance.chrono_main.json`。

6) CI ログ抽出 / タグ付け
- 目的: chrono-C-all の `filter_ci_failures.py` 相当を chrono-main ログに適用。
- 手順案:
  - テストログ末尾抽出（coupled/island タグ付け）を python スクリプトで実行。
  - Artifacts: `test_chrono_main.log`, `test_chrono_main_tagged.log` のみ。

7) 条件数/auto_drop 日次集計スクリプト
- 目的: CI の descriptor/endurance 生成物から κ 警告と auto_drop を集計し、Markdown に貼る。
- 手順案:
  - 入力: `data/coupled_constraint_endurance_chrono_main.csv` と descriptor CSV。
  - 出力: `artifacts/reports/chrono_main_daily.md`（簡易テーブル: date, run_id, kappa_max, warn_ratio, auto_drop 件数）。
  - CI では fail させず、Artifacts のみ。

8) weekly preset check（GitHub Actions schedule）
- 目的: preset/doc link チェックを週次で実行し、Artifacts を保存。
- 手順案:
  - `on: schedule` で cron 週1回。
  - 出力: `artifacts/weekly/preset_links.md`。
  - 通知なし。

9) bench_island_solver (TBB オプション)
- 目的: `bench_island_solver --scheduler tbb` を CI で実行するオプションを用意し、TBB 無しなら skip。
- 手順案:
  - matrix に `scheduler: [omp]` をデフォルト、`scheduler: [omp, tbb]` を opt-in。
  - TBB 無しの場合: `if scheduler == 'tbb' and !command -v tbb` で skip を明示ログ。
  - Artifacts: `bench_island_solver_${{ matrix.scheduler }}.log`。

10) descriptor CSV 整合性チェック
- 目的: descriptor-e2e が出力する CSV の列数/必須カラムを検証。
- 必須カラム例: `action`, `constraint_id`, `pivot_row`, `pivot_col`, `kappa`, `drop_flag`, `timestamp`.
- 欠落時は fail、整合性結果は `artifacts/descriptor/descriptor_schema_check.txt` に保存。

11) artifacts 最適化
- 保存期間: GitHub Actions デフォルト 90 日を前提に、容量軽減のためログを tail のみアップロード。
- 保存対象（最小）:
  - build/test: `test_chrono_main.log`(tail 400行), `*_tagged.log`, `doc_link_report.md`.
  - descriptor: CSV + pivot ディレクトリ。
  - endurance: CSV + JSON。
  - ABI: `abi_diff.log`（fail 時のみでも可）。
- 除外: フルビルド成果物や大容量中間ファイル。

12) Run ID 記録フロー（chrono-main 用の案）
- 目的: 週次で Run ID を `docs/abc_team_chat_handoff.md` などに記録するテンプレを準備。
- テンプレ案:
  ```
  - Run: #<GITHUB_RUN_ID> (chrono-main)
  - Artifact: coupled-endurance-chrono-main-<run>, descriptor-chrono-main-<run>
  - Logs: artifacts/reports/chrono_main_daily.md
  - Notes: κmax=<...>, warn_ratio=<...>, auto_drop=<...>
  ```
- 実装時に A/B/C チャットで共有し、C 版と混同しないように prefix を付ける。

## スキップ条件・実行時間の見積もり
- build-and-test: 10–20 分（初回見積り）。`[skip-chrono-main]` でスキップ可能にする案。
- descriptor-e2e: 3–5 分（CSV + pivot 出力）。
- ABI チェック: 1–2 分。
- Lint: 数十秒。
- endurance mini: 5–8 分（閾値緩和で fail しない形）。TBB 軸追加時は +数分。
- bench_island_solver (opt-in TBB): 2–4 分（スキップ可）。
- 週次 preset: 数十秒。

## 追加で必要な情報
- chrono-main のビルド要件（CMake オプション、サンプルテストの実行方法）。
- descriptor-e2e に相当するテストターゲットの有無・コマンド。
- ABI チェック対象のバイナリ名/テスト実行方法。
- endurance 用の chrono-main 対応ベンチ/テスト（coupled/prismatic/planar）の有無。
- oneTBB を CI ホストで簡単に入れられるか（apt で十分か、ソース要か）。
- アーティファクト許容量（組織設定）とアップロード上限ポリシー。

## 実装時の分割案（PR 単位）
1. 最小 build-and-test + Lint（preset/doc）を chrono-main に導入（通知なし、Artifacts 最小）。
2. descriptor-e2e + CSV 整合性チェック + Run ID テンプレを追加。
3. ABI チェック + bench_island_solver(TBB optional) + endurance mini（緩和閾値）を追加。
4. 週次 preset チェック（scheduled）と日次集計 Markdown 生成を追加。
5. Artifacts 最適化とログ抽出/タグ付けの整理、ドキュメント整備。

> このファイルは提案ドラフトです。要件確定後に `.github/workflows/` へ反映し、実装差分を別 PR で管理してください。
