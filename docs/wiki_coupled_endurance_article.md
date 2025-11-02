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

## 6. Wiki 投稿テンプレート（見出し・スクリーンショット指示）

以下は Confluence 想定の原稿テンプレートです。社内ツールへコピーして利用してください（角括弧内は差し替え）。

```
{info:title=Coupled Endurance Nightly / CI 運用}
最終更新: [yyyy-mm-dd]（担当: [氏名]） / リンク: [Pull Request URL]
{info}

h1. 概要
* 対象ジョブ: GitHub Actions *coupled_endurance.yml*
* 目的: Coupled 拘束の長時間安定性監視
* 成果物: latest.csv, latest.summary.{json,md,html}, GIF/MP4, CI artifacts

h1. クイックリンク
* [CI トラブルシュート手順書|<社内パス>/coupled_endurance_ci_troubleshooting]
* [チュートリアル / メディア生成|<社内パス>/chrono_coupled_constraint_tutorial]
* [パラメータ YAML|<Git リポジトリ URL>/data/coupled_constraint_presets.yaml]
* [チートシート PDF|<社内ストレージ>/coupled_constraint_presets_cheatsheet.pdf]
* [アーティファクト一覧|https://github.com/<org>/<repo>/actions?...]

h1. 運用フロー
1. 失敗検知 – Actions の *archive-and-summarize* ステップが赤になっていないか確認
2. ログ確認 – latest.summary.json の *max_condition*, *warn_ratio*, *rank_ratio* を記録
3. 再現と対処 – ローカルで `python tools/plot_coupled_constraint_endurance.py ...` を実行
4. 報告 – #chrono-constraints へ結果と対応案を共有

h2. 参考スクリーンショット
* Actions 実行画面（FAILED のハイライト箇所） – 最新 1 件は必ず更新
* latest.summary.html の KPI 表（例: max condition / warn ratio） – 余白を切り抜き 1024px 幅目安
* plot_coupled_constraint_endurance.py の出力グラフ – GIF または PNG、タイトル・凡例が見える解像度

h1. メディア更新フロー
* docs/media/coupled/ 配下の画像・GIF・MP4 を更新
* スクリーンショットは PNG 形式（幅 1280px 推奨）
* 旧データは cloud storage の `archive/<yyyy-mm>` へ退避

h1. メンテナンス
* ローテーションと定期レビューの記録
* しきい値変更時のサマリと決定者
```

スクリーンショットは UI 変更時に差し替え、キャプション（例:「Actions 実行ログ例」）を明記してください。Wiki 上では画像を中央揃えにし、縮小されないよう幅を指定します。
テンプレートを具現化した見本は `docs/wiki_samples/coupled_endurance_article_sample.md` に保存しているため、差し替え時は同ファイルを複製して日付・担当者・リンクを更新してください。

### 7.1 スクリーンショット添付要件

| ファイル名（提案） | 撮影タイミング | 取得手順 | 更新基準 |
|--------------------|---------------|----------|----------|
| `actions_overview.png` | CI 失敗検知時または月次レビュー | GitHub Actions の対象ワークフロー → `archive-and-summarize` ステップを展開し、ブラウザの開発者ツールで 1280px 幅に固定 → PNG で保存 | 週次（失敗発生時）、最低でも月次で更新 |
| `summary_kpi.png` | 新しい `latest.summary.html` が生成されたとき | アーティファクトから HTML を開き、表領域のみをスクリーンキャプチャ | `max_condition` など KPI が変化したタイミング |
| `endurance_media.gif` / `endurance_media.mp4` | 耐久テストを再実行したとき | `docs/chrono_coupled_constraint_tutorial.md` セクション 9 の手順を使用 | 四半期ごと、またはメディアが 6 か月以上古い場合 |
| `local_repro_terminal.png` | ローカル検証を共有したい場合 | `plot_coupled_constraint_endurance.py` 実行ターミナルを 80x25 以上でキャプチャ | 任意（重大インシデント時に添付） |

保存先は `docs/media/coupled/` 配下とし、Wiki へ添付後も Git で履歴を管理します。PNG は 1 MB 以下、GIF/MP4 は 10 MB 以下を目安にしてください。

### 7.2 運用チェックリスト
- [ ] 最新の CI 実行結果を確認し、失敗ステップがないかチェックした。
- [ ] 新しい `latest.summary.*` を取得し、KPI 値を記事に反映した。
- [ ] スクリーンショット / メディアを撮り直し、Wiki および `docs/media/coupled/` に更新した。
- [ ] 変更内容（パラメータ調整やしきい値変更）を Slack `#chrono-constraints` に報告した。
- [ ] 次回担当者へ引き継ぎメモ（更新日、未解決課題）を残した。

## 7. 公開プロセス（社内 Wiki 同期）
1. PR マージ後 24 時間以内にテンプレートを使用して記事を更新。
2. スクリーンショット・GIF を添付し、プレビュー表示を確認。
3. Wiki の「最終更新日」「同期に使用したコミット SHA」「関係者（レビュー／承認）」を明記。
4. 更新完了後、`#chrono-constraints` チャンネルで通知（記事 URL、更新内容要約、担当）。  
   誰かが引き継げるようにチケットまたは Wiki の「担当者」フィールドを更新する。
5. 次回ローテーション担当者へコメントまたは Wiki タスクを割り当て、次回レビュー予定日を設定。

## 8. 備考
- 内部 Wiki へ掲載する際は、GitHub や社内 GitLab 等の具体的な URL を正式なものに置き換えてください。
- 本草案は `docs/wiki_coupled_endurance_article.md` としてリポジトリ内で管理し、変更履歴を追跡します。Wiki の更新忘れを防ぐため、PR テンプレートに「Wiki 同期済み」チェックボックスを追加することを推奨します。
