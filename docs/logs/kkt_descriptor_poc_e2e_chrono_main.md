# KKT Descriptor PoC – E2E Log (chrono-main)

最新 Run ID: **19595392293**

- Run: [#19595392293](https://github.com/<owner>/<repo>/actions/runs/19595392293)  
- Artifact: `descriptor-e2e-19595392293` (descriptor CSVのみ)  
- Branch: `main`  
- Target repo: `third_party/chrono/chrono-main`  
- Date: 2025-11-17  

## Command
```bash
./build/bin/test_coupled_constraint --use-kkt-descriptor --descriptor-mode actions \
  --descriptor-log artifacts/descriptor/kkt_descriptor_actions_19595392293.csv \
  --pivot-artifact-dir artifacts/descriptor/run-19595392293
```

## Evidence
- Descriptor CSV: `artifacts/descriptor/kkt_descriptor_actions_19595392293.csv`
- Pivot/Spectral/Diagnostics: 本 Artifact には含まれず（CSV のみ）
- Notes (actions CSV 集計):
  - tele_yaw_control: κ_s≈3.64, pivot span≈[0.59, 2.16]（5サンプル）
  - cam_follow_adjust: κ_s=1.0, pivot=0.70
  - counterbalance_beam: κ_s=1.0, pivot=1.14
  - hydraulic_lift_sync: κ_s=1.0, pivot=0.54
  - optic_alignment_trim: κ_s=1.0, pivot=1.18

## Next steps
- 同一 Run ID を `docs/coupled_island_migration_plan.md` の chrono-main 行と `docs/abc_team_chat_handoff.md` の chrono-main テンプレに記載する。  
- pivot/diagnostics を取得する場合は次回 Actions で `run-<ID>` ディレクトリも artifacts に含める。  
- 新しい Run を追加する際は本ファイルを追記し、古い Run はアーカイブ扱いとして残す。  
