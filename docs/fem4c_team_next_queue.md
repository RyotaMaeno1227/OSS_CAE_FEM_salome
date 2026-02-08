# FEM4C Team Next Queue

更新日: 2026-02-08（A-14完了 / A-15着手 / 30分連続実行モード反映）  
用途: チャットで「作業を継続してください」だけが来た場合の、各チーム共通の次タスク起点。

## 継続運用ルール
- 1. 各チームは本ファイルの自チーム先頭の未完了タスクから着手する。
- 2. 着手時に `In Progress` へ更新し、完了時に `Done` へ更新する。
- 3. 作業結果は `docs/team_status.md` に追記する。
- 4. セッション終了時は `docs/session_continuity_log.md` の4項目を更新する。
- 5. 担当外ファイルはステージしない（混在コミット禁止）。
- 6. `session_continuity_log` だけ更新して完了報告することを禁止する（無効報告）。
- 7. 完了報告には、実装差分ファイル・実行コマンド・受入判定（pass/fail）を必ず含める。
- 8. 1セッションは 30分以上を必須とし、30-45分の連続実行を推奨レンジとする。
- 9. 先頭タスク完了後は、同セッション内で次タスクを `In Progress` にして継続する。
- 10. blocker 発生時も、30分未満なら同セッション内で次の実行可能タスクへ継続する（途中停止しない）。
- 11. PMチャットが「作業を継続してください」のみの場合は省略指示モードとして扱い、追加確認なしで本ルールを適用する。
- 12. 終了報告には `scripts/session_timer.sh` の出力（`session_token/start_utc/end_utc/start_epoch/end_epoch/elapsed_min`）を必ず含める（`team_status` に転記）。
- 13. 受入判定は `elapsed_min >= 30` を必須とし、実作業証跡（変更ファイル・実行コマンド・pass/fail）を同時に満たすこと。
- 14. `Done` 0件かつ次タスク `In Progress` なしの終了報告は不合格。
- 15. 手入力だけの時刻・経過分（`start_at/end_at/elapsed_min` のみ）は証跡として不合格。
- 16. `sleep` 等の人工待機で elapsed を満たす行為は不合格。
- 17. `elapsed_min < 30` の終了報告は原則不合格（PM事前承認の緊急停止のみ例外）。
- 18. PM受入時は `python scripts/audit_team_sessions.py --team-status docs/team_status.md --min-elapsed 30` を実行し、最新 A/B/C エントリの機械監査結果を確認する。

## セッション終了条件（共通）
- 以下を満たしたときのみ終了報告する:
  - 1) `scripts/session_timer.sh` の出力で `elapsed_min >= 30` が確認できる
  - 2) 受入基準を満たした `Done` タスクが 1 件以上ある
  - 3) かつ次タスクが `In Progress` または blocker が明記されている
  - 4) 人工待機なしで実作業証跡（変更ファイル・実行コマンド・pass/fail）が示される

## PMレビュー結果（2026-02-06）
- A: A-1 は受入済み（`make -C FEM4C` と `--mode=mbd` の入力あり/なし分岐を再確認済み）。
- A: A-2 は受入済み（`coupled_io_contract_t` と `runner.c` の契約フィールド参照 TODO を確認済み）。
- A: A-3 は受入済み（`make -C FEM4C mbd_regression` で1コマンド回帰を確認）。
- A: A-4 は受入済み（不正 `MBD_*` 入力で行番号付きエラー・non-zero終了を確認）。
- A: A-5 は受入済み（`analysis_run(...coupled...)` の契約初期化導線を確認）。
- B: B-1/B-2/B-3 は受入済み（`make -C FEM4C mbd_probe`、2ケースFD照合、`make -C FEM4C mbd_consistency`）。
- B: B-4/B-5 は受入済み（`make -C FEM4C mbd_negative`、`make -C FEM4C mbd_checks`）。
- C: C-1/C-2/C-3 は受入済み（2026-02-06 更新）。次は意図不明群の再分類へ移行。
- PM決定（2026-02-07）: C-5 論点 #1（`FEM4C/src/io/input.c` 旧 `SPC/FORCE` 互換方針）は `Option A` を採用。
- PM決定（2026-02-07）: C-5 論点 #2（`FEM4C/src/solver/cg_solver.c` 零曲率閾値）は `Option A` を採用（`1.0e-14` 維持, Option Bは3Dtria回帰で失敗）。
- PM決定（2026-02-07）: C-5 論点 #3（`FEM4C/src/elements/t3/t3_element.c`）は `Option B` を採用（既定は自動補正 + `--strict-t3-orientation` で即エラー）。
- PM決定（2026-02-07）: MBD時間積分の実装対象は `Newmark-β` と `HHT-α` とし、最終的に実行時スイッチで切替可能にする。

---

## Aチーム（実装）

### A-1 MBD入力アダプタ（最優先）
- Status: `Done`（2026-02-06 A-team 再提出で受入基準充足）
- Goal: `--mode=mbd` で「入力内に MBD 行がある場合は入力ケースを使用し、無い場合は内蔵ミニケースへフォールバック」する最小経路を作る。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/io/input.c` / `FEM4C/src/io/input.h`
- Acceptance:
  - `cd FEM4C && ./bin/fem4c --mode=mbd <mbd_case> out_mbd.dat` が exit 0。
  - MBD行あり入力で入力ケースが使われること、MBD行なし入力でフォールバックすることをログで判別できる。
  - ログに `constraint_equations` / `residual_l2` を表示。
  - `FEM4C/src/analysis/runner.c`（必要時は `runner.h` / `input.c`）に実装差分がある。
  - `docs/session_continuity_log.md` のみ更新した報告は不合格。

### A-2 Coupled I/O 契約定義
- Status: `Done`（2026-02-06 A-team）
- Goal: `coupled` 実装に必要な最小 I/O 契約（FEM状態, MBD状態, 時間積分パラメータ）をヘッダで定義する。
- Scope:
  - `FEM4C/src/analysis/runner.h`
  - 必要時のみ `FEM4C/src/common/types.h`
- Acceptance:
  - `runner.c` の TODO コメントを具体構造体/フィールド参照へ置換。
  - `make -C FEM4C` 成功。

### A-3 MBD最小回帰スクリプト
- Status: `Done`（2026-02-06 PM検証: `make -C FEM4C mbd_regression`）
- Goal: 手元で 1 コマンド回帰できるシェル手順を追加する。
- Scope:
  - `FEM4C/practice/ch09/` または `FEM4C/scripts/`（新規）
- Acceptance:
  - コマンド 1 行で `mbd` モード実行と結果判定まで完了。

### A-4 MBD入力診断の強化
- Status: `Done`（2026-02-06 PM検証: 行番号付きエラー + non-zero終了）
- Goal: 不正 `MBD_*` 行で失敗理由と行番号を出し、入力不備を即特定できるようにする。
- Scope:
  - `FEM4C/src/analysis/runner.c`
- Acceptance:
  - 不正入力で non-zero 終了し、エラーに行番号が含まれる。
  - 正常入力は既存回帰（`mbd_regression`）が引き続き pass。

### A-5 Coupled契約の初期化導線
- Status: `Done`（2026-02-06 PM検証）
- Goal: `analysis_run(...coupled...)` で `coupled_io_contract_t` 初期化コードを明示し、将来実装の入口を固定する。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/analysis/runner.h`
- Acceptance:
  - `coupled` 分岐で契約構造体の初期化が存在する。
  - `make -C FEM4C` 成功。

### A-6 MBD入力上限の拡張
- Status: `Done`（2026-02-06 A-team: 3拘束入力 + `mbd_checks` PASS）
- Goal: 現在の「2 body / 2 constraint 固定」を最小限で拡張し、入力件数に応じた配列上限を扱えるようにする。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/analysis/runner.h`
- Acceptance:
  - 既存回帰（`mbd_checks`）が pass。
  - 3個目の拘束行が「上限超過」ではなく、仕様上限に従った明示挙動を示す。

### A-7 MBD入力バリデーション強化
- Status: `Done`（2026-02-06 A-team）
- Goal: `MBD_BODY` 重複、未定義 body 参照、非数値/不正値を行番号付きで non-zero 終了に統一する。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/analysis/runner.h`
- Acceptance:
  - duplicate body id が行番号付きで失敗する。
  - 未定義 body 参照が行番号付きで失敗する。
  - 非数値/不正値（例: 欠損、NaN、負距離）で non-zero 終了する。

### A-8 回帰検証の拡張
- Status: `Done`（2026-02-06 A-team）
- Goal: A-7で追加した負系ケースを1コマンド回帰へ統合し、日常運用で自動検出する。
- Scope:
  - `FEM4C/scripts/check_mbd_invalid_inputs.sh`
  - `FEM4C/scripts/run_mbd_regression.sh`
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_regression` 1コマンドで正系+負系を検証できる。
  - `make -C FEM4C mbd_checks` が引き続き pass する。

### A-9 MBD診断コードの固定化
- Status: `Done`（2026-02-07 A-team）
- Goal: 負系入力エラーのメッセージに安定した診断コードを付与し、回帰判定の誤検知を減らす。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - `FEM4C/scripts/check_mbd_invalid_inputs.sh`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - 不正入力時に `MBD_INPUT_ERROR[<CODE>]` がログへ出力される。
  - `make -C FEM4C mbd_regression` が pass する。

### A-10 診断コード運用の回帰整備
- Status: `Done`（2026-02-07 A-team）
- Goal: 回帰ログと運用ドキュメントで診断コード運用を固定し、将来の判定揺れを防ぐ。
- Scope:
  - `FEM4C/scripts/run_mbd_regression.sh`
  - `FEM4C/practice/README.md`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - `make -C FEM4C mbd_checks` が pass し、負系判定が診断コード基準で実行される。
  - `docs/team_status.md` に診断コード運用の pass/fail 根拠が記録される。

### A-12 旧 parser 境界条件互換の復元（PM決定 #1）
- Status: `Done`（2026-02-07 A-team）
- Goal: `FEM4C/src/io/input.c` で旧 `SPC/FORCE` 入力を再び有効化し、`NastranBalkFile` 互換を維持する。
- Scope:
  - `FEM4C/src/io/input.c`
  - 必要時のみ `FEM4C/src/io/input.h`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `cd FEM4C && ./bin/fem4c NastranBalkFile/3Dtria_example.dat run_out part_0001 /tmp/fem4c_parser_new.dat` が exit 0。
  - 旧 `SPC/FORCE` 入力ケースでも BC/荷重が無言無視されずに反映される（または明示ログで反映有無を確認できる）。
  - `make -C FEM4C` と `make -C FEM4C mbd_checks` が引き続き pass する。
  - 実装差分（`input.c` 系）と pass/fail 根拠を `docs/team_status.md` に記録する。

### A-11 診断コード未カバー系の拡張
- Status: `Done`（2026-02-07 A-team）
- Goal: 負系回帰で未カバーの診断コード（例: `E_REVOLUTE_RANGE`, `E_BODY_RANGE`）を追加し、判定漏れを減らす。
- Scope:
  - `FEM4C/scripts/check_mbd_invalid_inputs.sh`
  - 必要時のみ `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_regression` が pass し、`DIAG_CODES_SEEN` に追加コードが反映される。
  - `make -C FEM4C mbd_checks` が引き続き pass する。

### A-13 parser互換回帰導線の整備
- Status: `Done`（2026-02-07 A-team）
- Goal: A-12 の旧 parser 互換を日常回帰で再検知できるよう、1コマンド導線を整備する。
- Scope:
  - `FEM4C/scripts/check_parser_compatibility.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C parser_compat` が pass する。
  - `make -C FEM4C test` など既存運用入口との接続方針（統合または明示的非統合理由）を `docs/team_status.md` に記録する。

### A-14 coupledスタブ契約ログの固定化
- Status: `Done`（2026-02-08 A-team）
- Goal: `coupled` スタブで I/O 契約スナップショットを安定ログ化し、将来の配線実装時に退行検知できる下地を作る。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/analysis/runner.h`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `cd FEM4C && ./bin/fem4c --mode=coupled examples/t3_cantilever_beam.dat /tmp/fem4c_coupled_stub_check.dat` が non-zero で終了し、契約スナップショット（fem/mbd/time）がログに出る。
  - `make -C FEM4C coupled_stub_check` が pass する。
  - `make -C FEM4C test` の運用入口で `coupled_stub_check` が実行される。
  - `make -C FEM4C` が pass する。

### A-15 Newmark-β 積分器の導入（MBD）
- Status: `In Progress`（2026-02-08 A-team）
- Goal: MBD 時間積分の第1方式として `Newmark-β` を導入し、方式名を契約ログへ固定する。
- Scope:
  - `FEM4C/src/analysis/runner.h`
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/mbd/*`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `Newmark-β` を選択する設定（CLI または環境変数）が追加される。
  - `--mode=coupled` 実行時ログに `integrator=newmark_beta` が出力される。
  - `make -C FEM4C coupled_stub_check` と `make -C FEM4C test` が pass する。

### A-16 HHT-α 積分器の導入と切替固定（MBD）
- Status: `Todo`
- Goal: 第2方式 `HHT-α` を追加し、`Newmark-β` / `HHT-α` の2方式を実行時に切替できるようにする。
- Scope:
  - `FEM4C/src/analysis/runner.h`
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/mbd/*`
  - 必要時のみ `FEM4C/practice/README.md`
- Acceptance:
  - `HHT-α` を選択する設定（CLI または環境変数）が追加される。
  - `--mode=coupled` 実行時ログに `integrator=hht_alpha` が出力される。
  - 2方式切替を1コマンドで確認できる回帰手順（`newmark_beta` / `hht_alpha`）が整備される。
  - `make -C FEM4C coupled_stub_check` と `make -C FEM4C test` が pass する。

---

## Bチーム（検証）

### B-1 検証ハーネスのビルド導線固定
- Status: `Done`（2026-02-06 B-team: `make -C FEM4C mbd_probe` で再現確認）
- Goal: `mbd_constraint_probe` を誰でも同じコマンドで実行できるように Makefile/README を固定化。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_probe`（または同等の短縮コマンド）で PASS まで再現。

### B-2 ヤコビアン検証ケース追加
- Status: `Done`（2026-02-06 B-team: 2状態FD照合へ拡張）
- Goal: 現在の1状態だけでなく、少なくとも2つの状態で FD 照合を実施。
- Scope:
  - `FEM4C/practice/ch09/mbd_constraint_probe.c`
- Acceptance:
  - 追加ケースでも `|analytic-fd| <= 1e-6` を満たし、fail 時は非0終了。

### B-3 MBD実行ログ照合
- Status: `Done`（2026-02-06 PM検証: `make -C FEM4C mbd_consistency`）
- Goal: `--mode=mbd` 実行結果の `constraint_equations` と probe 側の式数を整合チェック。
- Scope:
  - `FEM4C/practice/ch09/`（ログ照合用補助）
- Acceptance:
  - `team_status` に照合結果（期待値/実測値）を記録。

### B-4 MBD入力の負系検証
- Status: `Done`（2026-02-06 PM検証: `make -C FEM4C mbd_negative`）
- Goal: 不正 `MBD_*` 入力に対する non-zero 終了を自動確認する。
- Scope:
  - `FEM4C/practice/ch09/` または `FEM4C/scripts/`
- Acceptance:
  - 正常系/異常系を1コマンドで検証できる。
  - 異常系で non-zero と失敗理由ログを確認できる。

### B-5 MBD検証一括ターゲット運用
- Status: `Done`（2026-02-06 PM検証: `make -C FEM4C mbd_checks`）
- Goal: `mbd_probe` / `mbd_regression` / `mbd_consistency` の一括実行運用を固定する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_checks` で probe/regression/consistency/negative の4系統が連続実行される。
  - `team_status` に実行ログ共有フォーマット（最小）を記録する。

### B-6 CI/回帰導線への統合
- Status: `Done`（2026-02-06 B-team: `make -C FEM4C test` 経由で `mbd_checks` を常時実行）
- Goal: `mbd_checks` を日常回帰導線（`test` またはCIジョブ）へ統合し、手動実行漏れを防ぐ。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `.github/workflows/*` または運用docs
- Acceptance:
  - 開発者が既存の回帰入口から `mbd_checks` を必ず実行する構成になっている。
  - 既存コマンドの互換性（最低限 `make -C FEM4C`）を維持する。

### B-7 CIワークフローへのFEM4C検証統合
- Status: `Done`（2026-02-06 B-team: `ci.yaml` へ統合、失敗時ログ/最終判定を追加）
- Goal: GitHub Actions の既存CIで FEM4C `mbd_checks` を実行し、ローカル運用との差をなくす。
- Scope:
  - `.github/workflows/ci.yaml`
  - 必要時のみ `docs/team_status.md`
- Acceptance:
  - CI上で `make -C FEM4C test` または `make -C FEM4C mbd_checks` が実行される。
  - 既存の chrono 系ジョブを壊さず、失敗時に原因ログを確認できる。

### B-8 CI導線の静的保証と判定固定（run_id運用廃止）
- Status: `Done`（2026-02-07 PM決定: run_id共有必須を廃止）
- Goal: PMの run_id 共有なしで継続運用できるよう、CI導線の受入を「静的保証 + ローカル回帰」で固定する。
- Scope:
  - `.github/workflows/ci.yaml`
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
  - `docs/team_status.md`
- Acceptance:
  - `ci.yaml` に `Run FEM4C regression entrypoint`（`id: run_fem4c_tests`）と `fem4c_test.log` artifact 収集が存在する。
  - `make -C FEM4C test` が pass する。
  - `team_status` に静的保証の判定根拠（workflow step/artifact定義 + ローカル回帰結果）が記録される。
- Note:
  - `make -C FEM4C mbd_ci_evidence` は任意のスポット確認ツールとして維持する（毎セッション必須にしない）。
  - GitHub Actions 実ラン確認は、リリース前チェックなど必要な時のみ PM が単発実施する。

### B-9 CI証跡回収コマンドの整備
- Status: `Done`（2026-02-07 B-team: `mbd_ci_evidence` + `RUN_ID` 指定導線）
- Goal: 任意のスポット確認で CI証跡を1コマンド取得できるようにする。
- Scope:
  - `FEM4C/scripts/fetch_fem4c_ci_evidence.py`
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_ci_evidence` で API 回収処理が起動する。
  - `make -C FEM4C mbd_ci_evidence RUN_ID=<id>` で単一runの照会ができる。
  - `--help` で必須引数と出力項目が確認できる。

### B-10 CI証跡フォーマット固定
- Status: `Done`（2026-02-07 B-team: `acceptance_result` を含む標準フォーマットを固定）
- Goal: `team_status` で再利用できる CI証跡フォーマット（静的保証 + 任意スポット確認）を確定する。
- Scope:
  - `docs/team_status.md`
  - 必要時のみ `docs/session_continuity_log.md`
- Acceptance:
  - 静的保証の判定結果を標準フォーマットで `team_status` に記録できる。
  - 任意スポット確認時は `run_id/status/step_outcome/artifact_present/acceptance_result` を同フォーマットで追記できる。

### B-11 CI契約チェックのローカル自動化
- Status: `Done`（2026-02-07 PM実装: `make -C FEM4C mbd_ci_contract`）
- Goal: run_id運用なしでもCI契約の破壊を即検知できるよう、ローカル静的チェックを1コマンド化する。
- Scope:
  - `FEM4C/scripts/`（新規チェックスクリプト可）
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - 1コマンド（`make -C FEM4C mbd_ci_contract`）で `.github/workflows/ci.yaml` の必須要素（step名/id/artifact収集）を検査できる。
  - 必須要素欠落時は non-zero で失敗する。
  - `docs/team_status.md` に pass/fail 根拠を記録する。

### B-12 積分法切替回帰の固定化（Newmark/HHT）
- Status: `Todo`
- Goal: `Newmark-β` / `HHT-α` の切替回帰を自動化し、方式切替の退行を日次検出する。
- Scope:
  - `FEM4C/scripts/`（新規: 積分法切替チェックスクリプト可）
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - 1コマンドで `newmark_beta` と `hht_alpha` の両方を検証できる。
  - 方式名ログ不一致または実行失敗時は non-zero で fail する。
  - `make -C FEM4C test` 入口との接続方針を `docs/team_status.md` に記録する。

---

## Cチーム（差分整理）

### C-1 test削除群の確定判定
- Status: `Done`（2026-02-06 受入: `docs/fem4c_dirty_diff_triage_2026-02-06.md` 最終判定化）
- Goal: `docs/fem4c_dirty_diff_triage_2026-02-06.md` の復元候補/削除候補を最終判定へ更新。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - `FEM4C/test/data/*` と `FEM4C/test/unit/*` の扱いが `最終判定` として明記される。

### C-2 生成物除外の運用固定
- Status: `Done`（2026-02-06 受入: `docs/fem4c_dirty_diff_triage_2026-02-06.md`, `.gitignore`）
- Goal: 生成物が毎回混入しないよう、除外ポリシーを docs と ignore で整合させる。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - 必要時のみ `.gitignore`
- Acceptance:
  - `out_mbd.dat` を含む生成物の扱いが明文化される。

### C-3 コミット分割テンプレ作成
- Status: `Done`（2026-02-06 受入: `docs/fem4c_dirty_diff_triage_2026-02-06.md`）
- Goal: PM がそのまま使える「実装コミット / docsコミット / 保留差分」の手順を固定。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - 3種類のコミットに対して具体的 `git add` コマンドが提示される。

### C-4 意図不明群の再分類（PM判断待ち）
- Status: `Done`（2026-02-06 受入: `docs/fem4c_dirty_diff_triage_2026-02-06.md` で5件以上へ最終判定付与）
- Goal: triage の `意図不明` 群を「採用/破棄」へ再分類し、保留差分を縮小する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - `FEM4C/docs/*`, `FEM4C/src/*` の保留差分（レビュー対象）
- Acceptance:
  - `意図不明` 群のうち少なくとも 5 ファイル以上に最終判定を付与する。

### C-5 高リスク保留差分の採否確定
- Status: `Done`（2026-02-07 PM決定 #1/#2/#3 反映）
- Goal: C単独で確定できない高リスク差分の採否を PMレビューで確定する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - `FEM4C/src/io/input.c`
  - `FEM4C/src/solver/cg_solver.c`
  - `FEM4C/src/elements/t3/t3_element.c`
- Acceptance:
  - 上記3ファイルの採用/破棄を PM 承認つきで最終化し、triage 文書へ反映する。
- Blocker:
  - 論点 #1（`input.c` 旧 `SPC/FORCE` 互換）は PM決定済み（Option A）。
  - 論点 #2（`cg_solver.c` 閾値）は PM決定済み（Option A）。
  - 論点 #3（`t3_element.c` 補正方針）は PM決定済み（Option B）。

### C-6 PMレビュー用エビデンス整理
- Status: `Done`（2026-02-07 C-team: `docs/fem4c_dirty_diff_triage_2026-02-06.md` Section 8）
- Goal: C-5 の PM判断に必要な実行証跡（試行コマンド/結果/暫定判定/判断依頼）を1箇所に集約する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - `input.c` / `cg_solver.c` / `t3_element.c` の3ファイルそれぞれに、試行結果と PM判断依頼が明記される。

### C-7 PM判断オプション表の固定
- Status: `Done`（2026-02-07 C-team: `docs/fem4c_dirty_diff_triage_2026-02-06.md` Section 9）
- Goal: C-5 の意思決定を遅延させないため、採用/破棄オプションを PM が即選べる形で明文化する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - `input.c` / `cg_solver.c` / `t3_element.c` の各ファイルに Option A/B/C と推奨案が記載される。

### C-8 PM判断後の即時反映プレイブック固定
- Status: `Done`（2026-02-07 C-team: `docs/fem4c_dirty_diff_triage_2026-02-06.md` Section 10）
- Goal: PM判断後に停止せず反映できるよう、差分案・検証コマンド・安全 staging を決定別に固定する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - 3ファイルそれぞれに「差分案 / 検証コマンド / pass-fail判定」が記載される。
  - PM決定反映用の安全 staging コマンドが明記される。

### C-9 論点 #1 解決済み整合（input.c）
- Status: `Done`（2026-02-07 C-team: `docs/fem4c_dirty_diff_triage_2026-02-06.md` Section 8/11）
- Goal: PM決定済みの論点 #1（`input.c` Option A）を triage 全体へ反映し、未決 blocker から除外する。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - `input.c` が「解決済み」と明記され、PM判断依頼の対象外として整合している。

### C-10 論点 #2/#3 採否確定準備（最終）
- Status: `Done`（2026-02-07 PM反映完了）
- Goal: `cg_solver.c` と `t3_element.c` の PM最終判断に必要な採否材料を揃え、C-5 を最終化可能な状態へ進める。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - `FEM4C/src/solver/cg_solver.c`
  - `FEM4C/src/elements/t3/t3_element.c`
- Acceptance:
  - #2 は PM決定（Option A）へ更新され、#3 は PM決定（Option B）へ更新される。
  - 実行コマンド / pass-fail / 決定反映後の状態が `team_status` に記録される。

### C-11 strict orientation 回帰導線の固定
- Status: `Done`（2026-02-07 C-team: `make -C FEM4C t3_orientation_checks`）
- Goal: `t3_element.c` の Option B（default=補正継続, strict=即エラー）を運用回帰へ組み込み、再現性を固定する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
  - 必要時のみ `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - 1コマンドで「default PASS + strict expected-fail」の両方を確認できる。
  - 実行結果（pass/fail根拠）を `docs/team_status.md` に記録する。

### C-12 PM決定反映後の安全 staging 最終確認
- Status: `Done`（2026-02-07 C-team: triage Section 13 / cached dry-run + 220-run soak）
- Goal: C-5確定済み3ファイルと Cチーム docs を混在なく stage できる最終手順を確定し、誤コミットリスクを下げる。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - `docs/team_status.md`
- Acceptance:
  - 3ファイル（`input.c`, `cg_solver.c`, `t3_element.c`）+ docs の安全 staging コマンドが最新状態に同期している。
  - `git diff --cached --name-status` を使う最終確認手順が `team_status` に記録される。

### C-13 staging dry-run の定型化（次ラウンド）
- Status: `Done`（2026-02-08 C-team: `docs/fem4c_dirty_diff_triage_2026-02-06.md` Section 14, `docs/team_runbook.md` 6.1, `scripts/c_stage_dryrun.sh`）
- Goal: C-12 で実施した一時 index ドライランを定型化し、次回以降も同コマンドで混在チェックできるようにする。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - 必要時のみ `docs/team_runbook.md`
- Acceptance:
  - 一時 index を用いた dry-run 手順（前提/コマンド/判定）が docs に明記される。
  - `team_status` に実行ログ記録フォーマットが追加される。

### C-14 dry-run failパス検証と運用同期
- Status: `Done`（2026-02-08 C-team: triage Section 15 / `scripts/c_stage_dryrun.sh` pass+fail実証）
- Goal: `scripts/c_stage_dryrun.sh` の failパス（forbidden混在）を実証し、運用docsへ記録して定型運用を完成させる。
- Scope:
  - `scripts/c_stage_dryrun.sh`
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - 必要時のみ `docs/team_runbook.md`
- Acceptance:
  - passケースとfailケースの双方で期待どおりの判定が得られる。
  - 実行コマンド/判定結果が `docs/team_status.md` に記録される。

### C-15 dry-run 記録テンプレの固定
- Status: `In Progress`（2026-02-08 C-team）
- Goal: `team_status` へ貼る dry-run 記録テンプレを固定し、次回以降の報告粒度を統一する。
- Scope:
  - `docs/team_status.md`
  - 必要時のみ `docs/team_runbook.md`
- Acceptance:
  - `dryrun_method/dryrun_cached_list/forbidden_check/required_set_check/dryrun_result` の5項目が定型で記録される。
  - 次セッションでテンプレを再利用できる状態になっている。

---

## PMチェックポイント
- A/B/C の更新は毎回 `docs/team_status.md` の該当セクションで確認する。
- 継続指示のみの運用時でも、最終判断は本ファイルの `Status` 更新と受入基準で行う。
- 連続実行違反（タイマー証跡未記録、人工待機、Doneなし終了）は差し戻して同タスク継続とする。
