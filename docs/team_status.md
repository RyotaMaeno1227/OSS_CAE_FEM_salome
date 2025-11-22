# チーム完了報告（A/B/Cそれぞれ自セクションのみ編集）

## Aチーム
- 実行タスク: A1, A3（部分）, A4, A10  
  - Run ID: local-chrono2d-20251118-01（ローカルテストのみ、Artifacts未コミット）  
  - A1: `--threads` で OpenMP on/off/任意スレッド比較し、pivot/cond 差分を自動チェック（許容1e-6）。  
  - A3: 摩擦端点（ゼロ摩擦/高速/低法線）と質量比1:100ケースをデータセット・回帰に追加。複合拘束は未着手。  
  - A4: ベンチのスレッドスイープ(1/2/4/8)を本番化、baseline比1.5x超を警告/失敗切替できるよう `--warn-only` 追加。  
  - A10: `--dump-json`/`--verbose` 拡張で異常系を最小再現JSONに出力（cond/pivot/vn/vt/µs/µd/stick, J行等）。  
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
- 実行タスク: C1, C3, C6, C9, C16, C17（今回アサイン分、着手前の記録）  
  - C1: chrono-2d README/Hands-on を月次更新し、最新 Run ID / CSV サンプル差し替え＋ Changelog 記録を実施予定。  
  - C3: chrono-2d/chrono-main/Chrono C 用 Run ID ワンライナーをチャットテンプレへ組み込み、混在防止を徹底。  
  - C6: リンク/整合チェック手順を README に明記し、実行結果をチャット共有する運用を固める。  
  - C9: CSV スキーマ説明とサンプル生成手順をテンプレ化し、生成スクリプトで最新化する流れを文書化。  
  - C16: Hands-on で得た Run ID/CSV の反映先とチェックリストを整理（README/ハンドオフ/Changelog を同時更新）。  
  - C17: 逸脱/異常時の連絡テンプレ（何を貼るか、送付先）をチャット用に整備。  
- 進捗（今回実施分）: C1/C3/C6/C17 を一部更新。  
  - `docs/abc_team_chat_handoff.md` に Run ID ワンライナーと異常時テンプレを追加（C3/C17）。  
  - `docs/chrono_2d_readme.md` に月次更新サイクル、チャット共有ワンライナー、リンク/整合チェック手順を追記（C1/C6）。  
  - `docs/git_setup.md` に chrono-2d リンクチェック手順を追記（C6）。  
  - `docs/documentation_changelog.md` に今回の更新を記録。  
  - リンクチェック: `tools/check_doc_links.py` が未存在で実行不可。  
  - スキーマチェック: `python tools/check_chrono2d_csv_schema.py --csv chrono-2d/artifacts/kkt_descriptor_actions_local.csv` 実行 → Header mismatch（CSV 未整合）。Run ID/CSV 更新は未実施のため次回対応。  
