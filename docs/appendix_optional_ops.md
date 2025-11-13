# Optional Operations Appendix (Draft)

計算コアに直接影響しない運用・通知・公開系の手順をまとめた付録です。  
本編ドキュメントでは概要のみを扱い、詳細は本ファイルを参照してください。

---

## A. Media Publishing & Sharing (GitHub Pages / Animation)

### A.1 Stride / Matplotlib Animation Tips
```python
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib import animation

from coupled_constraint_endurance_analysis import load_csv

csv_path = Path("data/coupled_constraint_endurance.csv")
data = load_csv(csv_path)
stride = 5  # 5 サンプルごとに 1 フレームを生成

time = data["time"][::stride]
distance = data["distance"][::stride]
condition = data["condition"][::stride]

fig, ax = plt.subplots(figsize=(9.6, 4.8))
ax2 = ax.twinx()
line_dist, = ax.plot([], [], color="C0", label="distance [m]")
line_cond, = ax2.plot([], [], color="C3", label="condition number")
ax.set_xlim(time[0], time[-1])
ax.set_xlabel("time [s]")
ax.set_ylabel("distance [m]")
ax2.set_ylabel("condition number")

def update(frame_index: int):
    upto = frame_index + 1
    line_dist.set_data(time[:upto], distance[:upto])
    line_cond.set_data(time[:upto], condition[:upto])
    return line_dist, line_cond

ani = animation.FuncAnimation(fig, update, frames=len(time), blit=True, interval=40)
ani.save("docs/media/coupled/endurance_stride5.gif", writer="pillow", fps=20)
ani.save("docs/media/coupled/endurance_stride5.mp4", writer="ffmpeg", fps=24)
```

- `stride` は CSV サイズや目標ファイルサイズに合わせて調整する。`len(time) // stride > 4000` の場合はさらに間引きを検討。  
- `interval` はフレーム間隔 [ms]。GIF を軽量化したい場合は 60–80 ms に伸ばす。  
- GIF/MP4 を PR に含める際は `du -h docs/media/coupled/endurance_stride5.*` でサイズを確認し、10–12 MB 以下を目安にする。

### A.2 GitHub Pages 埋め込みリファレンス
- Markdown 埋め込み:
  ```markdown
  ![Endurance overview stride 5](media/coupled/endurance_stride5.gif)
  ```
- HTML `<video>` ブロック:
  ```html
  <video controls loop muted playsinline width="960">
    <source src="https://<org>.github.io/<repo>/media/coupled/endurance_stride5.mp4" type="video/mp4">
    <source src="https://<org>.github.io/<repo>/media/coupled/endurance_stride5.gif" type="image/gif">
    Your browser does not support the video tag.
  </video>
  ```
- 公開チェックリスト:
  - GitHub Pages デプロイログでファイルを確認。
  - 実機ブラウザで GIF/MP4 の再生を検証し、カクつきがあれば `stride` や `fps` を見直す。
  - `npm exec broken-link-checker -- --allow-redirect` 等でリンク検査を定期実行。

### A.3 Preset Cheat Sheet（Markdown 配布）
プリセットは `docs/coupled_constraint_presets_cheatsheet.md` を正とし、GitHub 上の Markdown で閲覧・配布する。PDF への変換は必須ではなく、希望者が個別に Pandoc で生成する場合のみ Appendix A.3.2 を参照する。

- Wiki / Appendix / README では Markdown への相対リンクを貼る（例: `docs/coupled_constraint_presets_cheatsheet.md`）。  
- 参照先を更新した場合は `docs/documentation_changelog.md` に記録し、Slack `#chrono-docs` / `#chrono-constraints` に周知する。  
- 印刷や外部配布が不要なため、`docs/media/coupled/` 以下に PDF を保持しない。

#### A.3.1 配布チェックリスト（Markdown ベース）
- [ ] `docs/coupled_constraint_presets_cheatsheet.md` を更新し、`scripts/check_doc_links.py` でリンク整合性を確認した。  
- [ ] README、Hands-on、Wiki（本編＋サンプル）のプリセットリンクが Markdown 版を指していることを確認。  
- [ ] `docs/documentation_changelog.md` に更新内容・担当者・コミット日を追記。  
- [ ] Appendix B.3/B.5 のローテーション表に更新日・担当を記録し、Slack へ通知。  
- [ ] 必要に応じて `docs/media/coupled/README.md` に Markdown 運用である旨の注意書きを追加。
- 差分記録テンプレ（PR 説明や README へ貼り付け可能）:
  ```
  ### Preset cheat sheet update log
  - Author: <Name>
  - Date: YYYY-MM-DD
  - Sections touched: e.g., Section 2 ratio table, Appendix notes
  - Link validation: scripts/check_doc_links.py ✔︎
  - Notified: README / Hands-on / Wiki links + Slack #chrono-docs
  ```

#### A.3.2 （任意）外部向け PDF を作る場合
- チーム外へ配布する必要がある場合のみ `pandoc docs/coupled_constraint_presets_cheatsheet.md -o presets.pdf` でローカル生成し、個別に共有する。  
- リポジトリにはアップロードしない。Slack テンプレ:
  ```
  [preset-md-export] Markdown 版プリセットを外部共有のため PDF 化しました（添付参照）。
  - 生成者: <名前> / 日付: YYYY-MM-DD
  - 元ファイル: docs/coupled_constraint_presets_cheatsheet.md
  ```

#### A.4 メディア命名規則・保存先
- ルート: `docs/media/coupled/{YYYYMM}/`（例: `docs/media/coupled/202511/`）。  
- ファイル命名: `<topic>_<variant>_v<rev>.{png,gif,mp4}`（例: `endurance_overview_v1.gif`）。  
- プリセットは Markdown 参照のため固定 PDF は不要。メディア（GIF/MP4）だけ `docs/media/coupled/` に配置し、履歴は README に追記する。  
- Wiki/Appendix では `docs/coupled_constraint_presets_cheatsheet.md` を直接リンクし、詳細な履歴は changelog とローテ表に集約。

##### A.4.1 メディア更新ワークフロー
1. **作成** – 担当者（例: Cチーム DevOps）が PNG/GIF/MP4 を `docs/media/coupled/YYYYMM/` へ配置し、`docs/media/coupled/README.md` に生成者・日付・元データを記録。  
2. **レビュー** – Appendix A/B のオーナーがサイズ・命名規則を確認し、必要であれば Slack `#chrono-docs` で承認依頼。  
3. **公開** – 承認後に固定名（例: `docs/media/coupled/endurance_overview.gif`）へコピーし、Wiki / Appendix のリンクを更新。  
4. **アナウンス** – Slack `#chrono-docs` / `#chrono-constraints` にテンプレを用いて共有し、Appendix B.5.1 のローテ表へ Run ID / 更新日を記入。

#### A.5 図版 (`docs/wiki_samples/schema_validation_gist.svg`) 差し替えガイド
1. **編集ツール**: Figma または Inkscape。フォントは Noto Sans CJK（Bold/Regular）を使用。  
2. **キャンバス**: 1280×720 px、背景透過。線幅 2 px、カラーパレットは `docs/styles/wiki_diagrams.json` に従う。  
3. **書き出し**: SVG（Plain）で保存し、ファイル名は `schema_validation_gist.svg`。バージョン管理用に `docs/media/wiki/202511/schema_validation_gist_v2.svg` も保存。  
4. **更新手順**:
   - `docs/wiki_samples/schema_validation_gist.svg` を差し替え。
   - Appendix A.5 と `docs/wiki_samples/README.md` に生成者・日付を記録。
   - Slack `#chrono-docs` で告知（テンプレ: `[wiki-diagram] schema_validation_gist.svg updated by <name> on YYYY-MM-DD`）。

##### A.5.1 承認フロー
| ステップ | 担当 | 内容 |
|---------|------|------|
| Draft | 作成者（例: テクニカルライター） | Figma/Inkscape で更新し、PNG サムネイルを Slack へ共有。 |
| Review | Appendix A 管理者 | フォント・サイズ・命名規則を確認し、`docs/media/wiki/YYYYMM/` の履歴をチェック。 |
| Approve | チームリード（Cチーム） | 差し替え可否を決定し、`docs/documentation_changelog.md` へ反映。 |
| Publish | 作成者 | 固定パスへコピーし、Wiki/Appendix のリンクと rotation 表を更新。 |

--- 

## B. Wiki Workflow Templates & Checklists

### B.1 Confluence テンプレート（コピー用）
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
* [チートシート (Markdown)|<Git リポジトリ URL>/docs/coupled_constraint_presets_cheatsheet.md]
* [アーティファクト一覧|https://github.com/<org>/<repo>/actions?...]

h1. 運用フロー
1. 失敗検知 – Actions の *archive-and-summarize* ステップを確認
2. ログ確認 – latest.summary.json の *max_condition*, *warn_ratio*, *rank_ratio* を記録
3. 再現と対処 – ローカルで `python tools/plot_coupled_constraint_endurance.py ...` を実行
4. 報告 – Slack #chrono-constraints へ共有

h2. 参考スクリーンショット
* Actions 実行画面（FAILED の箇所を強調）
* `latest.summary.html` の KPI 表
* `docs/media/coupled/endurance_overview.gif` などメディアのサムネイル

h1. メディア更新フロー
* docs/media/coupled/ に PNG/GIF/MP4 を配置
* スクリーンショットは PNG（幅 1280px 推奨）
* 旧データは `archive/<yyyy-mm>` へ退避

h1. メンテナンス
* ローテーションと定期レビュー
* しきい値変更時の決定者と理由
```

### B.2 スクリーンショット要件
| ファイル名 | 撮影タイミング | 取得手順 | 更新基準 |
|------------|---------------|----------|----------|
| `actions_overview.png` | CI 失敗時または月次レビュー | Actions → `archive-and-summarize` ステップを展開し 1280px 幅でキャプチャ | 週次（失敗時） |
| `summary_kpi.png` | `latest.summary.html` 更新時 | HTML の KPI 表をキャプチャ | KPI 変動時 |
| `endurance_media.gif` / `mp4` | 耐久テスト更新時 | A.1 の手順で再生成 | 四半期ごと、または 6 か月以上古い場合 |
| `local_repro_terminal.png` | 任意（重大インシデント共有） | 再現コマンドのターミナルを 80×25 以上で撮影 | 必要時 |

### B.3 運用チェックリスト
- [ ] 直近の CI 実行結果を確認した。  
- [ ] `latest.summary.*` を取得し、KPI を記事へ反映した。  
- [ ] スクリーンショット／メディアを更新し、`docs/media/coupled/` に保存した。  
- [ ] 変更内容を Slack `#chrono-constraints` へ報告した。  
- [ ] 引き継ぎ情報（更新日・未解決課題）を次担当へ共有した。  
- [ ] 公開後 24 時間以内にスクリーンショット一覧が最新か再確認した。
- [ ] README / Hands-on / Wiki / Appendix のプリセットリンクが Markdown（`.md`）を指していることを確認した。  
- [ ] `docs/pm_status_YYYY-MM-DD.md` と `docs/coupled_island_migration_plan.md` の KPI 表を同じ値に更新し、B.5.1 の担当欄を埋めた。  
- [ ] Contact + Coupled テスト運用（B.6）でログ解析／通知分岐をレビューした。  
- [ ] リンク検証チェックリスト（B.7）を実行し、結果を記録した。
- [ ] `workflow_dispatch` で `coupled_endurance.yml` を起動し、Slack/Webhook に schema validation 抜粋と failure-rate digest が届いたことを Appendix B.5.1 の表へ Run ID 付きで記録した。

### B.4 公開プロセス
1. PR マージ後 24 時間以内に Wiki を更新。  
2. スクリーンショット／GIF を添付しプレビュー確認。  
3. 「最終更新日」「同期コミット SHA」「関係者」を明記。  
4. 更新完了を Slack で通知し、担当者フィールドを更新。  
5. 次担当者へタスクを割り当て、次回レビュー予定日を設定。

#### B.4.1 Wiki 更新棚卸し（直近 4 週）
| 週 (開始日) | 担当 | B.3 チェック | Markdown 運用確認 | B.4 公開プロセス | 備考 |
|-------------|------|--------------|-------------------|-------------------|------|
| 2025-10-20 | Kobayashi | ✅ summary・スクリーンショット更新 | ✅ チートシートが `.md` を指していることを確認 | ✅ B.4 手順で再告知 | Contact+Coupled 判定ログを Appendix B.6 へ追記。 |
| 2025-10-27 | Suzuki | ⚠️ スクリーンショット差し替えが 12h 遅延（KPI は更新済み） | ✅ Markdown 方針維持 | ✅ 公開・通知済み | 遅延理由（CI 障害）を #chrono-docs に共有。 |
| 2025-11-03 | Tanaka | ✅ KPI & メディア更新、引き継ぎメモ作成 | ✅ Markdown のみ参照（PDF 無し） | ✅ Nightly 差分と Webhook ログを追記 | Appendix B.5.1 ローテーション表を最新化。 |
| 2025-11-10 | Mori | ✅ KPI / Appendix B.3/B.6 棚卸し、Markdown 注意書きを更新 | ✅ README/Hands-on/Wiki のリンクを再チェック | ✅ PR #6250 で Wiki 同期、Slack `#chrono-docs` & `#chrono-constraints` 通知 | `scripts/check_doc_links.py` の結果をローテ表に記録。 |

> B.3 の未完了事項は翌週担当が必ず引き継ぐ。⚠️ が付いた週は `docs/documentation_changelog.md` に理由と是正策を記録済み。

### B.5 Coupled Endurance Operations
1. **失敗検知** – GitHub Actions の `archive-and-summarize` ステップを監視し、終了コード 3/4/5（条件数/警告比率/ランク欠損）を切り分ける。  
2. **ログ確認** – `latest.summary.json` から `max_condition`, `warn_ratio`, `rank_ratio` を読み取り、必要に応じて `python tools/plot_coupled_constraint_endurance.py data/endurance_archive/latest.csv --skip-plot --summary-json out.json --no-show` を実行。  
3. **ローカル再現** – `docs/coupled_endurance_ci_troubleshooting.md` の手順で CI と同じしきい値を設定し、原因がプリセット由来なら `data/coupled_constraint_presets.yaml` / `docs/coupled_constraint_presets_cheatsheet.md` を突き合わせる。  
4. **報告** – Slack `#chrono-constraints` で状況と暫定対処案を共有し、必要なら週報を更新。  

| 項目 | ガイドライン |
|------|--------------|
| 担当ローテーション | Cチーム DevOps（Suzuki）→ Coupled 班（Mori）→ 数値解析班（Kobayashi）の月次ローテ。 |
| 定期レビュー | 四半期ごとに YAML/チートシート差異、CI しきい値、`docs/media/coupled/` の更新日を確認。 |
| 変更フロー | PR に Wiki 変更内容を含め、Merge 後 24 時間以内に Wiki を同期。Wiki には最終同期日と PR リンクを明記。 |
| 付随メモ | `docs/media/coupled/` 更新時は生成日時と `tests/test_coupled_constraint_endurance` ログを記録し、Slack へ報告。 |
| KPI ログ連携 | `latest.summary.json` の KPI と `docs/pm_status_YYYY-MM-DD.md` のスナップショットを Slack 投稿へ添付し、Appendix B.5.1 の表へ Run ID を記録する。 |

#### B.5.2 Slack / Webhook 検証メモ
- `tools/compose_endurance_notification.py --summary-validation-report latest.summary.validation.md --summary-validation-json latest.summary.validation.json --diagnostics-report latest.diagnostics_console.log --diagnostics-log latest.diagnostics.md` を指定すると、Slack 側で `<details>` ブロック付きの Schema / Diagnostics が展開できる。Status が `SKIPPED` の場合は `latest.summary.json` や `plan.csv` が生成されていない合図なので、`data/endurance_archive` をダウンロードして原因を確認する。  
- 最新 Run ID を取得するには以下を実行する（`--latest` は `--auto-latest` のエイリアス）:
  ```bash
  python tools/fetch_endurance_artifact.py \
    --latest \
    --workflow coupled_endurance.yml \
    --run-status failure \
    --job-name archive-and-summarize \
    --comment-file data/endurance_archive/repro/comment.md \
    --console-comment-only
  ```
  Slack 投稿テンプレ:  
  ```
  [coupled-endurance] Run https://github.com/<org>/<repo>/actions/runs/<run_id>
  Job log: <job_url>
  latest.summary.validation: PASS (gist: https://gist.github.com/<id>)
  Failure-rate digest: see archive_failure_rate.md/png
  ```
- Failure-rate digest (`archive_failure_rate_{png,md,json}`) は週次アーティファクトにも保存される。CI ログのコメントに Run ID を残し、`docs/reports/coupled_endurance_failure_history.md` へリンクを追記する。  
- Nightly で `endurance_plan_lint.json` を得た場合は `--plan-lint-json` へ渡す。Slack の「Plan lint report」欄に `status=PASS/FAIL/SKIPPED` と `message` が表示されるため、B.5.1 と B.5.3 の Run ID ログにも同じ値を控える。

#### B.5.3 workflow_dispatch / Slack 検証ログ（直近 4 週）
| 週 (開始日) | Run ID | Slack/Webhook 結果 | メモ | ToDo |
|-------------|--------|--------------------|------|------|
| 2024-07-22 | n/a | (Bチーム合流前) | 以降の棚卸し対象に追加。 | 初回 `workflow_dispatch` 実行で追記 |
| 2024-07-29 | pending | 未検証（ローカル環境から Actions 実行不可） | 本番環境で `workflow_dispatch` 実施後に追記。 | Actions 実行者: TBD / Slack スクショ必須 |
| 2024-08-05 | pending | 未検証（ローカル環境から Actions 実行不可） | Slack/Webhook 監視のみ。 | Run ID と gist URL を Appendix へ追加 |
| 2024-08-12 | pending | 未検証（ローカル環境から Actions 実行不可） | Appendix B.5.2 手順書に従い、Run ID を確定次第更新。 | Failure-rate digest の PNG/MD を history へ貼付 |

#### B.5.4 Nightly artifact sharing
- `artifacts/nightly/latest_summary_validation.json` と `artifacts/nightly/endurance_plan_lint.json` を `docs/reports/nightly/` 以下にコピーし、GitHub Pages（`gh-pages` ブランチなど）で参照できるようにする。  
- 公開フォーマット例: `docs/reports/nightly/latest_summary_validation_<run-id>.md` に Markdown テーブルを追加し、JSON 全文は `<details>` で折りたたむ。  
- Slack には Pages リンク（例: `https://<org>.github.io/<repo>/reports/nightly/latest_summary_validation_<run-id>.html`）を併記し、Run ID ログ（B.5.3）にも URL を控える。

#### B.5.5 Evidence Markdown テンプレ
Evidence 列には以下のテンプレを貼り、Run ID／Artifact／ログを紐づける。

```markdown
- Run: [#<run-id>](https://github.com/<org>/<repo>/actions/runs/<run-id>)
- Artifact: [`coupled-endurance-<run-id>`](https://github.com/<org>/<repo>/actions/runs/<run-id>/artifacts/<artifact-id>)
- Log: [`docs/logs/kkt_descriptor_poc_e2e.md`](../docs/logs/kkt_descriptor_poc_e2e.md)
```

状況に応じて `diagnostics_console.log` や `archive_failure_rate.md` へのリンクを増やし、`docs/a_team_handoff.md` / `docs/coupled_island_migration_plan.md` の Evidence 列で共通化する。

> Multi-ω 更新手順の詳細は README「Multi-ω 差分チェック手順」節、および `tools/update_multi_omega_assets.py --refresh-report` の説明を参照。

#### B.5.1 KPI Update Rotation
| 曜日 | 対象ドキュメント | 主担当 | バックアップ | Markdown 確認 | メモ |
|------|------------------|--------|--------------|----------------|------|
| 月曜 | `docs/pm_status_2024-11-08.md`（週次ステータス最新号をコピーして運用） | Mori | Suzuki | ✅ / ⚠️ を記入 | `docs/coupled_island_migration_plan.md` の KPI と揃えること。 |
| 水曜 | `docs/coupled_island_migration_plan.md` KPI 表 (§5.1) | Kobayashi | Tanaka | ✅ / ⚠️ | `docs/pm_status_YYYY-MM-DD.md` と数値一致を確認。 |
| 金曜 | Appendix B.5 ローテーション表／Slack 通知履歴 | Suzuki | Mori | ✅ / ⚠️ | 週次ログが揃っているかをチェックし、必要なら週報へ転記。 |

> KPI 更新を実施したら日付と担当を B.5.1 の表かコメント欄に残し、`docs/documentation_changelog.md` へまとめて通知する。Slack では `#chrono-docs` と `#chrono-constraints` の両方へ共有する。

- 2025-11-10: Mori（Monday slot）更新 — Coupled/Island/3D の KPI を 83 / 73 / 50 に揃え、`docs/pm_status_2024-11-08.md`、`docs/coupled_island_migration_plan.md`、`docs/chrono_3d_abstraction_note.md` を同期。
- プリセットは Markdown 運用。外部共有のために PDF 化する場合のみ Appendix A.3.2 を参照し、履歴は Appendix B.3/B.5.1 で管理。

##### B.5.1.a 外部カレンダー連携案
- Google カレンダーに「KPI Rotation」カレンダーを作成し、月・水・金の担当者をイベントとして登録。  
- `calendar_id` を `config/ops/kpi_rotation_calendar.txt` に保存し、スクリプトで Appendix B.5.1 表と同期する案を検討中。  
- 導入可否: **検討中**（組織ポリシーで外部共有が制限されているため、2025Q4 の Ops 会議で判断予定）。

### B.6 Contact + Coupled Test Operations
1. **ログ抽出** – `./chrono-C-all/tests/test_island_parallel_contacts --dump=log.json` を実行し、`tools/filter_coupled_endurance_log.py log.json --output log_contact.csv --keep contact_impulse,diagnostics_flags` で必要カラムのみに絞る。  
2. **条件分岐**  
   - `condition_warning` あり & `contact_impulse` が連続増加 → Slack `#chrono-constraints` に *Contact saturation* テンプレで通知。  
   - `diagnostics.rank != equations_active` → `docs/coupled_contact_test_notes.md` のチェックリストを参照し、`tests/test_coupled_constraint` ログとの突き合わせを要求。  
3. **通知フォーマット** – Appendix C のログテンプレに加え、Contact 付きかどうかを `tag=CONTACT+COUPLED` で明示。  
4. **週次レビュー** – `docs/coupled_contact_test_notes.md` の判定指標表を更新し、`docs/wiki_coupled_endurance_article.md` からのリンクを確認。  
5. **KPI 連携** – Contact 混在ランの `max_condition` / `warn_ratio_contact_only` を `docs/pm_status_YYYY-MM-DD.md` のメモ欄および Slack 通知に含める（テンプレ: `[contact-kpi] run=<id> max_cond=... warn_ratio=...`）。  

| 状況 | Slack テンプレ | 追加対応 |
|------|----------------|----------|
| 高荷重で WARN 連発 | `[contact-saturation] run=<id> max_condition=...` | `ratio_distance` を一時的に 10% 下げる案を提示。 |
| ランク欠損のみ発生 | `[rank-mismatch] run=<id> eq_active=... rank=...` | Island 割当ログを `chrono_island2d_build` から採取し、PoC ガントへ共有。 |
| Contact 反力発散 | `[contact-divergence] run=<id> impulse=...` | `target_offset` ステージングを Hands-on Chapter 03 に沿って再設定。 |
| KPI ログのみ共有 | `[contact-kpi] run=<id> max_cond=... warn_ratio=...` | `docs/pm_status_YYYY-MM-DD.md` の備考と Appendix B.5.1 に Run ID を記録。 |

更新履歴: 2025-11-10 版で KPI テンプレと `[contact-kpi]` 行を加筆（Slack `#chrono-constraints` アナウンス済み）。
### B.7 Link Validation Checklist
このチェックリストは `docs/coupled_constraint_tutorial_draft.md` のバイリンガル節と関連ドキュメント（Hands-on / Solver Math / Contact Notes）を対象にする。

1. `rg -n "docs/coupled_constraint_hands_on.md" docs/coupled_constraint_tutorial_draft.md` でリンク表記が最新か確認。  
2. `python - <<'PY'` スニペットで参照ファイルの存在を検査（`Path("docs/...").is_file()`）。  
3. Hands-on / Solver Math / Contact Notes 側で該当節の見出しが変更されていないか `git diff --stat HEAD~` を併せて確認。  
4. `docs/documentation_changelog.md` の最新エントリにリンク検証日と担当を追記。  
5. 不整合が見つかった場合は、チュートリアルと Hands-on の両方に統合案（E 章、および `docs/integration/learning_path_map.md`）を適用する。

> 2025-11-08 時点で `docs/coupled_constraint_tutorial_draft.md` → Hands-on / Solver Math / Contact Notes のリンクは確認済み。次回は Appendix B.5 のローテーションに従って更新する。

### B.8 Bチーム Pending / Net-required Tasks
| ステータス | 内容 | 備考 |
|------------|------|------|
| Pending | `coupled_endurance.yml` を `workflow_dispatch` 実行し Run ID を Appendix B.5.3 へ記録 | ネットワーク可用環境必須 |
| Pending | Failure-rate artifact (`archive_failure_rate.{png,md,json}`) を再生成して `docs/reports/coupled_endurance_failure_history.md` へ貼付 | `tools/report_archive_failure_rate.py` を `--dry-run` なしで実行 |
| Net-required | Nightly Slack で `--summary-validation-json` / `--plan-lint-json` ブロックのスクショ取得 | Appendix C.4 に添付 |
| Net-required | `docs/logs/notification_audit.md` にメール/Webhook 通知の記録を追加 | 非 Slack チャネル向け |
| Net-required | Multi-ω 実行結果チェックリスト（README 参照）を週次で回し、差分を `docs/reports/kkt_spectral_weekly.md` に貼付 | 担当: B チーム（Aタスク支援） |

`tools/compare_kkt_logs.py` への Issue/PR 依頼テンプレ:
```
### Summary
- chrono-c log: data/diagnostics/chrono_c_kkt_log.csv (commit ...)
- chrono-main log: data/diagnostics/chrono_main_kkt_log.csv
- diag-json: data/diagnostics/sample_diag.json
- observed delta: Δκ_s > 5% on scenario=<name>, eq=<count>

### Requested change
- [ ] add new diagnostics column / preset
- [ ] update report layout
```
B チームに依頼する際は上記テンプレで Issue を立て、Appendix B.8 の表にリンクを残す。

--- 

## C. Logging & Notification Guidance

- 長時間テスト (`tests/test_coupled_constraint_endurance`) の CSV にはドロップ回数・再解ステップなど詳細メトリクスが含まれる。ログと合わせて異常検知を行うこと。  
- `chrono_log_set_handler` は 1 つのみ登録可能。複数宛先が必要な場合はデマルチハンドラを用意し、内部でファイル記録や既存ロガーを呼び出す。  
- ログファイルのローテーション時は最新の `FILE*` を設定し直すか、ハンドラ側でローテーション検知を実装する。

### C.1 CI でのログ統合チェック
- `make tests` には `tests/test_coupled_logging_integration` が含まれ、GitHub Actions でも同テストが実行される。失敗時は以下を確認:
  1. カスタムハンドラ解放（`chrono_log_set_handler(NULL, NULL)`）を忘れていないか。  
  2. `ChronoCoupledConditionWarningPolicy_C` の `enable_logging`、`log_cooldown` を誤って無効化していないか。  
  3. ハンドラ内で重い処理や例外（`fprintf` 失敗など）が起きていないか。  
- 再現手順: `./chrono-C-all/tests/test_coupled_logging_integration` を実行し、標準出力に表示されるキャプチャ件数を確認する。

### C.2 GitHub API / Failure-rate Digests
- `tools/report_archive_failure_rate.py` は `--weeks 8` でも 2 リクエスト（workflow runs + jobs）で完了する。`GITHUB_TOKEN` の既定 `actions:read` で十分だが、組織ポリシーで `workflow` エンドポイントが制限されている場合は Personal Access Token（`repo`, `workflow` scope 推奨）を `GH_TOKEN` に設定する。  
- GitHub へ到達できないローカル環境では `--skip-chart` を付与し、`archive_failure_rate.md/json` だけを生成する。PNG は Actions artifact から取得して `docs/reports/coupled_endurance_failure_history.md` に貼る。  
- Slack/Webhook 向けには `archive_failure_rate_slack.json` をそのまま `curl -X POST ... --data` に渡せる。Webhook 切替時は `tools/mock_webhook_server.py` で 200 応答を確認してから本番 URL を設定する。  
- `tools/compose_endurance_notification.py` へ `--diagnostics-log data/endurance_archive/latest.diagnostics.md` を渡すと Markdown 版の Diagnostics を `<details>` ブロックで貼り付けられる。Nightly で `artifacts/nightly/latest.diagnostics.md` を共有する場合も同じフラグを利用する。
- Secrets 運用は `docs/git_setup.md` の「GitHub Secrets」節と合わせてチェックし、`ENDURANCE_ALERT_WEBHOOK`・`GH_TOKEN`（必要なら PAT）・`CHRONO_BASELINE_CSV` などを更新する。

### C.3 Endurance CSV 必須カラム一覧
`tools/filter_coupled_endurance_log.py --lint-only --require …` が PR 時に検証する列（2024-08 時点）:

```
condition_number
condition_number_spectral
condition_gap
min_eigenvalue
max_eigenvalue
active_equations
drop_events_total
```

列の追加・削除を伴う変更では、上記リストと CI ログ（`Validate endurance CSV filter` ステップ）を参照し、必要に応じて `--require` 引数を更新すること。

### C.4 Slack を使わない場合の通知
- **Webhook**: `curl -X POST <url> -H 'Content-Type: application/json' -d @archive_failure_rate_slack.json`。送信ログ（日時・HTTP ステータス・payload 抜粋）を `docs/logs/notification_audit.md` に追記する。  
- **メール**: `tools/compose_endurance_notification.py --format eml --output out/mail.eml` を使い、SMTP 経由で送信。宛先・件名・送信時刻を Appendix B.5.1 のコメント欄に記録する。  
- **テンプレ例**:
  ```
  Subject: [Coupled Nightly] WARN ratio exceeded (run <id>)
  Body:
  - max_condition: ...
  - warn_ratio: ...
  - log: s3://.../latest.summary.json
  ```
- **受信ログテンプレ（`docs/logs/notification_audit.md`）**
  ```
  - date: YYYY-MM-DD HH:MM
    channel: webhook | email
    endpoint: https://hooks.example/...
    payload: archive_failure_rate_slack.json
    status: 200
    notes: (optional)
  ```
  Slack を利用できない期間はこのテンプレで履歴を管理し、復旧後に Appendix B.5.1 へ抜粋を転記する。

### C.4 Webhook / Email Notification (Slack なし環境)
1. `tools/compose_endurance_notification.py` を実行する際、`--output-email-html notifications/email.html` を指定して HTML テンプレートを生成する。  
2. Webhook を使わない場合は `tools/mock_webhook_server.py` を省略し、メール送信スクリプト（例: `python tools/send_notification_email.py --body-file notifications/email.html`）で共有する。  
3. Run ID と job ログ URL は必ずメール本文と Appendix B.5.3 のログに記録する。  
4. 送信前チェックリスト:
   - [ ] Run URL（`https://github.com/<org>/<repo>/actions/runs/<run-id>`）を本文に含めた。  
   - [ ] schema/diagnostics/failure-rate (<details>) が正しく展開できるかプレビューした。  
   - [ ] `latest.summary.validation.{md,json}` と `archive_failure_rate.{png,md,json}` を添付またはリンクした。  
   - [ ] plan lint JSON が `SKIPPED` の場合、理由を本文へ追記した。

下図は Slack での `--summary-validation-json` / `--plan-lint-json` 表示例（`docs/wiki_samples/nightly_validation_blocks.svg`）:

![Nightly validation blocks](wiki_samples/nightly_validation_blocks.svg)

通知ログを蓄積する場合は `docs/logs/notification_audit.md` のテンプレートを利用し、Run ID／連絡先／リンクを追記する。

---

## D. Coupled Benchmark Automation

### D.1 事前準備
```bash
python3 -m pip install --upgrade pip
python3 -m pip install pyyaml
```
PyYAML は `tools/run_coupled_benchmark.py` が `config/coupled_benchmark_thresholds.yaml` を読み込む際に利用する。未インストールの場合は JSON 設定を渡す。

### D.2 ローカル実行
```bash
python3 tools/run_coupled_benchmark.py \
  --bench-path ./chrono-C-all/tests/bench_coupled_constraint \
  --config config/coupled_benchmark_thresholds.yaml \
  --csv-validation fail \
  --csv-validation-jsonl logs/csv_issues.jsonl \
  --output data/coupled_benchmark_metrics.csv \
  --fail-on-max-condition 1.0e8 \
  --fail-on-solve-time-us 1400
```
- `--bench-path` を指定すると make せずに任意ビルド済みバイナリを実行できる。  
- `--csv-validation` は `off / warn / fail`。CI と同条件を再現するには `fail` を推奨。  
- `--csv-validation-jsonl` へパスを渡すと検出した CSV 警告を JSON Lines 形式で蓄積できる。  
- `--fail-on-*` は CLI で一時的にしきい値を上書きする際に利用する。共有値は `config/coupled_benchmark_thresholds.yaml` に保持する。

**出力例（`data/coupled_benchmark_metrics.csv` 先頭行）**
```
run_id,avg_solve_time_us,max_condition,max_pending_steps,unrecovered_drops
2025-11-08T12:01:44Z,812.4,5.31e+05,0,0
```
`logs/csv_issues.jsonl` には検出された WARN が 1 行ずつ JSON で追記されるため、CI では `tail -n +1 logs/csv_issues.jsonl` で差分を確認する。

**しきい値のみ WARN 通知したい場合**
```bash
python3 tools/run_coupled_benchmark.py \
  --config config/coupled_benchmark_thresholds.yaml \
  --csv-validation warn \
  --output data/coupled_benchmark_metrics_warn.csv
```
`--csv-validation warn` は値を記録しつつ終了コード 0 を返す。CI で失敗させたい場合は `fail` を使う。

### D.3 静的サイト生成
```bash
python3 tools/build_coupled_benchmark_site.py \
  --output-dir site \
  --copy-data \
  --latest 12 \
  --threshold-config config/coupled_benchmark_thresholds.yaml \
  data/coupled_benchmark_metrics.csv
```
`site/index.html` に Chart.js の可視化を出力し、最新の CSV を `site/data/` へコピーする。Pages へデプロイする際はこのディレクトリをアップロードする。

**主要オプション**
- `--latest` – 直近 N 件のみを読み込む。履歴が多い場合は 12–24 を推奨。  
- `--copy-data` – 元 CSV を `site/data/` へコピーし、ダウンロードリンクを自動生成。  
- `--threshold-config` – サイト内の説明欄にしきい値ファイルへのリンクを表示する。  
- 位置引数で複数 CSV やグロブを渡すと、`bench_*.csv` をまとめて集計する。

**生成ファイル（抜粋）**
- `site/index.html` – KPI 折れ線／棒グラフ、しきい値リンク、生成日時。  
- `site/data/coupled_benchmark_metrics.csv` – コピー元 CSV。`--copy-data` 無指定の場合は作成されない。  
- `site/assets/app.js` – Chart.js を含むバンドル。必要に応じて差分をレビュー。

### D.4 継続デプロイ（CI）
- `.github/workflows/coupled_benchmark.yml` が `run_coupled_benchmark.py` → `build_coupled_benchmark_site.py` → Pages デプロイの流れを自動化。  
- 閾値は `config/coupled_benchmark_thresholds.yaml` を共通化し、ローカル実行と CI のズレを防ぐ。  
- 失敗時は `data/coupled_benchmark_metrics.csv` を添付してレビューを依頼する。

---

## E. Learning Path Integration Plan
`docs/coupled_constraint_hands_on.md` と `docs/coupled_constraint_tutorial_draft.md` は内容が部分的に重複しているため、以下のステップで統合を進める。章対応表とマイルストンは `docs/integration/learning_path_map.md` のドラフトでも管理する。

1. **章対応表の整備** – Tutorial §1–4 と Hands-on Chapter 01–04 を 1 対 1 で紐付け、差分（演習コード／CSV 出力など）を `integration/learning_path_map.md`（新規予定）に記録。  
2. **共通テンプレの抽出** – Hands-on の「Theory → Implementation → Verification」枠を Tutorial に流用し、実装ガイドは Tutorial、本番演習は Hands-on に集約する。  
3. **Appendix 参照化** – 実行ログ／スクリーンショットなど運用寄りの記述は Appendix A/B に移し、学習パスは数値・API 解説に限定する。  
4. **検証サイクル** – Appendix B.7 のリンク検証と同時に、Hands-on/Tutorial 双方で更新が必要な箇所をチェックリスト化する。  
5. **移行完了条件** – Hands-on へ移した演習コードを Tutorial から参照するだけになった段階で `docs/documentation_changelog.md` に統合完了を記録。

### E.1 Link Check Automation
1. `python scripts/check_doc_links.py docs/coupled_constraint_tutorial_draft.md docs/coupled_constraint_hands_on.md` を実行し、`docs/` 配下のリンク切れが無いか確認。  
2. CI では上記コマンドを `make lint-docs` などに組み込み、失敗時は Appendix B.7 のチェックボックスを未完にして次担当へ引き継ぐ。  
3. 追加の Markdown を検証したい場合はコマンド末尾にパスを追加する。`--repo-root` で別ツリーにも対応可能。  
4. スクリプトが欠落ファイルを報告したら `docs/documentation_changelog.md` に原因と修正内容を記録し、該当リンクを更新する。
5. GitHub Actions 例:
   ```yaml
   - name: Lint docs links
     run: python scripts/check_doc_links.py \
            docs/coupled_constraint_tutorial_draft.md \
            docs/coupled_constraint_hands_on.md \
            docs/coupled_contact_test_notes.md
   ```
   これが失敗した場合は Appendix B.7 の「リンク検証」チェックを未完にし、エラー出力を PR に貼り付けて再レビューを依頼する。
6. 失敗例と対処:
   ```
   Broken links detected:
     - docs/coupled_constraint_tutorial_draft.md: missing docs/foo.md
   ```
   - 対応①: リンク先ファイルを追加。  
   - 対応②: 不要リンクであれば記述を削除し、再度スクリプトを実行。  
   - 対応③: 修正が完了したら `docs/documentation_changelog.md` にリンク更新を記録し、Slack `#chrono-docs` へ報告。

> 上記プランは 2025-11-08 版のドラフト。Cチームは Appendix B.5 のローテーションに合わせて進捗をレビューし、統合用の追加ファイルを作成する。

> 本付録に掲載された内容は、ドキュメント本編から参照リンクで案内しています。計算コアに関連する作業と切り離したい場合は、本ファイルのみを別チャネルで管理してください。
