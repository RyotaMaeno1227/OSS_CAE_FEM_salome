# 社内 Wiki 記事草案: Coupled Endurance Nightly / CI 運用

本草案は社内ナレッジベース（Confluence / Notion 等）へ掲載することを想定した記事原稿です。`docs/coupled_endurance_ci_troubleshooting.md` と整合を取りながら、運用担当が日次・週次で参照できるよう設計しています。

## 1. 概要
- 対象ジョブ: GitHub Actions `coupled_endurance.yml`（週次 + 手動再実行）
- 目的: Coupled 拘束の長時間安定性を監視し、条件数・警告比率・ランク欠損を閾値管理する
- 主な成果物: `latest.csv`、`latest.summary.{json,md,html}`、PNG/GIF/MP4 可視化、CI アーティファクト

## 2. クイックリンク（Wiki では相対リンクに差し替え）
- [CI トラブルシュート手順書](../docs/coupled_endurance_ci_troubleshooting.md)
- [Coupled チュートリアル（実行ログ・メディア生成）](../docs/chrono_coupled_constraint_tutorial.md)
- [パラメータプリセット（YAML）](../data/coupled_constraint_presets.yaml)
- [パラメータチートシート](../docs/coupled_constraint_presets_cheatsheet.md)
- GitHub アーティファクト: `https://github.com/<org>/<repo>/actions?query=workflow%3A%22Coupled+Endurance%22`

## 3. 運用フロー（Wiki 掲載用）
1. **失敗検知**  
   - Actions ページで `archive-and-summarize` ステップが失敗していないかチェック。エラーコード 3/4/5 はそれぞれ条件数・警告比率・ランク欠損の閾値超過。
2. **ログ確認**  
   - アーティファクトをダウンロードし、`latest.summary.json` を開いて `max_condition`, `warn_ratio`, `rank_ratio` を確認。  
   - 追加調査には `python tools/plot_coupled_constraint_endurance.py data/endurance_archive/latest.csv --skip-plot --summary-json out.json --no-show` を使用。
3. **再現＆対処**  
   - `docs/coupled_endurance_ci_troubleshooting.md` の「ローカル再現手順」に従い、しきい値を CI と同値にして再現。  
   - 原因がパラメータ起因であれば `data/coupled_constraint_presets.yaml` とチートシートを参照し、候補値を検討。修正時は PR 説明にサマリを添付。
4. **報告**  
   - チーム Slack チャンネル `#chrono-constraints` で発生状況と暫定対処を共有。必要であれば週報に追記。

## 4. メディア／資料更新
- `docs/chrono_coupled_constraint_tutorial.md` のセクション 8 を基準に、`docs/media/coupled/` に静止画・GIF・MP4 を保存。Wiki へ掲載する際は同パスのファイルを添付またはリンク。
- 画像更新時は、`data/coupled_constraint_endurance.csv` の生成日時と `tests/test_coupled_constraint_endurance` の実行ログを記事内にメモする。
- GIF/MP4 の容量目安は 10 MB 以下。必要があれば `fps` や間引きを調整。

## 5. メンテナンス手順
- **担当ローテーション**: Cチーム DevOps（Suzuki）→ Coupled 班（Mori）→ 数値解析班（Kobayashi）の順で月次ローテート。
- **定期レビュー**: 四半期ごと（1月/4月/7月/10月）に以下を確認し、Wiki 記事を更新する。
  - YAML プリセットとチートシートに差異がないか
  - CI しきい値（`--fail-on-*`）が最新の議論と一致しているか
  - メディアファイルが半年以内に更新されているか
- **変更フロー**: 変更が発生した場合は Pull Request に記事更新内容を含め、Merge 後 24 時間以内に社内 Wiki を同期。Wiki 側には「最終同期日」と PR リンクを記載する。

## 6. 備考
- 内部 Wiki へ掲載する際は、GitHub や社内 GitLab 等の具体的な URL を正式なものに置き換えてください。
- 本草案は `docs/wiki_coupled_endurance_article.md` としてリポジトリ内で管理し、変更履歴を追跡します。Wiki の更新忘れを防ぐため、PR テンプレートに「Wiki 同期済み」チェックボックスを追加することを推奨します。
