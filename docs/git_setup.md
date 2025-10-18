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
