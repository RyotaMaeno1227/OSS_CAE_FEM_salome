# Coupled Constraint Hands-on Guide

FEM4C の `tutorial_manual.md` / `FEM_LEARNING_GUIDE.md` に倣い、Coupled 拘束を段階的に実装・検証するための演習メモです。  
各チャプターは「理論メモ → 実装タスク → FEM4C で確認 → 検証課題」の構成になっています。

> 学習パス統合のステータス: W2（TODO マーカー整理）が進行中、W3（リンク検証自動化）が着手済みです。`docs/appendix_optional_ops.md` **E. Learning Path Integration Plan** と `docs/integration/learning_path_map.md` を確認し、演習を更新した場合はチェックリスト経由で Tutorial に反映してください。
> プリセットは `docs/coupled_constraint_presets_cheatsheet.md`（Markdown）を参照してください。

### Learning Path Snapshot (2025-11-08)
| フェーズ | 状態 | 依存ファイル |
|----------|------|--------------|
| W2 – Hands-on TODO 抽出 | ⏳ `ch02/ch03` 整理中 | `practice/coupled/ch02_softness.c`, `ch03_contact.c` |
| W3 – Link Check 自動化 | 🆕 着手 | `scripts/check_doc_links.py`, Appendix B.7 |
| W4 – 統合レビュー | 未着手 | Appendix E, `docs/documentation_changelog.md` |

---

## Chapter 01. Warm-up – Ratio Sweep
- **Theory**: `docs/coupled_constraint_solver_math.md` の式 (1) を再確認し、`ratio_distance`, `ratio_angle` の意味を整理。  
- **Implementation**: `practice/coupled/ch01_ratio_sweep.c`（新規作成）で、`chrono_coupled_constraint2d_set_ratios` を 3 ステージに切り替え、`diagnostics.condition_number` を標準出力へ記録。  
- **FEM4C Reference**: `FEM4C/docs/tutorial_manual.md` Chapter 03 の「パラメータを段階的に変更する」節を読み、練習コードにコメントを付与。  
- **Verification**: `./chrono-C-all/tests/test_coupled_constraint` を実行し、ステージ毎の WARN ログが期待通りかをチェック。

## Chapter 02. Softness & Springs
- **Theory**: ソフトネス・Baumgarte・スプリングの役割を `docs/coupled_constraint_tutorial_draft.md#1-数式フェーズ` で復習。  
- **Implementation**: `practice/coupled/ch02_softness.c` にて `chrono_coupled_constraint2d_set_softness_distance/angle` と `chrono_coupled_constraint2d_set_*_spring` を段階的に変えながら、`diagnostics.min_pivot` と `max_pivot` を CSV に出力。  
- **FEM4C Reference**: `FEM4C/docs/tutorial_manual.md` Chapter 05（剛性・境界条件）を読み、ソフトネスが連立方程式にどのように入り込むかをノートにまとめる。  
- **Verification**: Multi-ω ベンチを実行してソフトネス掃引を比較。

  ```bash
  ./chrono-C-all/tests/bench_coupled_constraint \
    --omega 0.85 \
    --omega 1 \
    --omega 1.15 \
    --output data/diagnostics/bench_coupled_constraint_multi.csv \
    --result-json data/diagnostics/bench_coupled_constraint_multi.json
  ```
  `tools/plot_coupled_constraint_endurance.py --summary-json` で差分を可視化し、README の「Coupled Presets」と同じ条件で議論できるようにする。
- Multi-ω preset last updated: 2025-11-08T18:18:55Z
- **Sync note**: Multi-ω の再計測結果を `data/diagnostics/bench_coupled_constraint_multi.csv` と `data/coupled_constraint_presets.yaml`（`multi_omega_reference`）に反映し、README の「Coupled Presets」と同じ内容を保つ。PR では `docs/reports/kkt_spectral_weekly.md` の Multi-ω テーブルも再生成する。
  - `python3 tools/update_multi_omega_assets.py --refresh-report` で README/Hands-on/プリセット/CSV+JSON/kkt stats/週次レポートが一括更新される。

## Chapter 03. Contact + Coupled Integration
- **Theory**: `docs/coupled_contact_test_notes.md` を読み、Contact 併用テストの意図と判定指標を把握。  
- **Implementation**: `practice/coupled/ch03_contact.c` で `chrono_collision2d_detect_polygon_polygon` を呼び出しつつ Coupled 拘束を同一島で解く短いシミュレーションを実装。  
- **FEM4C Reference**: `FEM_LEARNING_GUIDE.md` の「接触境界条件」を参照し、拘束の組合せでどのように挙動が変わるかを議論。  
- **Verification**: `./chrono-C-all/tests/test_island_parallel_contacts` を実行し、`docs/coupled_contact_test_notes.md` のチェックリストに沿ってログを読み解く。

## Chapter 04. Endurance & Diagnostics
- **Theory**: `docs/coupled_constraint_solver_math.md#3-条件数評価と式ドロップ` を読み、`condition_policy` の挙動を整理。  
- **Implementation**: `practice/coupled/ch04_endurance.py`（Python）で `tools/plot_coupled_constraint_endurance.py` をラップし、条件数・ドロップ回数・Pivot を同一プロットへ表示するヘルパを実装。  
- **FEM4C Reference**: `tutorial_manual.md` Chapter 06 の「線形ソルバと検証」を参考に、解析結果の可視化メモを残す。  
- **Verification**: `./chrono-C-all/tests/test_coupled_constraint_endurance` を実行し、出力 CSV をヘルパで解析。`diagnostics.rank == active_equations` を assert する。

---

### 付録 A. 推奨フォルダ構成
```
practice/
  coupled/
    ch01_ratio_sweep.c
    ch02_softness.c
    ch03_contact.c
    ch04_endurance.py
```
- `make practice-coupled` などのターゲットを `practice/README.md` に追記しておくと複数章をまとめてビルドしやすい。

### 付録 B. レポートテンプレ
```
- Chapter: (例) 02 Softness & Springs
- Date / Author:
- Theory notes:
- Experiment setup (params / dt / ratios):
- Observations (cond numbers, pivots, WARN count):
- Comparison versus FEM4C chapter:
- Next steps / questions:
```

このハンズオンの成果は `docs/coupled_constraint_tutorial_draft.md` の演習や `docs/coupled_island_migration_plan.md` の移行計画にフィードバックできるよう、レポートをリポジトリ内で共有してください。
