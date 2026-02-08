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
  - Cチーム staging 検証は `scripts/c_stage_dryrun.sh` を優先使用し、`dryrun_result` を `team_status` に記録する。
  - C-19 以降は `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze` を実行し、`## Cチーム` 配下の最新報告が coupled凍結ポリシー込みで PASS であることを確認する。
  - タイマー完了まで厳格に確認する場合は `bash scripts/check_c_team_dryrun_compliance.sh docs/team_status.md pass_section_freeze_timer` を使用する。
  - 次タスク遷移の優先順（先頭完了後の迷い防止）:
    - A: A-16 完了後は A-17 へ遷移
    - B: B-12 完了後は B-14 へ遷移
    - C: C-19 完了後は C-20 へ遷移
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
  - コンテクスト切れ/新規チャット移行時は `docs/team_runbook.md` の「8. コンテクスト切れ時の新規チャット移行手順」を必ず適用する。

### Aチーム（実装）
- 目的: `mbd` モードを独立ソルバーとして段階的に完成させる。
- 対象ファイル:
  - `FEM4C/src/analysis/runner.c`
  - `FEM4C/src/analysis/runner.h`
  - 必要時のみ `FEM4C/src/fem4c.c`
- 指示:
  1. `mbd` モードで 2D 最小ケース（2 body + distance/revolute）を実行できる関数を追加する。
  2. `mbd_constraint_evaluate()` と `mbd_kkt_compute_layout_from_constraints()` を実際に呼び、残差ノルムとDOF情報をログ出力する。
  3. `coupled` モードは仕様確定までスタブ維持とし、新規機能追加は行わない。
- 受入基準:
  - `cd FEM4C && ./bin/fem4c --mode=mbd examples/t6_cantilever_beam.dat out_mbd.dat` が終了コード 0。
  - 実行ログに `Analysis mode: mbd` と拘束式本数/残差が表示される。
  - `docs/session_continuity_log.md` 以外に、少なくとも 1 つの実装ファイル差分があること。

### Bチーム（検証）
- 目的: 追加済み MBD拘束 API の数値妥当性を固定テストで担保する。
- 対象ファイル:
  - `FEM4C/practice/ch09/`（新規テストハーネス追加可）
  - `FEM4C/src/mbd/constraint2d.c`（必要最小限の修正のみ）
  - `FEM4C/src/mbd/kkt2d.c`（必要最小限の修正のみ）
- 指示:
  1. distance/revolute の残差・ヤコビアンを検証する最小ハーネスを追加する。
  2. 有限差分でヤコビアン照合（推奨 `eps=1e-7`）を実施し、閾値を明記する。
  3. `mbd_kkt_count_constraint_equations()` が revolute=2式を正しく数えることを確認する。
- 受入基準:
  - 検証コマンドを 1 行で実行できる（README かコメントに記載）。
  - 期待閾値を超えた場合は非0終了で失敗する。

### Cチーム（差分整理）
- 目的: 巨大 dirty 差分の誤コミットリスクを先に下げる。
- 対象ファイル:
  - `docs/` 配下に整理レポートを新規作成（例: `docs/fem4c_dirty_diff_triage_2026-02-06.md`）
  - 必要時のみ `.gitignore`
- 指示:
  1. 現在差分を 3分類（実装として残す / 生成物・不要物 / 意図不明）で一覧化する。
  2. `FEM4C/test/*` 削除群は「復元候補」「削除確定」の二択で暫定判定を付ける。
  3. PM がそのまま使える `git add` パス指定例（安全ステージング手順）を記載する。
- 受入基準:
  - 上記3分類がファイルパス付きで提示されている。
  - 混在コミット回避の staging 例が 3 コマンド以上ある。

---

## Legacy
- 旧 Chrono 運用の全文は以下へ退避しました（参照のみ）。
- `docs/archive/abc_team_chat_handoff_legacy_chrono_2025-11-14.md`
- 旧司令/計画文書とドラフトの一覧は以下を参照。
- `docs/archive/README.md`
