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

## 3. 監視用ワンライナー
```
watch -n 60 'tail -n 20 data/coupled_constraint_endurance.csv'
```
- 実行中に異常があればスクリーンショットをチャットへ貼る（リポジトリには残さない）。
- 条件数をざっくり見たい場合の例:
  - `watch -n 60 "tail -n 20 data/coupled_constraint_endurance.csv | cut -d, -f15 | tail"`
  - `tail -n 200 data/coupled_constraint_endurance.csv | awk -F, 'NR==1{next}{c=$15; if(c>12) printf\"\\033[31m\"; printf\"%s\\033[0m\\n\",c}'`
  - `tail -n 200 data/coupled_constraint_endurance.csv | grep -n "1.091"` （閾値超えがあるか簡易確認）

## 4. 条件数／Rank 欠落サマリ（Aチームへの共有用）
```
[Kappa/Rank Summary]
- Run: #<RUN_ID> (artifact: coupled-endurance-<RUN_ID>)
- Max κ: <max_kappa> / Mean κ: <mean_kappa> / Warn frames: <percent>%
- Rank issues: <none | step <id> Δrank=<n>>
- Exports: data/coupled_constraint_endurance.csv (steps <range>), data/latest.endurance.json
- Notes: {再測定の要否、依存する configuration}
```

## 5. 複数 Run を同日に取得した場合の報告例
```
[Multiple Runs Same Day]
- Runs: success=#<RUN_SUCCESS>, failure=#<RUN_FAIL>, legacy=#<RUN_OLD>
- Applied rule: latest success > latest failure > older success
- Recorded: #<RUN_SUCCESS> as primary in pm_status / handoff; others noted in appendix of chat
```
```
[Mixed Success/Failure Example]
- Runs: #<RUN_FAIL> (fail, latest), #<RUN_SUCC_PREV> (success, previous day)
- Applied rule: latest failure is temporarily primary; mark as "*to be replaced once success lands*"
- Action: re-run scheduled next day; keep #<RUN_SUCC_PREV> in notes for fallback comparison
```

## 6. 外部共有時の定型文（JSON 配布）
```
[Endurance JSON Distribution]
- Run: #<RUN_ID> (Actions link above)
- Attached: data/latest.endurance.json (hash <commit>)
- Scope: κ/diagnostics summary only。個人情報・環境依存のログは含まれません。
- Please confirm column schema matches docs/pm_status_2024-11-08.md (Nightly/B-section).
```

## 7. κ/RANK 異常の最短共有（3行版）
```
[Endurance Alert] Run #<ID> κmax=<max> warn=<pct>% / Rank issues: <none|Δrank@step>
Artifacts: coupled-endurance-<ID>, data/coupled_constraint_endurance.csv (steps <range>)
Next: rerun? {yes/no}, handoff to Team A with Kappa/Rank Summary if needed
```

## 8. Rank 欠落区間抽出ワンライナー（要エラー時のみ使用）
```
python - <<'PY'
import csv
from pathlib import Path
path = Path("data/coupled_constraint_endurance.csv")
with path.open() as f:
    reader = csv.reader(f)
    header = next(reader)
    for row in reader:
        step = row[0]
        # 22列目が drop_index_mask_step (0/1 bitmask 相当)
        if row[22] != "0":
            print(step, row[14], row[22])
PY
```
- 必要に応じて `row[23]` 以降の drop/recovery を併せて確認。

## 9. Slack 共有手順（週次 preset チェック結果）
```
python scripts/check_preset_links.py
# 出力をコピペ:
"Preset links verified for 2 file(s)." (週次チェック完了)
```
- チャットに貼り、Run ID があれば合わせて報告。
```
[Preset Check Share]
- Result: "Preset links verified for 2 file(s)."
- Context: after Run #<RUN_ID> log update (date: <YYYY-MM-DD>)
```

## 10. Endurance CSV 列チェック（ヘッダ順/列数）
```
head -n 1 data/coupled_constraint_endurance.csv | python - <<'PY'
import sys
cols = sys.stdin.read().strip().split(',')
print(f"cols={len(cols)} -> {cols}")
PY
```
- 列数変更多発時は A チームと合意の上で実施し、pm_status に記録。

## 11. 日次複数 Run 記入例（pm_status / handoff 用）
```
- Primary: #<RUN_PRIMARY> (success) / Artifact: coupled-endurance-<RUN_PRIMARY>
- Secondary: #<RUN_FAIL> (fail, same day) -> note only in chat; keep logs for A team troubleshooting
- JSON/CSV: updated from #<RUN_PRIMARY> (steps <range>)
```

## 12. Run 短縮サマリ（チャット即貼り用）
```
Run #<ID> summary: samples=<n>, duration=<sec>s, kappa_max=<max>, kappa_mean=<mean>, warn=<pct>%
```

> **運用メモ**: 上記テンプレを編集後はチャットへ貼り付け、必要なら社内 Wiki に転記してください。リポジトリ側にはテンプレのみを残し、個別 Run のメタデータはリポジトリ外に保管します。
