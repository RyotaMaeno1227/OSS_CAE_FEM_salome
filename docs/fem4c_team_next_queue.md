# FEM4C Team Next Queue

更新日: 2026-02-06  
用途: チャットで「作業を継続してください」だけが来た場合の、各チーム共通の次タスク起点。

## 継続運用ルール
- 1. 各チームは本ファイルの自チーム先頭の未完了タスクから着手する。
- 2. 着手時に `In Progress` へ更新し、完了時に `Done` へ更新する。
- 3. 作業結果は `docs/team_status.md` に追記する。
- 4. セッション終了時は `docs/session_continuity_log.md` の4項目を更新する。
- 5. 担当外ファイルはステージしない（混在コミット禁止）。

---

## Aチーム（実装）

### A-1 MBD入力アダプタ（最優先）
- Status: `In Progress`
- Goal: `--mode=mbd` で「入力内に MBD 行がある場合は入力ケースを使用し、無い場合は内蔵ミニケースへフォールバック」する最小経路を作る。
- Scope:
  - `FEM4C/src/analysis/runner.c`
  - 必要時のみ `FEM4C/src/io/input.c` / `FEM4C/src/io/input.h`
- Acceptance:
  - `cd FEM4C && ./bin/fem4c --mode=mbd <mbd_case> out_mbd.dat` が exit 0。
  - MBD行あり入力で入力ケースが使われること、MBD行なし入力でフォールバックすることをログで判別できる。
  - ログに `constraint_equations` / `residual_l2` を表示。

### A-2 Coupled I/O 契約定義
- Status: `Todo`
- Goal: `coupled` 実装に必要な最小 I/O 契約（FEM状態, MBD状態, 時間積分パラメータ）をヘッダで定義する。
- Scope:
  - `FEM4C/src/analysis/runner.h`
  - 必要時のみ `FEM4C/src/common/types.h`
- Acceptance:
  - `runner.c` の TODO コメントを具体構造体/フィールド参照へ置換。
  - `make -C FEM4C` 成功。

### A-3 MBD最小回帰スクリプト
- Status: `Todo`
- Goal: 手元で 1 コマンド回帰できるシェル手順を追加する。
- Scope:
  - `FEM4C/practice/ch09/` または `FEM4C/scripts/`（新規）
- Acceptance:
  - コマンド 1 行で `mbd` モード実行と結果判定まで完了。

---

## Bチーム（検証）

### B-1 検証ハーネスのビルド導線固定
- Status: `Todo`
- Goal: `mbd_constraint_probe` を誰でも同じコマンドで実行できるように Makefile/README を固定化。
- Scope:
  - `FEM4C/Makefile`
  - `FEM4C/practice/README.md`
- Acceptance:
  - `make -C FEM4C mbd_probe`（または同等の短縮コマンド）で PASS まで再現。

### B-2 ヤコビアン検証ケース追加
- Status: `Todo`
- Goal: 現在の1状態だけでなく、少なくとも2つの状態で FD 照合を実施。
- Scope:
  - `FEM4C/practice/ch09/mbd_constraint_probe.c`
- Acceptance:
  - 追加ケースでも `|analytic-fd| <= 1e-6` を満たし、fail 時は非0終了。

### B-3 MBD実行ログ照合
- Status: `Todo`
- Goal: `--mode=mbd` 実行結果の `constraint_equations` と probe 側の式数を整合チェック。
- Scope:
  - `FEM4C/practice/ch09/`（ログ照合用補助）
- Acceptance:
  - `team_status` に照合結果（期待値/実測値）を記録。

---

## Cチーム（差分整理）

### C-1 test削除群の確定判定
- Status: `Todo`
- Goal: `docs/fem4c_dirty_diff_triage_2026-02-06.md` の復元候補/削除候補を最終判定へ更新。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - `FEM4C/test/data/*` と `FEM4C/test/unit/*` の扱いが `最終判定` として明記される。

### C-2 生成物除外の運用固定
- Status: `Todo`
- Goal: 生成物が毎回混入しないよう、除外ポリシーを docs と ignore で整合させる。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
  - 必要時のみ `.gitignore`
- Acceptance:
  - `out_mbd.dat` を含む生成物の扱いが明文化される。

### C-3 コミット分割テンプレ作成
- Status: `Todo`
- Goal: PM がそのまま使える「実装コミット / docsコミット / 保留差分」の手順を固定。
- Scope:
  - `docs/fem4c_dirty_diff_triage_2026-02-06.md`
- Acceptance:
  - 3種類のコミットに対して具体的 `git add` コマンドが提示される。

---

## PMチェックポイント
- A/B/C の更新は毎回 `docs/team_status.md` の該当セクションで確認する。
- 継続指示のみの運用時でも、最終判断は本ファイルの `Status` 更新と受入基準で行う。
