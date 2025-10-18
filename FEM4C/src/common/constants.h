#ifndef CONSTANTS_H
#define CONSTANTS_H

/* FEM4C - High Performance Finite Element Method in C
 * Based on "High Performance Finite Element Method" by Takahiro Yamada
 * Converted from Fortran to C
 */

/* Initial allocation sizes (grow dynamically as needed) */
#define INITIAL_NODE_CAPACITY        4096
#define INITIAL_ELEMENT_CAPACITY     4096
#define INITIAL_MATERIAL_CAPACITY    16

#define NODE_ID_BLOCK_SIZE           4096
#define ELEMENT_ID_BLOCK_SIZE        4096
#define MATERIAL_ID_BLOCK_SIZE       128

#define MAX_FILENAME_LEN    256
#define MAX_TITLE_LEN       80

/* Numerical constants */
#define TOLERANCE           1.0e-8
#define MAX_ITERATIONS      10000
#define PI                  3.14159265358979323846

/* Element type constants */
#define ELEMENT_T3          3   /* 3-node triangle */
#define ELEMENT_Q4          4   /* 4-node quadrilateral */
#define ELEMENT_T6          6   /* 6-node triangle */
#define ELEMENT_Q8          8   /* 8-node quadrilateral */
#define ELEMENT_Q9          9   /* 9-node quadrilateral */
#define ELEMENT_T4          10  /* 4-node tetrahedron */
#define ELEMENT_H8          20  /* 8-node hexahedron */
#define ELEMENT_T10         21  /* 10-node tetrahedron */

/* Material type constants */
#define MATERIAL_ISOTROPIC      1
#define MATERIAL_ORTHOTROPIC    2
#define MATERIAL_PLANE_STRESS   4
#define MATERIAL_PLANE_STRAIN   5

/* Dimensions */
#define MAX_NODES_PER_ELEMENT   10  /* Maximum nodes per element (T10) */
#define MAX_DOF_PER_NODE        3   /* Maximum DOF per node (3D) */
#define MAX_GAUSS_POINTS        27  /* Maximum Gauss points (3x3x3) */
#define MAX_SURFACE_NODES       3
#define MAX_TRACTION_SURFACES   20000

/* T6 element specific constants */
#define T6_NODES_PER_ELEMENT    6
#define T6_DOF_PER_NODE         2
#define T6_TOTAL_DOF            12
#define T6_GAUSS_POINTS         3

/* Mathematical constants */
#define ZERO    0.0
#define ONE     1.0
#define TWO     2.0
#define THREE   3.0
#define FOUR    4.0
#define HALF    0.5
#define THIRD   0.33333333333333333
#define SIXTH   0.16666666666666667

#endif /* CONSTANTS_H */
