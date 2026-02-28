# Coupled + Contact Test Notes

Coupled 拘束と Contact 拘束を同一島で解く際の意図と判定方法をまとめるメモです。主に以下のテストを対象としています。

- `chrono-C-all/tests/test_island_parallel_contacts.c`
- `chrono-C-all/tests/test_coupled_constraint.c`（Contact を追加した派生ケース）
- `practice/coupled/ch03_contact.c`（Hands-on Chapter 03）

## 1. 目的
1. Coupled 拘束による距離・角度制御と、Contact 拘束による衝突応答が競合せず両立することを確認する。
2. Island 分割後でも Coupled 拘束が正しい島に割り当てられ、並列・直列で同じ結果になることを確認する。
3. 条件数 WARN / Drop が Contact の衝突状況と一致して記録されることを検証する。

## 2. 判定指標

| 指標 | 取得先 | 合格基準 |
|------|--------|----------|
| `diagnostics.rank` | `chrono_coupled_constraint2d_get_diagnostics` | アクティブ式数（`equation_active`）と一致している。 |
| `condition_warning` フラグ | 同上 | Contact が高荷重状態のときにのみ WARN が出る／`condition_policy` が機能している。 |
| Contact 反力 | `chrono_contact_manifold2d` | Coupled 拘束が距離を短く保持する際、接触反力が爆発しない（スムーズに収束する）。 |
| 並列 vs 直列 | `test_island_parallel_contacts` | `CHRONO_ENABLE_OPENMP` ON/OFF で結果差分（インパルス・条件数）が許容範囲内（1e-6 以内）。 |
| 3DOF Jacobian 行 | `test_island_parallel_contacts` | `chrono_contact2d_build_jacobian_3dof` の Normal/Rolling/Torsional 行が `ChronoContactPair2D` の姿勢と一致（許容誤差 1e-9）。 |

<!-- jacobian-status:start -->
_Last verified: 2025-11-15T18:23:36Z (max abs error 0.000e+00 ≤ tol 1.000e-09)._
<!-- jacobian-status:end -->

- `tests/test_island_parallel_contacts --jacobian-report docs/coupled_contact_test_notes.md --jacobian-log out/contact_jacobian.csv`（または `--jacobian-log-default`）を実行すると上記ステータスとログが自動更新される（CI では `--jacobian-log` のみ使用し、Markdown 変更はローカルで行う）。
- `python3 tools/run_contact_jacobian_check.py --output-dir data/diagnostics/jacobians --report docs/coupled_contact_test_notes.md` を使うと保存先ディレクトリを切り替えつつ Jacobian CSV と Markdown の両方を 1 コマンドで更新できる（`--skip-report` でログのみ取得も可）。`--list-presets` を付けると Hands-on/README で参照しているプリセット候補が表示されるので、レポートへ貼る際の脚注に利用できる。

### Placeholder → 実データ更新チェックリスト

1. 最新 CI Run ID を `python3 tools/update_descriptor_run_id.py --run-id <ID>` でドキュメントへ反映。
2. `python3 tools/run_contact_jacobian_check.py --output-dir artifacts/contact --report docs/coupled_contact_test_notes.md` を実行し、CI と同じディレクトリ構成で CSV を取得。
3. `git diff docs/coupled_contact_test_notes.md` を確認し、placeholder テキストが実測値に置き換わっていることをレビュー。
4. 必要に応じて `artifacts/contact/contact_jacobian_log.csv` を PR 添付、`docs/logs/kkt_descriptor_poc_e2e.md` の Run ID 表と突き合わせる。

Slack テンプレ（placeholder → 実データ差し替え時）:
```
[contact-jacobian] run=<id>
- report: docs/coupled_contact_test_notes.md#jacobian-status
- log: artifacts/contact/contact_jacobian_log.csv
- notes: pivot drift < 1e-6 / WARN=0
```

## 3. ログ読み解き手順
1. `./chrono-C-all/tests/test_island_parallel_contacts --dump=log.json` を実行し、Coupled 拘束 ID と Contact manifold ID を対応付ける。  
2. `tools/filter_coupled_endurance_log.py --input log.json --focus contact` で Contact 関連 WARN を抽出。  
3. 通知テンプレはリポジトリから撤去済み。必要なら各チームの環境に合わせて Slack/メールの書式を用意し、Run ID のみを共有する。  
4. CI アラートは Run ID とログのみ共有し、以前あった Wiki ローテーション表は廃止済み。

## 4. よくあるトラブル
- **WARN が常時発生する**: `ratio_distance` / `ratio_angle` の縮尺が Contact の剛性より強すぎる。ソフトネスを増やして `condition_number` を 1e5 以下に抑える。  
- **並列時に結果がズレる**: Island 分割で Coupled と Contact が別島に分離されている可能性。`chrono_island2d_build` にログを入れ、同じ `island_id` に属しているか確認。  
- **Contact 反力が発散する**: Coupled が距離を縮め続け、Contact 反力と押し合いになるケース。`target_offset` をステージングし、Contact の解消を待ってから Coupled を締める。

## 5. 参考リンク
- `docs/coupled_constraint_tutorial_draft.md` §3 Verification  
- `docs/coupled_constraint_solver_math.md` §3 条件数評価  
- `docs/archive/legacy_chrono/coupled_island_migration_plan.md` §6 進捗テンプレート  
- 運用系ドキュメントはすべて削除済み（ログ解析や Wiki 差し替えは各チーム運用の責任で実施）。

## 6. 3DOF Jacobian 統合メモ

- 2025-11 時点で `tests/test_island_parallel_contacts` に 3DOF Jacobian の NearEqual チェックを追加済み。Rolling/Torsional 行も含めて `ChronoContactPair2D` の姿勢と一致するかを CI で常時検証している。
- `tests/test_contact_jacobian_3dof` は依然としてスタンドアロンの数値例を維持しているが、将来的にはヘッダのサニティチェックへ縮退させる予定。
- Jacobian の検証結果は `docs/reports/kkt_spectral_weekly.md` の Multi-ω セクションと一緒に確認すると、Coupled 側の条件数や drop との相関が把握しやすい。
