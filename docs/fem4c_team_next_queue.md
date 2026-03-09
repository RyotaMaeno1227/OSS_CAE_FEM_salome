# FEM4C Team Next Queue

更新日: 2026-03-08（A/B/D short stale 差し戻し / C-49継続 / E plan-missing 整理）
用途: チャットで「作業してください」のみが来た場合の、PM/A/B/C/D/E 共通の次タスク起点。

## 0. 路線切替
- 旧 A/B/C の `A-59` / `B-45` / `C-59` 系タスクは凍結する。
- 凍結前の active docs は以下へ退避した。
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/fem4c_team_next_queue_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/abc_team_chat_handoff_legacy_2026-03-06.md`
  - `oldFile/docs/archive/roadmap_reset_2026-03-06/team_runbook_legacy_2026-03-06.md`
- 現在の正本ロードマップは以下とする。
  - `FEM4C/fem4c_2link_flexible_detailed_todo.md`
  - `FEM4C/fem4c_codex_team_prompt_pack.md`
  - `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md`
  - `docs/10_review_spec_priority_plan.md`
  - `docs/04_2d_coupled_scope.md`
  - `docs/05_module_ownership_2d.md`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/07_input_spec_coupled_2d.md`
  - `docs/08_merge_order_2d.md`
  - `docs/09_compare_schema_2d.md`

## 1. PM固定決定（2026-03-06）
- 対象モデルは `2-link planar mechanism` に固定する。
- 両リンク flexible を最終 target とする。
- FEM は step / iteration ごとに full mesh 再アセンブルする。
- MBD は explicit / Newmark-beta / HHT-alpha を実装対象とする。
- RecurDyn / AdamsFlex の実データは未投入のため、現時点では compare CSV schema の固定を先行する。
- 実データ未投入は M0-M3 の blocker にしない。M4 で数値比較を必須化する。
- Project Chrono の参照元は `third_party/chrono/chrono-main` のみとする。

## 2. 共通ルール
- 開始時に `python3 tools/team_timer/team_timer.py start <team_tag>` を実行する。
- `start` 後 10 分以内に `python3 tools/team_timer/team_timer.py declare <session_token> <primary_task> <secondary_task> ["plan_note"]` を実行し、`SESSION_TIMER_DECLARE` を `docs/team_status.md` へ転記する。
- `start` 後 20 分以内と 40 分以降に `python3 tools/team_timer/team_timer.py progress <session_token> <current_task> <work_kind> ["progress_note"]` を実行し、`SESSION_TIMER_PROGRESS` を `docs/team_status.md` へ転記する。
- 報告前に `python3 tools/team_timer/team_timer.py guard <session_token> 10`, `20`, `30`, `60` を記録する。
- 終了時に `python3 tools/team_timer/team_timer.py end <session_token>` を実行し、出力を `docs/team_status.md` に転記する。
- 旧 `scripts/session_timer*.sh` は互換ラッパーであり、今後の正本運用では使用しない。
- 1セッションは 60分以上を必須とし、60-90分を推奨レンジとする。
- 60分は開発前進に使う。実装系ファイル差分を毎セッション必須とし、primary task 完了後も `guard60` まで secondary/Auto-Next へ進める。
- 同一コマンド反復や長時間ソークで時間を消費しない。
- 先頭タスク完了後は、同一セッション内で次タスクへ自動遷移する。
- `docs/team_status.md` と `docs/session_continuity_log.md` を必ず更新する。
- `docs/team_status.md` に `## Dチーム` / `## Eチーム` が無ければ、D/Eチームが最初の報告時に見出しを作成してよい。

## 2A. PM運用メモ（チャット最小化用）
- この節は、PM/ユーザーが各チームへ個別チャットを増やさずに運用是正を伝えるための正本である。
- 各チームは毎セッション開始前にこの節を確認し、追加チャットが無くてもここに書かれた内容を自動適用する。
- ここに書く内容は、短時間ラン是正、超過ラン是正、差し戻し、禁止コマンド、再開点、優先度変更に限定する。
- ここに未記載の一般ルールは `## 2. 共通ルール` と `docs/abc_team_chat_handoff.md` Section 0 に従う。
- ユーザーから Codex への通常依頼キーワードは `確認してください`、ユーザーから各チームへの通常依頼キーワードは `作業してください` とする。
- 5チーム全員の終了を待たず、終了済みチームが出た時点で `確認してください` を送ってよい。Codex は終了済みチームから先に判定し、稼働中チームは継続中として扱う。
- 現在の常設注意:
  1. `elapsed_min < 60` は不受理。A/B/C/D/E いずれも、同一タスクを新規 `session_token` で再開し、`60 <= elapsed_min <= 90` を満たすまで受理しない。
  2. `elapsed_min > 90` も不受理。D/E を含め、作業量を分割し、`guard60=pass` 後に 90分を超える前に終了する。
  3. 同一コマンド反復、長時間ソーク、guard待ちのための検証積み増しは禁止する。時間が余った場合は同一スコープの次タスクへ進む。
  4. Bチームは旧 `B-45/B-46` 系と `mbd_b45_acceptance` を再開しない。新ロードマップ `B-01` 以降のみを対象とする。
  5. Cチームは build/FEM API に直結しない広域回帰を時間充足目的で実施しない。C-03 が早く終わった場合は C-04 へ進む。
  6. D/Eチームは 2-link flexible 本線を前進させる。docs更新のみで終了せず、90分超過もしない。
  7. `ACTIVE_UNCONFIRMED` かつユーザー確認で停止済みと分かったセッションは stale 扱いとする。旧 `session_token` の end 回収は行わず、queue 先頭タスクを新規 `session_token` で再開する。
  8. 現在の再開点は `A-15`, `B-08`, `C-49`, `D-52`, `E-14` とする。A は `A-14` 受理済み、C は `C-47` 受理済みで current run 継続中、D は `D-50` / `D-51` を受理済みとして閉じ、次を `D-52` に進める。E は受理済みで `E-14` へ進める。B は current run を据え置く。
  9. primary task の acceptance が 60分未満で見えた場合でも、その時点では `end` しない。queue 上の次タスクを同一セッションで `In Progress` 化し、`guard60=pass` 後にまとめて終了報告する。
  10. queue 末尾で後続未定義のチームは、実装開始前に同一スコープの `Auto-Next` を自分で起票してから着手する。後続未定義のまま 60分未満で停止したセッションは不受理とする。
  11. `guard10` が `block` のまま停止したセッションは、成果主張があっても queue を進めない。stale 扱いで同一タスクを新規 `session_token` からやり直す。
  12. verbal な「完了報告」があっても、`docs/team_status.md` に当該 session の `SESSION_TIMER_END` と pass/fail が無ければ queue は進めない。current 未受理 run は B がこの扱いとし、`B-08` のまま据え置く。
  13. 2026-03-08 観測の短時間停止 issue は B/D 側の stale short run として扱う。B は `B-08`、D は `D-21` を新規 `session_token` でやり直し、60分未満での終了主張を認めない。
  14. 各チームは `start` 後 10 分以内に「このセッションで primary task 完了後に何へ進むか」を queue 上で確認し、後続未定義ならその場で `Auto-Next` を起票する。次タスク未確定のまま短時間停止したランは stale 扱いとする。
  15. 2026-03-08 現在の確認結果として、A/B/D は current short run を破棄して `A-15` / `B-08` / `D-21` をやり直す。C は current run 継続で `C-49`。E は受理済み record を保持しつつ `E-14` をやり直す。
  16. 監視上の stale 判定は厳格化する。`start` から 12 分以内に guard が無い run、または最後の guard/heartbeat から 12 分以上更新が無いまま `elapsed_min < 60` の run は short stale run として無効化する。
  17. 追加策として、`start` から 10 分以内に `SESSION_TIMER_DECLARE` が無い session は `PLAN_MISSING` として扱う。`primary_task` と `secondary_task` の両方を必須とし、未記録のまま停止した run は queue を進めず同一タスクを新規 `session_token` でやり直す。
  18. 次ランから `scripts/run_team_acceptance_gate.sh` は `SESSION_TIMER_DECLARE` を既定で必須化する。pre-rollout の旧最新エントリは FAIL になり得るが、以後の新規ランは declare 付きでのみ受理する。
  19. 2026-03-08 観測の A チーム current run は 5 分未満で停止したため無効とする。`A-15` を新規 `session_token` で再実行し、`SESSION_TIMER_DECLARE A-15 A-16` を 10 分以内に残し、`guard60=pass` 前に終了しない。
  20. 2026-03-08 18:08Z 部分確認では、A=`STALE_BEFORE_60`, B=`STALE_BEFORE_60`, E=`PLAN_MISSING` だった。加えてユーザー確認で D も 17 分停止だったため、A=`A-15`, B=`B-08`, D=`D-21`, E=`E-14` を新規 `session_token` でやり直す。A/B/D は 60 分未満停止を再発させず、E は 10 分以内に `SESSION_TIMER_DECLARE E-14 E-15` を残す。
  21. A チームは短時間停止が再発しているため、`A-15` を 1 セッション専有タスクとして扱う。次回 accepted run を得るまでは queue を進めず、`A-15` の同一スコープ内で実装・probe・target整理を継続する。4分停止のような current run は即無効とし、新規 `session_token` で `A-15` をやり直す。
  22. 2026-03-08 D-team rerun は `session_token=/tmp/d_team_session_20260307T181518Z_1286257.token` で受理済み。`SESSION_TIMER_DECLARE D-21 D-22` を 10 分以内に満たし、`guard60=pass`, `elapsed_min=73` で `D-21` rerun を閉じた上で `D-23` を完了し、再開点を `D-24` へ進めた。
  23. `FEM4C/FEM4C_Codex_SingleFile_Review_Spec_2026-03-08.md` を最優先方針として採用する。今後の default dispatch は `docs/10_review_spec_priority_plan.md` の Run 1 -> Run 2 -> Run 3 を優先し、older open task は backlog 扱いとする。
  24. `PM-05` が閉じるまで、新しい wrapper / resilience pack 増設、contact/friction/3D、large monolith 全面分割は行わない。
  25. 直近 dispatch の優先タスクは `PM-R1`, `A-R1`, `B-R1`, `C-R1`, `D-R1`, `E-R1` とする。各チームは `作業してください` を受けたら、まず `docs/10_review_spec_priority_plan.md` の Run 1 を読む。
  26. formal accepted と provisional を分けて扱う。特に E-team は `team_status` formal log 上は `E-13` まで accepted とし、E-14 以降は provisional 実装として読む。
  27. D-team review-spec current run は `session_token=/tmp/d_team_session_20260308T051359Z_1876846.token` で `D-R1` を完了した。`SESSION_TIMER_DECLARE D-R1 D-R2` を 10 分以内に満たし、`guard60=pass`, `elapsed_min=64` で閉じているため、次回再開点は `D-R2` とし、旧 `D-24` は review-spec 優先期間中 backlog とする。
  28. `guard60` は「60分後に end する」意味ではなく、「60分時点まで継続実装した証跡を残す」意味とする。primary task が数分で終わっても、その時点では終了せず secondary task または同一スコープ Auto-Next へ遷移する。
  29. `SESSION_TIMER_PROGRESS` を最低 2 回必須とする。目安は 20 分以内に 1 回、40 分以降に 1 回である。受理ゲートは progress count と late progress を確認し、数分実装して token を開いたままの run を FAIL にする。
  30. D-team のように `elapsed_min=64` を満たしている session は受理対象とする。IDE の「数分作業」は UI の active 表示であり、session 総時間とは一致しない場合がある。受理判定は `SESSION_TIMER_*` 証跡を正とする。
  31. `guard60=pass` 前の非 blocker 中間報告は禁止する。途中で PM/ユーザーへ送ってよいのは blocker 報告だけとし、実装進捗の共有は `SESSION_TIMER_PROGRESS` とローカル差分で行う。
  32. `この token のまま継続します` という文言だけでは継続扱いにしない。実際に次の `guard` または `SESSION_TIMER_PROGRESS` が残らずチャット/IDE 側の応答が止まった場合は、停止済み stale run として同一タスクを新規 `session_token` でやり直す。
  33. 60-90分ラン安定化を最優先とする間は、PM/ユーザーは active run 中の通常進捗問い合わせを行わない。確認は `scripts/team_control_tower.py` の監視結果を正とし、チャット応答は `guard60=pass` 後または blocker 発生時のみに限定する。
  34. active run 中に PM/ユーザーから `確認してください` が来ても、各チームは原則応答しない。応答してよいのは blocker / destructive conflict / data loss risk のみとし、通常の進捗説明は `docs/team_status.md` 更新時にまとめる。
  35. 60-90分ラン安定化フェーズでは、各チームは 1 セッションで「primary 1件 + secondary 1件」までを上限目安とする。早期に 3件以上へ広げず、secondary を明示してから着手する。
  36. D-team `session_token=/tmp/d_team_session_20260308T053957Z_1888207.token` は受理済みとし、`D-R2=Done`, `D-R3=Done`, 次回再開点 `D-24` を正とする。
  33. D-team review-spec follow-up は `session_token=/tmp/d_team_session_20260308T053957Z_1888207.token` で受理済み。`SESSION_TIMER_DECLARE D-R2 D-R3` を 10 分以内、`SESSION_TIMER_PROGRESS D-R2` を 20 分以内、`SESSION_TIMER_PROGRESS D-R3` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=64` で閉じているため、`D-R2=Done`, `D-R3=Done` とし、次回再開点は `D-24` とする。
  37. D-team `session_token=/tmp/d_team_session_20260308T060318Z_1924276.token` は受理済みとし、`SESSION_TIMER_DECLARE D-24 D-25` を 10 分以内、`SESSION_TIMER_PROGRESS D-24` を 20 分以内、`SESSION_TIMER_PROGRESS D-25` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-24=Done`, `D-25=Done`, 次回再開点 `D-26` を正とする。
  38. D-team `session_token=/tmp/d_team_session_20260308T075814Z_2062134.token` は受理済みとし、`SESSION_TIMER_DECLARE D-26 D-27` を 10 分以内、`SESSION_TIMER_PROGRESS D-26` を 20 分以内、`SESSION_TIMER_PROGRESS D-27` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-26=Done`, `D-27=Done`, 次回再開点 `D-28` を正とする。
  39. D-team `session_token=/tmp/d_team_session_20260308T082001Z_2594858.token` は受理済みとし、`SESSION_TIMER_DECLARE D-28 D-29` を 10 分以内、`SESSION_TIMER_PROGRESS D-28` を 20 分以内、`SESSION_TIMER_PROGRESS D-29` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-28=Done`, `D-29=Done`, 次回再開点 `D-30` を正とする。
  40. D-team `session_token=/tmp/d_team_session_20260308T095823Z_3691791.token` は受理済みとし、`SESSION_TIMER_DECLARE D-30 D-31` を 10 分以内、`SESSION_TIMER_PROGRESS D-30` を 20 分以内、`SESSION_TIMER_PROGRESS D-31` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-30=Done`, `D-31=Done`, 次回再開点 `D-32` を正とする。
  40. 2026-03-08 最新 formal accepted を正とした現在の再開点は `A-15`, `B-09`, `C-59`, `D-48`, `E-44` とする。A の current `session_token=/tmp/a_team_session_20260308T082001Z_2594872.token` は `STALE_BEFORE_60` として不受理、B/C/D/E は formal accepted を採用する。
  44. D-team `session_token=/tmp/d_team_session_20260308T100739Z_3991936.token` は受理済みとし、`SESSION_TIMER_DECLARE D-32 D-33` を 10 分以内、`SESSION_TIMER_PROGRESS D-32` を 20 分以内、`SESSION_TIMER_PROGRESS D-33` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-32=Done`, `D-33=Done`, 次回再開点 `D-34` を正とする。
  45. D-team `session_token=/tmp/d_team_session_20260308T120544Z_1761475.token` は受理済みとし、`SESSION_TIMER_DECLARE D-34 D-35` を 10 分以内、`SESSION_TIMER_PROGRESS D-34` を 20 分以内、`SESSION_TIMER_PROGRESS D-35` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-34=Done`, `D-35=Done`, 次回再開点 `D-36` を正とする。
  41. A-team は `A-15` を据え置き、current stale session を破棄して新規 `session_token` で再実行する。`primary=A-15`, `secondary=A-16` を維持し、`guard60=pass` 前の停止を認めない。
  42. E-team は `E-43` 受理済みだが次タスク未定義だったため、本ファイルで `E-44` を新設した。次回 `作業してください` は `E-44` 着手を意味する。
  43. session 受理判定は IDE の「○分作業しました」ではなく `docs/team_status.md` に転記された `SESSION_TIMER_END elapsed_min` を正とする。ただし current formal end が無い live token は `team_control_tower.py` の runtime state を優先し、A のような stale current run は不受理とする。
  46. 2026-03-10 最新確認では、current run の扱いは `A=A-19 着手`, `B=B-13 継続`, `C=C-91 継続`, `D=D-52 着手`, `E=E-47 rerun` とする。A は `A-18` formal accepted を採用して次を `A-19` に進める。B は latest formal entry `B-12 Done / B-13 In Progress` を採用する。C は latest formal entry `C-84..C-90 Done / C-91 In Progress` を採用する。D は `D-50/D-51` accepted を採用して次を `D-52` に進める。E は `E-47 Formal Rerun Accepted` section に `SESSION_TIMER_END` 行が無く formal 不受理のため rerun とする。
  47. 2026-03-10 以降、all-team stop 後の次指示は queue 先頭を正とする。`作業してください` は A=`A-19`, B=`B-13`, C=`C-91`, D=`D-52`, E=`E-47` を意味する。
  48. D-team `session_token=/tmp/d_team_session_20260309T024814Z_7432.token` は受理済みとし、`SESSION_TIMER_DECLARE D-36 D-37` を 10 分以内、`SESSION_TIMER_PROGRESS D-36` を 20 分以内、`SESSION_TIMER_PROGRESS D-37` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-36=Done`, `D-37=Done`, 次回再開点 `D-38` を正とする。
  49. D-team `session_token=/tmp/d_team_session_20260309T040623Z_1682513.token` は受理済みとし、`SESSION_TIMER_DECLARE D-38 D-39` を 10 分以内、`SESSION_TIMER_PROGRESS D-38` を 20 分以内、`SESSION_TIMER_PROGRESS D-39` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-38=Done`, `D-39=Done`, 次回再開点 `D-40` を正とする。
  50. D-team `session_token=/tmp/d_team_session_20260309T042334Z_2059745.token` は受理済みとし、`SESSION_TIMER_DECLARE D-40 D-41` を 10 分以内、`SESSION_TIMER_PROGRESS D-40` を 20 分以内、`SESSION_TIMER_PROGRESS D-41` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=66` で閉じているため、`D-40=Done`, `D-41=Done`, 次回再開点 `D-42` を正とする。
  52. D-team `session_token=/tmp/d_team_session_20260309T053134Z_530932.token` は受理済みとし、`SESSION_TIMER_DECLARE D-42 D-43` を 10 分以内、`SESSION_TIMER_PROGRESS D-42` を 20 分以内、`SESSION_TIMER_PROGRESS D-43` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=69` で閉じているため、`D-42=Done`, `D-43=Done`, 次回再開点 `D-44` を正とする。
  53. D-team `session_token=/tmp/d_team_session_20260309T104951Z_1507871.token` は受理済みとし、`SESSION_TIMER_DECLARE D-44 D-45` を 10 分以内、`SESSION_TIMER_PROGRESS D-44` を 20 分以内、`SESSION_TIMER_PROGRESS D-45` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=62` で閉じているため、`D-44=Done`, `D-45=Done`, 次回再開点 `D-46` を正とする。
  54. D-team `session_token=/tmp/d_team_session_20260309T111821Z_2254716.token` は受理済みとし、`SESSION_TIMER_DECLARE D-46 D-47` を 10 分以内、`SESSION_TIMER_PROGRESS D-46` を 20 分以内、`SESSION_TIMER_PROGRESS D-47` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-46=Done`, `D-47=Done`, 次回再開点 `D-48` を正とする。
  55. D-team `session_token=/tmp/d_team_session_20260309T120002Z_3722651.token` は受理済みとし、`SESSION_TIMER_DECLARE D-48 D-49` を 10 分以内、`SESSION_TIMER_PROGRESS D-48` を 20 分以内、`SESSION_TIMER_PROGRESS D-49` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=62` で閉じているため、`D-48=Done`, `D-49=Done`, 次回再開点 `D-50` を正とする。
  56. D-team `session_token=/tmp/d_team_session_20260309T203614Z_7676.token` は受理済みとし、`SESSION_TIMER_DECLARE D-50 D-51` を 10 分以内、`SESSION_TIMER_PROGRESS D-50` を 20 分以内、`SESSION_TIMER_PROGRESS D-51` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=61` で閉じているため、`D-50=Done`, `D-51=Done`, 次回再開点 `D-52` を正とする。
  51. A-team `session_token=/tmp/a_team_session_20260309T042343Z_2066660.token` は受理済みとし、`SESSION_TIMER_DECLARE A-18 A-R1` を 10 分以内、`SESSION_TIMER_PROGRESS A-18` を 20 分以内、`SESSION_TIMER_PROGRESS A-R1` を 40 分以降に満たし、`guard60=pass`, `elapsed_min=68` で閉じているため、`A-18=Done` を正とする。次回再開点は `A-19` とする。

## 3. 現在のマイルストーン
- 現在位置: `M0 build recovery + M1 rigid MBD kickoff`
- 直近の merge gate:
  1. PM-01〜PM-06 で design freeze
  2. C-01 で build recovery
  3. A-01〜A-03 / B-01〜B-04 / E-01 / E-03 で rigid foundation

## 4. PMチーム
### PM-01
- Status: `In Progress`
- Goal: 2D PJ の必須要件を凍結する。
- Scope:
  - `docs/04_2d_coupled_scope.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - 両リンク flexible / full reassembly / explicit+Newmark+HHT / rigid解析比較 / flexible外部比較 / out-of-scope が 1 ページで読める。

### PM-02
- Status: `Todo`
- Goal: モジュール責務を固定する。

### PM-03
- Status: `Todo`
- Goal: 受入条件の数値指標を固定する。

## 5. Aチーム
### A-R1 (Run 1 Priority)
- Status: `Done`
- Goal: implicit logging / history label の stale `newmark_*` 命名を中立化し、M1 compare 用の出力語彙を揃える。
- Scope:
  - `FEM4C/src/mbd/output2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/practice/ch09/*`
- Acceptance:
  - HHT 実行でも stale な `newmark_*` ラベルが主要 summary に残らない。
  - `implicit_result` 系の中立ラベルで compare / history を読める。

### A-R2 (Run 2 Priority)
- Status: `Done`
- Goal: non-trivial rigid case で必要な output/history field を compare-ready に揃える。
- Scope:
  - `FEM4C/src/mbd/output2d.*`
  - `FEM4C/examples/mbd_2link_rigid_dyn.dat`
  - 必要時のみ `FEM4C/scripts/compare_2link_rigid_analytic.py`
- Acceptance:
  - rigid analytic compare に必要な field が stable に出る。
  - `make -C FEM4C mbd_rigid_compare_review_smoke` が PASS する。

### A-R3 (Run 2 Auto-Next)
- Status: `Done`
- Goal: rigid compare の direct sidecar route を provenance 付きで固定し、malformed sidecar 時の history fallback と区別できるようにする。
- Scope:
  - `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - `FEM4C/scripts/test_compare_2link_rigid_analytic_real*.sh`
  - `FEM4C/scripts/test_compare_2link_rigid_analytic_fallback.sh`
  - `FEM4C/scripts/test_run_e08_rigid_analytic_wrappers.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - rigid summary 正常系で compare / normalize log に `normalization_source=rigid_compare_csv` が出る。
  - malformed な `rigid_compare_csv` tip geometry では `normalization_source=history_csv` と fallback reason が出る。
  - normalized rigid schema CSV が `rigid_compare_csv` と drift せず一致する。
  - `make -C FEM4C mbd_rigid_compare_route_review_smoke` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_fallback_review_smoke` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_integrator_review_smoke` が PASS する。

### A-01
- Status: `Done`
- Goal: `mbd_body2d_t` を新設し、剛体 body 実体を `runner.c` から切り出す。
- Scope:
  - `FEM4C/src/mbd/body2d.h`
  - `FEM4C/src/mbd/body2d.c`
- Acceptance:
  - `id/mass/inertia/q/v/a/force/is_ground` を保持する。
  - `mbd_body2d_zero()`, `mbd_body2d_init_dyn()`, `mbd_body2d_clear_force()` が存在する。

### A-02
- Status: `Done`
- Goal: `MBD_BODY_DYN` / `MBD_GRAVITY` / `MBD_FORCE` を parse できるようにする。

### A-03
- Status: `Done`
- Goal: gravity / user load の assemble API を作る。

### A-04
- Status: `Done`
- Goal: marker / interface の幾何変換を作る。

### A-05
- Status: `Done`
- Goal: explicit integrator の器を作る。

### A-06
- Status: `Done`
- Goal: explicit path に body state 更新を接続する。

### A-07
- Status: `Done`
- Goal: 時系列 CSV writer と history sidecar を固定する。

### A-08
- Status: `Done`
- Goal: flexible generalized force の加算/clear API を作る。

### A-09
- Status: `Done`
- Goal: body reference frame accessor を追加する。

### A-10 (Auto-Next)
- Status: `Done`
- Goal: A-07/A-08/A-09 を `bin/fem4c` の full-link 経路でも再確認する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/src/mbd/output2d.c`
- Acceptance:
  - `make -C FEM4C` が `FEM4C/src/coupled/coupled_step_implicit2d.c` の外部 compile blocker 解消後に通る。
  - A-team smoke pack と `bin/fem4c` 実行の両方で history/flexible force/reference frame 契約が維持される。

### A-11 (Auto-Next)
- Status: `Done`
- Goal: A-side API adoption を coupled/runtime 呼び出し側へ寄せる。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/body2d.h`
  - 必要時のみ `FEM4C/src/coupled/*`
- Acceptance:
  - flexible generalized force の加算が raw `body.force` 直接加算ではなく `mbd_system2d_add_flexible_generalized_force()` 優先の契約へ前進する。
  - reference frame 取得が raw `q[]` 直接参照ではなく `mbd_body2d_get_reference_frame()` / `mbd_body2d_get_current_pose()` 利用へ前進する。

### A-12 (Auto-Next)
- Status: `Done`
- Goal: generalized force の履歴を system 側へ保持し、implicit/HHT が前ステップ荷重を helper 経由で参照できるようにする。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/forces2d.c`
  - 必要時のみ `FEM4C/src/mbd/output2d.c`
- Acceptance:
  - current/previous generalized force が body ごとに system-owned state として保持される。
  - HHT/Newmark caller が raw `body.force` の再利用ではなく helper API で previous-force snapshot を取得できる。
  - `make -C FEM4C mbd_a_team_foundation_smoke mbd_b_team_foundation_smoke` が PASS する。

### A-13 (Auto-Next)
- Status: `Done`
- Goal: system-owned generalized force history を summary / probe / smoke 契約として固定する。
- Scope:
  - `FEM4C/src/mbd/output2d.h`
  - `FEM4C/src/mbd/output2d.c`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/Makefile`
  - `FEM4C/practice/ch09/*`
- Acceptance:
  - summary 出力が `generalized_force_history_valid/current/previous` rows を Newmark/HHT で emit し、explicit では `valid=0` かつ current/previous rows を出さない。
  - `make -C FEM4C mbd_system2d_explicit_probe_smoke mbd_system2d_explicit_smoke` が PASS する。
  - `make -C FEM4C mbd_system2d_newmark_smoke mbd_system2d_newmark_constrained_smoke` が PASS する。
  - `make -C FEM4C mbd_system2d_hht_smoke mbd_system2d_hht_constrained_smoke` が PASS する。
  - `make -C FEM4C mbd_a_team_foundation_smoke` が PASS する。

### A-14 (Auto-Next)
- Status: `Done`
- Goal: generalized force history の free/constrained console・summary 契約を direct probe で固定する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/ch09/mbd_system2d_console_history_probe.c`
  - `FEM4C/practice/ch09/mbd_system2d_constrained_history_output_probe.c`
  - 必要時のみ `FEM4C/practice/ch09/*`
- Acceptance:
  - direct probe が `explicit/newmark/hht/newmark_constrained/hht_constrained` の console history marker を固定する。
  - direct probe が `newmark/hht` constrained summary の current/previous rows を固定する。
  - `make -C FEM4C mbd_system2d_newmark_constrained_console_history_smoke mbd_system2d_hht_constrained_console_history_smoke` が PASS する。
  - `make -C FEM4C mbd_system2d_newmark_constrained_history_output_smoke mbd_system2d_hht_constrained_history_output_smoke` が PASS する。
  - `make -C FEM4C mbd_a_team_foundation_smoke` が PASS する。

### A-15 (Auto-Next)
- Status: `Done`
- Goal: generalized force history regression を probe-first aggregate target へ整理し、残る history marker 用の ad-hoc grep recipe を縮退させる。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/ch09/mbd_system2d_console_history_probe.c`
  - `FEM4C/practice/ch09/mbd_system2d_constrained_history_output_probe.c`
  - 必要時のみ `FEM4C/practice/ch09/*`
- Acceptance:
  - `mbd_system2d_history_contract_smoke` が explicit/free, Newmark free/constrained, HHT free/constrained の console/summary contract を一括で cover する。
  - free/constrained history marker 検証が direct probe / aggregate target 側へ寄り、既存 CLI smoke の ad-hoc grep は integrator/body summary 中心に縮退している。
  - `make -C FEM4C mbd_system2d_history_contract_smoke` が PASS する。
  - `make -C FEM4C mbd_a_team_foundation_smoke` が PASS する。

### A-16 (Auto-Next)
- Status: `Done`
- Goal: history contract bundle と A-team foundation pack の依存を整理し、history 契約を 1 つの pack target 経由で再利用できる形にする。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/practice/ch09/*`
- Acceptance:
  - `mbd_a_team_foundation_smoke` が history 関連の個別 target 群を直接列挙せず、`mbd_system2d_history_contract_smoke` などの bundle target 経由で再利用している。
  - `make -C FEM4C mbd_system2d_history_contract_smoke` が PASS する。
  - `make -C FEM4C mbd_a_team_foundation_smoke` が PASS する。

### A-17 (Auto-Next)
- Status: `Done`
- Goal: history contract bundle と A-team foundation pack の help/current command surface を single-source 化し、bundle 名 drift を防ぐ。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/README.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `make -C FEM4C help | rg "mbd_system2d_history_contract_smoke|mbd_a_team_foundation_smoke"` が PASS する。
  - history bundle と foundation pack の役割差が help/current command surface で読める。
  - `make -C FEM4C mbd_system2d_history_contract_smoke` と `make -C FEM4C mbd_a_team_foundation_smoke` が継続して PASS する。

### A-18 (Auto-Next)
- Status: `Done`
- Goal: Run 1 MBD surface docs sync target を history/foundation bundle にも拡張し、README / acceptance matrix / Make help の current command surface を 1 本の focused self-test で再検証できるようにする。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/README.md`
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
- Acceptance:
  - `mbd_a_team_foundation_smoke` と `mbd_system2d_history_contract_smoke` の role boundary が docs/help に記載される。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が PASS する。
  - A-team history/foundation bundle の current command surface が Run 1 docs sync で再検証できる。

### A-19 (Auto-Next)
- Status: `Todo`
- Goal: A-team foundation/history bundle の focused docs-sync validator を review-spec priority plan と runbook surface にも同期し、PM 側から再開点と self-test entrypoint を 1 箇所で辿れるようにする。
- Scope:
  - `docs/10_review_spec_priority_plan.md`
  - `docs/team_runbook.md`
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/README.md`
  - 必要時のみ `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
- Acceptance:
  - A-team history/foundation bundle の current command surface と focused self-test entrypoint が runbook / review-spec / acceptance matrix で矛盾しない。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が継続して PASS する。
  - PM が A-team 次タスクと self-test entrypoint を docs だけで追跡できる。

### A-20 (Auto-Next)
- Status: `Todo`
- Goal: A-team foundation/history bundle の focused docs-sync validator を handoff Section 0 にも同期し、PM が queue + handoff だけでも current command surface と self-test entrypoint を辿れるようにする。
- Scope:
  - `docs/abc_team_chat_handoff.md`
  - `docs/fem4c_team_next_queue.md`
  - 必要時のみ `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
- Current command surface:
  - `make -C FEM4C mbd_system2d_history_contract_smoke`
    history-only current command surface。generalized-force history の probe + CLI/system summary contract だけを bundle として確認する。
  - `make -C FEM4C mbd_a_team_foundation_smoke`
    full foundation current command surface。history contract 再利用 bundle を含む rigid MBD foundation 全体を確認する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test`
    focused self-test entrypoint。queue / handoff / runbook / review-spec / acceptance matrix の A-team surface が上の 2 コマンドと矛盾していないことを確認する。
  - Summary: `mbd_system2d_history_contract_smoke` = history-only current command surface, `mbd_a_team_foundation_smoke` = full foundation current command surface, `mbd_run1_surface_docs_sync_test` = focused self-test entrypoint.
- Acceptance:
  - A-team history/foundation bundle の current command surface と focused self-test entrypoint が handoff / queue / runbook / review-spec / acceptance matrix で矛盾しない。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が継続して PASS する。
  - PM が handoff Section 0 と queue だけで A-team current docs surface を辿れる。

### A-21 (Auto-Next)
- Status: `Todo`
- Goal: A-team focused docs-sync validator の doc-source inventory と current-command inventory を PM 向けの inspection surface として固定する。
- Scope:
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync_surfaces.sh`
  - 必要時のみ `docs/fem4c_team_next_queue.md`
- Acceptance:
  - `--print-doc-sources` が `README / acceptance / runbook / review-plan / handoff / queue / Makefile` を stable に返す。
  - `--print-current-command-surface` が A-team history/foundation entrypoints を含む current command inventory を返す。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_test` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が PASS する。

### A-22 (Auto-Next)
- Status: `Todo`
- Goal: A-team focused docs-sync validator の inspection surface を専用 summary option まで縮約し、PM が A-team だけの再開導線を 1 コマンドで取得できるようにする。
- Scope:
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync_surfaces.sh`
  - 必要時のみ `FEM4C/README.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `--print-a-team-surface-summary` が `history/foundation/selftest` の 3 entrypoint と `review-plan / runbook / acceptance / handoff / queue` を stable に返す。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_test` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が PASS する。

## 6. Bチーム
### B-R1 (Run 1 Priority)
- Status: `Done`
- Goal: `src/mbd/system2d.c` の `fclose` 起因 warning を局所修正で解消する。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - `-Wuse-after-free` warning が消える。
  - runtime behavior は変えない。

### B-R2 (Run 2 Priority)
- Status: `Done`
- Goal: HHT-alpha step を non-trivial rigid 2-link の formal acceptance へ接続する。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/integrator_hht2d.c`
  - 必要時のみ `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - 必要時のみ `FEM4C/scripts/test_compare_2link_rigid_analytic*.sh`
- Acceptance:
  - rigid 2-link の HHT run が `mbd_rigid_analytic_hht_compare_test` と `mbd_m1_rigid_acceptance` へ接続される。
  - compare / normalize route が summary の `history_snapshot_count` / `rigid_compare_snapshot_count` を provenance として追跡できる。

### B-01
- Status: `Done`
- Goal: `mbd_system2d_t` を新設し、body/constraint/gravity/time control を保持する。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - 必要時のみ `FEM4C/src/mbd/constraint2d.h`
- Acceptance:
  - runtime state が `runner.c` ローカル配列に残らない方針へ前進している。

### B-02
- Status: `Done`
- Goal: dense KKT assembler を作る。
- Acceptance:
  - rigid 2-link の KKT 行列と RHS が数値で出せる。
  - `make -C FEM4C mbd_assembler2d_probe_smoke` / `make -C FEM4C mbd_assembler2d_smoke` が PASS する。

### B-03
- Status: `Done`
- Goal: 小規模 dense solver を作る。
- Acceptance:
  - KKT の小規模系を単体で解ける。
  - `make -C FEM4C mbd_dense_solver_probe_smoke` / `make -C FEM4C mbd_dense_solver_singular_smoke` / `make -C FEM4C mbd_dense_solver_invalid_smoke` が PASS する。

### B-04 (Auto-Next)
- Status: `Done`
- Goal: acceleration-level constraint RHS を作る。
- Scope:
  - `FEM4C/src/mbd/constraint2d.c`
  - `FEM4C/src/mbd/assembler2d.c`
- Acceptance:
  - explicit / implicit 共通で constraint RHS を使える。
  - `make -C FEM4C mbd_constraint_rhs_probe_smoke` と `make -C FEM4C mbd_b_team_foundation_smoke` が PASS する。

### B-05
- Status: `Done`
- Goal: Newmark-beta の器を作る。
- Scope:
  - `FEM4C/src/mbd/integrator_newmark2d.h`
  - `FEM4C/src/mbd/integrator_newmark2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - unconstrained single body で Newmark 更新が動く。
  - `make -C FEM4C mbd_newmark2d_smoke` と `make -C FEM4C mbd_system2d_newmark_probe_smoke` が PASS する。

### B-06 (Auto-Next)
- Status: `Done`
- Goal: Newmark-beta implicit step を完成させる。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/integrator_newmark2d.c`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - rigid 2-link の Newmark 計算が 1 run 完了する。
  - constrained/free の system-owned update が共通 helper 経由で保守できる状態へ前進している。

### B-07
- Status: `Done`
- Goal: HHT-alpha の係数計算と前段 residual hook を固定する。
- Scope:
  - `FEM4C/src/mbd/integrator_hht2d.h`
  - `FEM4C/src/mbd/integrator_hht2d.c`
  - 必要時のみ `FEM4C/src/mbd/system2d.c`
- Acceptance:
  - `alpha ∈ [-1/3,0]` の validation が helper 内に固定される。
  - modified Newton / effective residual へ渡す前段 API が分離される。
  - `make -C FEM4C mbd_hht2d_probe_smoke mbd_hht2d_invalid_smoke` が PASS する。

### B-08 (Auto-Next)
- Status: `Done`
- Goal: HHT-alpha step を完成させる。
- Scope:
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/mbd/integrator_hht2d.c`
- Acceptance:
  - rigid 2-link の HHT 計算が 1 run 完了する。
  - predictor / residual / update の各段が system-owned helper で保守できる状態へ前進している。

### B-09 (Auto-Next)
- Status: `Done`
- Goal: constraint projection を入れる。
- Scope:
  - `FEM4C/src/mbd/projection2d.h`
  - `FEM4C/src/mbd/projection2d.c`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/practice/ch09/mbd_system2d_projection_compare_probe.c`
  - `FEM4C/Makefile`
- Acceptance:
  - 長時間積分で constraint drift が減る。
  - `mbd_system2d_projection_compare_smoke` と `mbd_system2d_projection_history_output_smoke` が PASS する。
  - isolated build でも projection/history surface が維持される。

### B-10 (Auto-Next)
- Status: `Done`
- Goal: history CSV projection/implicit field index 契約を single-source 化し、history consumer の raw column drift を防ぐ。
- Scope:
  - `FEM4C/src/mbd/output2d.h`
  - `FEM4C/practice/ch09/mbd_probe_utils.h`
  - `FEM4C/practice/ch09/mbd_output2d_probe.c`
  - `FEM4C/practice/ch09/mbd_system2d_*history*_probe.c`
  - 必要時のみ `FEM4C/scripts/test_mbd_output2d_history_field_count_sync.sh`
- Acceptance:
  - `make -C FEM4C mbd_output2d_history_field_count_sync_smoke` が PASS する。
  - `make -C FEM4C mbd_b_team_foundation_probe_smoke` が PASS する。
  - `make -C FEM4C mbd_b_team_foundation_isolated_smoke` が PASS する。

### B-11 (Auto-Next)
- Status: `Done`
- Goal: rigid compare review/artifact route metadata 契約を single-source 化し、wrapper / manifest consumer の raw route-row・field-name drift を防ぐ。
- Scope:
  - `FEM4C/scripts/compare_2link_artifact_route_fields.sh`
  - `FEM4C/scripts/get_compare_2link_artifact_route_fields.sh`
  - `FEM4C/scripts/compare_2link_artifact_route_fields.py`
  - `FEM4C/scripts/check_compare_2link_artifact_manifest.py`
  - `FEM4C/scripts/test_compare_2link_artifact_route_fields_sync.sh`
  - `FEM4C/scripts/test_get_compare_2link_artifact_route_fields.sh`
  - 必要時のみ `FEM4C/scripts/test_make_compare_2link_artifact_*.sh`
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_route_fields_getter_test` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_review_smoke` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_route_review_smoke` が PASS する。
  - `make -C FEM4C mbd_m1_rigid_acceptance_test` が PASS する。

### B-12 (Auto-Next)
- Status: `Done`
- Goal: compare artifact target/integrator contract を shell helper 起点で single-source 化し、core wrapper / Makefile / rigid+flex self-test の raw literal drift を防ぐ。
- Scope:
  - `FEM4C/scripts/compare_2link_artifact_targets.sh`
  - `FEM4C/scripts/get_compare_2link_artifact_targets.sh`
  - `FEM4C/scripts/compare_2link_artifact_targets.py`
  - `FEM4C/scripts/compare_2link_artifact_integrators.sh`
  - `FEM4C/scripts/get_compare_2link_artifact_integrators.sh`
  - `FEM4C/scripts/compare_2link_artifact_integrators.py`
  - `FEM4C/scripts/check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/run_e08_rigid_analytic_*.sh`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_c16_flex_reference_compare.sh`
  - `FEM4C/scripts/test_compare_2link_artifact_*`
  - `FEM4C/scripts/test_check_compare_2link_artifact_*`
  - `FEM4C/scripts/test_make_compare_2link_artifact_*`
  - `FEM4C/scripts/test_compare_2link_rigid_analytic_*`
  - `FEM4C/scripts/test_compare_2link_flex_*`
  - `FEM4C/Makefile`
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_targets_getter_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_targets_sync_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_integrators_getter_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_integrators_sync_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_route_review_smoke` が PASS する。
  - `make -C FEM4C mbd_m1_rigid_acceptance_test` が PASS する。

### B-13 (Auto-Next)
- Status: `In Progress`
- Goal: compare artifact helper contract の docs/help/current-command surface を Run 1 docs-sync targetへ固定し、README / acceptance matrix / runbook / Make help / validator self-surface の drift を防ぐ。
- Scope:
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync.sh`
  - `FEM4C/scripts/test_check_mbd_run1_surface_docs_sync_surfaces.sh`
  - `FEM4C/scripts/test_make_mbd_run1_surface_docs_sync_surfaces_help.sh`
  - `FEM4C/scripts/test_make_mbd_run1_surface_docs_sync_surface_smoke.sh`
  - `FEM4C/README.md`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/team_runbook.md`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - `make -C FEM4C mbd_run1_surface_docs_sync_test` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_test` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surfaces_help_test` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surface_smoke` が PASS する。
  - `make -C FEM4C mbd_run1_surface_docs_sync_surface_smoke_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。
  - `make -C FEM4C mbd_rigid_compare_route_review_smoke` が PASS する。
  - `make -C FEM4C mbd_m1_rigid_acceptance_test` が PASS する。

## 7. Cチーム
### C-R1 (Run 1 Priority)
- Status: `Done`
- Goal: Q4/T3 の stiffness function pointer warning を adapter または整合 wrapper で解消する。
- Scope:
  - `FEM4C/src/elements/q4/q4_element.c`
  - `FEM4C/src/elements/t3/t3_element.c`
  - 必要時のみ `FEM4C/src/elements/t6/t6_element.c`
- Acceptance:
  - `make clean && make -j2` で Q4/T3 の incompatible pointer warning が消える。

### C-R2 (Run 3 Priority)
- Status: `Done`
- Goal: `coupled_step_common2d.{c,h}` の helper 抽出を開始し、explicit/implicit 共通部の重複を 1 つ減らす。
- Scope:
  - `FEM4C/src/coupled/coupled_step_common2d.h`
  - `FEM4C/src/coupled/coupled_step_common2d.c`
  - `FEM4C/src/coupled/coupled_step_explicit2d.c`
  - `FEM4C/src/coupled/coupled_step_implicit2d.c`
- Acceptance:
  - node set build / pose capture / reaction apply のうち少なくとも 1 系統が共通 helper 化される。
  - behavior change を伴わない。

### C-R3 (Review-Spec Auto-Next)
- Status: `Done`
- Goal: `C-R2` で抽出した helper path を parser-free smoke と focused review-spec bundle で固定する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_q4_t3_stiffness_adapter_warnings.sh`
  - `FEM4C/scripts/test_coupled_step_common2d.sh`
  - `FEM4C/scripts/test_coupled_step_common2d_solve.sh`
- Acceptance:
  - `make -C FEM4C q4_t3_stiffness_adapter_warning_test` が PASS する。
  - `make -C FEM4C coupled_step_common2d_test` と `make -C FEM4C coupled_step_common2d_solve_test` が PASS する。
  - `make -C FEM4C c_review_spec_smoke_test` が PASS する。

### C-01
- Status: `Done`
- Goal: `make -j2` が通る build recovery を実施する。
- Scope:
  - `FEM4C/src/elements/t6/t6_element.c`
  - 必要時のみ `FEM4C/src/elements/element_base.h`
- Acceptance:
  - `make -j2` が成功する。
  - warning を増やさない。

### C-02
- Status: `Done`
- Goal: globals ベースの FE model を deep copy 可能にする。

### C-03
- Status: `Done`
- Goal: model-centric assembly API を作る。
- Acceptance:
  - `flex_solver2d_prepare_model()` / `flex_solver2d_assemble_full_mesh()` が populated model を扱える。
  - runtime BC を持つ model snapshot が host globals を汚さずに full assembly を再利用できる。

### C-04
- Status: `Done`
- Goal: Dirichlet BC を runtime で差し替えられるようにする。
- Acceptance:
  - `flex_bc2d_list_append()` が同一 `node_id/dof` の再指定を override として扱う。
  - `flex_bc2d_build_node_set_entries()` と FE solve smoke で step ごとの BC 差し替えが確認できる。

### C-05
- Status: `Done`
- Goal: full mesh 再アセンブルを明示化する。
- Acceptance:
  - `flex_solver2d_reassemble_and_solve()` ごとに `full_reassembly_count` が進む。
  - `static_solve_count` と合わせて per-model の再アセンブル/solve 監査の土台がある。

### C-06
- Status: `Done`
- Goal: full reassembly のログを出す。
- Acceptance:
  - coupled output に各 flexible body の `full_reassembly_count` / `static_solve_count` が記録される。
  - step 単位でも `coupling_iteration_index` と紐づく counter 行が残る。
  - integrator switch smoke でも counter 出力列が維持される。

### C-07
- Status: `Done`
- Goal: nodeset データを専用モジュールとして固定する。
- Acceptance:
  - `flex_nodeset.*` に `node_set_contains()` / `node_set_center()` / `node_set_local_coordinates()` が揃う。
  - root/tip interface が `node_set_t` で管理され、duplicate node guard がある。

### C-08
- Status: `Done`
- Goal: inertial equivalent load の受け口を作る。
- Acceptance:
  - runtime body-force 相当の入口が `flex_solver2d` 側にあり、snapshot solve へ注入できる。

### C-09
- Status: `Done`
- Goal: 変形形状の snapshot 出力を作る。
- Acceptance:
  - local FE displacement を world 座標へ写した CSV が出力される。
  - body_id / step / iteration を含む snapshot ファイル名が固定される。
  - compare 側スクリプトが iteration 行あり/なしの両方を読める。

### C-10 (Auto-Next)
- Status: `Done`
- Goal: snapshot CSV schema を解析/比較向けに拡張する。
- Acceptance:
  - snapshot CSV に `x_local_def` / `y_local_def` / `x_world_ref` / `y_world_ref` / `ux_world` / `uy_world` が出力される。
  - `flex_snapshot2d_build_output_path()` が public helper として使える。
  - `make -C FEM4C flex_snapshot2d_test` が PASS する。

### C-11 (Auto-Next)
- Status: `Done`
- Goal: accepted-step snapshot の summary manifest を固定する。
- Acceptance:
  - coupled summary に `snapshot_columns` / `snapshot_record` が出力される。
  - `make -C FEM4C coupled_snapshot_output_test` と `make -C FEM4C coupled_implicit_snapshot_output_test` が PASS する。

### C-12 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit compare helper が snapshot manifest を優先利用できるようにする。
- Acceptance:
  - `compare_rigid_limit_2link.py` が `snapshot_record` を優先し、未記録時のみ glob fallback する。
  - `make -C FEM4C coupled_rigid_limit_compare_test` と `make -C FEM4C coupled_rigid_limit_manifest_test` が PASS する。

### C-13 (Auto-Next)
- Status: `Done`
- Goal: integrator success matrix に snapshot manifest 契約を織り込む。
- Acceptance:
  - `scripts/check_coupled_integrators.sh` が success case で `snapshot_columns` / `snapshot_record` を検証する。
  - `cd FEM4C && bash scripts/check_coupled_integrators.sh` が PASS する。

### C-14 (Auto-Next)
- Status: `Done`
- Goal: snapshot manifest producer/consumer 契約を broader acceptance へ展開する。
- Acceptance:
  - rigid-limit 以外の compare / acceptance helper でも `snapshot_record` を優先利用する経路が追加される。
  - manifest-first fallback を複数 helper で共有できる。

### C-15 (Auto-Next)
- Status: `Done`
- Goal: real 2-link acceptance で normalized flex compare artifact を固定する。
- Acceptance:
  - 実際の coupled 2-link run を入力に `compare_2link_flex_reference.py --fem-summary` を回す経路が 1 コマンドで再現できる。
  - example acceptance が `snapshot_record` producer 契約だけでなく、normalized schema artifact 生成まで監査する。

### C-16 (Auto-Next)
- Status: `Done`
- Goal: flex compare mode でも normalized FEM artifact を併記できるようにする。
- Acceptance:
  - `compare_2link_flex_reference.py --reference-csv --normalized-fem-csv` が compare CSV / PNG に加えて normalized FEM schema CSV も出力する。
  - `make -C FEM4C coupled_flex_reference_compare_test` が PASS する。

### C-17 (Auto-Next)
- Status: `Done`
- Goal: example acceptance の stdout から normalized artifact path を追えるようにする。
- Acceptance:
  - `scripts/check_coupled_2link_examples.sh` が `normalized_artifact_columns` / `normalized_artifact` 行を出力する。
  - `make -C FEM4C coupled_example_check` が PASS する。

### C-18 (Auto-Next)
- Status: `Done`
- Goal: rigid/flex compare artifact の最小スイートを 1 コマンド target として固定する。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check` が PASS する。
  - rigid analytic / flex normalize / flex compare-mode の 3 経路が同じ target から再現できる。

### C-19 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite の manifest を stdout とファイルの両方で固定する。
- Acceptance:
  - `scripts/check_compare_2link_artifacts.sh` が `compare_suite_manifest=` を出力し、manifest CSV を保存する。
  - `make -C FEM4C compare_2link_artifact_check_test` が PASS する。

### C-20 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite target が出力先 override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check OUT_DIR=<dir> MANIFEST_CSV=<path>` が指定先へ artifact と manifest を生成する。
  - `make -C FEM4C compare_2link_artifact_check_vars_test` が PASS する。

### C-21 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite target が integrator override も尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_check INTEGRATOR=newmark_beta` が integrator 別 case stem で artifact を生成する。
  - `make -C FEM4C compare_2link_artifact_check_integrator_test` が PASS する。

### C-22 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite を explicit / newmark_beta / hht_alpha の matrix target で再現できるようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_matrix_check` が PASS する。
  - `make -C FEM4C compare_2link_artifact_matrix_check_test` が PASS する。

### C-23 (Auto-Next)
- Status: `Done`
- Goal: compare artifact matrix target が `INTEGRATORS` subset と `EXPECTED_INTEGRATORS` validator override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_matrix_integrators_test` が PASS する。
  - `make -C FEM4C compare_2link_artifact_matrix_manifest_expected_integrators_test` が PASS する。

### C-24 (Auto-Next)
- Status: `Done`
- Goal: compare artifact matrix stdout から per-target artifact を追跡でき、unsupported integrator は fail-fast するようにする。
- Acceptance:
  - `scripts/check_compare_2link_artifact_matrix.sh` が `compare_matrix_artifact_columns` / `compare_matrix_artifact` を出力する。
  - `make -C FEM4C compare_2link_artifact_matrix_invalid_integrator_test` が PASS する。

### C-25 (Auto-Next)
- Status: `Done`
- Goal: compare artifact self-test 群を Cチーム向けの 1 コマンド target に束ねる。
- Acceptance:
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。

### C-26 (Auto-Next)
- Status: `Done`
- Goal: focused coupled compare / manifest check を 1 コマンド target に束ねる。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks` が PASS する。

### C-27 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の nested make 出力を PM 監査向けの stable summary 行へ整形する。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks` の wrapper が `coupled_compare_suite_columns` / `coupled_compare_suite` を出力する。
  - focused compare / manifest suite の pass/fail を nested make の雑多なログに埋もれず追える。

### C-28 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` wrapper が `OUT_DIR` override を尊重するようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks OUT_DIR=<dir>` が log を指定先へ生成する。
  - `make -C FEM4C coupled_compare_checks_out_dir_test` が PASS する。

### C-29 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が aggregate manifest も保存し、summary 行とファイルの両方から監査できるようにする。
- Acceptance:
  - wrapper が `coupled_compare_suite_manifest=` を出力し、target/status/log_path を束ねた CSV を保存する。
  - PM が nested log を読まなくても suite 全体の pass/fail を追える。

### C-30 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` aggregate manifest の validator target を用意する。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks_manifest_test` が PASS する。
  - manifest の target/status/log_path 契約が機械的に検証できる。

### C-31 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` と manifest validator が custom `MANIFEST_CSV` override でも整合するようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_checks OUT_DIR=<dir> MANIFEST_CSV=<path>` が指定先 manifest を生成する。
  - `make -C FEM4C coupled_compare_checks_manifest_test MANIFEST_CSV=<path>` が PASS する。

### C-32 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が target subset override でも stable summary / manifest を維持するようにする。
- Acceptance:
  - wrapper が `CHECK_TARGETS="coupled_example_check compare_2link_artifact_checks"` のような subset 指定を受け付ける。
  - summary 行と aggregate manifest が subset 実行でも整合する。

### C-33 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks_manifest_test` が subset 実行時の expected target override も受け付けるようにする。
- Acceptance:
  - subset manifest に対しても validator が target 数と順序を正しく検証できる。
  - `CHECK_TARGETS` と validator 側の expected targets 指定が矛盾なく運用できる。

### C-34 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` が fail 時にも summary 行と manifest に failure reason を残せるようにする。
- Acceptance:
  - wrapper が fail target を summary 行で特定できる。
  - aggregate manifest だけでも fail target の特定に必要な最小情報が追える。

### C-35 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` fail-path の `result_note` について validator 側でも expected note 契約を持てるようにする。
- Acceptance:
  - failfast manifest に対しても `result_note` の最低契約を機械検証できる。
  - pass/fail 両経路で manifest contract が揃う。

### C-36 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の fail note を make 固有文言から正規化し、PM が横断監査しやすい短い reason code へ寄せる。
- Acceptance:
  - fail target の `result_note` が長い raw make log ではなく、比較的安定した短い reason で残る。
  - wrapper と validator がその reason 形式に追従する。

### C-37 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` fail note の reason code を enum として文書化し、将来の追加 reason でも互換を保てるようにする。
- Acceptance:
  - wrapper / validator / self-test が reason code の追加規約を共有する。
  - PM が `result_note` を見て fail 分類を安定解釈できる。

### C-38 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` reason code の一覧を runbook/queue へ同期し、PM 監査基準を文書化する。
- Acceptance:
  - `result_note=pass|make_missing_target|make_failed|FAIL:*` のような契約が文書に残る。
  - 新規 reason code 追加時の更新先 `FEM4C/scripts/coupled_compare_reason_codes.sh`, `FEM4C/scripts/check_coupled_compare_checks_manifest.py`, `docs/team_runbook.md`, `docs/fem4c_team_next_queue.md` が明記される。

### C-39 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` reason code 契約を machine-readable な printer target と self-test で固定し、PM が wrapper 実行前でも正本を取得できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_codes` が `coupled_compare_reason_codes=` と `coupled_compare_reason_code_update_points=` を出力する。
  - `make -C FEM4C coupled_compare_reason_codes_print_test` が PASS する。
  - wrapper 契約 self-test は coupled solver の実行成否から独立して PASS する。

### C-40 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root 監査 wrapper を固定し、PM が `FEM4C/` 配下へ移動せずに one-shot 実行できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_contract_audit.sh` が `contract_audit_target=`, `contract_audit_mode=`, `contract_audit_log_path=`, `contract_audit_cache_log=`, `contract_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit_stdout.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_audit_nested_log_dir.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root wrapper の使用箇所が明記される。

### C-41 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root stack wrapper を固定し、PM が FEM4C bundle と repo-root wrapper modes を 1 コマンドで回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_contract_stack.sh` が `contract_stack_components=`, `contract_stack_out_dir=`, `contract_stack_bundle_log=`, `contract_stack_audit_modes_log=`, `contract_stack_contract_report_log=`, `contract_stack_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_modes.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root stack wrapper の使用箇所が明記される。

### C-42 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の PM surface wrapper を固定し、PM が repo root の 1 コマンドで FEM4C bundle / audit wrapper modes / stack wrapper modes を同時回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_pm_surface.sh` が `pm_surface_components=`, `pm_surface_out_dir=`, `pm_surface_fem4c_log=`, `pm_surface_audit_modes_log=`, `pm_surface_stack_modes_log=`, `pm_surface_contract_report_log=`, `pm_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_modes.sh` が PASS する。
  - `docs/team_runbook.md` に PM surface wrapper の使用箇所が明記される。

### C-43 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root entrypoint bundle を固定し、PM が audit/stack/PM surface の 3 系列を 1 コマンドで回帰確認できるようにする。
- Acceptance:
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes.sh` が PASS する。
  - `bash scripts/run_coupled_compare_reason_code_root_modes.sh` が `root_modes_components=`, `root_modes_out_dir=`, `root_modes_audit_log=`, `root_modes_stack_log=`, `root_modes_pm_surface_log=`, `root_modes_pm_surface_contract_log=`, `root_modes_pm_surface_contract_report_log=`, `root_modes_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes_wrapper_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root mode bundle の使用箇所が明記される。

### C-44 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root 最上位 wrapper を固定し、PM が root surface 1 コマンドで PM surface と root mode bundle を同時回収できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface.sh` が `root_surface_components=`, `root_surface_out_dir=`, `root_surface_pm_surface_log=`, `root_surface_root_modes_log=`, `root_surface_contract_report_log=`, `root_surface_root_modes_contract_report_log=`, `root_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root surface wrapper の使用箇所が明記される。

### C-45 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root surface 提出ログ validator を固定し、PM が最上位 wrapper の transitive log 欠落を 1 コマンドで検出できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` が `root_surface_components`, `root_surface_out_dir`, `root_surface_pm_surface_log`, `root_surface_root_modes_log`, `root_surface_contract_report_log`, `root_surface_root_modes_contract_report_log`, `root_surface_result` を検証し、nested log 欠落時に fail する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_report.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_report_missing_nested_log.sh` が PASS する。
  - `docs/team_runbook.md` に validator の使用箇所が明記される。

### C-46 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root audited entrypoint を固定し、PM が root surface wrapper と validator を 1 コマンドで実行できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh` が `root_surface_audit_components=`, `root_surface_audit_out_dir=`, `root_surface_audit_log=`, `root_surface_audit_contract_report_log=`, `root_surface_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_default_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_nested_out_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_modes.sh` が PASS する。
  - `docs/team_runbook.md` に root surface audit wrapper の使用箇所が明記される。

### C-47 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の docs-sync checker に root surface validator / audit wrapper を組み込み、runbook/queue drift を自動検出できるようにする。
- Acceptance:
  - `FEM4C/scripts/check_coupled_compare_reason_code_docs_sync.sh` が `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py <root_surface_log>` と `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh [out_dir]` の runbook/queue 記載を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_report.py --print-required-keys` の記載が runbook/queue の docs-sync 対象に含まれる。
  - `make -C FEM4C coupled_compare_reason_code_docs_sync_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の root surface validator / audit wrapper 記述が docs-sync の検査対象として維持される。

### C-48 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の root surface focused bundle target を FEM4C Makefile に固定し、C側が repo root wrapper へ依存せず root surface 契約一式を 1 target で回帰できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が PASS する。
  - `docs/team_runbook.md` に bundle target の使用箇所が明記される。

### C-49 (Auto-Next)
- Status: `Done`
- Goal: coupled_compare reason-code contract の repo-root audit wrapper を固定し、PM が root surface focused bundle を 1 コマンドで logfile/stdout 両モード監査できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_audit.sh` が `root_surface_contract_audit_target=`, `root_surface_contract_audit_mode=`, `root_surface_contract_audit_log_path=`, `root_surface_contract_audit_cache_log=`, `root_surface_contract_audit_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_stdout.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_nested_log_dir.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_audit_modes.sh` が PASS する。
  - `docs/team_runbook.md` に repo-root audit wrapper の使用箇所が明記される。

### C-50 (Auto-Next)
- Status: `Done`
- Goal: focused root-surface contract audit wrapper の提出ログ validator を固定し、PM が wrapper 出力と logfile/stdout 境界を 1 コマンドで検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py <audit_report_log>` が `root_surface_contract_audit_target`, `root_surface_contract_audit_mode`, `root_surface_contract_audit_log_path`, `root_surface_contract_audit_cache_log`, `root_surface_contract_audit_result` を検証し、logfile/cache log 欠落時に fail する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_audit_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_audit_report_test` が PASS する。
  - `docs/team_runbook.md` に validator の使用箇所が明記される。

### C-51 (Auto-Next)
- Status: `Done`
- Goal: focused root-surface contract suite に C-50 validator を組み込み、C 側の 1 target で bundle + audit wrapper + report validator を回帰できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` が C-50 validator self-test を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が C-50 validator pass lines まで grep して PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の focused bundle 記述が docs-sync で維持される。

### C-52 (Auto-Next)
- Status: `Done`
- Goal: shared audit cache helper を runbook / docs-sync / focused bundles に固定し、contract/root-surface 両系統が helper 回帰を bundle 内で共有できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_audit_cache_test` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_contract_checks_test` が audit cache helper pass line を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が audit cache helper pass line を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_docs_sync_test` が runbook / queue の audit cache helper 記述を含めて PASS する。

### C-53 (Auto-Next)
- Status: `Done`
- Goal: contract audit wrapper の提出ログ validator を固定し、PM が contract 側も `audit_report_log` 1 本で `log_path/cache_log` 境界まで fail-fast 検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_contract_audit_report.py <audit_report_log>` が `contract_audit_target`, `contract_audit_mode`, `contract_audit_log_path`, `contract_audit_cache_log`, `contract_audit_result` を検証し、logfile/cache log 欠落時に fail する。
  - `python3 scripts/check_coupled_compare_reason_code_contract_audit_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_contract_audit_report_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の validator 記述が docs-sync で維持される。

### C-54 (Auto-Next)
- Status: `Done`
- Goal: focused contract suite に contract audit report validator を組み込み、contract 側の 1 target で bundle + audit wrapper + report validator を回帰できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_contract_checks` が contract audit report validator self-test を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_contract_checks_test` が contract audit report validator pass lines まで grep して PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の focused contract bundle 記述が docs-sync で維持される。

### C-55 (Auto-Next)
- Status: `Done`
- Goal: repo-root contract stack / PM surface からも contract audit report validator の coverage を追跡できるようにし、上位 wrapper 監査で contract validator の存在を見失わないようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_contract_stack.sh [out_dir]` が `contract_stack_contract_report_log=` を出力し、contract audit report validator の coverage 手掛かりを残す。
  - `bash scripts/run_coupled_compare_reason_code_pm_surface.sh [out_dir]` が `pm_surface_contract_report_log=` を出力し、contract audit report validator の coverage 手掛かりを残す。
  - `bash scripts/test_run_coupled_compare_reason_code_contract_stack_modes.sh` が新 surface を含めて PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_pm_surface_modes.sh` が新 surface を含めて PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の stack / PM surface 記述が docs-sync で維持される。

### C-56 (Auto-Next)
- Status: `Done`
- Goal: root_modes / root_surface からも contract audit report validator の coverage を追跡できるようにし、最上位 wrapper 監査で contract validator の存在を end-to-end で見えるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_modes.sh [out_dir]` が `root_modes_pm_surface_contract_log=`, `root_modes_pm_surface_contract_report_log=` を出力し、contract audit report coverage の手掛かりを残す。
  - `bash scripts/run_coupled_compare_reason_code_root_surface.sh [out_dir]` が `root_surface_contract_report_log=`, `root_surface_root_modes_contract_report_log=` を出力し、最上位 wrapper から contract audit report coverage を追跡できる。
  - `bash scripts/test_run_coupled_compare_reason_code_root_modes.sh` が新 surface を含めて PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_modes.sh` が新 surface を含めて PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の root entrypoint 記述が docs-sync で維持される。

### C-57 (Auto-Next)
- Status: `Done`
- Goal: root_surface_audit 側にも contract audit report coverage の handoff を持ち込み、最上位 audited entrypoint から contract validator の存在を 1 本の提出ログで追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_audit.sh [out_dir]` が `root_surface_audit_contract_report_log=` を出力し、root surface 経由の contract audit report coverage を残す。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit.sh` が新 surface を含めて PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_modes.sh` が新 surface を含めて PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の audited entrypoint 記述が docs-sync で維持される。

### C-58 (Auto-Next)
- Status: `Done`
- Goal: root_surface_audit 提出ログ validator を固定し、PM が最上位 audited entrypoint の metadata と contract-report handoff を 1 本の提出ログで fail-fast 検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_audit_report.py <audit_report_log>` が `root_surface_audit_components`, `root_surface_audit_out_dir`, `root_surface_audit_log`, `root_surface_audit_contract_report_log`, `root_surface_audit_result` を検証し、audit log / contract report log の欠落・不一致・親dir外参照を fail-fast する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_audit_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_audit_report_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の audit report validator 記述が docs-sync で維持される。

### C-59 (Auto-Next)
- Status: `Done`
- Goal: root_surface_audit wrapper と audit-report validator を 1 コマンドへ束ね、PM が audited entrypoint と validator coverage を repo-root surface 1 本で再確認できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_audit_surface.sh [out_dir]` が `root_surface_audit_surface_components=`, `root_surface_audit_surface_out_dir=`, `root_surface_audit_surface_report_log=`, `root_surface_audit_surface_validator_log=`, `root_surface_audit_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_surface.sh` が PASS する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_audit_surface_modes.sh` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の surface wrapper 記述が docs-sync で維持される。

### C-60 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract の focused bundle に audited surface wrapper を編入し、PM が 1 target で root surface / audit / audited surface / contract audit の契約を再確認できるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks` が audited surface wrapper 回帰を bundle に含めて PASS する。
  - `bash FEM4C/scripts/test_make_coupled_compare_reason_code_root_surface_contract_checks.sh` が PASS する。
  - `docs/team_runbook.md` に bundle が audited surface wrapper を含むことが明記される。

### C-61 (Auto-Next)
- Status: `Done`
- Goal: root-surface audited surface wrapper の report/validator path を 1 行 summary でも追跡できるよう、bundle 側の pass surface を追加で固定する。
- Acceptance:
  - `root_surface_audit_surface_components=`, `root_surface_audit_surface_out_dir=`, `root_surface_audit_surface_report_log=`, `root_surface_audit_surface_validator_log=`, `root_surface_audit_surface_result=pass` が `FEM4C/scripts/test_make_coupled_compare_reason_code_root_surface_contract_checks.sh` の bundle log からも追跡できる。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_checks_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の bundle 記述が docs-sync で維持される。

### C-62 (Auto-Next)
- Status: `Done`
- Goal: focused root-surface contract bundle log validator を追加し、saved bundle log から `root_surface_audit_surface_*` metadata と required pass lines を再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_report.py <bundle_log>` が `root_surface_audit_surface_components=`, `root_surface_audit_surface_out_dir=`, `root_surface_audit_surface_report_log=`, `root_surface_audit_surface_validator_log=`, `root_surface_audit_surface_result=pass` と required pass lines を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report_missing_key.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report_print_required_keys.sh` が PASS する。

### C-63 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle と bundle-log validator を repo-root 1 コマンドへ束ね、PM が bundle 実行結果と再検証ログを surface wrapper 1 本で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface.sh [out_dir]` が `root_surface_contract_bundle_surface_components=`, `root_surface_contract_bundle_surface_out_dir=`, `root_surface_contract_bundle_surface_bundle_log=`, `root_surface_contract_bundle_surface_validator_log=`, `root_surface_contract_bundle_surface_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の surface wrapper 記述が docs-sync で維持される。

### C-64 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface 提出ログ validator を追加し、saved surface log から `root_surface_contract_bundle_surface_*` metadata と validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.py <surface_log>` が `root_surface_contract_bundle_surface_components=`, `root_surface_contract_bundle_surface_out_dir=`, `root_surface_contract_bundle_surface_bundle_log=`, `root_surface_contract_bundle_surface_validator_log=`, `root_surface_contract_bundle_surface_result=pass` を検証し、bundle/validator log の欠落・不一致・親dir外参照を fail-fast する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_report_test` が PASS する。

### C-65 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface 提出ログ validator の negative coverage を広げ、wrong-component / escaped-path / nested mismatch を focused self-test で固定する。
- Acceptance:
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_wrong_component.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_escape.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_nested_mismatch.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_report_test` が negative coverage を含めて PASS する。

### C-66 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface wrapper と surface-log validator を repo-root 1 コマンドへ束ね、PM が saved surface log と validator log を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh [out_dir]` が `root_surface_contract_bundle_surface_report_components=`, `root_surface_contract_bundle_surface_report_out_dir=`, `root_surface_contract_bundle_surface_report_surface_log=`, `root_surface_contract_bundle_surface_report_validator_log=`, `root_surface_contract_bundle_surface_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_test` が PASS する。
  - `docs/team_runbook.md` と `docs/fem4c_team_next_queue.md` の wrapper 記述が docs-sync で維持される。

### C-67 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface report 提出ログ validator を追加し、saved report wrapper log から `root_surface_contract_bundle_surface_report_*` metadata と validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.py <report_wrapper_log>` が `root_surface_contract_bundle_surface_report_components=`, `root_surface_contract_bundle_surface_report_out_dir=`, `root_surface_contract_bundle_surface_report_surface_log=`, `root_surface_contract_bundle_surface_report_validator_log=`, `root_surface_contract_bundle_surface_report_result=pass` を検証し、surface/validator log の欠落・不一致・親dir外参照を fail-fast する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_test` が PASS する。

### C-68 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface report 提出ログ validator の negative coverage を広げ、wrong-component / escaped-path / nested mismatch を focused self-test で固定する。
- Acceptance:
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_wrong_component.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_escape.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_nested_mismatch.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_test` が negative coverage を含めて PASS する。

### C-69 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface report wrapper と wrapper-report validator を repo-root 1 コマンドへ束ね、PM が saved report wrapper log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh [out_dir]` が `root_surface_contract_bundle_surface_wrapper_report_components=`, `root_surface_contract_bundle_surface_wrapper_report_out_dir=`, `root_surface_contract_bundle_surface_wrapper_report_log=`, `root_surface_contract_bundle_surface_wrapper_report_validator_log=`, `root_surface_contract_bundle_surface_wrapper_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_test` が PASS する。


### C-70 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface wrapper-report wrapper の提出ログ validator を追加し、saved wrapper-surface log から `root_surface_contract_bundle_surface_wrapper_report_*` metadata と validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.py <wrapper_surface_log>` が `root_surface_contract_bundle_surface_wrapper_report_components=`, `root_surface_contract_bundle_surface_wrapper_report_out_dir=`, `root_surface_contract_bundle_surface_wrapper_report_log=`, `root_surface_contract_bundle_surface_wrapper_report_validator_log=`, `root_surface_contract_bundle_surface_wrapper_report_result=pass` を検証し、wrapper/validator log の欠落・不一致・親dir外参照を fail-fast する。
  - `python3 scripts/check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_wrong_component.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_escape.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_nested_mismatch.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_test` が negative coverage を含めて PASS する。

### C-71 (Auto-Next)
- Status: `Done`
- Goal: root-surface contract bundle surface wrapper-report wrapper と C-70 validator を repo-root 1 コマンドへ束ね、PM が saved wrapper-surface log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh [out_dir]` が `root_surface_contract_bundle_surface_wrapper_report_components=`, `root_surface_contract_bundle_surface_wrapper_report_out_dir=`, `root_surface_contract_bundle_surface_wrapper_report_log=`, `root_surface_contract_bundle_surface_wrapper_report_validator_log=`, `root_surface_contract_bundle_surface_wrapper_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_test` が PASS する。

### C-72 (Auto-Next)
- Status: `Done`
- Goal: `COUPLED_COMPARE_SKIP_NESTED_SELFTESTS=1` の root-surface stack 伝播を focused make target と repo-root wrapper で固定し、nested wrapper 経路の再帰 self-test を抑止する。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_test` が PASS する。
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests.sh [out_dir]` が `skip_nested_selftests_components=`, `skip_nested_selftests_out_dir=`, `skip_nested_selftests_pm_surface_log=`, `skip_nested_selftests_root_modes_log=`, `skip_nested_selftests_root_surface_log=`, `skip_nested_selftests_root_surface_contract_bundle_surface_log=`, `skip_nested_selftests_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_skip_nested_selftests.sh` が PASS する。

### C-73 (Auto-Next)
- Status: `Done`
- Goal: saved skip wrapper log の report validator を追加し、component/path/pass-line 契約を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_report.py <wrapper_log>` が `skip_nested_selftests_*` metadata と pass lines を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_wrong_component.sh` が PASS する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_escape.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_report_test` が negative coverage を含めて PASS する。

### C-74 (Auto-Next)
- Status: `Done`
- Goal: skip wrapper と C-73 validator を repo-root 1 コマンドへ束ね、PM が saved skip wrapper log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_report.sh [out_dir]` が `skip_nested_selftests_report_components=`, `skip_nested_selftests_report_out_dir=`, `skip_nested_selftests_report_log=`, `skip_nested_selftests_report_validator_log=`, `skip_nested_selftests_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_surface_test` が PASS する。

### C-75 (Auto-Next)
- Status: `Done`
- Goal: saved skip wrapper/report log validator を追加し、`skip_nested_selftests_report_*` metadata と validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_surface_report.py <surface_log>` が `skip_nested_selftests_report_*` metadata / pass lines を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_surface_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_surface_report_test` が PASS する。

### C-76 (Auto-Next)
- Status: `Done`
- Goal: C-74 wrapper と C-75 validator を repo-root 1 コマンドへ束ね、PM が saved skip wrapper/report log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh [out_dir]` が `skip_nested_selftests_surface_report_components=`, `skip_nested_selftests_surface_report_out_dir=`, `skip_nested_selftests_surface_report_log=`, `skip_nested_selftests_surface_report_validator_log=`, `skip_nested_selftests_surface_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_test` が PASS する。

### C-77 (Auto-Next)
- Status: `Done`
- Goal: saved skip wrapper/report surface log validator を追加し、`skip_nested_selftests_surface_report_*` metadata と validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.py <surface_log>` が `skip_nested_selftests_surface_report_*` metadata / pass lines を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_test` が PASS する。

### C-78 (Auto-Next)
- Status: `Done`
- Goal: skip wrapper/report surface と C-77 validator を repo-root 1 コマンドに束ね、saved wrapper/report surface log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh [out_dir]` が `skip_nested_selftests_wrapper_surface_report_components=`, `skip_nested_selftests_wrapper_surface_report_out_dir=`, `skip_nested_selftests_wrapper_surface_report_log=`, `skip_nested_selftests_wrapper_surface_report_validator_log=`, `skip_nested_selftests_wrapper_surface_report_result=pass` を出力する。
  - `bash scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_test` が PASS する。

### C-79 (Auto-Next)
- Status: `Done`
- Goal: saved skip wrapper-surface report log validator を追加し、`skip_nested_selftests_wrapper_surface_report_*` metadata と nested wrapper/validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.py <wrapper_surface_log>` が `skip_nested_selftests_wrapper_surface_report_*` metadata / pass lines と nested `skip_nested_selftests_surface_report_*` handoff を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `bash scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.sh` が PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_test` が PASS する。

### C-80 (Auto-Next)
- Status: `Done`
- Goal: skip-nested-selftests chain を focused bundle target に束ね、C チームの current entrypoint を 1 コマンドへ固定する。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks` が docs sync + skip stack + report/surface/wrapper validators を順に実行する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_test` が bundle target の PASS surface を検証できる。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_help_test` が bundle target と bundle self-test の help surface を検証できる。
  - runbook / help に bundle target と bundle self-test が記載され、次セッションの入口コマンドとして参照できる。

### C-81 (Auto-Next)
- Status: `Done`
- Goal: skip-nested-selftests focused bundle を repo-root wrapper に束ね、saved bundle log と rerun out_dir を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks.sh [out_dir]` が `skip_nested_selftests_contract_checks_out_dir=`, `skip_nested_selftests_contract_checks_log=`, `skip_nested_selftests_contract_checks_result=pass` を出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_bundle_test` が explicit/default/nested out_dir の wrapper surface を検証できる。
  - runbook / help に wrapper command と wrapper self-test が記載される。

### C-82 (Auto-Next)
- Status: `Done`
- Goal: saved skip-nested-selftests contract bundle wrapper log validator を追加し、rerun out_dir と saved bundle log handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.py <wrapper_log>` が `skip_nested_selftests_contract_checks_*` metadata と saved bundle log の PASS surface を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_test` が PASS する。

### C-83 (Auto-Next)
- Status: `Done`
- Goal: C-82 contract bundle report validator を skip-nested-selftests focused bundle target へ編入し、entrypoint 1 コマンドで current chain 全体を回せるようにする。
- Acceptance:
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks` が `coupled_compare_reason_code_skip_nested_selftests_contract_report_test` を含めて PASS する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_checks_test` が C-82 report validator の PASS surface を bundle self-test に含めて PASS する。
  - runbook / help / queue に bundle target と C-82 report validator の関係が current surface として残る。

### C-84 (Auto-Next)
- Status: `Done`
- Goal: C-83 で導入した core/public split の current surface を docs / help / docs-sync に固定し、wrapper から non-recursive target を参照できるようにする。
- Acceptance:
  - `docs/team_runbook.md` に `coupled_compare_reason_code_skip_nested_selftests_contract_checks_core` の role が記載される。
  - `make -C FEM4C help` に core/public split target が残る。
  - `make -C FEM4C coupled_compare_reason_code_docs_sync_test` が PASS する。

### C-85 (Auto-Next)
- Status: `Done`
- Goal: C-81 wrapper と C-82 validator を repo-root 1 コマンドへ束ね、saved contract bundle wrapper log と validator handoff を single entrypoint で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.sh [out_dir]` が `skip_nested_selftests_contract_checks_report_components=`, `skip_nested_selftests_contract_checks_report_out_dir=`, `skip_nested_selftests_contract_checks_report_log=`, `skip_nested_selftests_contract_checks_report_validator_log=`, `skip_nested_selftests_contract_checks_report_result=pass` を出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_test` が explicit/default/nested out_dir の wrapper-report surface を検証できる。
  - help / runbook に wrapper-report command と self-test が記載される。

### C-86 (Auto-Next)
- Status: `Done`
- Goal: saved contract bundle report wrapper log validator を追加し、`skip_nested_selftests_contract_checks_report_*` metadata と nested bundle-log handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report.py <wrapper_log>` が report wrapper metadata と nested bundle log の handoff を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_report_test` が PASS する。

### C-87 (Auto-Next)
- Status: `Done`
- Goal: C-85 wrapper-report と C-86 validator を repo-root 1 コマンドへ束ね、saved contract report wrapper log と validator handoff を wrapper-surface 1 本で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.sh [out_dir]` が `skip_nested_selftests_contract_checks_wrapper_surface_report_components=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_out_dir=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_validator_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_report_result=pass` を出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_test` が explicit/default/nested out_dir の wrapper-surface を検証できる。
  - help / runbook に wrapper-surface command と self-test が記載される。

### C-88 (Auto-Next)
- Status: `Done`
- Goal: saved contract bundle report wrapper-surface log validator を追加し、`skip_nested_selftests_contract_checks_wrapper_surface_report_*` metadata と nested report-wrapper/validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.py <wrapper_surface_log>` が wrapper-surface metadata と nested report-wrapper log の handoff を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_report_test` が PASS する。

### C-89 (Auto-Next)
- Status: `Done`
- Goal: C-87 wrapper-surface と C-88 validator を repo-root 1 コマンドへ束ね、saved contract wrapper-surface log と validator handoff を wrapper-surface-wrapper 1 本で追跡できるようにする。
- Acceptance:
  - `bash scripts/run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.sh [out_dir]` が `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_components=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_out_dir=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_validator_log=`, `skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_result=pass` を出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_test` が explicit/default/nested out_dir の wrapper-surface-wrapper を検証できる。
  - help / runbook に wrapper-surface-wrapper command と self-test が記載される。

### C-90 (Auto-Next)
- Status: `Done`
- Goal: saved contract bundle report wrapper-surface log validator を追加し、`skip_nested_selftests_contract_checks_wrapper_surface_report_*` metadata と nested report-wrapper/validator handoff を fail-fast 再検証できるようにする。
- Acceptance:
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.py <wrapper_surface_log>` が wrapper-surface metadata と nested report-wrapper log の handoff を検証する。
  - `python3 scripts/check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.py --print-required-keys` が required keys / required pass lines を機械可読に出力する。
  - `make -C FEM4C coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_report_test` が PASS する。

### C-91 (Auto-Next)
- Status: `In Progress`
- Goal: C-89/C-90 の wrapper-surface-wrapper current surface を runbook / docs-sync / queue へ formal 同期し、focused acceptance target を current source-of-truth に固定する。
- Acceptance:
  - `docs/team_runbook.md` に `run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.sh` と validator usage が記載される。
  - `make -C FEM4C help` に `coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_test` と `..._wrapper_report_test` が残る。
  - `make -C FEM4C coupled_compare_reason_code_docs_sync_test` が PASS する。

## 8. Dチーム
### D-R1 (Run 1 Priority)
- Status: `Done`
- Goal: 1-link flexible meaningful case の最小骨格を作る。
- Scope:
  - `FEM4C/examples/`
  - `FEM4C/src/coupled/flex_snapshot2d.*`
  - 必要時のみ `FEM4C/scripts/compare_2link_flex_reference.py`
- Acceptance:
  - nonzero load / nonzero reaction を観測できる 1-link case の input / observation point が定義される。

### D-R2 (Run 3 Auto-Next)
- Status: `Done`
- Goal: 1-link meaningful case を reaction mapping artifact まで閉じる。
- Scope:
  - `FEM4C/examples/`
  - `FEM4C/src/coupled/flex_reaction2d.*`
  - 必要時のみ `FEM4C/scripts/compare_2link_flex_reference.py`
- Acceptance:
  - nonzero reaction が artifact で確認できる。
  - 1-link case を M2 の main debug case として再利用できる。

### D-R3 (Run 3 Auto-Next)
- Status: `Done`
- Goal: 1-link reaction mapping artifact を compare-side auxiliary CSV export まで閉じる。
- Scope:
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - `FEM4C/scripts/test_compare_2link_flex_reference_artifact_only.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - `--artifact-only` で 1-link summary から `interface_centers_csv` / `reaction_map_csv` を直接出力できる。
  - 2-link normalize を要求せず、nonzero reaction / mapped body force を auxiliary CSV で再利用できる。

### D-01
- Status: `Done`
- Goal: `flex_body2d_t` を定義し、1 flexible link の wrapper を作る。
- Scope:
  - `FEM4C/src/coupled/flex_body2d.h`
  - `FEM4C/src/coupled/flex_body2d.c`
- Acceptance:
  - `body_id`, `model`, `root_set`, `tip_set`, `u_local`, `reaction_root[3]`, `reaction_tip[3]` を保持できる。
  - init/free がある。

### D-02
- Status: `Done`
- Goal: interface rigid interpolation を node BC へ展開する。

### D-03
- Status: `Done`
- Goal: 1 flexible link の snapshot solve wrapper を作る。

### D-04
- Status: `Done`
- Goal: FE reaction を generalized force に変換する。

### D-05
- Status: `Done`
- Goal: 1-link flexible coupling を成立させる。

### D-06
- Status: `Done`
- Goal: 2-link flexible に拡張する。

### D-07
- Status: `Done`
- Goal: coupling residual と iteration 管理を入れる。

### D-08
- Status: `Done`
- Goal: snapshot 出力を coupled に接続する。

### D-09
- Status: `Done`
- Goal: 高剛性 limit test を作る。
- Follow-up:
  - implicit rigid-limit default run は `max_iter=12` / `marker_relaxation=6.2e-1` で Newmark/HHT とも収束化済み。
  - D の追加作業が必要なら、次候補は integrator 別 rigid-limit compare 閾値設計または compare runner の汎用化。

### D-10 (Auto-Next)
- Status: `Done`
- Goal: `flex_body2d` に deformed interface centroid helper を追加し、compare/export 側が root/tip center を直接取得できる土台を作る。
- Scope:
  - `FEM4C/src/coupled/flex_body2d.h`
  - `FEM4C/src/coupled/flex_body2d.c`
  - `FEM4C/scripts/test_flex_body2d_interface_center.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - root/tip node set の deformed local/world center を `u_local` から計算できる。
  - standalone smoke と既存 marker/implicit rigid-limit regression が共存して PASS する。
- Follow-up:
  - compare/export が interface center helper を直接利用する接続は未着手。
  - D の次候補は rigid-limit implicit compare 閾値設計か、interface center helper の coupled/export 採用。

### D-11 (Auto-Next)
- Status: `Done`
- Goal: interface center helper を snapshot export へ採用し、root/tip center metadata を accepted-step artifact に残す。
- Scope:
  - `FEM4C/src/coupled/case2d.h`
  - `FEM4C/src/coupled/case2d.c`
  - `FEM4C/src/coupled/flex_body2d.c`
  - `FEM4C/src/coupled/flex_snapshot2d.h`
  - `FEM4C/src/coupled/flex_snapshot2d.c`
  - `FEM4C/src/coupled/coupled_run2d.c`
  - `FEM4C/scripts/test_coupled_snapshot_output.sh`
- Acceptance:
  - accepted-step snapshot CSV に `root_center_local/tip_center_local/root_center_world/tip_center_world` が出る。
  - standalone snapshot smoke と marker/interface-center smoke、implicit rigid-limit regression が共存して PASS する。
- Follow-up:
  - compare script 本体が新しい interface center metadata を直接読む経路は未着手。
  - D の次候補は rigid-limit implicit compare 閾値設計か、snapshot metadata の compare/export 直接利用。

### D-12 (Auto-Next)
- Status: `Done`
- Goal: snapshot interface center metadata を compare/export 側で直接利用し、node set 再走査なしの normalized artifact path を固定する。
- Scope:
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_c16_flex_reference_compare.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/test_compare_2link_flex_manifest.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_manifest.sh`
- Acceptance:
  - `compare_2link_flex_reference.py --fem-summary` は snapshot に `tip_center_world` metadata があれば `--coupled-input` なしでも normalized schema CSV を生成できる。
  - `compare_rigid_limit_2link.py` は `root_center_world` / `tip_center_world` metadata を優先し、旧 node table 平均は fallback に留める。
  - manifest smoke、real wrapper smoke、`check_coupled_2link_examples.sh` が共存して PASS する。
- Follow-up:
  - compare schema 自体には root/tip center の専用列がまだ無く、metadata は normalize 内部利用に留まる。
  - D の次候補は rigid-limit implicit compare 閾値設計、または interface center を compare schema/aux artifact へ露出する拡張。

### D-13 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit implicit compare の閾値契約を 1 箇所へ集約し、Newmark/HHT の acceptance を PM/compare 側で再利用できる形に固定する。
- Scope:
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/run_d09_rigid_limit_compare.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_implicit_metrics.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - explicit / Newmark / HHT の rigid-limit compare 閾値が helper/table 1 箇所から供給される。
  - `test_compare_rigid_limit_implicit_metrics.sh` と `check_coupled_2link_examples.sh` が同じ閾値定義を使う。
  - `make -C FEM4C coupled_rigid_limit_compare_test coupled_rigid_limit_implicit_compare_test` が PASS する。
- Follow-up:
  - PM-03 の数値受入基準へ転記する閾値候補をこのタスクの出力から採る。
  - compare schema 本体の列拡張は D-13 では行わず、必要なら別 Auto-Next を起票する。

### D-14 (Auto-Next)
- Status: `Done`
- Goal: interface center metadata を compare schema とは別の auxiliary CSV として露出し、root/tip local/world center を artifact に残す。
- Scope:
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_c16_flex_reference_compare.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/test_compare_2link_flex_manifest.sh`
  - `FEM4C/scripts/test_compare_2link_flex_reference_real.sh`
  - `FEM4C/scripts/test_compare_2link_flex_reference_compare_mode.sh`
  - `docs/09_compare_schema_2d.md`
- Acceptance:
  - `compare_2link_flex_reference.py --interface-centers-csv <path>` が `step_index/body_id/time/marker/root_center/tip_center` の auxiliary CSV を出す。
  - real/compare/example wrapper が auxiliary CSV を生成し、flex manifest/real/compare smoke が PASS する。
  - compare schema 本体は変更せず、補助 artifact として contract を文書化する。

### D-15 (Auto-Next)
- Status: `Done`
- Goal: compare artifact suite / manifest に `interface_centers_csv` 列を追加し、flex auxiliary artifact を監査導線へ乗せる。
- Scope:
  - `FEM4C/scripts/check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_manifest.py`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix_manifest.py`
  - `FEM4C/scripts/test_check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/test_check_compare_2link_artifact_matrix.sh`
- Acceptance:
  - artifact suite stdout / manifest に `interface_centers_csv` 列が追加される。
  - rigid row は `-`、flex row は file path を持つ。
  - `make -C FEM4C compare_2link_artifact_checks` が PASS する。

### D-16 (Auto-Next)
- Status: `Done`
- Goal: compare/example wrapper が stale `bin/fem4c` を踏まないようにしつつ、`interface_centers_csv` の列契約まで validator 側で検証する。
- Scope:
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - `FEM4C/scripts/run_d09_rigid_limit_compare.sh`
  - `FEM4C/scripts/check_compare_2link_artifacts.sh`
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/check_compare_2link_artifact_manifest.py`
  - `FEM4C/scripts/check_compare_2link_artifact_matrix_manifest.py`
- Acceptance:
  - wrapper 実行前に incremental `make bin/fem4c` が走る。
  - flex manifest row の `interface_centers_csv` について required columns と非空を validator が検証する。
  - `make -C FEM4C compare_2link_artifact_checks` と `bash FEM4C/scripts/check_coupled_2link_examples.sh` が PASS する。
- Follow-up:
  - compare schema 本体には root/tip interface center の専用列がまだ無く、auxiliary CSV は補助 artifact の位置づけに留まる。
  - D の次候補は auxiliary CSV を higher-level compare summary へ束ねる拡張、または PM-03 向け rigid-limit 閾値の文書化。

### D-17 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の上位 summary/manifest に compare artifact manifest と interface-center auxiliary CSV 群を載せ、PM が wrapper 1 本から flex auxiliary artifact を追えるようにする。
- Scope:
  - `FEM4C/scripts/run_coupled_compare_checks.sh`
  - `FEM4C/scripts/check_coupled_compare_checks_manifest.py`
  - `FEM4C/scripts/test_run_coupled_compare_checks.sh`
  - `FEM4C/scripts/test_run_coupled_compare_checks_artifact_manifest.sh`
  - `FEM4C/scripts/test_make_coupled_compare_checks_out_dir.sh`
  - `FEM4C/scripts/test_make_coupled_compare_checks_subset.sh`
  - `FEM4C/scripts/test_run_coupled_compare_checks_failfast.sh`
  - `FEM4C/scripts/test_check_coupled_compare_checks_manifest_reason_codes.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - `coupled_compare_checks` default target に `compare_2link_artifact_check` が含まれ、wrapper 管理下 `OUT_DIR` に artifact manifest を生成できる。
  - stdout / aggregate manifest に `artifact_manifest_path` / `interface_centers_csvs` 列が追加される。
  - validator が artifact manifest と semicolon join された auxiliary CSV 群の整合を検証し、`make -C FEM4C coupled_compare_checks_test coupled_compare_checks_artifact_manifest_test coupled_compare_checks_manifest_test` が PASS する。

### D-18 (Auto-Next)
- Status: `Done`
- Goal: `coupled_2d_acceptance` stage summary/manifest へ compare-matrix の auxiliary interface-center CSV 群を持ち上げ、higher-level acceptance からも flex artifact を辿れるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_integrators.sh`
- Acceptance:
  - `compare_matrix` stage row が `artifact_manifest_path` と semicolon join された `interface_centers_csvs` を出力する。
  - `build/rigid_matrix/flex_matrix` stage は `artifact_manifest_path=-` / `interface_centers_csvs=-` を維持する。
  - `make -C FEM4C coupled_2d_acceptance_test coupled_2d_acceptance_integrators_test coupled_2d_acceptance_manifest_test` が PASS する。
- Follow-up:
  - compare schema 本体には root/tip center 専用列がまだ無く、acceptance 側でも auxiliary CSV を参照する構成に留まる。
  - 次候補は PM-03 向け rigid-limit 閾値の文書化と helper/doc sync。

### D-19 (Auto-Next)
- Status: `Done`
- Goal: PM-03 向け rigid-limit compare 閾値を helper と文書の両方で同期し、integrator 別受入基準の参照元を 1 箇所に固定する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - `FEM4C/scripts/compare_rigid_limit_2link.py`
  - `FEM4C/scripts/check_rigid_limit_threshold_docs_sync.sh`
  - `FEM4C/scripts/test_compare_rigid_limit_implicit_metrics.sh`
  - `FEM4C/scripts/test_check_rigid_limit_threshold_docs_sync.sh`
- Acceptance:
  - explicit / Newmark / HHT の rigid-limit compare 閾値が `docs/06_acceptance_matrix_2d.md` に転記される。
  - helper table と文書の整合を `make -C FEM4C coupled_rigid_limit_threshold_docs_sync_test` の 1 コマンドで確認できる。
  - PM-03 が D 側 threshold source をそのまま参照できる。

### D-20 (Auto-Next)
- Status: `Done`
- Goal: rigid-limit threshold contract を machine-readable printer / Make target として固定し、PM が helper current value を直接取得できるようにする。
- Scope:
  - `FEM4C/scripts/print_rigid_limit_thresholds.sh`
  - `FEM4C/scripts/test_print_rigid_limit_thresholds.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `make -C FEM4C coupled_rigid_limit_thresholds` が `rigid_limit_threshold_columns` / `rigid_limit_threshold` / `rigid_limit_threshold_update_points` を出力する。
  - `make -C FEM4C coupled_rigid_limit_thresholds_test` と `make -C FEM4C coupled_rigid_limit_threshold_docs_sync_test` が PASS する。
  - `docs/06_acceptance_matrix_2d.md` が printer 出力と grep 同期できる。

### D-21 (Auto-Next)
- Status: `Done`
- Goal: example / acceptance wrapper が rigid-limit threshold contract の参照元を summary 行へ露出し、PM が run log から threshold source を辿れるようにする。
- Scope:
  - `FEM4C/scripts/check_coupled_2link_examples.sh`
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - example / acceptance wrapper が rigid-limit threshold contract の source command または update point を summary 行に出す。
  - compare evidence と threshold source を同じ log から追える。
  - 既存 rigid-limit compare regression を壊さない。

### D-22 (Auto-Next)
- Status: `Done`
- Goal: acceptance manifest / validator / focused smoke に rigid-limit threshold provenance 契約を追加し、summary 行を machine-checkable にする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_integrators.sh`
  - `FEM4C/scripts/test_check_2d_coupled_acceptance_manifest_threshold_contract.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - acceptance manifest row が `rigid_limit_threshold_source_command` と `rigid_limit_threshold_update_points` を持つ。
  - `check_2d_coupled_acceptance_manifest.py` が上記 provenance 列を検証する。
  - `make -C FEM4C coupled_2d_acceptance_test coupled_2d_acceptance_stages_test coupled_2d_acceptance_stage_integrators_test coupled_2d_acceptance_integrators_test coupled_2d_acceptance_threshold_contract_test` が PASS する。

### D-23 (Auto-Next)
- Status: `Done`
- Goal: `coupled_compare_checks` の aggregate summary/manifest に threshold provenance を持ち上げ、PM が higher-level suite から child log を開かずに threshold source を辿れるようにする。
- Scope:
  - `FEM4C/scripts/run_coupled_compare_checks.sh`
  - `FEM4C/scripts/check_coupled_compare_checks_manifest.py`
  - `FEM4C/scripts/test_run_coupled_compare_checks.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - `coupled_example_check` を含む aggregate row が rigid-limit threshold source command または update points を持つ。
  - validator が上位 manifest と child log の provenance 整合を検証する。
  - `make -C FEM4C coupled_compare_checks_test coupled_compare_checks_manifest_test` が PASS する。

### D-24 (Auto-Next)
- Status: `Done`
- Goal: `coupled_2d_acceptance_gate` まで threshold provenance を持ち上げ、PM が最上位 gate log から threshold source を辿れるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance_gate.sh`
  - `FEM4C/scripts/test_run_2d_coupled_acceptance_gate.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `docs/team_runbook.md`
- Acceptance:
  - gate summary または gate log が `rigid_limit_threshold_source_command` と `rigid_limit_threshold_update_points` を持つ。
  - `make -C FEM4C coupled_2d_acceptance_gate_test` が provenance surfacing を検証して PASS する。
  - PM が nested manifest を開かずに gate 出力から threshold source を確認できる。

### D-25 (Auto-Next)
- Status: `Done`
- Goal: `coupled_2d_acceptance_gate` provenance surface を focused self-test と surface bundle まで固定する。
- Scope:
  - `FEM4C/scripts/test_run_2d_coupled_acceptance_gate_threshold_provenance.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_surface_checks.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - synthetic self-test が gate row の threshold provenance columns を検証して PASS する。
  - `coupled_2d_acceptance_surface_checks` が gate wrapper / docs sync に加えて provenance self-test も束ねる。

### D-26 (Auto-Next)
- Status: `Done`
- Goal: gate provenance surface の docs sync を機械検証できるようにする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_gate` の provenance fields 記載を検証する。
  - acceptance doc / runbook / README の current command surface に provenance row surface が明記される。

### D-27 (Auto-Next)
- Status: `Done`
- Goal: gate provenance self-test target を docs surface と docs sync contract に固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_gate_threshold_provenance_test` の記載を検証する。
  - acceptance doc / runbook / README の current command surface に gate provenance self-test target が明記される。

### D-28 (Auto-Next)
- Status: `Done`
- Goal: surface bundle docs に gate provenance self-test の内包関係を明記し、docs sync でも固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_surface_checks` の bundle 内容として `coupled_2d_acceptance_gate_threshold_provenance_test` を要求する。
  - acceptance doc / runbook / README の current command surface が surface bundle の child target を明記する。

### D-29 (Auto-Next)
- Status: `Done`
- Goal: lightweight / wrapper smoke への surface bundle 接続を docs sync surface に固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_lightweight_checks_test` の記載を検証する。
  - acceptance doc / runbook / README の current command surface に `lightweight_checks_test + ensure_fem4c_binary_test` への接続が明記される。

### D-30 (Auto-Next)
- Status: `Done`
- Goal: wrapper smoke / resilience pack の bundle composition を docs と docs sync で固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_wrapper_smoke_test` と `coupled_2d_acceptance_resilience_checks_test` の記載を検証する。
  - acceptance doc / runbook / README の current command surface が wrapper smoke / resilience pack の child target を明記する。

### D-31 (Auto-Next)
- Status: `Done`
- Goal: coupled acceptance docs sync script の required surface を単一テーブルへ集約する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs sync script が acceptance doc / runbook / README 向けに共通の required-pattern list を使う。
  - `make -C FEM4C coupled_2d_acceptance_docs_sync_test` と `make -C FEM4C coupled_2d_acceptance_surface_checks_test` が継続して PASS する。

### D-32 (Auto-Next)
- Status: `Done`
- Goal: gate+resilience smoke bundle の composition を docs と docs sync で固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_gate_resilience_smoke` と `coupled_2d_acceptance_gate_resilience_smoke_test` の記載を検証する。
  - acceptance doc / runbook / README の current command surface が gate+resilience smoke bundle の child target を明記する。

### D-33 (Auto-Next)
- Status: `Done`
- Goal: coupled acceptance docs sync failure diagnostics を強化する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - missing token / bundle regex failure 時に、どの pattern とどの doc が欠けたか stderr へ出る。
  - `make -C FEM4C coupled_2d_acceptance_docs_sync_test` と `make -C FEM4C coupled_2d_acceptance_surface_checks_test` が継続して PASS する。

### D-34 (Auto-Next)
- Status: `Done`
- Goal: gate+resilience smoke bundle の top-level docs surface 接続を固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が gate+resilience smoke bundle の top-level role 記述を検証する。
  - acceptance doc / runbook / README の current command surface が `coupled_2d_acceptance_gate_resilience_smoke` の位置付けを wrapper/resilience hierarchy 上で明記する。

### D-35 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の failure diagnostics を semantic label ベースへ揃える。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - missing pattern / regex failure が `[label]` 付きで報告される。
  - `make -C FEM4C coupled_2d_acceptance_docs_sync_test` と `make -C FEM4C coupled_2d_acceptance_surface_checks_test` が継続して PASS する。

### D-36 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の contract inventory を機械可読に出せるようにする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - validator が required pattern labels / bundle regex labels を一覧出力できる。
  - current docs sync tests が継続して PASS する。

### D-37 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の inventory output に header/count surface を追加する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - validator が inventory header と `pattern_count` / `regex_count` を出せる。
  - current docs sync tests が継続して PASS する。

### D-38 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の inventory flags を usage/help surface に載せる。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - validator が supported inventory flags を usage/help として出せる。
  - current docs sync tests と inventory outputs が継続して PASS する。

### D-39 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の supported option list を machine-readable に出せるようにする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - validator が `--print-supported-options` を持つ。
  - unsupported option 時に usage/help へ fallback する。

### D-40 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator の help/inventory surfaces を focused self-test で固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - help / supported-options / contract-inventory / contract-counts を focused self-test で検証できる。
  - current docs sync tests と inventory outputs が継続して PASS する。

### D-41 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator surface self-test を acceptance surface bundle と docs surface に接続する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_surface_checks.sh`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/team_runbook.md`
  - `FEM4C/README.md`
- Acceptance:
  - `coupled_2d_acceptance_surface_checks` が `coupled_2d_acceptance_docs_sync_surfaces_test` を含む。
  - docs surface が focused self-test target と bundle composition を継続して説明する。

### D-42 (Auto-Next)
- Status: `Done`
- Goal: docs sync validator surface self-test target を `make help` surface と focused smoke で固定する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_docs_sync_surfaces_help.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_docs_sync_surface_smoke.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `make -C FEM4C help` に `coupled_2d_acceptance_docs_sync_surfaces_test` が残る。
  - focused self-test target 自体の help/docs drift を短い smoke で検知できる。

### D-43 (Auto-Next)
- Status: `Done`
- Goal: docs sync surface smoke bundle を acceptance surface bundle と validator contract に接続する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_surface_checks.sh`
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/team_runbook.md`
  - `FEM4C/README.md`
- Acceptance:
  - `coupled_2d_acceptance_surface_checks` が `coupled_2d_acceptance_docs_sync_surface_smoke_test` を含む。
  - docs sync validator が help-test と smoke-bundle children を contract として検証する。

### D-44 (Auto-Next)
- Status: `Done`
- Goal: docs sync surface smoke target 群を機械可読に列挙できる helper surface を追加する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
  - `FEM4C/Makefile`
- Acceptance:
  - docs sync validator か companion helper から docs-sync surface target roster を機械可読に取得できる。
  - current help/smoke/surface bundle tests が継続して PASS する。

### D-45 (Auto-Next)
- Status: `Done`
- Goal: help surface contract を docs-sync surface roster printer の single-source に寄せる。
- Scope:
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_docs_sync_surfaces_help.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - help surface self-test が validator roster printer を source-of-truth として使う。
  - current help/smoke/surface bundle tests が継続して PASS する。

### D-46 (Auto-Next)
- Status: `Done`
- Goal: docs-sync surface roster printer に count/header surface を追加し、外部監査しやすくする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
- Acceptance:
  - docs-sync surface roster printer が header/count 付きで機械可読に読める。
  - current help/smoke/surface bundle tests が継続して PASS する。

### D-47 (Auto-Next)
- Status: `Done`
- Goal: validator surface self-test を roster inventory/count output まで広げ、surface 契約を tighten する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - self-test が roster inventory/count output と help/options surface を検証する。
  - current docs sync / surface bundle tests が継続して PASS する。

### D-48 (Auto-Next)
- Status: `Done`
- Goal: docs-sync surface roster の label/count を 1 回で監査できる combined surface を追加する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
- Acceptance:
  - roster labels と count をまとめて取得できる machine-readable surface がある。
  - current docs sync / surface bundle tests が継続して PASS する。

### D-49 (Auto-Next)
- Status: `Done`
- Goal: help surface contract を combined docs-sync roster audit output の single-source に寄せる。
- Scope:
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_docs_sync_surfaces_help.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
- Acceptance:
  - help surface self-test が combined audit output から target 群を読む。
  - current docs sync / surface bundle tests が継続して PASS する。

### D-50 (Auto-Next)
- Status: `Done`
- Goal: combined docs-sync roster audit output の schema/field contract を機械可読に出せるようにする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
- Acceptance:
  - audit output の count key と tabular columns を外部監査側が machine-readable に取得できる。
  - current docs sync / surface bundle tests が継続して PASS する。

### D-51 (Auto-Next)
- Status: `Done`
- Goal: combined docs-sync audit schema contract を validator / help companion test まで反映する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_docs_sync_surfaces_help.sh`
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - focused validator self-test が audit schema option を検証する。
  - help surface contract が audit schema と audit output の key/header drift を検知する。

### D-52 (Auto-Next)
- Status: `Todo`
- Goal: docs-sync roster audit output の schema と data を単一 inventory surface で一括列挙できるようにする。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh`
- Acceptance:
  - schema と data を external audit 側が 1 回で取得できる machine-readable inventory surface がある。
  - current docs sync / surface bundle tests が継続して PASS する。

## 9. Eチーム
### E-R1 (Run 1 Priority)
- Status: `Done`
- Goal: default acceptance path を M1/M2 に必要な最小コアへ絞り、extra wrapper を non-default 扱いにする。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/README.md`
  - `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - default path に残す target と non-default target が明文化される。
  - gate / resilience pack の default 実行を一時停止する方針が docs で読める。

### E-R2 (Run 2 Auto-Next)
- Status: `Done`
- Goal: rigid analytic compare を含む top-level rigid acceptance target を 1 本に縮退する。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - 必要時のみ `FEM4C/scripts/run_mbd_regression.sh`
- Acceptance:
  - M1 の default acceptance route が 1 本で説明できる。

### E-01
- Status: `Done`
- Goal: `runner.c` から `mbd_system2d_run()` へ処理を寄せ、入口と mode 分岐に縮退させる。
- Scope:
  - `FEM4C/src/mbd/system2d.h`
  - `FEM4C/src/mbd/system2d.c`
  - `FEM4C/src/analysis/runner.c`
- Acceptance:
  - `runner.c` に parse / mode 分岐 / run 呼び出しだけが残る方向へ前進する。

### E-02
- Status: `Done`
- Goal: `COUPLED_FLEX_BODY` / `ROOT_SET` / `TIP_SET` を parse できるようにする。

### E-03
- Status: `Done`
- Goal: rigid 2-link benchmark input を作る。

### E-04
- Status: `Done`
- Goal: explicit coupled run を作る。

### E-05
- Status: `Done`
- Goal: implicit coupled run (Newmark) を作る。

### E-06
- Status: `Done`
- Goal: implicit coupled run (HHT) を作る。

### E-07
- Status: `Done`
- Goal: 2-link flexible input を作る。
- Acceptance:
  - `make -C FEM4C coupled_example_check` が PASS し、`examples/coupled_2link_flex_master.dat` / `examples/flex_link1_q4.dat` / `examples/flex_link2_q4.dat` が current runner で runnable。

### E-08 (Auto-Next)
- Status: `Done`
- Goal: current MBD/coupled output を compare schema へ正規化し、2-link compare artifact を固定する。
- Scope:
  - `FEM4C/scripts/compare_2link_rigid_analytic.py`
  - `FEM4C/scripts/compare_2link_flex_reference.py`
  - 必要時のみ `docs/09_compare_schema_2d.md`
- Acceptance:
  - rigid compare は current MBD summary/history から schema CSV を生成し、analytic reference との RMS/max error と PNG を出せる。
  - flexible compare は current coupled summary/snapshot から schema CSV を生成し、reference 未投入でも schema-validation artifact を出せる。
  - 着手点は `--fem-summary` 正規化経路の固定を先に行い、compare script のために runner/parser を広域改造しない。

### E-09 (Auto-Next)
- Status: `Done`
- Goal: E-08 で固定した compare entrypoint を 1 コマンド acceptance/orchestration に束ねる。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/check_coupled_2link_examples.sh`
- Acceptance:
  - build、rigid explicit/Newmark/HHT、flexible explicit/Newmark/HHT、compare invocation、pass/fail summary を 1 コマンドで回せる。
  - orchestration は E-08 の compare CSV/PNG 出力を呼び出すだけに留め、compare schema や parser を追加拡張しない。
  - 着手点は E-08 acceptance の CSV/PNG 出力 path が固定された後であり、compare script 先行修正を巻き戻さない。

### E-10 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper が integrator subset rerun と manifest validator override を受け付けるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance INTEGRATORS="explicit hht_alpha"` のような subset rerun ができる。
  - manifest validator が subset 実行の expected stage/result_note を検証できる。
  - orchestration 本体は引き続き E-08 の wrapper 群を呼び出すだけに留め、compare schema / parser は拡張しない。

### E-11 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper の invalid-integrator fail-fast 契約を固定する。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_integrators.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance INTEGRATORS="explicit bogus"` が fail-fast し、unsupported integrator を stable な文言で返す。
  - invalid subset では manifest/OUT_DIR 生成に進まないことを self-test で確認できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで契約を固定する。

### E-12 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper が stage subset rerun を受け付け、比較や再実行の反復を最小化できるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build rigid_matrix"` のような stage subset rerun ができる。
  - manifest validator が subset 実行の expected stage 行を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper の orchestration 境界だけで完結する。

### E-13 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper の invalid-stage fail-fast 契約を固定する。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stages.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build bogus"` が fail-fast し、unsupported stage を stable な文言で返す。
  - invalid stage subset では manifest/OUT_DIR 生成に進まないことを self-test で確認できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで契約を固定する。

### E-14 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper が `STAGES` と `INTEGRATORS` の複合 subset rerun を安定して受け付けるようにする。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stages.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="build rigid_matrix" INTEGRATORS="explicit hht_alpha"` のような複合 subset rerun ができる。
  - manifest validator が subset stage と subset integrator の両方を同時に検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper の orchestration 境界だけで完結する。

### E-15 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance wrapper の `compare_matrix` stage でも `STAGES` と `INTEGRATORS` の複合 subset rerun 契約を固定する。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stage_integrators.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance STAGES="compare_matrix" INTEGRATORS="explicit hht_alpha"` が pass する。
  - coupled acceptance manifest が `compare_matrix` stage 行のみを持ち、subset integrator note を維持する。
  - compare child manifest も `explicit,hht_alpha` のみを含み、`newmark_beta` は生成されない。
  - compare schema / parser / runner には手を入れず、acceptance wrapper と既存 compare validator の再利用だけで完結する。

### E-16 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance の subset/invalid contract 群を 1 コマンド bundle target に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_stage_integrators.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_compare_stage_integrators.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_contract_checks` が stage subset / integrator subset / combined subset / compare-only combined subset / invalid stage / invalid integrator をまとめて PASS させる。
  - `make -C FEM4C coupled_2d_acceptance_contract_checks_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 周辺の orchestration に限定する。

### E-17 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance contract bundle に threshold provenance contract も含め、監査面の最小 bundle を 1 本に固定する。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_contract_checks.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_contract_checks` が既存 subset/invalid checks に加えて `coupled_2d_acceptance_threshold_contract_test` も実行する。
  - `make -C FEM4C coupled_2d_acceptance_contract_checks_test` が threshold contract の PASS line も含めて bundle surface を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 周辺の orchestration に限定する。

### E-18 (Auto-Next)
- Status: `Done`
- Goal: coupled acceptance / contract bundle の command surface を docs に同期し、PM が current 入口を迷わず辿れるようにする。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - acceptance matrix か README の少なくとも一方に `coupled_2d_acceptance`, `coupled_2d_acceptance_contract_checks`, `coupled_2d_acceptance_contract_checks_test`, `coupled_rigid_limit_thresholds` の current entrypoint が記載される。
  - command の役割が `full acceptance`, `focused contract bundle`, `bundle self-test`, `threshold printer` として区別できる。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-19 (Auto-Next)
- Status: `Done`
- Goal: coupled 2D acceptance の full run と focused contract bundle を 1 コマンド gate target に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_contract_checks.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_gate` が `coupled_2d_acceptance` と `coupled_2d_acceptance_contract_checks` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_gate_test` が gate target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、既存 acceptance target の再利用だけで完結する。

### E-20 (Auto-Next)
- Status: `Done`
- Goal: coupled acceptance gate の command surface を docs に同期し、PM が one-command gate 入口も docs から辿れるようにする。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - acceptance matrix か README の少なくとも一方に `coupled_2d_acceptance_gate` と `coupled_2d_acceptance_gate_test` が記載される。
  - gate target の役割が `full acceptance + focused contract bundle` であることが区別できる。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-21 (Auto-Next)
- Status: `Done`
- Goal: E-team の current acceptance entrypoint を runbook にも同期し、README / acceptance matrix と同じ command surface を PM運用側からも辿れるようにする。
- Scope:
  - `docs/team_runbook.md`
- Acceptance:
  - runbook に `coupled_2d_acceptance`, `coupled_2d_acceptance_contract_checks`, `coupled_2d_acceptance_gate`, `coupled_rigid_limit_thresholds` の current entrypoint が記載される。
  - 役割が `full acceptance`, `focused contract bundle`, `one-command gate`, `threshold printer` として区別できる。
  - compare schema / parser / runner には手を入れず、runbook sync のみで完結する。

### E-22 (Auto-Next)
- Status: `Done`
- Goal: runbook 上の E-team acceptance entrypoint に self-test target も同期し、main target と test target の対応を 1 箇所で確認できるようにする。
- Scope:
  - `docs/team_runbook.md`
- Acceptance:
  - runbook の E-team acceptance block に `coupled_2d_acceptance_contract_checks_test` と `coupled_2d_acceptance_gate_test` が追加される。
  - main target と self-test target の役割が区別できる。
  - compare schema / parser / runner には手を入れず、runbook sync のみで完結する。

### E-23 (Auto-Next)
- Status: `Done`
- Goal: 直近更新した acceptance matrix / runbook の `最終更新` 表記を current date に揃える。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - `docs/team_runbook.md`
- Acceptance:
  - 上記 2 ファイルの `最終更新` が current date と一致する。
  - compare schema / parser / runner には手を入れず、docs freshness sync のみで完結する。

### E-24 (Auto-Next)
- Status: `Done`
- Goal: coupled acceptance gate target を stable summary line を持つ wrapper へ寄せる。
- Scope:
  - `FEM4C/scripts/run_2d_coupled_acceptance_gate.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance_gate.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_gate` が stable な `coupled_acceptance_gate_columns` / `coupled_acceptance_gate,...` 行を出す。
  - full acceptance log と contract bundle log が分離保存される。
  - `make -C FEM4C coupled_2d_acceptance_gate_test` が PASS する。
  - compare schema / parser / runner には手を入れず、acceptance orchestration wrapper に限定する。

### E-25 (Auto-Next)
- Status: `Done`
- Goal: gate wrapper の stable summary surface を docs に同期する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - acceptance matrix か README の少なくとも一方に `coupled_2d_acceptance_gate` が stable summary rows を出す wrapper であることが記載される。
  - full acceptance log / contract bundle log を分離保存する点が読み取れる。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-26 (Auto-Next)
- Status: `Done`
- Goal: runbook も gate wrapper の stable summary surface に同期する。
- Scope:
  - `docs/team_runbook.md`
- Acceptance:
  - runbook の E-team acceptance block に `coupled_2d_acceptance_gate` が stable summary rows を出す wrapper であることが記載される。
  - full acceptance log / contract bundle log を分離保存する点が読み取れる。
  - compare schema / parser / runner には手を入れず、runbook sync のみで完結する。

### E-27 (Auto-Next)
- Status: `Done`
- Goal: gate wrapper 化で不要になった旧 gate test script を除去し、参照 surface を一意に保つ。
- Scope:
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_gate.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - 旧 `test_make_coupled_2d_acceptance_gate.sh` が削除される。
  - gate test の参照先が `test_run_2d_coupled_acceptance_gate.sh` のみになる。
  - compare schema / parser / runner には手を入れず、wrapper cleanup のみで完結する。

### E-28 (Auto-Next)
- Status: `Done`
- Goal: E-14 の combined subset rerun でも custom `MANIFEST_CSV` override を安定して受け付けるようにする。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_stage_integrators.sh`
  - 必要時のみ `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance OUT_DIR=<dir> MANIFEST_CSV=<custom.csv> STAGES="build rigid_matrix" INTEGRATORS="explicit hht_alpha"` が pass する。
  - custom manifest path に build / rigid_matrix の 2 行だけが保存され、validator もその path を使って PASS する。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで完結する。

### E-29 (Auto-Next)
- Status: `Done`
- Goal: E-15 の compare-only subset rerun でも custom `MANIFEST_CSV` override を安定して受け付けるようにする。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_compare_stage_integrators.sh`
  - 必要時のみ `FEM4C/scripts/check_2d_coupled_acceptance_manifest.py`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance OUT_DIR=<dir> MANIFEST_CSV=<custom.csv> STAGES="compare_matrix" INTEGRATORS="explicit hht_alpha"` が pass する。
  - custom manifest path に `compare_matrix` の 1 行だけが保存され、child compare manifest は `OUT_DIR` 配下に残る。
  - compare schema / parser / runner には手を入れず、acceptance wrapper 境界だけで完結する。

### E-30 (Auto-Next)
- Status: `Done`
- Goal: E-14 / E-15 の custom `MANIFEST_CSV` override contract を docs にも同期する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
- Acceptance:
  - acceptance docs のどこかに combined subset / compare-only subset でも custom `MANIFEST_CSV` を `OUT_DIR` 外へ出せることが記載される。
  - parent manifest と child compare manifest の所在が区別できる。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-31 (Auto-Next)
- Status: `Done`
- Goal: acceptance docs の current entrypoint / manifest override surface を self-test で固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - `FEM4C/Makefile`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - self-test が `coupled_2d_acceptance`, `coupled_2d_acceptance_contract_checks`, `coupled_2d_acceptance_gate`, `coupled_rigid_limit_thresholds`, `MANIFEST_CSV=<custom.csv>`, `OUT_DIR/compare_matrix/` の記載を docs 三点で検証する。
  - `make -C FEM4C coupled_2d_acceptance_docs_sync_test` が PASS する。
  - compare schema / parser / runner には手を入れず、docs sync test のみで完結する。

### E-32 (Auto-Next)
- Status: `Done`
- Goal: gate wrapper self-test と docs sync test を 1 コマンド surface bundle に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance_gate.sh`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_surface_checks` が `coupled_2d_acceptance_gate_test` と `coupled_2d_acceptance_docs_sync_test` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_surface_checks_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance surface bundle のみで完結する。

### E-33 (Auto-Next)
- Status: `Done`
- Goal: acceptance docs に `coupled_2d_acceptance_surface_checks` の current entrypoint を同期する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs のどこかに `coupled_2d_acceptance_surface_checks` と `coupled_2d_acceptance_surface_checks_test` が記載される。
  - 役割が `gate wrapper + docs sync` の focused surface bundle として区別できる。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-34 (Auto-Next)
- Status: `Done`
- Goal: contract bundle と surface bundle を 1 コマンド lightweight pack に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_surface_checks.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_contract_checks.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_lightweight_checks` が `coupled_2d_acceptance_contract_checks_test` と `coupled_2d_acceptance_surface_checks_test` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_lightweight_checks_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、acceptance check bundle のみで完結する。

### E-35 (Auto-Next)
- Status: `Done`
- Goal: compare / acceptance child wrapper が stale `bin/fem4c` を拾ったときも self-heal できるようにする。
- Scope:
  - `FEM4C/scripts/ensure_fem4c_binary.sh`
  - `FEM4C/scripts/run_e08_rigid_analytic_normalize.sh`
  - `FEM4C/scripts/run_e08_rigid_analytic_compare.sh`
  - `FEM4C/scripts/run_c15_flex_reference_normalize.sh`
  - 必要時のみ `FEM4C/scripts/test_ensure_fem4c_binary.sh`
  - 必要時のみ `FEM4C/Makefile`
- Acceptance:
  - compare child wrapper が `src/*.c|*.h` または `Makefile` の newer-than-`bin/fem4c` を検知したら clean rebuild に入る。
  - source が fresh なときは不要な clean rebuild を行わない。
  - `make -C FEM4C ensure_fem4c_binary_test` が PASS し、E-15 compare-only subset rerun が stale binary 起因で偽 fail しない。

### E-36 (Auto-Next)
- Status: `Done`
- Goal: stale binary guard の current command surface を docs に同期し、PM が compare wrapper 側の self-heal 前提を見失わないようにする。
- Scope:
  - `FEM4C/README.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs の少なくとも 1 箇所に `ensure_fem4c_binary_test` と compare wrapper の stale-binary self-heal 前提が記載される。
  - `coupled_2d_acceptance_lightweight_checks` と `ensure_fem4c_binary_test` の役割差が区別できる。
  - docs sync test も上記 current surface を検証する。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-37 (Auto-Next)
- Status: `Done`
- Goal: lightweight acceptance pack と stale-binary guard をまとめた focused wrapper smoke pack を追加する。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_lightweight_checks.sh`
  - 必要時のみ `FEM4C/scripts/test_ensure_fem4c_binary.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_wrapper_smoke.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_wrapper_smoke` が `coupled_2d_acceptance_lightweight_checks_test` と `ensure_fem4c_binary_test` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_wrapper_smoke_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、wrapper smoke bundle のみで完結する。

### E-38 (Auto-Next)
- Status: `Done`
- Goal: wrapper smoke pack の current command surface を docs に同期する。
- Scope:
  - `FEM4C/README.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs の少なくとも 1 箇所に `coupled_2d_acceptance_wrapper_smoke` と `coupled_2d_acceptance_wrapper_smoke_test` が記載される。
  - `coupled_2d_acceptance_lightweight_checks` / `ensure_fem4c_binary_test` / `coupled_2d_acceptance_wrapper_smoke` の役割差が区別できる。
  - docs sync test も上記 current surface を検証する。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-39 (Auto-Next)
- Status: `Done`
- Goal: compare-only subset rerun が stale binary でも self-heal することを top-level contract として固定する。
- Scope:
  - `FEM4C/scripts/test_make_coupled_2d_acceptance_compare_stage_integrators_stale_binary.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/ensure_fem4c_binary.sh`
- Acceptance:
  - temp source/header を使って `bin/fem4c` を stale 状態にした後でも `make -C FEM4C coupled_2d_acceptance_compare_stage_integrators_test` が PASS する。
  - `make -C FEM4C coupled_2d_acceptance_compare_stage_integrators_stale_binary_test` が top-level compare-only self-heal contract を self-test する。
  - compare schema / parser / runner には手を入れず、acceptance wrapper と helper script の境界で完結する。

### E-40 (Auto-Next)
- Status: `Done`
- Goal: compare-only stale-binary self-heal contract の current command surface を docs に同期する。
- Scope:
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs の少なくとも 1 箇所に `coupled_2d_acceptance_compare_stage_integrators_stale_binary_test` が記載される。
  - compare-only subset contract / wrapper smoke pack / helper test の役割差が区別できる。
  - docs sync test も上記 current surface を検証する。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-41 (Auto-Next)
- Status: `Done`
- Goal: helper + wrapper smoke + compare-only stale-binary contract を 1 コマンド resilience pack に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_wrapper_smoke.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_compare_stage_integrators_stale_binary.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_resilience_checks.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_resilience_checks` が `coupled_2d_acceptance_wrapper_smoke_test` と `coupled_2d_acceptance_compare_stage_integrators_stale_binary_test` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_resilience_checks_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、resilience bundle のみで完結する。

### E-42 (Auto-Next)
- Status: `Done`
- Goal: resilience pack の current command surface を docs に同期する。
- Scope:
  - `FEM4C/README.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs の少なくとも 1 箇所に `coupled_2d_acceptance_resilience_checks` と `coupled_2d_acceptance_resilience_checks_test` が記載される。
  - wrapper smoke / compare-only stale-binary contract / resilience pack の役割差が区別できる。
  - docs sync test も上記 current surface を検証する。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-43 (Auto-Next)
- Status: `Done`
- Goal: gate wrapper と resilience pack を 1 コマンド smoke pack に束ねる。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_run_2d_coupled_acceptance_gate.sh`
  - 必要時のみ `FEM4C/scripts/test_make_coupled_2d_acceptance_resilience_checks.sh`
- Acceptance:
  - `make -C FEM4C coupled_2d_acceptance_gate_resilience_smoke` が `coupled_2d_acceptance_gate_test` と `coupled_2d_acceptance_resilience_checks_test` を順に実行する。
  - `make -C FEM4C coupled_2d_acceptance_gate_resilience_smoke_test` が bundle target の PASS surface を検証できる。
  - compare schema / parser / runner には手を入れず、focused smoke bundle のみで完結する。

### E-44 (Auto-Next)
- Status: `Done`
- Goal: gate+resilience focused smoke bundle の current command surface を docs と docs sync で固定する。
- Scope:
  - `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `docs/team_runbook.md`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - docs sync test が `coupled_2d_acceptance_gate_resilience_smoke` と `coupled_2d_acceptance_gate_resilience_smoke_test` の記載を検証する。
  - acceptance doc / runbook / README の current command surface に、focused smoke bundle とその child target (`coupled_2d_acceptance_gate_test`, `coupled_2d_acceptance_resilience_checks_test`) の関係が明記される。
  - compare schema / parser / runner には手を入れず、docs sync のみで完結する。

### E-45 (Auto-Next)
- Status: `Done`
- Goal: non-default `mbd_regression` の negative-path expectation mismatch を current binary に合わせて閉じる。
- Scope:
  - `FEM4C/scripts/run_mbd_regression.sh`
  - `FEM4C/scripts/check_mbd_invalid_inputs.sh`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/README.md`
  - 必要時のみ `docs/06_acceptance_matrix_2d.md`
- Acceptance:
  - `make -C FEM4C mbd_regression` が `case_incomplete` を含む negative diagnostics まで current binary と整合して PASS する。
  - `check_mbd_invalid_inputs.sh` の expected error-code / message が current parser/system2d behavior と矛盾しない。
  - `mbd_regression` は引き続き non-default 扱いを維持し、default Run 1 route へは戻さない。

### E-46 (Auto-Next)
- Status: `Done`
- Goal: non-default `mbd_checks` の broken probe/link path を current tree に合わせて整理し、`mbd_constraint_probe` 起因の FAIL を切り分ける。
- Scope:
  - `FEM4C/Makefile`
  - 必要時のみ `FEM4C/practice/ch09/mbd_constraint_rhs_probe.c`
  - 必要時のみ `FEM4C/practice/ch09/mbd_probe_utils.h`
  - 必要時のみ `FEM4C/README.md`
- Acceptance:
  - `make -C FEM4C mbd_checks` が current tree で PASS、または broken probe を non-default split target へ切り出したうえで `mbd_checks` の role が docs/help と整合する。
  - `mbd_constraint_probe` の unresolved symbol failure が build wiring か probe source かを切り分け、再発時に原因が追える。
  - default Run 1 route (`mbd_m1_rigid_acceptance`) には手を入れない。

### E-47 (Auto-Next)
- Status: `Done`
- Goal: non-default `mbd_checks` / `mbd_negative` / `mbd_regression` の current command surface を docs/help に同期し、Run 1 default route との境界を固定する。
- Scope:
  - `FEM4C/README.md`
  - `docs/06_acceptance_matrix_2d.md`
  - 必要時のみ `FEM4C/Makefile`
  - 必要時のみ `FEM4C/scripts/test_check_coupled_2d_acceptance_docs_sync.sh`
- Acceptance:
  - docs/help の少なくとも 1 箇所に `mbd_checks`, `mbd_negative`, `mbd_regression` の non-default role が current behavior と整合して記載される。
  - `mbd_m1_rigid_acceptance` との役割差が区別できる。
  - focused self-test か docs sync で current command surface を再検証できる。

## 10. 比較データに関する扱い
- RecurDyn / AdamsFlex の実データは現時点では不要。
- 今必要なのは `docs/09_compare_schema_2d.md` に定義した列構成に合わせて、FEM4C 側の出力を固定すること。
- 実データ比較は M4 で行う。
