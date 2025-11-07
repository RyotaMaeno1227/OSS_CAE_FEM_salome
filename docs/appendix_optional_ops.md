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

### B.4 公開プロセス
1. PR マージ後 24 時間以内に Wiki を更新。  
2. スクリーンショット／GIF を添付しプレビュー確認。  
3. 「最終更新日」「同期コミット SHA」「関係者」を明記。  
4. 更新完了を Slack で通知し、担当者フィールドを更新。  
5. 次担当者へタスクを割り当て、次回レビュー予定日を設定。

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
  --config config/coupled_benchmark_thresholds.yaml \
  --csv-validation fail \
  --output data/coupled_benchmark_metrics.csv
```
- `--csv-validation` は `off / warn / fail`。CI と同条件を再現するには `fail` を推奨。  
- 警告・失敗は CI ワークフローと同じ書式で出力される。

### D.3 静的サイト生成
```bash
python3 tools/build_coupled_benchmark_site.py \
  --output-dir site \
  --copy-data \
  --latest 12 \
  data/coupled_benchmark_metrics.csv
```
`site/index.html` に Chart.js の可視化を出力し、最新の CSV を `site/data/` へコピーする。Pages へデプロイする際はこのディレクトリをアップロードする。

### D.4 継続デプロイ（CI）
- `.github/workflows/coupled_benchmark.yml` が `run_coupled_benchmark.py` → `build_coupled_benchmark_site.py` → Pages デプロイの流れを自動化。  
- 閾値は `config/coupled_benchmark_thresholds.yaml` を共通化し、ローカル実行と CI のズレを防ぐ。  
- 失敗時は `data/coupled_benchmark_metrics.csv` を添付してレビューを依頼する。

---

> 本付録に掲載された内容は、ドキュメント本編から参照リンクで案内しています。計算コアに関連する作業と切り離したい場合は、本ファイルのみを別チャネルで管理してください。

