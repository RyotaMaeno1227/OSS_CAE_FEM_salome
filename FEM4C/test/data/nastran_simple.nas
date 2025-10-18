$ Simple Nastran Test - Single Triangle Element
ID FEMAP,FEMAP
SOL 101
CEND
BEGIN BULK
$ Nodes
GRID    1               0.0     0.0     0.0
GRID    2               1.0     0.0     0.0
GRID    3               0.5     1.0     0.0
$ Elements  
CTRIA3  1       1       1       2       3
$ Material
MAT1    1       2.1E5           0.3
$ Boundary Conditions
SPC     1       1       123     0.0
SPC     1       2       123     0.0
$ Loads
FORCE   1       3       0       1000.0  0.0     1.0     0.0
ENDDATA
