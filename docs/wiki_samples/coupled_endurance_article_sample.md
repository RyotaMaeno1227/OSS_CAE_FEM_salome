# Coupled Endurance Nightly / CI 運用（サンプル記事）

{info:title=Coupled Endurance Nightly / CI 運用}
最終更新: 2025-10-21（担当: Suzuki） / Git 同期: [PR #1234](https://github.com/example/highperformanceFEM/pull/1234)
{info}

## 概要
- 対象ジョブ: GitHub Actions `coupled_endurance.yml`
- 目的: Coupled 拘束の長時間安定性を監視し、条件数・警告比率・ランク欠損を閾値管理する
- 成果物: `latest.csv`, `latest.summary.{json,md,html}`, PNG/GIF/MP4 可視化、CI アーティファクト

## クイックリンク
- [CI トラブルシュート手順書](../coupled_endurance_ci_troubleshooting.md)
- [Coupled チュートリアル（メディア生成手順）](../chrono_coupled_constraint_tutorial.md)
- [パラメータ YAML](../../data/coupled_constraint_presets.yaml)
- [チートシート PDF](../coupled_constraint_presets_cheatsheet.md)
- [アーティファクト一覧](https://github.com/example/highperformanceFEM/actions?query=workflow%3A%22Coupled+Endurance%22)

## 運用フロー
1. **失敗検知** – Actions の *archive-and-summarize* ステップが失敗 (赤) していないかを確認する。
2. **ログ確認** – `latest.summary.json` の `max_condition`, `warn_ratio`, `rank_ratio` を記録し、閾値との乖離を把握する。
3. **再現 & 対処** – ローカルで  
   ```bash
   python tools/plot_coupled_constraint_endurance.py \
     data/endurance_archive/latest.csv \
     --skip-plot \
     --summary-json out/summary.json \
     --fail-on-max-condition 1.0e8 \
     --fail-on-warning-ratio 0.05 \
     --fail-on-rank-ratio 0.01 \
     --no-show
   ```  
   を実行。超過要因を特定し、`data/coupled_constraint_presets.yaml` を見直す。
4. **報告** – Slack `#chrono-constraints` に検知状況・対処案・次アクションを共有。

### スクリーンショットガイド
- Actions 実行画面（最新 1 件、失敗ステップを赤枠で囲む）
- `latest.summary.html` の KPI 表（max condition / warn ratio / rank ratio が見える範囲）
- `docs/media/coupled/endurance_overview.gif` または MP4 サムネイル

## メディア更新
- コマンド例:
  ```bash
  python tools/plot_coupled_constraint_endurance.py \
    data/coupled_constraint_endurance.csv \
    --output docs/media/coupled/endurance_overview.png \
    --no-show
  python tools/make_coupled_animation.py \
    --csv data/coupled_constraint_endurance.csv \
    --gif docs/media/coupled/endurance_overview.gif \
    --mp4 docs/media/coupled/endurance_overview.mp4 \
    --fps 24 --stride 4
  ```
- 生成物は `docs/media/coupled/` に配置し、更新日時と元 CSV のコミット SHA を `docs/media/coupled/README.md` へ追記する。
- 旧ファイルは `cloud://chrono/shared/coupled/endurance/archive/<YYYY-MM>/` に退避。

## メンテナンス
- **担当ローテーション**: DevOps（Suzuki）→ Coupled 班（Mori）→ 数値解析班（Kobayashi）を月単位で交代。
- **四半期レビュー** (1/4/7/10 月):  
  - YAML とチートシートの差分確認  
  - CI しきい値の妥当性評価  
  - メディアが 6 か月以内に更新されているかをチェック
- **Wiki 同期フロー**
  1. Git 更新内容を確認し、本ファイルをテンプレとして流用。
  2. 最終更新日・担当・PR リンクを差し替え。
  3. スクリーンショットを貼り替え、キャプションを付ける。
  4. 公開後、Slack で通知し次担当へタスクを引き継ぐ。

---

## 付録A: Wiki 投稿テンプレ（コピー用）

```
{info:title=Coupled Endurance Nightly / CI 運用}
最終更新: [yyyy-mm-dd]（担当: [氏名]） / Git 同期: [PR URL]
{info}

## 概要
- 対象ジョブ: GitHub Actions `coupled_endurance.yml`
- 目的: Coupled 拘束の長時間安定性を監視し、条件数・警告比率・ランク欠損を閾値管理する
- 成果物: latest.csv, latest.summary.{json,md,html}, PNG/GIF/MP4, CI アーティファクト

## クイックリンク
- [CI トラブルシュート手順書|<社内パス>/coupled_endurance_ci_troubleshooting]
- [Coupled チュートリアル|<社内パス>/chrono_coupled_constraint_tutorial]
- [パラメータ YAML|<Git リポジトリ URL>/data/coupled_constraint_presets.yaml]
- [チートシート PDF|<社内ストレージ>/coupled_constraint_presets_cheatsheet.pdf]
- [アーティファクト一覧|https://github.com/<org>/<repo>/actions?query=workflow%3A%22Coupled+Endurance%22]

## 運用フロー
1. 失敗検知 – Actions の *archive-and-summarize* ステップを確認
2. ログ確認 – `latest.summary.json` の KPI を記録
3. 再現と対処 – ローカルで `plot_coupled_constraint_endurance.py` を実行
4. 報告 – Slack `#chrono-constraints` へ共有

### 参考スクリーンショット
- Actions 実行画面
- KPI 表（`latest.summary.html`）
- `docs/media/coupled/endurance_overview.gif`（または MP4 サムネ）

## メディア更新
- `plot_coupled_constraint_endurance.py` で PNG/GIF/MP4 を再生成
- `docs/media/coupled/README.md` に更新日時と元 CSV のコミット SHA を追記
- 旧メディアは `archive/<yyyy-mm>/` へ退避

## メンテナンス
- 担当ローテーションと四半期レビューを記録
- しきい値変更時のサマリと決定者を明記
- 公開後は Slack 通知と次担当への引き継ぎ
```

## 付録B: スクリーンショット一覧

| ファイル名 | 取得方法 | 更新基準 |
|------------|----------|----------|
| `actions_overview.png` | GitHub Actions ワークフロー一覧 → `archive-and-summarize` ステップを展開して 1280px 幅でキャプチャ | 失敗時／月次 |
| `summary_kpi.png` | `latest.summary.html` の KPI 表を切り抜き | KPI が変動したタイミング |
| `endurance_media.gif` / `endurance_media.mp4` | `docs/chrono_coupled_constraint_tutorial.md` セクション 9 の手順で再生成 | 四半期ごと、または 6 か月超の古いメディアを更新 |
| `local_repro_terminal.png` | ローカル検証ターミナルを 80x25 以上でキャプチャ | 任意（重大インシデント時） |

## 付録C: 公開フロー確認リスト

1. PR マージ後 24 時間以内に Wiki を更新し、最終更新日と PR リンクを差し替える。
2. テンプレに沿ってスクリーンショットとメディアを貼り替え、ファイル名は付録Bと一致させる。
3. Wiki の公開設定とアクセス権を確認し、Slack `#chrono-constraints` へ更新通知を送付する。
4. 次回担当者へタスクを引き継ぎ、更新予定日と未完了項目をコメントとして残す。
