# A/B/C チームチャット向け引き継ぎメモ

作成日: 2025-11-14  
現行運用: FEM4C スプリント（PM-3）

---

## 0. PM-3 優先ディスパッチ（2026-02-06, FEM4C）
この節は **今回スプリント専用の優先指示** です。
FEM4C スプリント中は **この Section 0 と `docs/fem4c_team_next_queue.md` だけを参照** してください。

- 対象スコープ: `FEM4C` の Phase 2（MBD最小実装）と安全な差分整理。
- 共通ルール:
  - 長期目標とスコープ定義は `docs/long_term_target_definition.md` を最優先で参照する。
  - コミットは担当範囲のファイルのみ。`FEM4C/test/*` 削除群や `chrono-2d` 差分を混在させない。
  - 生成物（`*.dat`, `*.csv`, `*.vtk`, `*.f06`）はコミットしない。
  - 作業終了時に `docs/team_status.md` と `docs/session_continuity_log.md` を更新する。
  - 連絡テンプレは `docs/fem4c_team_dispatch_2026-02-06.md` を使用する。
  - 個別チャットを送る場合は `docs/fem4c_team_dispatch_2026-02-06.md` の「PMレビュー後の次ラウンド指示（最新コピペ用）」を使う。
  - 継続運用（省略指示モード）: PMチャットが「作業を継続してください」のみの場合、追加指示待ちはせず、`docs/fem4c_team_next_queue.md` の自チーム先頭タスクから即時着手する。
  - 省略指示モードでは、タスク選定の問い合わせを禁止する（問い合わせ可能なのは blocker 発生時のみ）。
  - 無効報告ルール: `session_continuity_log` のみ更新して実装/検証差分がない報告は完了扱いにしない。
  - セッション時間の証跡として、`scripts/session_timer.sh start <team_tag>` と `scripts/session_timer.sh end <session_token>` の出力を `team_status` に必ず記載する（手入力時刻のみは無効）。
  - 報告前に `bash scripts/session_timer_guard.sh <session_token> 30` を実行し、`guard_result=pass` になるまで終了報告しない。
  - 受入には `elapsed_min >= 30` を必須とし、実作業証跡（変更ファイル・実行コマンド・pass/fail）を同時に満たすこと。
  - 30分は「開発前進」に使う。実装系ファイル差分（コード差分）を毎セッション必須とする。
  - 長時間の反復ソーク/耐久ループで時間を消費する運用は禁止（PM明示指示時のみ例外）。
  - 検証は短時間スモークに限定し、最大3コマンド程度で受入を確認する。
  - 動的自走プロトコル:
    - 先頭タスクを完了したら、同一セッション内で次の `Todo` / `In Progress` へ自動遷移する（PM確認不要）。
    - 次タスク候補が無い場合は、同一スコープで `Auto-Next`（最小実装タスク）を自分で定義し、`next_queue` へ追記して継続する。
    - 同一コマンドの反復実行のみで時間を使うことを禁止する（コード変更なしの連続反復は禁止）。
  - 30分未満で先頭タスクが完了した場合は、待機せず次タスクへ継続する（早期終了は原則不合格）。
  - `elapsed_min < 30` の途中報告を禁止し、同セッションで実装を継続する。
  - `sleep` 等の人工待機で elapsed を満たす行為は禁止し、不合格とする。
  - 完了報告の必須セット:
    - 変更ファイル（実装ファイルを含む）
    - 実行コマンド
    - 受入基準に対応した pass/fail 根拠
  - Cチーム staging 検証は `scripts/c_stage_dryrun.sh` を優先使用し、`dryrun_result` と `safe_stage_command=git add <path-list>` を `team_status` に記録する。
  - C-19 以降は `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze` を実行し、`## Cチーム` 配下の最新報告が coupled凍結ポリシー込みで PASS であることを確認する。
  - タイマー完了まで厳格に確認する場合は `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer` を使用する。
  - safe staging 記録まで厳格に確認する場合は `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer_safe` を使用する。
  - 次タスク遷移の優先順（先頭完了後の迷い防止）:
    - A: A-31 完了後は A-32 へ遷移
    - B: B-24 完了後は B-25 へ遷移
    - C: C-35 完了後は C-36 へ遷移
  - 上記の優先遷移先が完了済み/候補なしの場合は `Auto-Next` を `next_queue` に追記し、同一セッションで継続する。
  - PM決定（2026-02-07）:
    - `FEM4C/src/io/input.c` の旧 `SPC/FORCE` / `NastranBalkFile` 互換は維持する（Option A）。
    - 「旧形式を明示エラー化（Option B）」は現スプリントでは採用しない。
    - `FEM4C/src/solver/cg_solver.c` の零曲率閾値は `Option A`（`1.0e-14` 維持）を採用する。
    - `FEM4C/src/elements/t3/t3_element.c` は `Option B`（既定は自動補正 + `--strict-t3-orientation` で即エラー）を採用する。
    - B-8 の run_id共有必須運用は廃止し、日次受入は「CI導線の静的保証 + ローカル `make -C FEM4C test`」で判定する。
    - GitHub Actions 実ラン確認は毎セッション必須にせず、必要時のみスポット確認とする。
    - MBD 時間積分は `Newmark-β` と `HHT-α` の 2 種を実装対象とし、最終的に実行時スイッチで切替できるようにする。
  - PM決定（2026-02-08）:
    - 連成（`coupled`）仕様が未確定のため、`coupled` はスタブ維持・新規機能追加凍結とする。
    - 直近実装は FEM / MBD の独立ソルバー前進を最優先とする。
    - 外部CI未接続環境では、日次受入をローカル（`make -C FEM4C test` / `mbd_ci_contract` / `mbd_ci_contract_test`）で完結する。
    - GitHub Actions 実Run確認は PM/ユーザーが節目で数回のみスポット実施し、毎セッション必須にしない。
  - コンテクスト切れ/新規チャット移行時は `docs/team_runbook.md` の「8. コンテクスト切れ時の新規チャット移行手順」を必ず適用する。

### Aチーム（実装）
- 目的: `mbd` モードを独立ソルバーとして段階的に完成させる。
- 現在の先頭タスク: `A-32`（`docs/fem4c_team_next_queue.md` を正とする）
- 対象ファイル:
  - `FEM4C/scripts/run_a24_acceptance_serial.sh`
  - `FEM4C/scripts/test_run_a24_acceptance_serial.sh`
  - `FEM4C/scripts/check_ci_contract.sh`
  - `FEM4C/scripts/test_check_ci_contract.sh`
  - 必要時のみ `FEM4C/Makefile`
- 指示:
  1. A-32 の受入基準に沿って、A-24 serial acceptance の step-log 契約（step_log_dir/failed_log）を固定する（実装差分必須）。
  2. `mbd_a24_acceptance_serial_test` / `mbd_ci_contract_test` / `mbd_a24_acceptance_serial` の再入失敗を潰し、失敗時に原因ログ位置を 1 行サマリで追跡できる状態を維持する。
  3. 先頭完了後は `next_queue` の次タスクへ同一セッションで自動遷移する。
- 受入基準:
  - `docs/fem4c_team_next_queue.md` の A-32 `Acceptance` を満たすこと。
  - `docs/session_continuity_log.md` 以外に、少なくとも 1 つの実装ファイル差分があること。

### Bチーム（検証）
- 目的: B-8 系回帰ラッパーの運用安定性を固定する。
- 現在の先頭タスク: `B-25`（`docs/fem4c_team_next_queue.md` を正とする）
- 対象ファイル:
  - `FEM4C/scripts/test_run_b8_regression.sh`
  - `FEM4C/scripts/test_run_b8_regression_full.sh`
  - `FEM4C/scripts/test_run_b8_guard_contract.sh`
  - 必要時のみ `FEM4C/scripts/check_ci_contract.sh`
- 指示:
  1. B-25 の受入基準に沿って、B-8 自己テストの temp-copy ディレクトリノブ契約（`B8_TEST_TMP_COPY_DIR`）を固定する。
  2. 既定ディレクトリ、存在チェック、書込チェックと静的契約チェックを同期し、再入時の衝突余地を減らす。
  3. 先頭完了後は `next_queue` の次タスクへ同一セッションで自動遷移する。
- 受入基準:
  - `docs/fem4c_team_next_queue.md` の B-25 `Acceptance` を満たすこと。
  - 1行再現コマンドと pass/fail 根拠を `team_status` に記録すること。

### Cチーム（差分整理）
- 目的: preflight 認証ログを staging bundle 導線へ統合し、提出前品質ゲートを固定する。
- 現在の先頭タスク: `C-36`（`docs/fem4c_team_next_queue.md` を正とする）
- 対象ファイル:
  - `scripts/collect_c_team_session_evidence.sh`
  - `scripts/check_c_team_submission_readiness.sh`
  - `scripts/run_c_team_staging_checks.sh`
  - 必要時のみ `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- 指示:
  1. C-36 の受入基準に沿って、strict latest 理由ログ運用の提出前安定化を進める。
  2. strict-safe（timer + safe stage + placeholderなし）監査で PASS を維持する。
  3. 先頭完了後は `next_queue` の次タスクへ同一セッションで自動遷移する。
- 受入基準:
  - `docs/fem4c_team_next_queue.md` の C-36 `Acceptance` を満たすこと。
  - `scripts/c_stage_dryrun.sh` と `check_c_team_submission_readiness.sh` の結果を `team_status` に記録すること。

---

## Legacy
- 旧 Chrono 運用の全文は以下へ退避しました（参照のみ）。
- `docs/archive/abc_team_chat_handoff_legacy_chrono_2025-11-14.md`
- 旧司令/計画文書とドラフトの一覧は以下を参照。
- `docs/archive/README.md`
