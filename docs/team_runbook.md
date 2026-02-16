# チーム別 Runbook（現行運用）

最終更新: 2026-02-08  
対象: PM-3 / Aチーム / Bチーム / Cチーム

## 1. 目的
- 現在のプロジェクト目標（`docs/long_term_target_definition.md`）に沿って、A/B/C の日次運用を統一する。
- 実装対象を `FEM4C` の現行スプリントへ限定し、旧 Chrono 運用の誤参照を防ぐ。

## 2. 参照優先順位（必須）
1. 長期目標: `docs/long_term_target_definition.md`
2. 現行ディスパッチ: `docs/abc_team_chat_handoff.md`（Section 0 のみ）
3. 次タスクキュー: `docs/fem4c_team_next_queue.md`
4. 進捗報告: `docs/team_status.md`
5. 継続ログ: `docs/session_continuity_log.md`

## 3. スコープ
- In Scope:
  - FEM ソルバー維持・補強（FEM4C）
  - 2D MBD コア移植（Project Chrono コア）
  - 共通 parser 契約の整備
  - 学習用ドキュメント強化
- Out of Scope（当面）:
  - FEM-MBD 連成本実装
  - `coupled` モードの新規機能追加（連成仕様確定まで凍結）
  - 3D MBD 本実装
  - 通知運用・Nightly 周辺オペレーション

## 4. チーム運用ルール
- チャット指示が「作業を継続してください」のみの場合:
  - これを「省略指示モード」と定義する。
  - 各チームは追加質問なしで、次の順に自律実行する:
    1. `docs/abc_team_chat_handoff.md` の Section 0 を確認
    2. `docs/fem4c_team_next_queue.md` の自チーム先頭 `Todo` / `In Progress` を着手対象として確定
    3. 着手タスクを `In Progress` に更新して実行開始
  - PM への確認は blocker 発生時のみ許可する（「次に何をやるか」の確認は不要）。
- 自走セッション（短時間・連続運転）:
  - 1回の指示で 30分以上を必須とし、30-45分を推奨レンジとして連続実行する。
  - 本運用は「30分開発モード」とし、30分は実装前進（コード差分作成）に使う。
  - 1セッションで少なくとも1つの実装系ファイル差分を必須とする（docs単独更新での完了は禁止）。
  - 検証は短時間スモークを基本とし、長時間の反復ソーク/耐久ループは PM 明示指示がない限り禁止する。
  - 目安: 実装20分以上、検証10分以下。検証は最大3コマンド程度に抑える。
  - 動的自走プロトコル（必須）:
    - 先頭タスクが早く終わったら、同一セッション内で次の `Todo` / `In Progress` へ自動遷移する（PM確認不要）。
    - 次タスク候補が空なら、同一スコープ内で「最小実装タスク（Auto-Next）」を自分で定義して継続する。
    - Auto-Next は `Goal / Scope / Acceptance` を `docs/fem4c_team_next_queue.md` に追記し、`In Progress` にして着手する。
    - 同じ検証コマンドの反復実行だけで時間を使わない（コード変更なしの連続反復は禁止）。
  - 進捗連絡は原則セッション末尾に 1 回のみ（小分け報告をしない）。
  - 先頭タスクが早く終わった場合は、同じセッション内で次タスクへ連続着手する。
  - PM 判断が必要な blocker が出ても、30分未満の時点では終了せず、同一セッション内で次の実行可能タスクへ継続する。
  - 30分未満で終了できる例外は、PMが事前承認した緊急停止（環境障害/誤破壊リスク）のみとする。
  - `sleep` 等の待機コマンドで elapsed を稼ぐ行為を禁止する（人工待機禁止）。
- セッション時間の証跡（必須）:
  - 手入力の `start_at/end_at/elapsed_min` は証跡として無効とする。
  - 開始時に `scripts/session_timer.sh start <team_tag>` を実行し、`session_token` を取得する。
  - 報告前に `bash scripts/session_timer_guard.sh <session_token> 30` を実行し、`guard_result=pass` を確認する。
  - `guard_result=block` の間は報告せず、同一セッションで実装を継続する。
  - 終了時に `scripts/session_timer.sh end <session_token>` を実行し、出力（`start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）を `docs/team_status.md` にそのまま貼る。
  - 受入には `elapsed_min >= 30` を必須とし、あわせて実作業証跡（変更ファイル・実行コマンド・pass/fail根拠）を確認する。
  - 30分未満で先頭タスクが完了した場合は、待機せず次タスク着手または blocker 解消作業へ進む。
  - `elapsed_min < 30` の途中報告は禁止（継続中であることを前提に同セッションで実装を続ける）。
  - blocker 終了時は「試した対応」「失敗理由」「PMに必要な判断」を 3 点セットで記録する。
- 作業終了時は必ず以下を更新する:
  - `docs/team_status.md`
  - `docs/session_continuity_log.md`（4項目: `Current Plan`, `Completed This Session`, `Next Actions`, `Open Risks/Blockers`）
- `docs/team_status.md` の記録位置:
  - A/B/Cチームは自チーム見出し配下（`## Aチーム` / `## Bチーム` / `## Cチーム`）にのみ追記する。
  - PM記録は `## PMチーム` 見出し配下へ追記し、A/B/Cセクションへ混在させない。
- コミット時の制約:
  - 担当外ファイルをステージしない。
  - 生成物（`*.dat`, `*.csv`, `*.vtk`, `*.f06`）をコミットしない。
  - `FEM4C` と `chrono-2d` の混在コミットを禁止する。

## 5. A/B/C の責務（現行スプリント）
- Aチーム（実装）:
  - `FEM4C/src/analysis/runner.*` と `FEM4C/src/mbd/*` を中心に、`mbd` 実行経路を段階実装する。
- Bチーム（検証）:
  - `practice/ch09` の拘束検証ハーネスと有限差分照合を維持・拡張する。
- Cチーム（差分整理）:
  - dirty 差分の 3分類（残す/除外/保留）と安全ステージング手順を維持する。

## 6. 受入・検証
- 各タスクの受入条件は `docs/fem4c_team_next_queue.md` の `Acceptance` を一次基準とする。
- 受入に使うコマンド・結果（pass/fail）は `docs/team_status.md` へ記録する。
- 外部CI接続制約があるため、日次受入はローカル完結を標準とする:
  - `make -C FEM4C test`
  - `make -C FEM4C mbd_ci_contract`
  - `make -C FEM4C mbd_ci_contract_test`
- GitHub Actions 実Run確認は毎セッション必須ではない。PM/ユーザーが節目で数回のみスポット実施する。
- PM受入時は最新エントリの機械監査を実行する:
  - `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30`
  - 実装差分必須を同時監査する場合: `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30 --require-impl-changes`
  - Cチーム staging 運用の遵守監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze`
  - Cチーム staging + タイマー完了の厳格監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer`
  - Cチーム staging + タイマー完了 + safe staging 記録 + テンプレ残骸なしの厳格監査: `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer_safe`
  - 差し戻し文面まで一括生成する場合:
    - `bash scripts/run_team_audit.sh docs/team_status.md 30 pass_section_freeze`
    - 実装差分必須を同時監査する場合: `TEAM_AUDIT_REQUIRE_IMPL_CHANGES=1 bash scripts/run_team_audit.sh docs/team_status.md 30 pass_section_freeze`
  - 遵守率の履歴確認（原因分析）:
    - `python scripts/audit_team_history.py --team-status docs/team_status.md --min-elapsed 30`
  - 監査レポートの保存先（例）:
    - `docs/reports/team_session_compliance_audit_2026-02-08.md`
- 以下のいずれかに該当する報告は差し戻す:
  - `scripts/session_timer.sh` の出力証跡が未記載
  - `elapsed_min < 30`（PM事前承認の緊急停止を除く）
  - 人工待機（`sleep` 等）で elapsed を満たした痕跡がある
  - 実作業証跡（変更ファイル・実行コマンド・pass/fail）が不足している
  - `Done` 0件かつ次タスク `In Progress` なし
- 外部CI未接続を理由に日次のローカル検証を省略している

### 6.1 Cチーム staging dry-run（定型）
- 目的:
  - `FEM4C` と `chrono-2d` の混在ステージを、実 index を汚さずに毎回同じ手順で検査する。
- 手順:
  - `GIT_INDEX_FILE` を一時 index に切替える。
  - 対象ファイル（3実装 + C docs）のみ `git add` した後、`git diff --cached --name-status` を取得する。
  - 可能な限り `scripts/c_stage_dryrun.sh` を使用し、同一フォーマットで記録する。
- 判定:
  - `forbidden_check`: staged set に `chrono-2d/` と `.github/` が無いこと。
  - `required_set_check`: 必須対象セットがすべて staged set に含まれること。
  - `safe_stage_command`: `git add <path-list>` 形式で、`safe_stage_targets` と一致すること（`check_c_stage_dryrun_report.py --policy pass` で検査）。
  - strict-safe（timer完了監査）では最新Cエントリに `<pending>` / `token missing` / `<記入>` / `<PASS|FAIL>` などのテンプレ残骸が残っていないこと。
  - 上記2条件を満たした場合のみ `dryrun_result=pass` とする。
- 記録フォーマット（`docs/team_status.md`）:
  - `dryrun_method=GIT_INDEX_FILE`
  - `dryrun_cached_list=<name-status>`
  - `forbidden_check=pass|fail`
  - `coupled_freeze_file=<path>`
  - `coupled_freeze_hits=<path-list|->`
  - `coupled_freeze_check=pass|fail`
  - `required_set_check=pass|fail`
  - `safe_stage_targets=<path-list>`
  - `safe_stage_command=git add <path-list>`
  - `dryrun_result=pass|fail`
- 参考コマンド:
  - coupled凍結禁止パス定義: `scripts/c_coupled_freeze_forbidden_paths.txt`
  - coupled凍結禁止パス定義の検査: `python scripts/check_c_coupled_freeze_file.py scripts/c_coupled_freeze_forbidden_paths.txt`
  - `scripts/c_stage_dryrun.sh --log /tmp/c_stage_dryrun_<date>.log`
  - `python scripts/check_c_stage_dryrun_report.py /tmp/c_stage_dryrun_<date>.log --policy pass`（dry-runログ契約検査）
  - `python scripts/render_c_stage_team_status_block.py /tmp/c_stage_dryrun_<date>.log`（`team_status` へ貼る dry-run 記録ブロック生成）
  - `python scripts/render_c_stage_team_status_block.py /tmp/c_stage_dryrun_<date>.log --output /tmp/c_stage_team_status_block.md`（貼り付け用ファイルも同時生成）
  - `python scripts/apply_c_stage_block_to_team_status.py --team-status docs/team_status.md --block-file /tmp/c_stage_team_status_block.md --in-place`（最新Cエントリへ生成ブロックを適用）
  - `python scripts/apply_c_stage_block_to_team_status.py --team-status docs/team_status.md --block-file /tmp/c_stage_team_status_block.md --target-start-epoch <start_epoch> --in-place`（適用先を明示）
  - `python scripts/render_c_team_session_entry.py --task-title "<task>" --session-token <token> --timer-end-file <end_file> --timer-guard-file <guard_file> --dryrun-block-file /tmp/c_stage_team_status_block.md`（セッション記録エントリ雛形を生成）
  - `python scripts/render_c_team_session_entry.py --task-title "<task>" --session-token <token> --timer-end-file <end_file> --timer-guard-file <guard_file> --dryrun-block-file /tmp/c_stage_team_status_block.md --c-stage-dryrun-log /tmp/c_stage_dryrun.log`（strict-safe 用の dry-run コマンド証跡も同時出力）
  - `python scripts/render_c_team_session_entry.py --task-title "<task>" --session-token <token> --timer-end-file <end_file> --timer-guard-file <guard_file> --done-line "<done>" --in-progress-line "<next>" --command-line "<cmd> -> PASS" --pass-fail-line "PASS（...）"`（Done/In Progress/command/pass-fail を雛形へ直接埋め込む）
  - `python scripts/render_c_team_session_entry.py --task-title "<task>" --session-token <token> --collect-timer-end --collect-timer-guard --timer-end-output /tmp/c_team_timer_end.txt --timer-guard-output /tmp/c_team_timer_guard.txt --dryrun-block-file /tmp/c_stage_team_status_block.md`（end/guard出力を自動取得して雛形生成）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --entry-out /tmp/c_team_session_entry.md`（dry-run + guard + end + 雛形生成を一括実行）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --collect-preflight-log /tmp/c_team_collect.log`（生成エントリに collect preflight ログ証跡を埋め込む）
  - `bash scripts/collect_c_team_session_evidence.sh ... > /tmp/c_team_collect.log && python scripts/check_c_team_collect_preflight_report.py /tmp/c_team_collect.log`（collect 出力の preflight 契約を検査）
  - `python scripts/append_c_team_entry.py --team-status docs/team_status.md --entry-file /tmp/c_team_session_entry.md --in-place`（生成済み雛形を Cセクションへ追記）
  - `python scripts/mark_c_team_entry_token_missing.py --team-status docs/team_status.md --target-start-epoch <start_epoch> --token-path <missing_token> --in-place`（token missing 旧エントリを無効化）
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --target-start-epoch <start_epoch> --token-path <missing_token> --new-team-tag c_team`（旧エントリ無効化 + 新規timer開始を一括実行、`next_finalize_command` を出力）
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <session_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe`（復旧セッションの証跡収集 + Cエントリ追記 + strict-safe確認を一括実行）
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <session_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --collect-log-out /tmp/c_team_collect.log`（collect出力を保存し、preflight契約チェックまで一括実行）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --entry-out /tmp/c_team_session_entry.md --team-status docs/team_status.md --append-to-team-status`（収集から追記まで一括実行）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --team-status docs/team_status.md --append-to-team-status --check-submission-readiness-minutes 30`（validation用 `team_status` で preflight 監査し、PASS時のみ本番へ追記）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --team-status docs/team_status.md --append-to-team-status --check-submission-readiness-minutes 30 --collect-latest-require-found 1`（latest 解決不能/契約不一致を fail-fast する厳格提出モード）
  - `bash scripts/collect_c_team_session_evidence.sh --task-title "<task>" --session-token <token> --guard-minutes 30 --team-status docs/team_status.md --check-compliance-policy pass_section_freeze_timer_safe`（appendせず preflight 判定のみ実行）
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <session_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30`（token-missing 復旧セッションの最終反映 + 提出前ゲートを一括実行）
  - `bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token <session_token> --task-title "<task>" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1`（復旧 finalize を strict latest fail-fast で実行）
  - C-34 以降の提出テンプレは上記 2 コマンド（collect / recover finalize）の `--collect-latest-require-found 1` を既定とする。
  - `scripts/render_c_team_session_entry.py` が出力する `preflight_latest_require_found=0|1` を `team_status` の実行証跡として扱う。
  - strict 失敗時は `collect_preflight_check_reason=*` を `team_status` に転記し、fail要因（`latest_not_found_strict` / `latest_invalid_report_strict`）を明示する。
  - `python scripts/check_c_team_collect_preflight_report.py /tmp/c_team_collect.log --require-enabled`（collectログの preflight 契約検証）
  - `python scripts/check_c_team_collect_preflight_report.py /tmp/c_team_collect.log --require-enabled --expect-team-status docs/team_status.md`（preflightログの `preflight_team_status` が提出対象と一致していることを検証）
  - `C_TEAM_SKIP_STAGING_BUNDLE=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30`（テスト時のみ staging bundle を省略して elapsed 監査を確認）
  - `scripts/c_stage_dryrun.sh --add-target chrono-2d/tests/test_coupled_constraint`（forbidden fail の再現確認）
  - `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass`（最新C報告の dry-run 記録監査）
  - `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section`（C報告が `## Cチーム` 配下にあることも監査）
  - `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze`（`## Cチーム` 配下 + coupled凍結ポリシー監査）
  - `COUPLED_FREEZE_FILE=/tmp/custom_freeze_paths.txt bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze`（監査禁止パスを一時差し替え）
  - `python scripts/audit_c_team_staging.py --team-status docs/team_status.md --coupled-freeze-file scripts/c_coupled_freeze_forbidden_paths.txt --print-coupled-freeze-patterns`（監査に使う禁止パス一覧の確認）
  - `bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（C-team staging監査 + 関連テスト一括実行）
  - `C_TEAM_STATUS_BLOCK_OUT=/tmp/c_stage_team_status_block.md bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（生成ブロックの出力先を指定）
  - `C_APPLY_BLOCK_TO_TEAM_STATUS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（生成ブロックを最新Cエントリへ自動適用）
  - `C_DRYRUN_POLICY=pass_section_freeze_timer bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（タイマー完了まで含む厳格監査）
  - `C_DRYRUN_POLICY=pass_section_freeze_timer_safe bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（タイマー完了 + safe staging記録まで含む厳格監査）
  - `C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（collect preflight 契約ログを bundle 実行内で検証）
  - `C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（collectログの team_status 一致まで bundle で検証）
  - `C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md`（collect preflight 契約検証だけを単独実行）
  - `C_COLLECT_PREFLIGHT_LOG=latest bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md`（最新Cエントリから collect ログを自動解決して検証）
  - `C_COLLECT_PREFLIGHT_LOG=latest` は、解決先ログが preflight 契約を満たさない場合も既定は `collect_preflight_check=skipped`（日次運用を停止しない）。
  - latest 既定skip/strict fail の分岐理由は `collect_preflight_check_reason=*`（`latest_not_found_*`, `latest_invalid_report_*`）で追跡する。
  - `collect_c_team_session_evidence.sh` は preflight 判定理由が検出できた場合、`collect_preflight_check_reason=*` を `team_status` エントリの実行コマンド欄へ自動転記する。
  - latest 候補が複数ある場合は `check_c_team_collect_preflight_report.py <log>` の明示コマンド由来を優先し、`collect_log_out` 由来候補より先に採用する。
  - `C_COLLECT_PREFLIGHT_LOG=latest C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/run_c_team_collect_preflight_check.sh docs/team_status.md`（latest 解決不能または契約不一致を FAIL にする厳格モード）
  - `C_SKIP_NESTED_SELFTESTS=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md`（nested self-test を省略して staging/preflight 契約だけを短時間確認）
  - `run_c_team_staging_checks.sh` は nested self-test 実行前に `C_COLLECT_LATEST_REQUIRE_FOUND` をクリアし、strict提出モード環境変数が自己テスト判定へ波及しないようにする。
  - `bash scripts/run_c_team_staging_checks.sh docs/team_status.md` は `C_COLLECT_PREFLIGHT_LOG` 未指定時に `latest` を自動解決（未検出は既定 skip）。
  - `C_COLLECT_PREFLIGHT_LOG= bash scripts/run_c_team_staging_checks.sh docs/team_status.md` で preflight 検証を明示的に無効化できる。
  - `C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/run_c_team_staging_checks.sh docs/team_status.md` で latest 解決不能/契約不一致を staging bundle 内で fail-fast できる。
  - `bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30`（strict-safe + C単独30分監査の提出前一括確認）
  - `bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30` も `C_COLLECT_PREFLIGHT_LOG` 未指定時は `latest` 自動解決（未検出は既定 skip）。
  - `C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30`（提出前ゲートで collect preflight 契約を同時検証）
  - `C_COLLECT_PREFLIGHT_LOG=/tmp/c_team_collect.log C_COLLECT_EXPECT_TEAM_STATUS=docs/team_status.md bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30`（提出前ゲートで preflightログの team_status 一致を必須化）
  - `C_COLLECT_LATEST_REQUIRE_FOUND=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30` で latest 解決不能/契約不一致を提出前ゲートで fail-fast できる。
  - `C_COLLECT_PREFLIGHT_LOG= C_TEAM_SKIP_STAGING_BUNDLE=1 bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30`（collect preflight 検証を明示的に無効化して最小監査だけ確認）

## 7. アーカイブ方針
- 旧 Chrono 運用・旧 PM 司令文書は以下へ退避済み（参照のみ）:
  - `docs/archive/legacy_chrono/`
- 実装前ドラフトは以下へ退避済み:
  - `docs/archive/drafts/`
- 現行運用では、上記アーカイブ配下をタスク指示の一次参照に使わない。

## 8. コンテクスト切れ時の新規チャット移行手順（必須）
- 適用条件:
  - チャットが長文化し、PMが継続運用リスクを感じた場合。
  - 途中中断やコンテクスト喪失で、同一チャット継続が不安定な場合。
- PMの移行前作業:
  1. `docs/session_continuity_log.md` の末尾に 4項目（`Current Plan` / `Completed This Session` / `Next Actions` / `Open Risks/Blockers`）を追記する。
  2. `docs/team_status.md` に PM判断（差し戻し/受入/次指示）を追記する。
  3. 必要なルール変更があれば、`docs/abc_team_chat_handoff.md` Section 0 と `docs/fem4c_team_next_queue.md` に先に反映する。
- 新規チャット開始時の初動:
  1. `docs/long_term_target_definition.md` と `docs/abc_team_chat_handoff.md` Section 0 を再確認する。
  2. `docs/fem4c_team_next_queue.md` の先頭 `In Progress` / `Todo` を起点に再開する。
  3. 各チームへの初回送信は `docs/fem4c_team_dispatch_2026-02-06.md` のテンプレを使う。
  4. 2回目以降は「作業を継続してください」の1行運用へ戻す。
- 差し戻しの固定基準:
  - `elapsed_min < 30`、または `scripts/session_timer.sh` 証跡なしは不受理。
  - 人工待機（`sleep` 等）で elapsed を満たした報告は不受理。
  - 不受理時は、同一タスク継続で再実行させる（タスク飛ばし禁止）。
