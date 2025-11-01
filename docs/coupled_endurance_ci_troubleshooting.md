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

---

## 4. 失敗原因別の対処フロー

| トリガー | 典型的な対処 | 備考 |
|---------|--------------|------|
| `--fail-on-max-condition` (exit code 3) | Coupled 式のスケーリング見直し、`softness_*` の調整、診断ポリシーで自動ドロップ閾値を下げる。 | サマリ JSON の `max_condition` と履歴をグラフで確認。 |
| `--fail-on-warning-ratio` (exit code 4) | 連続して閾値を超えていないかチェックし、`ratio_*` やターゲット更新の段階的適用を検討。 | `warn_ratio` が高止まりしている場合は式の組み合わせが悪化している可能性。 |
| `--fail-on-rank-ratio` (exit code 5) | 拘束セットの線形従属を疑い、補助式の削減やポリシーの自動ドロップ設定を強化。 | `eq*_impulse` が 0 付近なら不要な式の可能性。 |
| `--max-file-size-mb` | CSV が肥大化しているため、ログ期間の見直しやサンプリング間隔の調整、または環境変数で緩和。 | 圧縮せずに 25 MiB 以上になる場合は記録内容を精査。 |
| `--max-age-days` / `--max-entries` | アーカイブの整理が滞っている。古いデータを明示的に残したい場合は閾値を環境変数で上書き。 | `manifest.json` に残したいエントリは別途退避してから再実行。 |

---

## 5. 役に立つコマンド
- 最新ログのサマリを CLI で確認:  
  `python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot --summary-json /tmp/summary.json --no-show`
- 重複判定や世代管理を確認:  
  `python tools/archive_coupled_constraint_endurance.py --dry-run --prune-duplicates --max-entries 10 --max-age-days 120`

CI での失敗を再現したら、原因調査の結果やパラメータ変更をこのドキュメント、または `docs/chrono_2d_development_plan.md` の CI セクションに追記して共有してください。
