# Git 初期設定メモ

このプロジェクトは既に Git を初期化し、GitHub 上のリモートと接続済みです。  
同じ手順を再実行したいときや新規プロジェクトを立ち上げるときは、以下のチェックリストを参考にしてください。

## 1. リポジトリの初期化

```bash
git init
git branch -m main        # 必要ならデフォルトブランチ名を main に変更
```

## 2. コミット作者情報の設定

コミットに正しいメタデータが付くよう、作者情報を設定します。

```bash
git config user.name "RyotaMaeno1227"
git config user.email "rmaeno1227@gmail.com"

# 念のため値を確認したいとき
git config --get user.name
git config --get user.email
```

## 3. 現在のプロジェクト状態をコミット

```bash
git add .
git status                # コミット対象を確認
git commit -m "Initial commit"
```

## 4. GitHub リモートの登録

```bash
git remote add origin https://github.com/RyotaMaeno1227/OSS_CAE_FEM_salome.git
git remote -v             # リモート設定の確認
```

## 5. main ブランチをプッシュ

```bash
git push -u origin main   # upstream を設定して今後の push を簡略化
```

HTTPS リモートの場合、パスワード欄には GitHub の Personal Access Token を使用します。

## 6. コミット後に作者情報を修正したいとき

直近のコミットに誤ったメールアドレスが含まれてしまった場合は次の手順で修正します。

```bash
git config user.email "修正後のアドレス@example.com"
git commit --amend --reset-author --no-edit
git push --force-with-lease origin main
```

## 7. 初期セットアップ後のおすすめ作業

- `.gitignore` を整備してビルド成果物や編集用バックアップがコミット対象に入らないようにする  
  （現在の `.gitignore` は一般的な Fortran の成果物を除外する設定になっています）。
- GitHub リポジトリの設定（デフォルトブランチ、ブランチ保護等）を確認・調整する。
- `readme.txt` などのドキュメントにビルド手順や運用ルールを追記する。

## 8. Chrono サブモジュールの管理

このリポジトリでは `third_party/chrono` を Git サブモジュールとして管理しています。  
Chrono 側のリポジトリ URL: `https://github.com/RyotaMaeno1227/OSS_CAE_MBD.git`

### 初回クローン直後

```bash
git submodule update --init --recursive
```

### Chrono を最新版に更新したいとき

```bash
cd third_party/chrono
git checkout main
git pull origin main
cd ../..
git add third_party/chrono
git commit -m "Update Chrono submodule"
```

### 既存コミットに合わせてサブモジュールを同期したいとき

```bash
git submodule update --init --recursive
```

### 注意事項

- Chrono リポジトリ側では OpenCASCADE や Salome-Meca のアーカイブは含めません（必要に応じて各自取得）。
- サブモジュールの変更を反映する際は、親リポジトリで `git add third_party/chrono` を忘れずに実行してください。

## 9. Codex の承認ダイアログを無効化したいとき

常に承認なしで作業を進めたい場合は、ホームディレクトリにある Codex 設定ファイル `~/.codex/config.toml` を編集します。

```bash
nano ~/.codex/config.toml
```

以下を追加・変更すると、workspace 内の操作はすべて自動で承認されます（外部ファイルやネットワークはブロックされます）。

```toml
approval_policy = "never"
sandbox_mode    = "workspace-write"

[sandbox_workspace_write]
network_access = true   # ネットを無効化したい場合は false
```

作業モードを切り替えられるようにしたい場合は、プロファイルを定義しておくと便利です。

```toml
[profiles.auto]
approval_policy = "on-request"
sandbox_mode    = "workspace-write"

[profiles.handsfree]
approval_policy = "never"
sandbox_mode    = "workspace-write"

[sandbox_workspace_write]
network_access = false
```

使用例:

```bash
# 従来どおり承認ありで動かす
codex --profile auto

# 手放しモードで起動
codex --profile handsfree
```

※設定キー名や値は OpenAI Codex の公式ドキュメントに準拠しています。

## 10. 教育資料と Run ID の整合チェック

- README / Hands-on / Tutorial / Cheat Sheet を同時に更新した場合は、コミット前に `python scripts/check_preset_links.py` を実行してプリセット参照がずれていないか確認してください（CI でも同スクリプトが実行されます）。
- Run ID を記録したら `python tools/update_descriptor_run_id.py --run-id <GITHUB_RUN_ID>` を走らせ、`docs/logs/kkt_descriptor_poc_e2e.md` と `docs/archive/legacy_chrono/coupled_island_migration_plan.md`、および `docs/abc_team_chat_handoff.md` の Evidence テンプレを同期します。
- 教育資料系ドキュメントを更新したときは `docs/documentation_changelog.md` にエントリを追加し、必要に応じて `docs/abc_team_chat_handoff.md` のタスク表にも結果を反映します。

## 11. Bチーム向け Git 差分確認チートシート

Nightly / Diagnostics で `data/coupled_constraint_endurance.csv` や `docs/archive/legacy_chrono/pm_status_2024-11-08.md` を更新した直後は、以下の流れでコミット対象を確認してください。

```bash
# 1. 代表差分を確認（長尺 CSV は tail/head でポイント確認）
git status -sb
git diff --stat
git diff data/coupled_constraint_endurance.csv | tail

# 2. 必須ファイルをステージング
git add data/coupled_constraint_endurance.csv \
        data/latest.endurance.json \
        docs/archive/legacy_chrono/pm_status_2024-11-08.md \
        docs/abc_team_chat_handoff.md \
        docs/documentation_changelog.md

# 3. テンプレ更新時（docs/templates/ 配下）も忘れずに追加
git add docs/templates/b_team_endurance_templates.md

# 4. コミットメッセージ例
git commit -m \"B-team: log Run #19381234567 and refresh endurance artifacts\"
```

- `data/endurance_archive/` にファイルを置かないルールを守るため、`ls data/endurance_archive` の結果も記録しておく。  
- 共有チャットには `git status` と `python scripts/check_preset_links.py` の実行結果を貼り付け、A/C チームに通知する。  
- Run ID が複数ある場合は、本ドキュメントの手順に従い優先順位ルール（最新成功 > 最新失敗 > 旧成功）でログを整理する。

## 12. chrono-2d/chrono-main と Chrono C の区別
- Run ID にプレフィックスを付けて混同を防ぐ: `local-chrono2d-...`, `#<ID> (chrono-main)`, `#<ID> (Chrono C)`。
- preset チェックや Run ID 連携は `python scripts/check_preset_links.py` 実行 → `docs/abc_team_chat_handoff.md` の各テンプレ（Chrono C / chrono-main / chrono-2d）に記録する。
- chrono-2d はリンクチェックも実施: `python scripts/check_doc_links.py docs/chrono_2d_readme.md docs/abc_team_chat_handoff.md` を更新時に走らせ、結果をチャットへ貼る（表記揺れ/命名ポリシーも確認）。

### Endurance 更新後の最小確認ブロック（抜粋して使う）
```bash
git status -sb
tail -n 3 data/coupled_constraint_endurance.csv
python tools/plot_coupled_constraint_endurance.py data/coupled_constraint_endurance.csv --skip-plot --summary-json data/latest.endurance.json --no-show
python scripts/check_preset_links.py  # 週次フローの一環
```
- 上記出力をスクリーンショット化し、Run ID と併せてチャットへ共有。必要なら `head -n 1 ... | python - <<'PY' ...` で列数も確認する。
