# Practice Sources

Chrono C 版 Hands-on で利用する演習コードは `practice/coupled/` に配置します。
`make practice-coupled` などのターゲットは任意のビルドシステムから追加してください。現状は各サンプルを単独でビルド・実行する想定です。

| ファイル | 目的 |
|----------|------|
| `coupled/ch01_ratio_sweep.c` | Ratio / ω を掃引し、Pivot と条件数を標準出力に記録する。 |
| `coupled/ch02_softness.c` | ソフトネス・バネパラメータを段階的に変更し、CSV を生成する。 |
| `coupled/ch03_contact.c` | Contact API と Coupled 拘束を同一島で組み合わせるミニシミュレーション。 |
| `coupled/ch04_endurance.py` | Endurance ログを集約し、ベンチ結果を自動可視化する。 |

> include パスは `-I../chrono-C-all/include`、リンク時は Chrono C ライブラリ（`-lchrono_c` など）を指定してください。演習コードは `docs/coupled_constraint_hands_on.md` の手順と Run ID テンプレを参照して進めます。
