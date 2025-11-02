# Coupled Constraint Presets Cheat Sheet

このチートシートは `data/coupled_constraint_presets.yaml` に定義されたユースケース用パラメータを即参照できるようにまとめたものです。距離比（`ratio_distance`）、角度比（`ratio_angle`）、柔構造（`softness_*`）、バネ／ダンパ係数などの意味と推奨範囲を記載し、スクリプトやドキュメントからの導線を提供します。

## 1. プリセット一覧（抜粋）

| ID (`use_cases[].id`) | 主用途 | 距離比 | 角度比 | 柔構造 (距離/角度) | バネ係数 (距離/角度) | 主な備考 |
|----------------------|--------|--------|--------|---------------------|----------------------|----------|
| `tele_yaw_control` | ブーム距離＋ヨー連動 | 1.00 | 0.40 | 0.014 / 0.028 | 38.0 N/m / 18.0 N·m/rad | 90° 超の切替時は `max_correction` を 0.09 以上へ。 |
| `cam_follow_adjust` | カム位相補正 | 0.48 | -0.32 | 0.018 / 0.024 | 24.0 / 12.0 | 追加式（index 1）で比率 0.55 / -0.25 を設定。 |
| `counterbalance_beam` | カウンターバランス梁 | 0.85 | -0.30 | 0.013 / 0.022 | 42.0 / 20.0 | 補助式（index 1）を `max_drop=1` で自動ドロップ対象に。 |
| `docking_guide` | ドッキング誘導 | 0.72 | -0.25 | 0.020 / 0.034 | 30.0 / 14.0 | ステージ毎に `target_offset` や比率を更新。 |

> YAML の正確な値は `data/coupled_constraint_presets.yaml` を参照してください。`extra_equations` や `staged_adjustments` も同ファイルに含まれています。

## 2. パラメータの意味と推奨範囲

| パラメータ | 意味 | 推奨範囲 | 注意点 | YAML 参照 |
|------------|------|-----------|--------|-----------|
| `ratio_distance` | 距離項の係数。正で引き込み、負で押し戻し。 | 0.3 – 1.2（用途により最大 1.5 まで） | 1.0 を超えると距離リードが強くなり、条件数悪化に注意。 | `use_cases[].base.ratio_distance` |
| `ratio_angle` | 角度項の係数。正負で回転方向を制御。 | -0.5 – 0.5 | 大きい値は角速度のスパイクを誘発。距離側の 0.3–0.6 倍程度が無難。 | `use_cases[].base.ratio_angle` |
| `target_offset` | 線形結合の目標値。距離と角度の組み合わせに定数バイアスを与える。 | -0.03 – 0.03 | 段階的に更新することで過渡振動を抑制。ステージごとに設定可。 | `use_cases[].base.target_offset` / `staged_adjustments[]` |
| `softness_distance` | 距離軸のコンプライアンス。値が大きいほど柔らかい。 | 0.012 – 0.025 | 小さすぎると振動、大きすぎると収束が遅い。条件数に直結。 | `use_cases[].base.softness_distance` |
| `softness_angle` | 角度軸のコンプライアンス。 | 0.020 – 0.040 | 距離側の 1.5 – 2.0 倍を目安に設定。 | `use_cases[].base.softness_angle` |
| `spring_distance.stiffness` | 距離バネ係数 [N/m]。 | 20 – 45 | ステップ当たりの追従性に影響。剛性が高いほどドリフト抑制だが発散リスクも増。 | `use_cases[].base.spring_distance.stiffness` |
| `spring_distance.damping` | 距離ダンパ係数 [N·s/m]。 | 2.0 – 3.5 | 剛性に合わせて比率 1:10 程度で設定。 | `use_cases[].base.spring_distance.damping` |
| `spring_angle.stiffness` | 角度バネ係数 [N·m/rad]。 | 10 – 22 | 高すぎると角度側が主導になり距離軸と競合。 | `use_cases[].base.spring_angle.stiffness` |
| `spring_angle.damping` | 角度ダンパ係数 [N·m·s/rad]。 | 0.7 – 0.9 | ステップ時間 `dt` に応じて 0.6 – 1.0 を目安に。 | `use_cases[].base.spring_angle.damping` |
| `baumgarte` | 位置誤差補正のゲイン。 | 0.35 – 0.40 | Time step 0.003 – 0.005 s を想定。より小さい dt では 0.2 – 0.3 に調整。 | `use_cases[].base.baumgarte` |
| `slop` | 許容誤差ウィンドウ [m]。 | 5e-4 – 7e-4 | 高周波ノイズ対策。粗いメッシュでは 1e-3 まで拡大可。 | `use_cases[].base.slop` |
| `max_correction` | 1 ステップで許容する最大補正量 [m]。 | 0.08 – 0.10 | 目標切替が急な場合は 0.12 まで検討。 | `use_cases[].base.max_correction` |
| `extra_equations[]` | 補助式のパラメータ。 | 各式で ratio 0.3 – 0.8 | 自動ドロップ対象にする場合は `ChronoCoupledConditionWarningPolicy_C` の `max_drop` と併用。 | `use_cases[].extra_equations` |
| `staged_adjustments[]` | ステージごとのパラメータ上書き。 | 任意 | CSV ログに `phase_id` を追加して分析すると効果が見えやすい。 | `use_cases[].staged_adjustments` |

## 3. YAML の読み込み例

```python
import yaml
from pathlib import Path

presets = yaml.safe_load(Path("data/coupled_constraint_presets.yaml").read_text())
tele = next(item for item in presets["use_cases"] if item["id"] == "tele_yaw_control")
print(tele["base"]["ratio_distance"], tele["base"]["ratio_angle"])
```

- CI やスクリプトから値を反映する場合、YAML を直接読み込むことでパラメータの一元管理ができます。
- `extra_equations` や `staged_adjustments` は存在しない場合もあるため、辞書アクセス時は `.get()` を用いてください。

## 4. 更新手順（チーム向けメモ）

1. `data/coupled_constraint_presets.yaml` の更新時は **必ず** このチートシートのテーブルも同期し、変更理由を `docs/chrono_2d_development_plan.md` の Coupled セクションへ追記してください。
2. 値の推奨レンジが変わった場合は、本ドキュメントの表だけでなく、`docs/chrono_coupled_constraint_tutorial.md` の該当セクションと README のチューニング例も併せて更新します。
3. CI（`coupled_endurance.yml`）でパラメータを参照している場合は、Pull Request のチェックリストに「YAML → CI → ドキュメント反映済み」を追加してください。

このチートシートは、Coupled 拘束のチューニングやトラブルシュートを素早く行うためのショートカットとして活用してください。

## 5. PDF/ポスター化ガイド

### 5.1 Pandoc を用いた Markdown → PDF
1. 依存インストール（TeX Live または MiKTeX 等の LaTeX 環境）。  
   Ubuntu 例: `sudo apt-get install pandoc texlive-latex-extra texlive-fonts-recommended`
2. コマンド例:
   ```bash
   pandoc docs/coupled_constraint_presets_cheatsheet.md \
     -o out/coupled_constraint_presets_cheatsheet.pdf \
     --pdf-engine=xelatex \
     -V geometry:margin=18mm \
     -V mainfont="Noto Sans CJK JP"
   ```
   - `geometry` で余白を指定し、ポスター用途なら `margin=12mm` に縮めると情報量を増やせます。
   - `mainfont` は日本語フォントを適宜調整（Windows: `"Yu Gothic"`、macOS: `"Hiragino Sans"` など）。
3. 表が見切れる場合は、Markdown 部分に `:---:` より幅の広いカラムを使うか、`-V tables=true` オプションで LaTeX の longtable を有効化してください。

### 5.2 LaTeX テンプレート（再利用可）
シンプルなポスター用テンプレート例。Pandoc を使わず直接 LaTeX に差し込みたい場合に利用します。

```tex
\documentclass[a4paper,landscape]{article}
\usepackage{geometry}
\geometry{left=12mm,right=12mm,top=12mm,bottom=12mm}
\usepackage{fontspec}
\setmainfont{Noto Sans CJK JP}
\usepackage{array,longtable,xcolor}
\definecolor{headerbg}{HTML}{1F4E79}
\definecolor{headerfg}{HTML}{FFFFFF}

\begin{document}
\section*{Coupled Constraint Presets}

\rowcolors{2}{gray!10}{white}
\begin{longtable}{>{\bfseries}p{35mm}p{40mm}p{18mm}p{18mm}p{25mm}p{30mm}p{40mm}}
\rowcolor{headerbg}{\color{headerfg}ID} & {\color{headerfg}用途} & {\color{headerfg}距離比} & {\color{headerfg}角度比} & {\color{headerfg}柔構造} & {\color{headerfg}バネ係数} & {\color{headerfg}備考} \\
\endfirsthead
\rowcolor{headerbg}{\color{headerfg}ID} & {\color{headerfg}用途} & {\color{headerfg}距離比} & {\color{headerfg}角度比} & {\color{headerfg}柔構造} & {\color{headerfg}バネ係数} & {\color{headerfg}備考} \\
\endhead
tele\_yaw\_control & ブーム距離＋ヨー連動 & 1.00 & 0.40 & 0.014 / 0.028 & 38.0 N/m / 18.0 N·m/rad & 90°超は max\_correction↑ \\
cam\_follow\_adjust & カム位相補正 & 0.48 & -0.32 & 0.018 / 0.024 & 24.0 / 12.0 & 補助式 index1 を追加 \\
counterbalance\_beam & カウンターバランス梁 & 0.85 & -0.30 & 0.013 / 0.022 & 42.0 / 20.0 & max\_drop=1 で補助式ドロップ \\
docking\_guide & ドッキング誘導 & 0.72 & -0.25 & 0.020 / 0.034 & 30.0 / 14.0 & ステージ更新で target\_offset 調整 \\
\end{longtable}

\bigskip
推奨レンジ: ratio\_distance 0.3–1.2, ratio\_angle -0.5–0.5, softness 距離 0.012–0.025, 角度 0.020–0.040

\end{document}
```

- YAML から自動生成したい場合は、上記テンプレートのテーブル行をスクリプト（Python + Jinja2 など）で動的に埋め込むと繰り返し利用が容易です。
- 長尺ポスターにする際は `landscape` を `portrait` に変更し、`p{mm}` の幅を目的に合わせて調整してください。

### 5.3 運用メモ
- PDF に変換したファイルは `docs/media/coupled/` もしくは社内ストレージの `/posters/` に保存し、Wiki のクイックリンクから参照できるようにします。
- 印刷時の推奨サイズ：A3（ランドスケープ）。A4 で印刷する場合はフォントサイズ 10–11pt 目安。
