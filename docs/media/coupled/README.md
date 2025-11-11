# Coupled Media Assets

| File | Description | Source | Notes |
|------|-------------|--------|-------|
| `presets.pdf` | Printable version of `docs/coupled_constraint_presets_cheatsheet.md` | Generated via fallback Markdown→PDF script (pandoc unavailable in env) | ⚠︎ 現在は暫定版。Appendix A.3.1 のチェックリストに沿って正式版へ差し替え予定（担当: Mori / 次回計画日: 2025-11-20）。

## Slack 通知テンプレ（#chrono-docs / #chrono-constraints）
```
[preset-pdf-update] docs/media/coupled/presets.pdf を更新しました。
- 生成者: <名前> / 生成日: YYYY-MM-DD
- コマンド: pandoc docs/coupled_constraint_presets_cheatsheet.md -o docs/media/coupled/presets.pdf --pdf-engine=xelatex
- サイズ: <xx> MB / フォント確認済み
- 参照ドキュメント: wiki / appendix A.3 / samples
```
暫定版のままの場合は、上記テンプレの代わりに `[preset-pdf-placeholder]` タグを使い、正式版差し替えが未完了である旨を共有してください。
