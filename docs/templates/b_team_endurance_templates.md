# Bチーム向け Endurance 共有テンプレ

このファイルはチャット投稿用テンプレを保管する場所です。記入後に内容をコピーして外部チャネルで共有し、リポジトリには個別 Run 情報を残さないようにしてください。

---

## 1. 失敗 Run アラート（条件数／Rank 欠落）
```
[Endurance Failure Alert]
- Run: #<RUN_ID> (workflow_dispatch by <operator> at <UTC timestamp>)
- Artifact: coupled-endurance-<RUN_ID> (保存先: docs/logs/ or data/diagnostics/...)
- Condition number: max=<value> / mean=<value>
- Rank issues: <有無・対象 step> (例: step 5421 rank drop -> Δrank=1)
- Log refs: data/coupled_constraint_endurance.csv (steps <range>), data/latest.endurance.json, watch tail output snippet
- Next actions: {Aチームへ提供する kappa log, 再実行判断, その他リンク}
```

## 2. `data/latest.endurance.json` 共有テンプレ
```
[Endurance Summary Share]
- Run: #<RUN_ID> (link: https://github.com/<owner>/<repo>/actions/runs/<RUN_ID>)
- Duration: <duration> s / Samples: <count>
- κ stats: max=<value>, mean=<value>, warnings=<percent>%
- Diagnostics: max|distance|=<value>, max|angle|=<value>
- Export: data/latest.endurance.json (commit <hash>) / data/coupled_constraint_endurance.csv (steps <start>-<end>)
- Notes: {特記事項（回復イベント、drop flags など）}
```

> **運用メモ**: 上記テンプレを編集後はチャットへ貼り付け、必要なら社内 Wiki に転記してください。リポジトリ側にはテンプレのみを残し、個別 Run のメタデータはリポジトリ外に保管します。
