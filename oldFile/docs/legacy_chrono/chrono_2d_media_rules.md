# chrono-2d 図版・スクリーンショットルール

目的: chrono-2d の図版/スクリーンショットを一貫した命名・配置・参照で管理する。

## 保存先
- `docs/integration/assets/`（学習パス/図版）
- `docs/media/chrono-2d/`（README/補助資料）

## 命名規則
- 形式: `chrono-2d-<topic>-<yyyymmdd>.svg|png`
- 例: `chrono-2d-pivot-20251201.svg`

## 参照ルール
- Markdown から相対パスで参照する。
- 画像のキャプションに Run ID を含める。
- 追加時は `docs/documentation_changelog.md` に記録する。

## 更新時の最小チェック
1. ファイル名が命名規則に沿っていること。
2. 参照リンクが相対パスになっていること。
3. `docs/chrono_2d_readme.md` に導線があること。
