# チーム完了報告（A/B/Cそれぞれ自セクションのみ編集）

## Aチーム
- 実行タスク: A1, A2, A3, A4, A9, A10, A12（可視化雛形）  
  - Run ID: local-chrono2d-20251118-02（ローカルテストのみ、Artifacts未コミット）  
  - A1: `--threads` で OpenMP on/off/任意スレッド比較し、pivot/cond 差分を自動チェック（許容1e-6）。  
  - A2: dump-json/verbose を拡張し、J行・入力パラメータ（axis/anchors/contact/mass/inertia）・cond/pivot/vn/vt/µs/µd/stick を最小再現 JSON に含める。  
  - A3: 摩擦端点（ゼロ摩擦/高速/低法線）＋質量比1:100 ケースをデータセット/回帰に追加（複合拘束は未着手）。  
  - A4: ベンチスレッドスイープ(1/2/4/8)を本番化、baseline比1.5x超で警告/失敗を切替可能（`--warn-only`）。  
  - A9: 手計算ミニケース（pivot=0.5/cond=1 の gear 行）を `tests/test_minicase.c` で厳密比較。  
  - A10: 異常系ダンプ/復帰機構として dump-json に診断フィールドを集約。  
  - A12: ベンチ可視化スクリプト雛形 `tools/plot_bench.py` を追加（matplotlib 無でも要約表示）。  
  - 生成物: `artifacts/*.csv` は報告のみでコミットしていません。

## Bチーム
- 実行タスク: B1, B2, B4, B5, B13, B18
  - Run ID: 未取得（ワークフロー安定化中、次回 dispatch/cron 実行後に記載）
  - Artifacts: chrono-2d-ci-*（stable/experimental）、bench_drift.txt（実験版）、env.txt（安定版）
  - 備考: 安定/実験ワークフローを分離（dispatch/cron＋スキップタグ適用）、拡張スキーマチェックを fail 運用に移行、ベンチ 1.5x 警告ロジック実装済み（安定版への反映検討中）、timeout/リトライを導入しフレーク検証を開始、月次カバレッジ拡大計画・ベンチ履歴Markdown出力の枠組み設計中
- 実行タスク: B1, B2, B4, B5（安定版更新）
  - Run ID: 未取得（安定版/実験版ともに cron/dispatch 実行待ち）
  - Artifacts: chrono-2d-ci-${run}, artifacts_env.txt, bench_drift.txt（実験版 opt-in）
  - 備考: 安定版に timeout (20m/10m) と環境ログを追加、拡張スキーマを本番 fail 条件で維持。実験版に fail-on-drift オプションを追加し、compare_bench_csv.py で drift 時に exit 1 可能とした。

## Cチーム
- 実行タスク: C1, C3, C6, C9, C16, C17  
  - C1: chrono-2d README 月次更新手順・ワンライナー共有例を追記、サンプル CSV を最新スキーマ（vn/vt/µs/µd/stick 含む）に差し替え。  
  - C3: chrono-main/chrono-2d/Chrono C 用 Run ID 貼付ワンライナーを `docs/abc_team_chat_handoff.md` に追加。  
  - C6: リンク/整合チェック手順を README に追記（check_doc_links スクリプトはリポジトリ未収載のため未実行）。  
  - C9: CSV スキーマ説明と生成スクリプト (`tools/check_chrono2d_csv_schema.py --emit-sample ...`) を README に明記し、テンプレ (`docs/chrono_2d_cases_template.csv`) と同期。  
  - C16: 学習ステップチェックリストに反映先・リンクチェック・Changelog 記録までの手順を拡張。  
  - C17: 逸脱/異常時の連絡テンプレをチャット向けに整理。  
- Run/Artifacts: 実 Run なし（サンプル CSV のみ差し替え）。`python tools/check_chrono2d_csv_schema.py --csv chrono-2d/artifacts/kkt_descriptor_actions_local.csv` → OK。  
- リンクチェック: `scripts/check_doc_links.py` が存在せず未実行。  
