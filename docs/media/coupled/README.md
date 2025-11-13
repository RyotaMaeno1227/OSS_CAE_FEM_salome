# Coupled Media Assets

| File | Description | Source | Notes |
|------|-------------|--------|-------|
| _N/A_ | プリセットは Markdown (`docs/coupled_constraint_presets_cheatsheet.md`) を直接参照 |  | PDF を生成する場合は Appendix A.3.2 の任意手順を利用（リポジトリには含めない）。

## Slack 通知テンプレ（Markdown 運用）
※ Appendix B.5 の「Coupled Endurance Operations」にも同テンプレを掲載
```
[preset-md-update] docs/coupled_constraint_presets_cheatsheet.md を更新しました。
- 変更者: <名前> / 変更日: YYYY-MM-DD
- 関連ファイル: README / Hands-on Chapter 02 / Wiki クイックリンク
- チェック: scripts/check_doc_links.py, Appendix B.3/B.5 ローテ表更新済み
```

## Markdown 更新チェックリスト
- [ ] `scripts/check_doc_links.py ...` / `scripts/check_preset_links.py` を実行して問題が無いことを確認。  
- [ ] README / Hands-on / Wiki（本編＋サンプル）の該当セクションを更新。  
- [ ] `docs/documentation_changelog.md` と Appendix B.3/B.5 のローテ欄に更新日・担当を追記。  
- [ ] 上記 Slack テンプレで `#chrono-docs` / `#chrono-constraints` へ告知。  
- [ ] この README に更新内容を追記し、Appendix A.3.1 から参照できるようにする。
