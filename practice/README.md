# Practice Sources

Chrono C 版 Hands-on で利用する演習コードは `practice/coupled/` に配置します。
`make practice-coupled` などのターゲットは任意のビルドシステムから追加してください。現状は各サンプルを単独でビルド・実行する想定です。

| ファイル | 目的 |
|----------|------|
| `coupled/ch01_ratio_sweep.c` | Ratio / ω を掃引し、Pivot と条件数を標準出力に記録する。 |
| `coupled/ch02_softness.c` | ソフトネス・バネパラメータを段階的に変更し、CSV を生成する。 |
| `coupled/ch03_contact.c` | Contact API と Coupled 拘束を同一島で組み合わせるミニシミュレーション。 |
| `coupled/ch04_endurance.py` | Endurance ログを集約し、ベンチ結果を自動可視化する。 |

## ビルド・実行例

Chrono C の include パスとライブラリパスを指定してください（makefile 使用を推奨）。

```bash
# ch01: 条件数サンプル（要: chrono-C-all ビルド済み）
gcc -I../chrono-C-all/include coupled/ch01_ratio_sweep.c -L../chrono-C-all/lib -lchrono_c -o ch01_ratio_sweep
./ch01_ratio_sweep > data/diagnostics/ch01_ratio_sweep.log

# ch02: ソフトネス掃引（実行で CSV を生成）
gcc -I../chrono-C-all/include coupled/ch02_softness.c -L../chrono-C-all/lib -lchrono_c -o ch02_softness
./ch02_softness

# ch03: Contact + Coupled 統合（ボディ定義を埋めてから実行）
gcc -I../chrono-C-all/include coupled/ch03_contact.c -L../chrono-C-all/lib -lchrono_c -o ch03_contact
./ch03_contact

# ch04: Endurance 可視化ヘルパ（ログ必須）
python coupled/ch04_endurance.py
```

演習コードは `docs/coupled_constraint_hands_on.md` の手順と Run ID テンプレを参照して進めます。Evidence を残す際は `docs/abc_team_chat_handoff.md` の Run ID テンプレ表に記載してください。
