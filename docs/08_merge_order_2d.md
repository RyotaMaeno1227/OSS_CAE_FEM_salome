# 2D merge order

最終更新: 2026-03-06

## Phase 1: design freeze
- PM-01
- PM-02
- PM-03
- PM-04
- PM-05
- PM-06

## Phase 2: build and rigid foundation
- C-01
- A-01
- A-02
- A-03
- B-01
- B-02
- B-03
- B-04
- E-01
- E-03

## Phase 3: rigid integrators
- A-04
- A-05
- A-06
- A-07
- B-05
- B-06
- B-07
- B-08
- B-09

## Phase 4: flexible FEM API
- C-02
- C-03
- C-04
- C-05
- C-06
- C-07
- C-08
- C-09
- D-01
- D-02
- D-03
- D-04

## Phase 5: coupled execution
- D-05
- D-06
- D-07
- D-08
- D-09
- E-02
- E-04
- E-05
- E-06
- E-07

## Phase 6: compare and acceptance
- E-08
- E-09

## 同時マージ可
- PM-01 〜 PM-06 は相互に同時進行可。
- A-01/A-03 と B-01/B-02/B-03 はファイル衝突が少ないため並行可。
- C-02/C-03/C-04/C-05 は Cチーム内で順序依存が強い。
- D系は C-02/C-03/C-04/C-07 完了後のマージを原則とする。
- E-04/E-05/E-06 は A/B/C/D の基礎実装が揃ってから進める。

## 待ち条件
- C-01 完了前に本格マージしない。
- D-05 以降は C-03/C-04/C-05 と D-01/D-02/D-03/D-04 が揃ってから進める。
- E-08/E-09 は PM-06 の schema 固定後に進める。
