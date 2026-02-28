# chrono-2d CI 設計案（ドラフト・実装前の雛形）

目的: Makefile ベースの chrono-2d を最小コストで CI 実行する提案書。現時点で YAML を変更せず、ジョブ構成と手順を整理する。

## 1. ジョブ概要（最小フロー）
- **build-and-test (chrono-2d / OpenMP 前提)**
  - checkout（サブモジュール含む）
  - 依存: `sudo apt-get update && sudo apt-get install -y build-essential python3 python3-pip`
  - `make clean && make tests`（chrono-2d 用 Makefile）
  - `make test` を実行し、結果をログ化
  - artifacts: `test.log` の tail（例: 400 行）と生成 CSV のみ
- **CSV 整合性チェック**
  - 目的: CI で生成した CSV の列数/必須カラムを検証
  - 手順: python ワンライナーまたは簡易スクリプトで `cols==<expected>` と必須カラム存在を確認し、欠落時は fail
  - artifacts: `artifacts/ci/csv_schema_check.txt`
- **週次スケジュール（軽量）**
  - cron 週 1 回で `make test` のみ実行し、ログ tail と CSV をアップロード
  - 実行時間見積り: 3–8 分（ビルド時間 + テスト件数に依存）

## 2. YAML 雛形（適用時に .github/workflows/ へ追加）
```yaml
name: chrono-2d-ci (draft)
on:
  workflow_dispatch:
  schedule:
    - cron: "0 1 * * 0"   # 週1回

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: true
      - name: Install deps (OpenMP only)
        run: sudo apt-get update && sudo apt-get install -y build-essential python3 python3-pip
      - name: Build tests
        run: |
          make clean
          make tests
      - name: Run tests
        id: run_tests
        continue-on-error: true
        run: |
          rm -f test.log
          set -o pipefail
          if make test > test.log 2>&1; then
            tail -n 120 test.log
            exit 0
          else
            tail -n 400 test.log
            exit 1
          fi
      - name: Check CSV schema
        run: |
          python - <<'PY'
          import csv, sys
          from pathlib import Path
          target = Path("data/chrono2d_results.csv")
          required = ["step","time","value"]
          cols = target.read_text().splitlines()[0].split(",")
          missing = [c for c in required if c not in cols]
          if missing:
            print(f"Missing columns: {missing}")
            sys.exit(1)
          print(f"cols={len(cols)} -> {cols}")
          PY
      - name: Upload artifacts (minimal)
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: chrono-2d-ci
          path: |
            test.log
            data/chrono2d_results.csv
            artifacts/ci/csv_schema_check.txt
      - name: Fail if tests failed
        if: steps.run_tests.outcome == 'failure'
        run: exit 1
```
※ 実際の CSV 名や Makefile ターゲットに合わせて調整する。

## 3. artifacts 最小化ポリシー（chrono-2d）
- アップロードするのは以下に限定:
  - `test.log` の tail（400 行程度）
  - 生成した CSV（1–2 個）
  - CSV スキーマチェック結果（テキスト数行）
- アップロードしない:
  - フルビルド成果物、巨大な中間ファイル、生成物重複
- 保存期間: デフォルト（90 日）のまま。容量が逼迫する場合は 30 日に短縮検討。

## 4. 失敗時のログ抽出・タグ付け
- `make test` 失敗時は tail で概要を記録（上記 YAML 参照）。
- 追加タグ付け案（任意）:
  - `python tools/filter_ci_failures.py test.log --tag-input --output test_chrono2d_tagged.log`
  - artifacts に `test_chrono2d_tagged.log` を含める

## 5. OpenMP のみの依存インストール
- `build-essential` のみで対応。oneTBB は扱わず、CMake/Makefile オプションも `-fopenmp` 前提。
- TBB を試す場合は opt-in で matrix に `tbb: [off, on]` を追加し、`tbb=on` かつ未導入なら skip。

## 6. Run ID 記録テンプレ（chrono-2d）
```
- Run: #<GITHUB_RUN_ID> (chrono-2d)
- Artifact: chrono-2d-ci-<run>
- CSV: data/chrono2d_results.csv (schema OK)
- Notes: κ/metrics: <簡易値>  # 任意で集計結果を付記
```

## 7. 週次ジョブ案（make test のみ）
- cron 週 1 回（例: 日曜 01:00 UTC）。
- ステップ: checkout → deps → make tests → make test → artifacts（log tail + CSV）。
- 実行時間見積: 3–8 分。

## 8. PR 分割プラン
1) **最小 build+test 追加**: OpenMP 前提でテスト実行と log/CSV アップロードのみ。
2) **CSV スキーマチェック追加**: 必須カラム・列数検証ステップをジョブに追加。
3) **週次ジョブ追加**: schedule を入れ、平常時の負荷を軽減。
（必要に応じて TBB 軸やタグ付け/フィルタを後続 PR に分割）

## 9. トラブルシュートチェックリスト（失敗時）
- 依存不足: `apt-get` のログに失敗がないか確認。CMake/Makefile が要求する追加パッケージを洗い出し。
- リンクエラー: `test.log` の冒頭に missing symbol/flags が無いか確認。`-fopenmp` が通っているか。
- CSV 欠落: 生成パスの有無、ファイル名が想定と一致しているか。スキーマチェックで列欠落なら必須カラムを追加。
- Disk/full: artifacts アップロードが失敗していないか確認。

## 10. README / ドキュメントへのリンク計画
- README に「chrono-2d CI (draft)」セクションを追加し、本ファイルへのリンクを貼る。
- `docs/abc_team_chat_handoff.md` に Run ID テンプレを追記（chrono-2d 用と明記）。
- CI 実装後は `.github/workflows/chrono_2d_ci.yml` を作成し、本ファイルの項目と同期させる。

> 本書は提案ベースです。CI 要件が固まった時点で、上記 YAML 雛形をもとに `.github/workflows/` へ新規追加してください。
