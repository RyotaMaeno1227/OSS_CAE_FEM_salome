# Team Session Compliance Audit (2026-02-08)

目的: 30分連続実行ルール（`elapsed_min >= 30`）の受入を機械監査し、差し戻し判断を統一する。  
実行者: PM-3

## 1. 最新エントリ監査

実行コマンド:

```bash
python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30
```

結果:

```text
AUDIT_TARGET: latest entries (A/B/C)  threshold=elapsed_min>=30
----------------------------------------------------------------------------------------------------------------
team  verdict  elapsed  timer  sleep  changes  commands  pass/fail  start_epoch  entry
----------------------------------------------------------------------------------------------------------------
   A  FAIL          19  ok       False  True     True      True        1770438531  - 実行タスク: A-14 継続（coverage拡張: expected failure message + 境界ケース）
      reasons: elapsed_min<30
   B  FAIL          17  ok       False  True     True      True        1770437994  - 実行タスク: B-8（Done: 静的保証 + ローカル回帰再検証）/ B-8（In Progress, Blocker: スポット証跡）
      reasons: elapsed_min<30
   C  FAIL          17  ok       False  True     True      True        1770437939  - 実行タスク: C-12 完了（安全 staging 最終確認） + C-13 着手
      reasons: elapsed_min<30
----------------------------------------------------------------------------------------------------------------
RESULT: FAIL (at least one team entry does not satisfy compliance)
```

## 2. 履歴傾向監査

実行コマンド:

```bash
python scripts/audit_team_history.py --team-status docs/team_status.md --min-elapsed 30
```

結果（要約）:

```text
HISTORY_AUDIT threshold=elapsed_min>=30 require_evidence=True
------------------------------------------------------------------------------------------------------------
team  total  pass  fail  pass_rate  short(<th)  missing_timer  sleep  avg_elapsed
------------------------------------------------------------------------------------------------------------
   A     10     0    10      0.0%           7              4      0        15.9
   B      8     1     7     12.5%           3              4      0        38.3
   C      6     0     6      0.0%           5              1      0        14.5
```

所見:
- A/Cは短時間終了が主因。
- Bは平均時間は長いが、旧フォーマット混在（timer欠落等）で fail が残る。

## 3. 差し戻し文面の自動生成

実行コマンド:

```bash
bash scripts/run_team_audit.sh docs/team_status.md 30
```

用途:
- 監査JSONを `/tmp/team_audit_*.json` に出力。
- A/B/C 向けの差し戻し文面をそのままコピーして送信可能。
