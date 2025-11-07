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

> 付録（テンプレート全文・スクリーンショット一覧・公開チェックリスト）は `docs/appendix_optional_ops.md` の **B. Wiki Workflow Templates & Checklists** に移動しました。本サンプルでは記事本文のみを管理し、運用手順は付録を参照してください。
