# FEM4C Team Dispatch Messages (2026-02-06)

このファイルは PM-3 から各チームへ送る連絡文テンプレです。  
そのまま各チームチャットへ貼り付けて使ってください。

---

## Team A 向け連絡文（実装）

```
@A-team
PM-3 依頼です。今回スプリントは FEM4C Phase 2 を優先してください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- mbd/coupled 実行モードの実装側
- 対象は handoff の Aチーム欄どおり（runner/fem4c 周辺）

[進捗報告先]
1) docs/team_status.md に「Aチーム」欄で実行内容・コマンド・結果を追記
2) docs/session_continuity_log.md にセッション終了時の4項目
   - Current Plan
   - Completed This Session
   - Next Actions
   - Open Risks/Blockers

[受入チェック]
- handoff の Aチーム受入基準を満たしたら、team_status に完了判定を明記してください。
```

---

## Team B 向け連絡文（検証）

```
@B-team
PM-3 依頼です。今回スプリントは FEM4C MBD拘束APIの数値検証を優先してください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- distance/revolute の残差・ヤコビアン検証ハーネス
- 有限差分照合と閾値設定

[進捗報告先]
1) docs/team_status.md に「Bチーム」欄で実行コマンド、閾値、pass/fail を追記
2) docs/session_continuity_log.md にセッション終了時の4項目を追記

[受入チェック]
- handoff の Bチーム受入基準を満たしたら、再現コマンド1行を team_status に必ず記載してください。
```

---

## Team C 向け連絡文（差分整理）

```
@C-team
PM-3 依頼です。今回スプリントは FEM4C の巨大 dirty 差分の整理を最優先で進めてください。

[指示確認先]
1) docs/abc_team_chat_handoff.md の「0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）」
2) docs/team_runbook.md の共通ルール

[担当]
- 差分3分類（実装として残す / 生成物・不要物 / 意図不明）
- FEM4C/test/* 削除群の暫定判定
- 安全な git add 手順案の作成

[進捗報告先]
1) docs/team_status.md に「Cチーム」欄で分類結果と判断根拠を追記
2) docs/session_continuity_log.md にセッション終了時の4項目を追記

[成果物]
- docs/abc_team_chat_handoff.md の Cチーム受入基準を満たす整理レポートを docs/ 配下へ追加してください。
```

---

## PM メモ

- 全チーム共通で、まず `docs/abc_team_chat_handoff.md` の Section 0 を読む。
- 進捗は `docs/team_status.md`、セッション引継ぎは `docs/session_continuity_log.md`。
- 混在コミット回避のため、担当範囲外ファイルはステージしない。
