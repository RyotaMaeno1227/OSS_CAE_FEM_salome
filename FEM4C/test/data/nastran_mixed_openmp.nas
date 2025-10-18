$ OpenMP Performance Test - Mixed T3/Q4 Elements
ID FEMAP,FEMAP
SOL 101
CEND
BEGIN BULK
$ Nodes for a larger mesh
GRID    1               0.0     0.0     0.0
GRID    2               1.0     0.0     0.0
GRID    3               2.0     0.0     0.0
GRID    4               0.0     1.0     0.0
GRID    5               1.0     1.0     0.0
GRID    6               2.0     1.0     0.0
GRID    7               0.0     2.0     0.0
GRID    8               1.0     2.0     0.0
GRID    9               2.0     2.0     0.0
GRID    10              0.5     0.5     0.0
GRID    11              1.5     0.5     0.0
GRID    12              0.5     1.5     0.0
GRID    13              1.5     1.5     0.0
$ T3 Elements (triangular mesh)
CTRIA3  1       1       1       2       10
CTRIA3  2       1       2       5       10
CTRIA3  3       1       10      5       4
CTRIA3  4       1       4       10      1
$ Q4 Elements (quadrilateral mesh)
CQUAD4  5       1       2       3       6       5
CQUAD4  6       1       5       6       9       8
CQUAD4  7       1       4       5       8       7
$ T6 Element (if available, 6-node triangle)
CTRIA6  8       1       7       8       9       12      13      12
$ Material
MAT1    1       2.1E5           0.3
$ Boundary Conditions - Fixed left edge
SPC     1       1       123     0.0
SPC     1       4       123     0.0
SPC     1       7       123     0.0
$ Loads - Distributed load on right edge
FORCE   1       3       0       1000.0  1.0     0.0     0.0
FORCE   1       6       0       1000.0  1.0     0.0     0.0
FORCE   1       9       0       1000.0  1.0     0.0     0.0
ENDDATA
