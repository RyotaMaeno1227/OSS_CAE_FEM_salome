# Coupled Endurance CI トラブルシュート

Coupled 拘束の耐久スイートは `.github/workflows/coupled_endurance.yml` で週次実行され、`tools/plot_coupled_constraint_endurance.py` の `--fail-on-*` オプションを使って異常を検出します。このドキュメントでは、CI で失敗が起きた際のログ確認方法とローカルでの再現・調査手順をまとめます。

---

## 1. 失敗検知とステップ概要
- **Archive endurance CSV logs**  
  `tools/archive_coupled_constraint_endurance.py` を実行。`ARCHIVE_MAX_*` 環境変数で設定された閾値（件数・期間・ファイルサイズ）を満たさない場合はエラー終了します。
- **Generate endurance summaries**  
  `tools/plot_coupled_constraint_endurance.py` を実行し、プロット・Markdown/HTML/JSON サマリを出力します。`--fail-on-*` しきい値を超えると終了コードが 3（max condition）、4（warning ratio）、5（rank ratio）になります。
- **Upload endurance artifacts**  
  `latest.csv` と `latest.summary.*`、`manifest.json` などをアーティファクト化します。閾値違反時もアップロードは行われるため、調査に活用できます。

---

## 2. CI ログの読み方
1. GitHub Actions の該当ワークフローを開き、`archive-and-summarize` ジョブを選択します。
2. ステップ「Generate endurance summaries」が赤く表示されている場合、ログ末尾に `Max condition number ... exceeds threshold`, `Warning ratio ... exceeds threshold`, `Rank-deficient ratio ... exceeds threshold` のいずれかが出力されています。メッセージ内に期待値と実測値が含まれるので、超過量の目安にしてください。
3. ステップ「Archive endurance CSV logs」で失敗した場合は、サイズ上限や保持期間に関するエラーメッセージが表示されます（例: `exceeds the limit of ... MiB`）。当該閾値は `ARCHIVE_MAX_*` 環境変数で上書きしているケースが多いため、ログ上部の `env:` セクションも確認すると便利です。
4. アーティファクト `coupled-endurance-<run-id>` をダウンロードすると `latest.summary.json` や `manifest.json` を参照できます。JSON サマリは `_validate_summary_schema` で検証済みなので、キーが欠けている場合は CI が既に失敗しているはずです。

---

## 3. ローカル再現手順
1. 必要に応じて CI のアーティファクトから `latest.csv` を取得し、`data/endurance_archive/latest.csv` として保存します。実機データをそのまま使用する場合はこのステップを省略できます。
2. しきい値違反を再現するには、CI と同じコマンドラインを使います。例:

   ```bash
   python tools/plot_coupled_constraint_endurance.py \
     data/endurance_archive/latest.csv \
     --skip-plot \
     --summary-json out/summary.json \
     --fail-on-max-condition 1.0e8 \
     --fail-on-warning-ratio 0.05 \
     --fail-on-rank-ratio 0.01 \
     --no-show
   ```

   - しきい値は CI のログに出ている値をそのまま使うか、`.github/workflows/coupled_endurance.yml` 内の `ARCHIVE_MAX_*` と合わせて確認してください。
   - `--summary-json` で生成されたファイルは `_validate_summary_schema` による検証を通過済みです。フォーマット違反がある場合はコマンドが終了コード 2 で即終了します。
3. 可視化が必要な場合は `--skip-plot` を外し、`--output figure.png` を追加するとグラフを生成できます（ローカルでのみ推奨）。
4. GitHub CLI (`gh`) を利用できる環境であれば、`tools/fetch_endurance_artifact.py` を実行することで失敗したワークフローのアーティファクト取得と再現コマンド生成を自動化できます。Run ID が分からない場合は `--interactive` を付与すると `gh run list` の結果から選択できます。`--comment-file` で Markdown を保存し、`--post-comment --comment-target pr/<番号>` のように指定すれば `gh pr comment` で即時共有も可能です（コメントには再現コマンド、plan.csv/plan.md のローカルパス、summary version が自動で含まれます）。

   ```bash
   python tools/fetch_endurance_artifact.py 1234567890 \
     --output-dir tmp/endurance \
     --summary-out repro/latest.summary.json
   ```

   - `1234567890` には Actions の Run ID もしくは Run URL を指定します。
   - ダウンロード後、スクリプトが提示するコマンドを実行すると CI と同じ閾値で検証できます。

---

## 4. Plan CSV (`--plan-csv`) の読み方

`tools/archive_coupled_constraint_endurance.py` で `--plan-csv` と `--plan-markdown` を指定すると、実行（またはドライランで予定）された操作を `plan.csv` と `plan.md` に記録します。CI では両ファイルがアーティファクトとして添付され、Webhook 通知にも `plan.md` の抜粋が流れるため、削除や整理の判断材料になります。

素早く重複や自己相殺をチェックしたい場合は `python tools/lint_endurance_plan.py data/endurance_archive/plan.csv` を実行すると、同一ハッシュの多重登録や同一ターゲットに対する `archive`/`delete` の衝突を一覧できます。`--max-delete 4 --max-delete-max-age 2` のような閾値を指定しておけば、削除予定件数が異常に多いプランを即座に弾けます。

| 列 | 概要 | 判断のヒント |
|----|------|--------------|
| `action` | 実施（予定）した操作の種別。`archive`、`refresh-latest`、`write-summaries`、`delete` 等。 | `delete` が意図しないファイルに向いていないか確認。 |
| `target` | 操作対象パス。 | `latest.*` か履歴 CSV かを識別し、保護すべきファイルが含まれていないかをチェック。 |
| `detail` | 補足情報。生成ファイル名や `markdown/html/json` など。 | 新規作成されたアーカイブ名を把握可能。 |
| `hash` | 入力 CSV の SHA-256。 | 同一ハッシュが並ぶ場合は重複データとしてスキップされた記録。 |
| `reason` | 操作理由や状態。`planned`、`completed`、`dry-run`、`max-entries`、`max-age` など。 | 上限超過による削除 (`max-entries` / `max-age`) だけを抽出してレビューすると効率的。 |

ドライランでは `reason` が `planned` や `dry-run` になり、本番実行では `completed` に変わります。`max-entries` や `max-age` で削除されるケースが想定外であれば、閾値や保持ポリシーを見直してください。

---

## 5. 失敗原因別の対処フロー

| トリガー | 典型的な対処 | 備考 |
|---------|--------------|------|
| `--fail-on-max-condition` (exit code 3) | Coupled 式のスケーリング見直し、`softness_*` の調整、診断ポリシーで自動ドロップ閾値を下げる。 | サマリ JSON の `max_condition` と履歴をグラフで確認。 |
| `--fail-on-warning-ratio` (exit code 4) | 連続して閾値を超えていないかチェックし、`ratio_*` やターゲット更新の段階的適用を検討。 | `warn_ratio` が高止まりしている場合は式の組み合わせが悪化している可能性。 |
| `--fail-on-rank-ratio` (exit code 5) | 拘束セットの線形従属を疑い、補助式の削減やポリシーの自動ドロップ設定を強化。 | `eq*_impulse` が 0 付近なら不要な式の可能性。 |
| `--max-file-size-mb` | CSV が肥大化しているため、ログ期間の見直しやサンプリング間隔の調整、または環境変数で緩和。 | 圧縮せずに 25 MiB 以上になる場合は記録内容を精査。 |
| `--max-age-days` / `--max-entries` | アーカイブの整理が滞っている。古いデータを明示的に残したい場合は閾値を環境変数で上書き。 | `manifest.json` に残したいエントリは別途退避してから再実行。 |

---

## 6. 役に立つコマンド
- 最新ログのサマリを CLI で確認:  
  `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot --summary-json /tmp/summary.json --no-show`
- 最低限の列だけにトリミングしてアーティファクト容量を削減:  
  `python tools/filter_coupled_endurance_log.py data/endurance_archive/latest.csv --drop-step`
- Plan CSV の重複・削除重複を lint:  
  `python tools/lint_endurance_plan.py data/endurance_archive/plan.csv`
- Coupled ベンチの JSONL から条件数異常を抽出:  
  `python tools/extract_condition_anomalies.py logs/csv_issues.jsonl`
- 診断 CSV ログを Markdown レポート化:  
  `python tools/diagnostic_log_report.py data/coupled_constraint_endurance.csv --output out/diagnostics.md --anomaly-jsonl logs/csv_issues.jsonl`
- CI テストログから Coupled/Island 向け失敗のみ抜粋:  
  `python tools/filter_ci_failures.py test.log --output coupled_island_failures.log`
- 重複判定や世代管理を確認:  
  `python tools/archive_coupled_constraint_endurance.py --dry-run --prune-duplicates --max-entries 10 --max-age-days 120 --plan-csv /tmp/endurance_plan.csv`
- Webhook 通知のローカル確認:  
  `python tools/mock_webhook_server.py --port 9000`

CI での失敗を再現したら、原因調査の結果やパラメータ変更をこのドキュメント、または `docs/chrono_2d_development_plan.md` の CI セクションに追記して共有してください。

---

## 7. サマリ JSON の互換性ポリシー
- JSON には `version` キーが含まれており、現在のスキーマは `1` です。フィールド追加など互換な拡張を行う場合は既存ツールが理解できる初期値（例: `0.0` や空文字）を付与し、破壊的変更が必要な場合のみ `version` をインクリメントしてください。
- ツール側の `_validate_summary_schema` は、自身が対応していない新しい `version` を検出すると終了コード 2 で停止します。CI に反映する際は、バージョンを上げたスクリプトとワークフロー設定を一度に更新すること。
- バージョン更新時はこのセクションに変更点と後方互換ガイドを追記し、`COUPLED_SUMMARY_VERSION` の値と合わせて管理します。

---

## 8. Webhook 通知サンプルと対応フロー

Webhook を有効化すると、Slack 等には次のようなメッセージが届きます。

```
[Coupled Endurance] FAILURE - Run 1234567890
- Samples: 7200 / Max condition: 5.43e+08
- Warning ratio: 100.00% / Rank ratio: 0.00%
Run details ▶ https://github.com/acme/highperformanceFEM/actions/runs/1234567890

Plan overview
| Action | Target | Detail | Hash | Reason |
| archive | ... | ... | ... | completed |
```

推奨される一次対応フローは以下の通りです。

1. 通知に貼り付けられた `plan.md` で削除予定のファイルが想定通りか確認する。閾値超過 (`max-entries`/`max-age`) が妥当か判断。
2. `tools/fetch_endurance_artifact.py ${{run_id}} --output-dir tmp/endurance --comment-file tmp/comment.md` を実行し、再現コマンドとコメントテンプレートを取得。
3. 生成されたコメントを Pull Request や Issue に投稿（`--post-comment --comment-target pr/<番号>`）。通知を避けたい場合は `--console-comment-only` を付けてコンソール出力のみにする。
4. 必要に応じて `plan.csv`/`plan.md` を用いて保持ポリシーを調整し、再実行または閾値変更を提案する。

ローカルで通知内容を検証したい場合は `tools/mock_webhook_server.py` を起動し、Webhook 先を `http://127.0.0.1:9000` などに向けると受信 payload が `mock_webhook_logs/` に保存されます。

---

## 9. 最頻発失敗ケースまとめ

| 失敗パターン | 想定原因 | 対応手順 | 再発防止策 |
|--------------|----------|----------|------------|
| `--fail-on-max-condition` 超過 | Coupled 式が不安定（ソフトネス不足）やターゲット急変 | `tools/fetch_endurance_artifact.py` で再現 → `plan.md` を見て該当 CSV を確認 → ソフトネス/スプリング調整を検討<br>最新 Run ID: [#8723419085](https://github.com/RyotaMaeno1227/OSS_CAE_FEM_salome/actions/runs/8723419085)<br>対処例: `softness_scale` を 0.75→0.9 に戻し、`docs/coupled_constraint_presets_cheatsheet.md` へ逸脱値を追記 | 診断ポリシーで auto-drop を有効化し、条件数上限を調整 |
| `--max-file-size-mb` 超過 | CSV ログが肥大化（長期実行/高頻度サンプリング） | `plan.md` で該当ファイルを確認し、必要に応じて `exclude-config` に追加 → サンプリング周期を見直し<br>最新 Run ID: [#8612045772](https://github.com/RyotaMaeno1227/OSS_CAE_FEM_salome/actions/runs/8612045772)<br>対処例: `tests/test_coupled_constraint_endurance --dump_stride` を 1→4 に変更し、ログローテーション設定を `config/endurance_archive.yaml` に追加 | `tools/archive_coupled_constraint_endurance.py --exclude-config config/endurance_exclude.yaml` で例外設定を管理 |
| `--max-entries` での削除 | アーカイブ保持上限を超える | `plan.md` の `max-entries` 行をレビュー → 削除したくないファイルは保留リストに追加し再実行<br>最新 Run ID: [#8539901140](https://github.com/RyotaMaeno1227/OSS_CAE_FEM_salome/actions/runs/8539901140)<br>対処例: `plan.csv` の hash を `data/endurance_archive/retention_allowlist.txt` に移し、次回実行で `--max-entries 32` を設定 | `exclude-config` に保護するファイル名を記載し、定期的に manifest を整理 |
| Webhook が未送信 | `ENDURANCE_ALERT_WEBHOOK` 未設定 | シークレット設定を見直し、CI ログの "WEBHOOK_URL not set" を確認<br>最新 Run ID: [#8495513377](https://github.com/RyotaMaeno1227/OSS_CAE_FEM_salome/actions/runs/8495513377)<br>対処例: `gh secret set ENDURANCE_ALERT_WEBHOOK < token.txt` で修正し、`tools/mock_webhook_server.py` で 200 応答を確認 | `tools/mock_webhook_server.py` でローカル検証し、実環境でも 200 応答を確認 |

---

## 10. Webhook 試験（`tools/mock_webhook_server.py`）

Webhook 連携を有効化する前に、以下の手順でペイロードを検証する。

1. `python tools/mock_webhook_server.py --port 9000 --log-dir mock_webhook_logs` を実行し、ローカルで待ち受ける。  
2. `.github/workflows/coupled_endurance.yml` の `ENDURANCE_ALERT_WEBHOOK` を一時的に `http://127.0.0.1:9000/hooks/endurance` に設定する（または `workflow_dispatch` 実行時に `env` で上書き）。  
3. ワークフローを `workflow_dispatch` で起動し、「Send webhook notification」「Send failure-rate digest」の両ステップが 200 応答で完了するか確認。  
4. `mock_webhook_logs/<timestamp>.json` に保存された payload をレビューし、Slack で想定している `attachments` と `tags` (`COUPLED`, `ENDURANCE`) が含まれているかチェックする。  
5. 問題なければ Webhook URL を本番値へ戻し、`mock_webhook_logs/` から検証ログを Issue/PR に添付して周知する。
