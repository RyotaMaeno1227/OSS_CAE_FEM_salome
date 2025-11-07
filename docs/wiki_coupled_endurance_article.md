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

## 6. Wiki 投稿テンプレート（概要）

- 詳細なテンプレート、スクリーンショット要件、チェックリストは `docs/appendix_optional_ops.md` の **B. Wiki Workflow Templates & Checklists** に移動しました。
- 本編では以下のポイントのみ押さえてください:
  1. 記事を更新する際は最新の KPI とスクリーンショットを添付する。  
  2. Slack `#chrono-constraints` への通知と次担当への引き継ぎを忘れない。  
  3. `docs/wiki_samples/coupled_endurance_article_sample.md` は Appendix 参照のうえ差し替える。

## 7. 公開プロセス（社内 Wiki 同期）
- PR マージ後 24 時間以内に Wiki を更新し、`appendix_optional_ops.md` のチェックリストを活用する。  
- `docs/media/coupled/` の最新メディアと KPI を同期し、Slack へ報告する。  
- 次回担当とレビュー予定日は必ず記録しておく。

## 8. 備考
- 具体的な URL・テンプレート・チェックリストは `docs/appendix_optional_ops.md` を参照。  
- 本草案は引き続き数値検証観点で更新し、運用情報は付録へ切り出している。
