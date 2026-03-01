# parser -> solver 操作手順 (FEM4C)

このドキュメントは、NastranBalkFile を手元に持っている前提で、
parser -> solver -> 出力 の一連操作をまとめたものです。

## 0. 前提
- 作業ディレクトリ: `/home/rmaen/highperformanceFEM/FEM4C`
- 解析は x-y 平面 (2D) を想定し、厚みは単位厚さ (1.0) を用います。
- parser は `FEM4C/parser/parser`（Windows は `parser.exe`）に配置されている想定です。

## 1. parser のビルド (未ビルドの場合)
```bash
cd /home/rmaen/highperformanceFEM/FEM4C/parser
gcc parser.c -o parser -lm
```

## 2. NastranBalkFile を parser にかける
parser の使い方:
```
parser <input_bdf> <out_root> [part_name] [--part=<name>] [--dofnames] [--dump] [--plane=xz|xy]
```

例:
```bash
cd /home/rmaen/highperformanceFEM/FEM4C
./parser/parser NastranBalkFile/2Dmesh.dat run_out part_0001
# Windows の場合
parser\\parser.exe NastranBalkFile\\2Dmesh.dat run_out part_0001
```

出力ディレクトリ構成:
```
run_out/part_0001/mesh/mesh.dat
run_out/part_0001/material/material.dat
run_out/part_0001/Boundary Conditions/boundary.dat
```

## 3. solver を実行
`mesh/mesh.dat` を含むディレクトリを指定します。
```bash
cd /home/rmaen/highperformanceFEM/FEM4C
./bin/fem4c run_out/part_0001
```

出力ファイル (カレントに生成):
- `output.dat`
- `output.vtk`
- `output.f06`

出力ファイル名を指定する場合:
```bash
./bin/fem4c run_out/part_0001 result.dat
```

## 4. parser一体実行（推奨）
NastranBalkFile を直接 `fem4c` に渡すと、parser → solver を連続実行します。
```bash
cd /home/rmaen/highperformanceFEM/FEM4C
./bin/fem4c NastranBalkFile/2Dmesh.dat run_out part_0001 output.dat
```

環境変数で出力先を指定する場合:
- `FEM4C_PARSE_OUTROOT`（出力ルート）
- `FEM4C_PARSE_PART`（パート名）

## 5. 解析上の注意点
- Z成分は読み込みますが、現状は2D解析のため Z方向の拘束や荷重は無視されます。
- 厚みは unit thickness (1.0) として扱われます。
- parser 出力の `material.dat` は N/mm^2 (E), kg/mm^3 (density) に正規化済みです。

## 6. トラブルシューティング
- `mesh/mesh.dat` が見つからない場合: parser の出力先指定を確認してください。
- NastranBalkFile が複数パートの場合: `part_name` または `--part=` で対象パートを指定してください。
