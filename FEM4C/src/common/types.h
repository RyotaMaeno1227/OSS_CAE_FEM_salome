#ifndef TYPES_H
#define TYPES_H

/* FEM4C - High Performance Finite Element Method in C
 * Data type definitions
 */

#include "constants.h"

/* Basic data types */
typedef struct {
    int id;
    double coords[3];        /* Node coordinates (x, y, z) */
    double displ[3];         /* Node displacements (u, v, w) */
    double force[3];         /* Node external forces (fx, fy, fz) */
    int bc_flags[3];         /* Boundary condition flags (0=free, 1=fixed) */
} node_t;

typedef struct {
    int id;
    int type;                /* Element type (T3, Q4, T6, etc.) */
    int nodes[MAX_NODES_PER_ELEMENT];  /* Node connectivity */
    int material_id;         /* Material identifier */
    int num_nodes;           /* Number of nodes for this element */
} element_t;

typedef struct {
    int id;
    int type;                /* Material type */
    double young_modulus;    /* Young's modulus */
    double poisson_ratio;    /* Poisson's ratio */
    double thickness;        /* Thickness (for 2D elements) */
    double density;          /* Material density */
} material_t;

/* Analysis control structure */
typedef struct {
    int analysis_type;       /* Analysis type (static, dynamic, etc.) */
    int num_nodes;           /* Total number of nodes */
    int num_elements;        /* Total number of elements */
    int num_materials;       /* Total number of materials */
    int spatial_dimension;   /* Problem spatial dimension */
    int max_iterations;      /* Maximum solver iterations */
    double tolerance;        /* Convergence tolerance */
    char title[MAX_TITLE_LEN]; /* Problem title */
} analysis_control_t;

/* Solver information structure */
typedef struct {
    int iterations;          /* Number of iterations performed */
    double residual;         /* Final residual norm */
    double elapsed_time;     /* Solution time */
    int status;              /* Solver status */
} solver_info_t;

/* Matrix storage structure (for sparse matrices) */
typedef struct {
    int size;                /* Matrix size */
    int nnz;                 /* Number of non-zero entries */
    double *values;          /* Non-zero values */
    int *row_ptr;            /* Row pointer array */
    int *col_ind;            /* Column index array */
} sparse_matrix_t;

/* Error codes enumeration */
typedef enum {
    FEM_SUCCESS = 0,
    FEM_ERROR_FILE_NOT_FOUND,
    FEM_ERROR_FILE_READ,
    FEM_ERROR_FILE_WRITE,
    FEM_ERROR_MEMORY_ALLOCATION,
    FEM_ERROR_INVALID_INPUT,
    FEM_ERROR_INVALID_ELEMENT_TYPE,
    FEM_ERROR_INVALID_MATERIAL,
    FEM_ERROR_INVALID_NODE,
    FEM_ERROR_CONVERGENCE_FAILED,
    FEM_ERROR_SINGULAR_MATRIX,
    FEM_ERROR_MAX_ITERATIONS,
    FEM_ERROR_UNKNOWN
} fem_error_t;

/* Element shape function structure */
typedef struct {
    double N[MAX_NODES_PER_ELEMENT];           /* Shape functions */
    double dN_dx[MAX_NODES_PER_ELEMENT];       /* Shape function derivatives w.r.t. x */
    double dN_dy[MAX_NODES_PER_ELEMENT];       /* Shape function derivatives w.r.t. y */
    double dN_dz[MAX_NODES_PER_ELEMENT];       /* Shape function derivatives w.r.t. z */
    double jacobian;                           /* Jacobian determinant */
} shape_functions_t;

/* Gauss point structure */
typedef struct {
    double xi, eta, zeta;    /* Natural coordinates */
    double weight;           /* Gauss weight */
} gauss_point_t;

#endif /* TYPES_H */
