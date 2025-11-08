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

### A.3 Preset Cheat Sheet PDF
`docs/coupled_constraint_presets_cheatsheet.md` を Pandoc + XeLaTeX で PDF 化し、`docs/media/coupled/presets.pdf` へ保存してから Wiki / Appendix から参照する。

```bash
mkdir -p docs/media/coupled
pandoc docs/coupled_constraint_presets_cheatsheet.md \
  -o docs/media/coupled/presets.pdf \
  --pdf-engine=xelatex \
  -V mainfont="Noto Sans CJK JP"
```
- 依存: `pandoc`, `texlive-latex-extra`, `texlive-fonts-recommended`。  
- 生成した PDF は Appendix A/B や Wiki のクイックリンクに貼り付け、更新日と担当を `docs/media/coupled/README.md` に追記する。  
- GIF/MP4 と同様に `du -h docs/media/coupled/presets.pdf` でサイズを確認し、20 MB 未満に保つ。

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
* [チートシート PDF|<社内ストレージ>/coupled_constraint_presets_cheatsheet.pdf]
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
- [ ] `docs/pm_status_YYYY-MM-DD.md` と `docs/coupled_island_migration_plan.md` の KPI 表を同じ値に更新し、B.5.1 の担当欄を埋めた。  
- [ ] Contact + Coupled テスト運用（B.6）でログ解析／通知分岐をレビューした。  
- [ ] リンク検証チェックリスト（B.7）を実行し、結果を記録した。

### B.4 公開プロセス
1. PR マージ後 24 時間以内に Wiki を更新。  
2. スクリーンショット／GIF を添付しプレビュー確認。  
3. 「最終更新日」「同期コミット SHA」「関係者」を明記。  
4. 更新完了を Slack で通知し、担当者フィールドを更新。  
5. 次担当者へタスクを割り当て、次回レビュー予定日を設定。

#### B.4.1 Wiki 更新棚卸し（直近 4 週）
| 週 (開始日) | 担当 | B.3 チェック | B.4 公開プロセス | 備考 |
|-------------|------|--------------|-------------------|------|
| 2024-07-22 | Suzuki | ✅ KPI 反映 / メディア更新済み | ✅ PR #482 で Wiki 同期、Slack 通知完了 | `docs/media/coupled/endurance_stride5.gif` を差し替え |
| 2024-07-29 | Mori | ✅ summary / 画面キャプチャ更新 | ✅ B.4 手順に沿って告知 | Appendix B.5 ローテーション表を wiki 記事へ貼付 |
| 2024-08-05 | Kobayashi | ⚠️ スクリーンショット再確認を 24h 遅延 | ✅ 公開・通知済み | 遅延内容を #chrono-docs へ共有し、再発防止策を追加 |
| 2024-08-12 | Suzuki | ✅ KPI & メディア更新、引き継ぎメモ作成 | ✅ Nightly 差分と Webhook ログを追記 | ENDURANCE_ALERT_WEBHOOK の検証結果を wiki と Appendix C へ転記 |

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

#### B.5.1 KPI Update Rotation
| 曜日 | 対象ドキュメント | 主担当 | バックアップ | メモ |
|------|------------------|--------|--------------|------|
| 月曜 | `docs/pm_status_2024-11-08.md`（週次ステータス最新号をコピーして運用） | Mori | Suzuki | `docs/coupled_island_migration_plan.md` の KPI と揃えること。 |
| 水曜 | `docs/coupled_island_migration_plan.md` KPI 表 (§5.1) | Kobayashi | Tanaka | `docs/pm_status_YYYY-MM-DD.md` と数値一致を確認。 |
| 金曜 | Appendix B.5 ローテーション表／Slack 通知履歴 | Suzuki | Mori | 週次ログが揃っているかをチェックし、必要なら週報へ転記。 |

> KPI 更新を実施したら日付と担当を B.5.1 の表かコメント欄に残し、`docs/documentation_changelog.md` へまとめて通知する。Slack では `#chrono-docs` と `#chrono-constraints` の両方へ共有する。

### B.6 Contact + Coupled Test Operations
1. **ログ抽出** – `./chrono-C-all/tests/test_island_parallel_contacts --dump=log.json` を実行し、`tools/filter_coupled_endurance_log.py log.json --output log_contact.csv --keep contact_impulse,diagnostics_flags` で必要カラムのみに絞る。  
2. **条件分岐**  
   - `condition_warning` あり & `contact_impulse` が連続増加 → Slack `#chrono-constraints` に *Contact saturation* テンプレで通知。  
   - `diagnostics.rank != equations_active` → `docs/coupled_contact_test_notes.md` のチェックリストを参照し、`tests/test_coupled_constraint` ログとの突き合わせを要求。  
3. **通知フォーマット** – Appendix C のログテンプレに加え、Contact 付きかどうかを `tag=CONTACT+COUPLED` で明示。  
4. **週次レビュー** – `docs/coupled_contact_test_notes.md` の判定指標表を更新し、`docs/wiki_coupled_endurance_article.md` からのリンクを確認。  

| 状況 | Slack テンプレ | 追加対応 |
|------|----------------|----------|
| 高荷重で WARN 連発 | `[contact-saturation] run=<id> max_condition=...` | `ratio_distance` を一時的に 10% 下げる案を提示。 |
| ランク欠損のみ発生 | `[rank-mismatch] run=<id> eq_active=... rank=...` | Island 割当ログを `chrono_island2d_build` から採取し、PoC ガントへ共有。 |
| Contact 反力発散 | `[contact-divergence] run=<id> impulse=...` | `target_offset` ステージングを Hands-on Chapter 03 に沿って再設定。 |

### B.7 Link Validation Checklist
このチェックリストは `docs/coupled_constraint_tutorial_draft.md` のバイリンガル節と関連ドキュメント（Hands-on / Solver Math / Contact Notes）を対象にする。

1. `rg -n "docs/coupled_constraint_hands_on.md" docs/coupled_constraint_tutorial_draft.md` でリンク表記が最新か確認。  
2. `python - <<'PY'` スニペットで参照ファイルの存在を検査（`Path("docs/...").is_file()`）。  
3. Hands-on / Solver Math / Contact Notes 側で該当節の見出しが変更されていないか `git diff --stat HEAD~` を併せて確認。  
4. `docs/documentation_changelog.md` の最新エントリにリンク検証日と担当を追記。  
5. 不整合が見つかった場合は、チュートリアルと Hands-on の両方に統合案（E 章参照）を適用する。

> 2025-11-08 時点で `docs/coupled_constraint_tutorial_draft.md` → Hands-on / Solver Math / Contact Notes のリンクは確認済み。次回は Appendix B.5 のローテーションに従って更新する。

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

### D.4 継続デプロイ（CI）
- `.github/workflows/coupled_benchmark.yml` が `run_coupled_benchmark.py` → `build_coupled_benchmark_site.py` → Pages デプロイの流れを自動化。  
- 閾値は `config/coupled_benchmark_thresholds.yaml` を共通化し、ローカル実行と CI のズレを防ぐ。  
- 失敗時は `data/coupled_benchmark_metrics.csv` を添付してレビューを依頼する。

---

## E. Learning Path Integration Plan
`docs/coupled_constraint_hands_on.md` と `docs/coupled_constraint_tutorial_draft.md` は内容が部分的に重複しているため、以下のステップで統合を進める。

1. **章対応表の整備** – Tutorial §1–4 と Hands-on Chapter 01–04 を 1 対 1 で紐付け、差分（演習コード／CSV 出力など）を `integration/learning_path_map.md`（新規予定）に記録。  
2. **共通テンプレの抽出** – Hands-on の「Theory → Implementation → Verification」枠を Tutorial に流用し、実装ガイドは Tutorial、本番演習は Hands-on に集約する。  
3. **Appendix 参照化** – 実行ログ／スクリーンショットなど運用寄りの記述は Appendix A/B に移し、学習パスは数値・API 解説に限定する。  
4. **検証サイクル** – Appendix B.7 のリンク検証と同時に、Hands-on/Tutorial 双方で更新が必要な箇所をチェックリスト化する。  
5. **移行完了条件** – Hands-on へ移した演習コードを Tutorial から参照するだけになった段階で `docs/documentation_changelog.md` に統合完了を記録。

> 上記プランは 2025-11-08 版のドラフト。Cチームは Appendix B.5 のローテーションに合わせて進捗をレビューし、統合用の追加ファイルを作成する。

> 本付録に掲載された内容は、ドキュメント本編から参照リンクで案内しています。計算コアに関連する作業と切り離したい場合は、本ファイルのみを別チャネルで管理してください。
