# CI / 運用自走化プラン（ドラフト）

目的: Chrono 系 CI を「誰の判断を挟まず」回せるよう、必須チェック、Artifacts 方針、Run ID 運用、スキップ条件、頻度、タイムアウト、PR 分割、トラブルシュート、チャット共有テンプレを定義する。YAML 実装前に合意し、レビューなしで適用できる状態を目指す。

## 1. 必須チェック（CI で fail させるもの）
- Build + Test（Makefile/CMake 問わず）: ビルド失敗・テスト失敗で fail。
- CSV スキーマ: 列数と必須カラム欠落で fail（chrono-2d は time, case, method, vn, vt, mu_s, mu_d, stick, condition_bound, condition_spectral, min_pivot, max_pivot）。
- 値域: mu_s/mu_d∈[0,2], stick∈{0,1}, condition*/pivot>0, time>=0 でなければ fail。
- Lint（doc/preset links）: リンク切れで fail。
- ABI チェック（該当ターゲットがある場合）: オフセット/サイズずれで fail。

## 2. Artifacts 方針（最小セット・除外ルール）
- **残すもの**: `test.log` tail（~400 行）、スキーマチェック結果（数行）、生成 CSV/JSON（1–2 個）、ABI diff（fail 時のみ）、descriptor CSV+pivot（必要最小限）。
- **除外するもの**: フルビルド成果物、巨大中間ファイル、重複ログ、コアダンプ。
- 保存期間: デフォルト 90 日で開始。容量逼迫時に 30 日へ短縮（chrono-2d CI は 30 日、bench は 14 日）。

## 3. Run ID の扱い
- 命名: `chrono-2d-ci-<run_id>` / `chrono-main-ci-<run_id>` のように系統を prefix。
- 記録先: `docs/abc_team_chat_handoff.md` の専用テンプレに追記（Run, Artifact, CSV/JSON, Notes）。
- 反映プロセス: CI 完走後に Run ID と artifacts 名をチャットテンプレ（下記）で共有し、必要ならドキュメントへ記録。

## 4. スキップ条件（コミットタグ）
- `[skip-ci]`: すべてのジョブをスキップ。
- `[skip-descriptor]`: descriptor-e2e をスキップ。
- `[skip-abi]`: ABI チェックをスキップ。
- `[skip-weekly]`: 週次 cron のみスキップ（dispatch は実行）。
→ YAML 側で `if: contains(github.event.head_commit.message, '[skip-XXX]')` を設定。

## 5. 頻度（cron/dispatch）
- dispatch: 手動随時（機能検証・PR 用）。
- cron: 週 1 回（例: 日曜 01:00 UTC）で build+test+スキーマチェックのみ。許容時間 10–20 分（初期）。

## 6. CSV スキーマチェック
- 実行場所: build-and-test ジョブの末尾。
- 失敗条件: 列数不一致または必須カラム欠落で fail。
- 出力: `artifacts/ci/csv_schema_check.txt` に結果を残す。

## 7. 依存インストール（最小化）
- OpenMP 前提: `sudo apt-get update && sudo apt-get install -y build-essential python3 python3-pip`.
- oneTBB は opt-in: matrix 軸を追加し、無い場合は skip でログ明示。
- Python パッケージは必要最小限（例: `pip install pyyaml` 程度）。

## 8. タイムアウト / リトライ / フレーク対策
- タイムアウト: ビルド 20 分、テスト 10 分を上限目安に設定（chrono-2d CI に適用）。
- リトライ: フレークテストは 1 回リトライ許可（実験版で運用）。ログに `retry=1` を記録。
- フレーク検知: 同一テスト名の失敗→成功をログでマーク（将来オプション）。

## 9. PR 分割方針
1) 最小 build+test（OpenMP）＋ CSV アップロード + ログ tail。
2) CSV スキーマチェック追加（fail 条件化）。
3) 週次 cron ジョブ追加（軽量テストのみ）。
4) ABI/descriptor/bench (opt-in) を追加。
5) Artifacts 最適化・タグ付け・Run ID テンプレ整備を仕上げ。

## 10. Artifacts サイズ・整形・環境情報
- ログ整形: `tail -n 400 test.log` をアップロード。必要なら `filter_ci_failures.py` でタグ付け版を追加。
- 環境情報: `uname -a`, `gcc --version`, `python --version` を env ファイルに記録（数行）。
- サイズ上限: 1 ジョブあたり 10–20 MB を目標。超過しそうなら pivot/JSON を圧縮。

## 11. 失敗時トラブルシュート（チェックリスト）
- 依存不足: apt の失敗を確認。必要パッケージを追加。
- リンクエラー: `-fopenmp` 指定漏れやライブラリ未解決を test.log で確認。
- CSV 欠落: 生成パス/ファイル名を確認。スキーマチェック結果を参照。
- ABI ずれ: `abi_diff.log` で offset/size を確認、ヘッダ更新が必要か判断。
- ディスク/Artifact 失敗: アップロード上限や権限エラーを確認。

## 12. チャット共有テンプレ
```
[CI Run Share]
- Run: #<GITHUB_RUN_ID> (chrono-2d/chrono-main)
- Artifact: <name> (e.g., chrono-2d-ci-<run>)
- CSV/JSON: <paths> (schema OK/NG)
- Logs: tail uploaded (test.log, tagged if any)
- Notes: <pass/fail>, <next action>
```
- chrono-2d 例: `Run #<id> | Artifact: chrono-2d-ci-<id> | CSV schema OK (kkt_descriptor_actions_local.csv) | Logs: test.log tail | Env: env.txt`

## 13. 導入手順書（適用手順の骨子）
- 本ファイルを参照し、YAML 雛形を `.github/workflows/` に新規追加。
- スキップタグと cron 設定を入れた上で PR 分割方針に従って段階導入。
- 反映後、Run ID をチャットテンプレで共有し、`docs/abc_team_chat_handoff.md` に追記する。

> 本プランは実装前の合意用です。確定後に YAML へ反映し、運用開始します。
