/* FEM4C - Static Analysis Implementation
 * Linear static structural analysis
 */

#include "static.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include "../io/input.h"
#include "../io/output.h"
#include "../solver/assembly.h"
#include "../solver/cg_solver.h"
#include "../elements/t6/t6_stiffness.h"
#include "../elements/t3/t3_element.h"
#include "../elements/q4/q4_element.h"
#include "../elements/elements.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* Main static analysis function */
fem_error_t static_analysis(const char* input_filename, const char* output_filename)
{
    fem_error_t err;
    clock_t start_time, end_time;
    
    printf("FEM4C Static Analysis\n");
    printf("====================\n\n");
    
    start_time = clock();
    
    /* Initialize analysis */
    err = static_analysis_initialize();
    CHECK_ERROR(err);
    
    /* Preprocessing phase */
    err = static_analysis_preprocessing(input_filename);
    CHECK_ERROR(err);
    
    /* Solution phase */
    err = static_analysis_solve();
    CHECK_ERROR(err);
    
    /* Postprocessing phase */
    err = static_analysis_postprocessing(output_filename);
    CHECK_ERROR(err);
    
    /* Finalize analysis */
    err = static_analysis_finalize();
    CHECK_ERROR(err);
    
    end_time = clock();
    g_solver_info.elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\nStatic Analysis Complete\n");
    printf("========================\n");
    printf("Total elapsed time: %.3f seconds\n", g_solver_info.elapsed_time);
    
    return FEM_SUCCESS;
}

/* Initialize analysis */
fem_error_t static_analysis_initialize(void)
{
    fem_error_t err;
    
    printf("Phase 1: Initialization\n");
    printf("-----------------------\n");
    
    /* Initialize global variables */
    err = globals_initialize();
    CHECK_ERROR(err);
    
    /* Initialize element management system */
    err = elements_initialize();
    CHECK_ERROR(err);

    /* Legacy T6 initialization for compatibility */
    err = t6_initialize();
    CHECK_ERROR(err);
    
    printf("  System initialized successfully\n\n");
    return FEM_SUCCESS;
}

/* Preprocessing phase */
fem_error_t static_analysis_preprocessing(const char* input_filename)
{
    fem_error_t err;
    
    printf("Phase 2: Preprocessing\n");
    printf("----------------------\n");
    
    /* Read input data */
    printf("  Reading input file: %s\n", input_filename);
    err = input_read_data(input_filename);
    CHECK_ERROR(err);
    
    /* Validate input */
    err = static_validate_input();
    CHECK_ERROR(err);
    
    /* Print problem summary */
    printf("  Problem summary:\n");
    printf("    Title: %s\n", g_analysis.title);
    printf("    Nodes: %d\n", g_num_nodes);
    printf("    Elements: %d\n", g_num_elements);
    printf("    Materials: %d\n", g_num_materials);
    printf("    DOF: %d\n", g_total_dof);
    
    printf("  Preprocessing completed successfully\n\n");
    return FEM_SUCCESS;
}

/* Solution phase */
fem_error_t static_analysis_solve(void)
{
    fem_error_t err;
    
    printf("Phase 3: Solution\n");
    printf("-----------------\n");
    
    /* Assemble system */
    err = static_assemble_system();
    CHECK_ERROR(err);
    
    /* Solve equations */
    err = static_solve_equations();
    CHECK_ERROR(err);
    
    printf("  Solution phase completed successfully\n\n");
    return FEM_SUCCESS;
}

/* Postprocessing phase */
fem_error_t static_analysis_postprocessing(const char* output_filename)
{
    fem_error_t err;
    
    printf("Phase 4: Postprocessing\n");
    printf("-----------------------\n");
    
    /* Calculate element stresses */
    err = static_calculate_stresses();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Stress calculation failed, continuing...\n");
    }
    
    /* Write results */
    err = static_write_results(output_filename);
    CHECK_ERROR(err);
    
    /* Print solution summary */
    output_print_summary();
    
    printf("  Postprocessing completed successfully\n\n");
    return FEM_SUCCESS;
}

/* Finalize analysis */
fem_error_t static_analysis_finalize(void)
{
    fem_error_t err;
    
    err = globals_finalize();
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Assemble system matrices */
fem_error_t static_assemble_system(void)
{
    fem_error_t err;
    
    printf("  Assembling system matrices...\n");
    
    /* Assemble global stiffness matrix */
#ifdef _OPENMP
    err = assembly_parallel_stiffness_matrix();
#else
    err = assembly_global_stiffness_matrix();
#endif
    CHECK_ERROR(err);
    
    /* Assemble global force vector */
    err = assembly_global_force_vector();
    CHECK_ERROR(err);
    
    /* Apply boundary conditions */
    err = assembly_apply_boundary_conditions();
    CHECK_ERROR(err);
    
    /* Check matrix properties */
    err = assembly_check_matrix_properties();
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Solve system of equations */
fem_error_t static_solve_equations(void)
{
    fem_error_t err;
    
    printf("  Solving system of equations...\n");
    
    /* Solve using conjugate gradient method */
    err = cg_solve_system();
    CHECK_ERROR(err);
    
    /* Check equilibrium */
    err = static_check_equilibrium();
    if (err != FEM_SUCCESS) {
        printf("  Warning: Equilibrium check failed\n");
    }
    
    return FEM_SUCCESS;
}

/* Calculate element stresses */
fem_error_t static_calculate_stresses(void)
{
    fem_error_t err;
    int element_id;
    double stress[T6_STRESS_COMPONENTS];
    
    printf("  Calculating element stresses...\n");
    
    /* Calculate stresses for all elements */
    for (element_id = 0; element_id < g_num_elements; element_id++) {
        if (g_element_type[element_id] == ELEMENT_T6) {
            err = t6_calculate_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for element %d\n", element_id + 1);
                continue;
            }

            /* Store stresses (placeholder - would need global stress storage) */
            /* For now, just validate the calculation worked */
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            err = t3_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for T3 element %d\n", element_id + 1);
                continue;
            }
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            err = q4_element_stress(element_id, stress);
            if (err != FEM_SUCCESS) {
                printf("  Warning: Stress calculation failed for Q4 element %d\n", element_id + 1);
                continue;
            }
        }
    }
    
    return FEM_SUCCESS;
}

/* Write analysis results */
fem_error_t static_write_results(const char* output_filename)
{
    fem_error_t err;
    char vtk_filename[MAX_FILENAME_LEN];
    char csv_filename[MAX_FILENAME_LEN];
    
    printf("  Writing results to: %s\n", output_filename);

    /* Write CSV export (nodal displacements & element stresses) */
    strcpy(csv_filename, output_filename);
    char *dot_csv = strrchr(csv_filename, '.');
    if (dot_csv) {
        strcpy(dot_csv, ".csv");
    } else {
        strcat(csv_filename, ".csv");
    }
    err = output_export_csv(csv_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: CSV export failed (%s)\n", error_get_string(err));
    }
    
    /* Write standard results */
    err = output_write_results(output_filename);
    CHECK_ERROR(err);
    
    /* Create VTK filename */
    strcpy(vtk_filename, output_filename);
    char* dot = strrchr(vtk_filename, '.');
    if (dot) {
        strcpy(dot, ".vtk");
    } else {
        strcat(vtk_filename, ".vtk");
    }
    
    /* Write VTK results */
    printf("  Writing VTK results to: %s\n", vtk_filename);
    err = output_write_vtk_file(vtk_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: VTK output failed, continuing...\n");
    }

    /* Create F06 filename for Nastran-compatible output */
    char f06_filename[MAX_FILENAME_LEN];
    strcpy(f06_filename, output_filename);
    dot = strrchr(f06_filename, '.');
    if (dot) {
        strcpy(dot, ".f06");
    } else {
        strcat(f06_filename, ".f06");
    }

    /* Write F06 results */
    printf("  Writing Nastran F06 results to: %s\n", f06_filename);
    err = output_write_nastran_f06_file(f06_filename);
    if (err != FEM_SUCCESS) {
        printf("  Warning: F06 output failed, continuing...\n");
    }

    return FEM_SUCCESS;
}

/* Validate input data */
fem_error_t static_validate_input(void)
{
    int element_id;
    fem_error_t err;
    
    printf("  Validating input data...\n");
    
    /* Check problem size */
    if (g_num_nodes <= 0 || g_num_elements <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Invalid problem size");
    }
    
    /* Validate all elements */
    for (element_id = 0; element_id < g_num_elements; element_id++) {
        if (g_element_type[element_id] == ELEMENT_T6) {
            err = t6_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] t6_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT, 
                               "Element validation failed for element %d", element_id + 1);
            }
        } else if (g_element_type[element_id] == ELEMENT_T3) {
            err = t3_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] t3_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT,
                               "Element validation failed for element %d", element_id + 1);
            }
        } else if (g_element_type[element_id] == ELEMENT_Q4) {
            err = q4_validate_element(element_id);
            if (err != FEM_SUCCESS) {
                fprintf(stderr, "[validate] q4_validate_element error: %s\n", error_get_message());
                return error_set(FEM_ERROR_INVALID_INPUT,
                               "Element validation failed for element %d", element_id + 1);
            }
        } else {
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                           "Unsupported element type %d in element %d",
                           g_element_type[element_id], element_id + 1);
        }
    }
    
    /* Check material properties */
    for (int i = 0; i < g_num_materials; i++) {
        if (g_material_props[i][0] <= 0.0) {
            return error_set(FEM_ERROR_INVALID_MATERIAL, 
                           "Invalid Young's modulus for material %d", i + 1);
        }
        if (g_material_props[i][1] >= 0.5 || g_material_props[i][1] < -1.0) {
            return error_set(FEM_ERROR_INVALID_MATERIAL, 
                           "Invalid Poisson's ratio for material %d", i + 1);
        }
    }
    
    printf("    Input validation passed\n");
    return FEM_SUCCESS;
}

/* Check equilibrium after solution */
fem_error_t static_check_equilibrium(void)
{
    double max_residual = 0.0;
    double residual;

    if (g_total_dof <= 0) {
        return FEM_SUCCESS;
    }

    double *ku = malloc((size_t)g_total_dof * sizeof(double));
    CHECK_NULL(ku, "Residual workspace allocation failed");

    fem_error_t err = cg_matrix_vector_multiply(NULL, g_global_displ, ku, g_total_dof);
    if (err != FEM_SUCCESS) {
        free(ku);
        return err;
    }

    for (int i = 0; i < g_total_dof; i++) {
        residual = ku[i] - g_global_force[i];
        double abs_res = fabs(residual);
        if (abs_res > max_residual) {
            max_residual = abs_res;
        }
    }

    free(ku);

    printf("    Maximum residual: %e\n", max_residual);

    if (max_residual > 1.0e-6) {
        return error_set(FEM_ERROR_CONVERGENCE_FAILED, 
                        "Large equilibrium residual: %e", max_residual);
    }

    return FEM_SUCCESS;
}
