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
  - 継続運用: チャット指示が「作業を継続してください」のみの場合、`docs/fem4c_team_next_queue.md` の自チーム先頭タスクから着手する。

### Aチーム（実装）
- 目的: `mbd` / `coupled` モードを「未実装エラー」から最小実行経路へ進める。
- 対象ファイル:
  - `FEM4C/src/analysis/runner.c`
  - `FEM4C/src/analysis/runner.h`
  - 必要時のみ `FEM4C/src/fem4c.c`
- 指示:
  1. `mbd` モードで 2D 最小ケース（2 body + distance/revolute）を実行できる関数を追加する。
  2. `mbd_constraint_evaluate()` と `mbd_kkt_compute_layout_from_constraints()` を実際に呼び、残差ノルムとDOF情報をログ出力する。
  3. `coupled` モードはスタブ維持でも可。ただし TODO と必要I/Oをコメントで明記する。
- 受入基準:
  - `cd FEM4C && ./bin/fem4c --mode=mbd examples/t6_cantilever_beam.dat out_mbd.dat` が終了コード 0。
  - 実行ログに `Analysis mode: mbd` と拘束式本数/残差が表示される。

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
