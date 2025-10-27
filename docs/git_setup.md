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
