# FEM4C - High Performance Finite Element Method in C
# Makefile for building the FEM solver (+ parser)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -mcmodel=large
OPENMP_FLAGS = -fopenmp
DEBUG_FLAGS = -g -DDEBUG
INCLUDES = -Isrc/common -Isrc/elements -Isrc/solver -Isrc/io -Isrc/mesh -Isrc/material -Isrc/analysis -Isrc/mbd -Isrc/coupled

# Directories
SRCDIR = src
BUILDDIR = build
TESTDIR = test
BINDIR = bin
PARSERDIR = parser

ifeq ($(OS),Windows_NT)
PARSER_EXE_SUFFIX = .exe
else
PARSER_EXE_SUFFIX =
endif

# Create directories if they don't exist
$(shell mkdir -p $(BUILDDIR) $(BINDIR) $(PARSERDIR))

# Source files
COMMON_SRCS = $(SRCDIR)/common/globals.c $(SRCDIR)/common/error.c
IO_SRCS = $(SRCDIR)/io/input.c $(SRCDIR)/io/output.c
MESH_SRCS =
MATERIAL_SRCS =

# B-plan logical source groups keep current paths while separating future
# domain and numerics ownership. Physical moves will update paths later.
DOMAIN_FEM_ELEMENT_SRCS = $(SRCDIR)/elements/element_base.c $(SRCDIR)/elements/elements.c \
                          $(SRCDIR)/elements/t6/t6_element.c $(SRCDIR)/elements/t6/t6_stiffness.c \
                          $(SRCDIR)/elements/q4/q4_element.c $(SRCDIR)/elements/q4/q4_stiffness.c \
                          $(SRCDIR)/elements/t3/t3_element.c
DOMAIN_FEM_ASSEMBLY_SRCS = $(SRCDIR)/solver/assembly.c
DOMAIN_FEM_SRCS = $(DOMAIN_FEM_ELEMENT_SRCS) $(DOMAIN_FEM_ASSEMBLY_SRCS)

NUMERICS_CG_SRCS = $(SRCDIR)/numerics/cg/cg_solver.c
NUMERICS_DENSE_SRCS = $(SRCDIR)/numerics/dense/linear_solver_dense.c

DOMAIN_MBD_CORE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/forces2d.c \
                       $(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c \
                       $(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/mbd/kinematics2d.c \
                       $(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c
DOMAIN_MBD_RUNTIME_SRCS = $(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/projection2d.c $(SRCDIR)/mbd/system2d.c
DOMAIN_MBD_SRCS = $(DOMAIN_MBD_CORE_SRCS) $(DOMAIN_MBD_RUNTIME_SRCS)

DOMAIN_COUPLED_RUN_SRCS = $(SRCDIR)/coupled/case2d.c $(SRCDIR)/coupled/coupled_run2d.c \
                          $(SRCDIR)/coupled/coupled_run2d_config.c \
                          $(SRCDIR)/coupled/coupled_run2d_flex_case_validator.c \
                          $(SRCDIR)/coupled/coupled_run2d_flex_model_loader.c \
                          $(SRCDIR)/coupled/coupled_run2d_lifecycle.c \
                          $(SRCDIR)/coupled/coupled_run2d_master_input_loader.c \
                          $(SRCDIR)/coupled/coupled_run2d_route_descriptors.c \
                          $(SRCDIR)/coupled/coupled_run2d_step_dispatch.c \
                          $(SRCDIR)/coupled/coupled_run2d_step_history_capture.c \
                          $(SRCDIR)/coupled/coupled_run2d_step_snapshot_writer.c \
                          $(SRCDIR)/coupled/coupled_run2d_summary_writer.c \
                          $(SRCDIR)/coupled/coupled_run2d_writer_meta.c
DOMAIN_COUPLED_STEP_CORE_SRCS = $(SRCDIR)/coupled/coupled_step_history2d.c \
                                $(SRCDIR)/coupled/coupled_monolithic_assembly2d.c \
                                $(SRCDIR)/coupled/coupled_monolithic_state2d.c \
                                $(SRCDIR)/coupled/coupled_step_common2d.c \
                                $(SRCDIR)/coupled/coupled_step_delayed_cosim2d.c \
                                $(SRCDIR)/coupled/coupled_step_explicit2d.c \
                                $(SRCDIR)/coupled/coupled_step_monolithic2d.c \
                                $(SRCDIR)/coupled/coupled_step_oneway2d.c
DOMAIN_CONTACT_SRCS = $(SRCDIR)/coupled/contact_patch_load2d.c $(SRCDIR)/domain/contact/contact_patch2d.c
DOMAIN_COUPLED_STEP_TAIL_SRCS = $(SRCDIR)/coupled/coupled_step_implicit2d.c
DOMAIN_COUPLED_FLEX_SRCS = $(SRCDIR)/coupled/fem_model_copy.c \
                            $(SRCDIR)/coupled/flex_bc2d.c $(SRCDIR)/coupled/flex_body2d.c \
                            $(SRCDIR)/coupled/flex_nodeset.c \
                            $(SRCDIR)/coupled/flex_reaction2d.c $(SRCDIR)/coupled/flex_snapshot2d.c \
                            $(SRCDIR)/coupled/flex_solver2d.c
DOMAIN_COUPLED_SRCS = $(DOMAIN_COUPLED_RUN_SRCS) $(DOMAIN_COUPLED_STEP_CORE_SRCS) \
                      $(DOMAIN_CONTACT_SRCS) $(DOMAIN_COUPLED_STEP_TAIL_SRCS) \
                      $(DOMAIN_COUPLED_FLEX_SRCS)

ELEMENT_SRCS = $(DOMAIN_FEM_ELEMENT_SRCS)
SOLVER_SRCS = $(DOMAIN_FEM_ASSEMBLY_SRCS) $(NUMERICS_CG_SRCS)
ANALYSIS_SRCS = $(SRCDIR)/analysis/static.c $(SRCDIR)/analysis/runner.c
MBD_SRCS = $(DOMAIN_MBD_CORE_SRCS) $(NUMERICS_DENSE_SRCS) $(DOMAIN_MBD_RUNTIME_SRCS)
COUPLED_SRCS = $(DOMAIN_COUPLED_SRCS)
MAIN_SRCS = $(SRCDIR)/fem4c.c

# All source files
SRCS = $(COMMON_SRCS) $(IO_SRCS) $(MESH_SRCS) $(MATERIAL_SRCS) $(ELEMENT_SRCS) $(SOLVER_SRCS) $(ANALYSIS_SRCS) $(MBD_SRCS) $(COUPLED_SRCS) $(MAIN_SRCS)

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/fem4c
MBD_PROBE_SRC = practice/ch09/mbd_constraint_probe.c
MBD_PROBE_TARGET = $(BINDIR)/mbd_constraint_probe
# constraint2d.c uses kinematics helpers for world-anchor residual/Jacobian evaluation.
MBD_PROBE_SRCS = $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/common/error.c
MBD_NEWMARK2D_PROBE_SRC = practice/ch09/mbd_newmark2d_probe.c
MBD_NEWMARK2D_PROBE_TARGET = $(BINDIR)/mbd_newmark2d_probe
MBD_NEWMARK2D_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/common/error.c
MBD_HHT2D_PROBE_SRC = practice/ch09/mbd_hht2d_probe.c
MBD_HHT2D_PROBE_TARGET = $(BINDIR)/mbd_hht2d_probe
MBD_HHT2D_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/common/error.c
MBD_HHT2D_INVALID_PROBE_SRC = practice/ch09/mbd_hht2d_invalid_probe.c
MBD_HHT2D_INVALID_PROBE_TARGET = $(BINDIR)/mbd_hht2d_invalid_probe
MBD_HHT2D_INVALID_PROBE_SRCS = $(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/common/error.c
MBD_2D_FOUNDATION_PROBE_SRCS = $(SRCDIR)/common/error.c \
	$(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/forces2d.c \
	$(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c
MBD_SYSTEM2D_CLI_SRC = practice/ch09/mbd_system2d_cli_main.c
MBD_SYSTEM2D_CLI_TARGET = $(BINDIR)/mbd_system2d_cli
MBD_SYSTEM2D_CLI_SRCS = $(SRCDIR)/common/error.c $(SRCDIR)/common/globals.c $(SRCDIR)/io/input.c \
	$(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/forces2d.c \
	$(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c \
	$(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/mbd/kkt2d.c \
	$(SRCDIR)/mbd/assembler2d.c $(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/output2d.c \
	$(SRCDIR)/mbd/projection2d.c \
	$(SRCDIR)/mbd/system2d.c $(SRCDIR)/coupled/case2d.c $(SRCDIR)/coupled/flex_nodeset.c
MBD_SYSTEM2D_EXPLICIT_PROBE_SRC = practice/ch09/mbd_system2d_explicit_probe.c
MBD_SYSTEM2D_EXPLICIT_PROBE_TARGET = $(BINDIR)/mbd_system2d_explicit_probe
MBD_SYSTEM2D_EXPLICIT_PROBE_SRCS = $(MBD_SYSTEM2D_CLI_SRCS)
MBD_SYSTEM2D_NEWMARK_PROBE_SRC = practice/ch09/mbd_system2d_newmark_probe.c
MBD_SYSTEM2D_NEWMARK_PROBE_TARGET = $(BINDIR)/mbd_system2d_newmark_probe
MBD_SYSTEM2D_NEWMARK_PROBE_SRCS = $(MBD_SYSTEM2D_CLI_SRCS)
MBD_SYSTEM2D_HHT_PROBE_SRC = practice/ch09/mbd_system2d_hht_probe.c
MBD_SYSTEM2D_HHT_PROBE_TARGET = $(BINDIR)/mbd_system2d_hht_probe
MBD_SYSTEM2D_HHT_PROBE_SRCS = $(MBD_SYSTEM2D_CLI_SRCS)
MBD_ASSEMBLER2D_PROBE_SRC = practice/ch09/mbd_assembler2d_probe.c
MBD_ASSEMBLER2D_PROBE_TARGET = $(BINDIR)/mbd_assembler2d_probe
MBD_ASSEMBLER2D_PROBE_SRCS = $(MBD_2D_FOUNDATION_PROBE_SRCS)
MBD_KINEMATICS2D_PROBE_SRC = practice/ch09/mbd_kinematics2d_probe.c
MBD_KINEMATICS2D_PROBE_TARGET = $(BINDIR)/mbd_kinematics2d_probe
MBD_KINEMATICS2D_PROBE_SRCS = $(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/common/error.c
MBD_EXPLICIT2D_PROBE_SRC = practice/ch09/mbd_explicit2d_probe.c
MBD_EXPLICIT2D_PROBE_TARGET = $(BINDIR)/mbd_explicit2d_probe
MBD_EXPLICIT2D_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/common/error.c
MBD_BODY2D_REFERENCE_PROBE_SRC = practice/ch09/mbd_body2d_reference_probe.c
MBD_BODY2D_REFERENCE_PROBE_TARGET = $(BINDIR)/mbd_body2d_reference_probe
MBD_BODY2D_REFERENCE_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/common/error.c
MBD_CONSTRAINT_RHS_PROBE_SRC = practice/ch09/mbd_constraint_rhs_probe.c
MBD_CONSTRAINT_RHS_PROBE_TARGET = $(BINDIR)/mbd_constraint_rhs_probe
MBD_CONSTRAINT_RHS_PROBE_SRCS = $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/common/error.c
MBD_OUTPUT2D_PROBE_SRC = practice/ch09/mbd_output2d_probe.c
MBD_OUTPUT2D_PROBE_TARGET = $(BINDIR)/mbd_output2d_probe
MBD_OUTPUT2D_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c \
	$(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/system2d.c $(SRCDIR)/mbd/forces2d.c \
	$(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c \
	$(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/mbd/kinematics2d.c \
	$(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/projection2d.c $(SRCDIR)/common/globals.c \
	$(SRCDIR)/common/error.c $(SRCDIR)/io/input.c $(SRCDIR)/coupled/case2d.c \
	$(SRCDIR)/coupled/flex_nodeset.c
MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRC = practice/ch09/mbd_output2d_rigid_compare_probe.c
MBD_OUTPUT2D_RIGID_COMPARE_PROBE_TARGET = $(BINDIR)/mbd_output2d_rigid_compare_probe
MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRCS = $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c \
	$(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/system2d.c $(SRCDIR)/mbd/forces2d.c \
	$(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c \
	$(SRCDIR)/domain/mbd/integrator_hht2d.c $(SRCDIR)/mbd/kinematics2d.c \
	$(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/projection2d.c $(SRCDIR)/common/globals.c \
	$(SRCDIR)/common/error.c $(SRCDIR)/io/input.c $(SRCDIR)/coupled/case2d.c \
	$(SRCDIR)/coupled/flex_nodeset.c
MBD_SYSTEM2D_HISTORY_PROBE_SRC = practice/ch09/mbd_system2d_history_probe.c
MBD_SYSTEM2D_HISTORY_PROBE_TARGET = $(BINDIR)/mbd_system2d_history_probe
MBD_SYSTEM2D_HISTORY_PROBE_SRCS = $(SRCDIR)/common/globals.c $(SRCDIR)/common/error.c $(SRCDIR)/io/input.c \
	$(SRCDIR)/coupled/case2d.c $(SRCDIR)/coupled/flex_nodeset.c $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c \
	$(SRCDIR)/mbd/forces2d.c $(SRCDIR)/domain/mbd/integrator_explicit2d.c \
	$(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/domain/mbd/integrator_hht2d.c \
	$(SRCDIR)/mbd/kinematics2d.c \
	$(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/projection2d.c \
	$(SRCDIR)/mbd/system2d.c
MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRC = practice/ch09/mbd_system2d_rigid_compare_probe.c
MBD_SYSTEM2D_RIGID_COMPARE_PROBE_TARGET = $(BINDIR)/mbd_system2d_rigid_compare_probe
MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRC = practice/ch09/mbd_system2d_coupled_geometry_compare_probe.c
MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_TARGET = $(BINDIR)/mbd_system2d_coupled_geometry_compare_probe
MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_FLEXIBLE_FORCE_PROBE_SRC = practice/ch09/mbd_flexible_force_probe.c
MBD_FLEXIBLE_FORCE_PROBE_TARGET = $(BINDIR)/mbd_flexible_force_probe
MBD_FLEXIBLE_FORCE_PROBE_SRCS = $(SRCDIR)/common/globals.c $(SRCDIR)/common/error.c $(SRCDIR)/io/input.c \
	$(SRCDIR)/coupled/case2d.c $(SRCDIR)/coupled/flex_nodeset.c $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c \
	$(SRCDIR)/mbd/forces2d.c $(SRCDIR)/domain/mbd/integrator_explicit2d.c \
	$(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/domain/mbd/integrator_hht2d.c \
	$(SRCDIR)/mbd/kinematics2d.c \
	$(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/projection2d.c \
	$(SRCDIR)/mbd/system2d.c
MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRC = practice/ch09/mbd_forces2d_hht_effective_probe.c
MBD_FORCES2D_HHT_EFFECTIVE_PROBE_TARGET = $(BINDIR)/mbd_forces2d_hht_effective_probe
MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRCS = $(MBD_FLEXIBLE_FORCE_PROBE_SRCS)
MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRC = practice/ch09/mbd_system2d_force_baseline_probe.c
MBD_SYSTEM2D_FORCE_BASELINE_PROBE_TARGET = $(BINDIR)/mbd_system2d_force_baseline_probe
MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRCS = $(MBD_FLEXIBLE_FORCE_PROBE_SRCS)
MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRC = practice/ch09/mbd_system2d_force_history_probe.c
MBD_SYSTEM2D_FORCE_HISTORY_PROBE_TARGET = $(BINDIR)/mbd_system2d_force_history_probe
MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRCS = $(MBD_FLEXIBLE_FORCE_PROBE_SRCS)
MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRC = practice/ch09/mbd_system2d_newmark_history_output_probe.c
MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_TARGET = $(BINDIR)/mbd_system2d_newmark_history_output_probe
MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRC = practice/ch09/mbd_system2d_hht_history_output_probe.c
MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_TARGET = $(BINDIR)/mbd_system2d_hht_history_output_probe
MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRC = practice/ch09/mbd_system2d_constrained_history_output_probe.c
MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET = $(BINDIR)/mbd_system2d_constrained_history_output_probe
MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRC = practice/ch09/mbd_system2d_console_history_probe.c
MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET = $(BINDIR)/mbd_system2d_console_history_probe
MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_PROJECTION2D_PROBE_SRC = practice/ch09/mbd_projection2d_probe.c
MBD_PROJECTION2D_PROBE_TARGET = $(BINDIR)/mbd_projection2d_probe
MBD_PROJECTION2D_PROBE_SRCS = $(SRCDIR)/common/error.c $(SRCDIR)/common/globals.c $(SRCDIR)/io/input.c \
	$(SRCDIR)/coupled/case2d.c $(SRCDIR)/coupled/flex_nodeset.c $(SRCDIR)/mbd/body2d.c $(SRCDIR)/mbd/constraint2d.c \
	$(SRCDIR)/domain/mbd/integrator_explicit2d.c $(SRCDIR)/domain/mbd/integrator_newmark2d.c $(SRCDIR)/domain/mbd/integrator_hht2d.c \
	$(SRCDIR)/mbd/forces2d.c $(SRCDIR)/mbd/kinematics2d.c $(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/mbd/assembler2d.c \
	$(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/mbd/output2d.c $(SRCDIR)/mbd/system2d.c $(SRCDIR)/mbd/projection2d.c
MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRC = practice/ch09/mbd_system2d_projection_compare_probe.c
MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET = $(BINDIR)/mbd_system2d_projection_compare_probe
MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRCS = $(MBD_PROJECTION2D_PROBE_SRCS)
MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRC = practice/ch09/mbd_system2d_projection_history_output_probe.c
MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET = $(BINDIR)/mbd_system2d_projection_history_output_probe
MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRCS = $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
MBD_DENSE_SOLVER_PROBE_SRC = practice/ch09/mbd_dense_solver_probe.c
MBD_DENSE_SOLVER_PROBE_TARGET = $(BINDIR)/mbd_dense_solver_probe
MBD_DENSE_SOLVER_PROBE_SRCS = $(MBD_2D_FOUNDATION_PROBE_SRCS)
MBD_DENSE_SOLVER_SINGULAR_PROBE_SRC = practice/ch09/mbd_dense_solver_singular_probe.c
MBD_DENSE_SOLVER_SINGULAR_PROBE_TARGET = $(BINDIR)/mbd_dense_solver_singular_probe
MBD_DENSE_SOLVER_SINGULAR_PROBE_SRCS = $(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/common/error.c
MBD_DENSE_SOLVER_INVALID_PROBE_SRC = practice/ch09/mbd_dense_solver_invalid_probe.c
MBD_DENSE_SOLVER_INVALID_PROBE_TARGET = $(BINDIR)/mbd_dense_solver_invalid_probe
MBD_DENSE_SOLVER_INVALID_PROBE_SRCS = $(SRCDIR)/numerics/dense/linear_solver_dense.c $(SRCDIR)/common/error.c
MBD_REGRESSION_SCRIPT = scripts/run_mbd_regression.sh
MBD_M1_RIGID_ACCEPTANCE_TEST_SCRIPT = scripts/test_make_mbd_m1_rigid_acceptance.sh
MBD_CONSISTENCY_SCRIPT = scripts/check_mbd_equation_consistency.sh
MBD_NEGATIVE_SCRIPT = scripts/check_mbd_invalid_inputs.sh
MBD_RUN1_SURFACE_DOCS_SYNC_TEST_SCRIPT = scripts/test_check_mbd_run1_surface_docs_sync.sh
MBD_RUN1_SURFACE_DOCS_SYNC_SURFACES_TEST_SCRIPT = scripts/test_check_mbd_run1_surface_docs_sync_surfaces.sh
MBD_RUN1_SURFACE_DOCS_SYNC_SURFACES_HELP_TEST_SCRIPT = scripts/test_make_mbd_run1_surface_docs_sync_surfaces_help.sh
MBD_RUN1_SURFACE_DOCS_SYNC_SURFACE_SMOKE_TEST_SCRIPT = scripts/test_make_mbd_run1_surface_docs_sync_surface_smoke.sh
MBD_CI_EVIDENCE_SCRIPT = scripts/fetch_fem4c_ci_evidence.py
MBD_CI_CONTRACT_SCRIPT = scripts/check_ci_contract.sh
MBD_CI_CONTRACT_TEST_SCRIPT = scripts/test_check_ci_contract.sh
MBD_B14_REGRESSION_SCRIPT = scripts/run_b14_regression.sh
MBD_B8_REGRESSION_SCRIPT = scripts/run_b8_regression.sh
MBD_B8_REGRESSION_TEST_SCRIPT = scripts/test_run_b8_regression.sh
MBD_B8_REGRESSION_FULL_SCRIPT = scripts/run_b8_regression_full.sh
MBD_B8_REGRESSION_FULL_TEST_SCRIPT = scripts/test_run_b8_regression_full.sh
MBD_B8_KNOB_MATRIX_TEST_SCRIPT = scripts/test_b8_knob_matrix.sh
MBD_A21_REGRESSION_SCRIPT = scripts/run_a21_regression.sh
MBD_A21_REGRESSION_TEST_SCRIPT = scripts/test_run_a21_regression.sh
MBD_A24_REGRESSION_SCRIPT = scripts/run_a24_regression.sh
MBD_A24_REGRESSION_TEST_SCRIPT = scripts/test_run_a24_regression.sh
MBD_A24_REGRESSION_FULL_SCRIPT = scripts/run_a24_regression_full.sh
MBD_A24_REGRESSION_FULL_TEST_SCRIPT = scripts/test_run_a24_regression_full.sh
MBD_A24_BATCH_SCRIPT = scripts/run_a24_batch.sh
MBD_A24_BATCH_TEST_SCRIPT = scripts/test_run_a24_batch.sh
MBD_A24_ACCEPTANCE_SERIAL_SCRIPT = scripts/run_a24_acceptance_serial.sh
MBD_A24_ACCEPTANCE_SERIAL_TEST_SCRIPT = scripts/test_run_a24_acceptance_serial.sh
T3_ORIENTATION_SCRIPT = scripts/check_t3_orientation_modes.sh
PARSER_COMPAT_SCRIPT = scripts/check_parser_compatibility.sh
COUPLED_STUB_SCRIPT = scripts/check_coupled_stub_contract.sh
COUPLED_RUN1_SURFACE_DOCS_SYNC_TEST_SCRIPT = scripts/test_check_coupled_run1_surface_docs_sync.sh
COUPLED_EXAMPLE_SCRIPT = scripts/check_coupled_2link_examples.sh
COUPLED_RIGID_LIMIT_MANIFEST_TEST_SCRIPT = scripts/test_compare_rigid_limit_manifest.sh
COUPLED_FLEX_MANIFEST_TEST_SCRIPT = scripts/test_compare_2link_flex_manifest.sh
COUPLED_FLEX_REFERENCE_REAL_TEST_SCRIPT = scripts/test_compare_2link_flex_reference_real.sh
COUPLED_FLEX_REFERENCE_COMPARE_TEST_SCRIPT = scripts/test_compare_2link_flex_reference_compare_mode.sh
COUPLED_FLEX_REFERENCE_ARTIFACT_ONLY_TEST_SCRIPT = scripts/test_compare_2link_flex_reference_artifact_only.sh
MBD_RIGID_ANALYTIC_REAL_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_real_normalize.sh
MBD_RIGID_ANALYTIC_COMPARE_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_real.sh
MBD_RIGID_ANALYTIC_HHT_COMPARE_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_hht.sh
MBD_RIGID_ANALYTIC_INTEGRATOR_COMPARE_READY_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_integrator_compare_ready.sh
MBD_RIGID_ANALYTIC_FALLBACK_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_fallback.sh
MBD_RIGID_ANALYTIC_EXPECT_ROUTE_GUARD_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_expect_route_guard.sh
MBD_RIGID_ANALYTIC_EXPECT_FALLBACK_REASON_GUARD_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_expect_fallback_reason_guard.sh
MBD_RIGID_ANALYTIC_EXPECT_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_expect_snapshot_count_guard.sh
MBD_RIGID_ANALYTIC_MULTI_REFERENCE_TEST_SCRIPT = scripts/test_compare_2link_rigid_analytic_multi_reference.sh
MBD_RIGID_ANALYTIC_WRAPPER_TEST_SCRIPT = scripts/test_run_e08_rigid_analytic_wrappers.sh
COMPARE_2LINK_ARTIFACT_CHECK_SCRIPT = scripts/check_compare_2link_artifacts.sh
COMPARE_2LINK_ARTIFACT_CHECK_TEST_SCRIPT = scripts/test_check_compare_2link_artifacts.sh
MBD_SYSTEM2D_CLI_READY_CONTRACT_TEST_SCRIPT = scripts/test_make_mbd_system2d_cli_ready_contract.sh
MBD_B_TEAM_FOUNDATION_ISOLATED_TEST_SCRIPT = scripts/test_make_mbd_b_team_foundation_isolated.sh
MBD_SYSTEM2D_PROJECTION_LONG_COMPARE_ISOLATED_TEST_SCRIPT = scripts/test_make_mbd_system2d_projection_long_compare_isolated.sh
MBD_SYSTEM2D_PROJECTION_LONG_HISTORY_ISOLATED_TEST_SCRIPT = scripts/test_make_mbd_system2d_projection_long_history_isolated.sh
MBD_OUTPUT2D_HISTORY_FIELD_COUNT_SYNC_TEST_SCRIPT = scripts/test_mbd_output2d_history_field_count_sync.sh
MBD_OUTPUT2D_RIGID_COMPARE_HEADER_SINGLE_SOURCE_TEST_SCRIPT = scripts/test_mbd_output2d_rigid_compare_header_single_source.sh
MBD_OUTPUT2D_RIGID_COMPARE_FIELD_COUNT_SYNC_TEST_SCRIPT = scripts/test_mbd_output2d_rigid_compare_field_count_sync.sh
MBD_RIGID_REVIEW_COLUMNS_SYNC_TEST_SCRIPT = scripts/test_compare_2link_rigid_review_columns_sync.sh
COMPARE_2LINK_ARTIFACT_ROUTE_FIELDS_SYNC_TEST_SCRIPT = scripts/test_compare_2link_artifact_route_fields_sync.sh
COMPARE_2LINK_ARTIFACT_ROUTE_FIELDS_GETTER_TEST_SCRIPT = scripts/test_get_compare_2link_artifact_route_fields.sh
COMPARE_2LINK_ARTIFACT_TARGETS_GETTER_TEST_SCRIPT = scripts/test_get_compare_2link_artifact_targets.sh
COMPARE_2LINK_ARTIFACT_TARGETS_SYNC_TEST_SCRIPT = scripts/test_compare_2link_artifact_targets_sync.sh
COMPARE_2LINK_ARTIFACT_INTEGRATORS_GETTER_TEST_SCRIPT = scripts/test_get_compare_2link_artifact_integrators.sh
COMPARE_2LINK_ARTIFACT_INTEGRATORS_SYNC_TEST_SCRIPT = scripts/test_compare_2link_artifact_integrators_sync.sh
COMPARE_2LINK_ARTIFACT_MANIFEST_TEST_SCRIPT = scripts/check_compare_2link_artifact_manifest.py
COMPARE_2LINK_ARTIFACT_MANIFEST_HEADER_CONTRACT_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_manifest_header_contract.sh
COMPARE_2LINK_ARTIFACT_MANIFEST_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_manifest_snapshot_count_guard.sh
COMPARE_2LINK_ARTIFACT_CHECK_VARS_TEST_SCRIPT = scripts/test_make_compare_2link_artifact_check_vars.sh
COMPARE_2LINK_ARTIFACT_CHECK_INTEGRATOR_TEST_SCRIPT = scripts/test_make_compare_2link_artifact_check_integrator.sh
COMPARE_2LINK_ARTIFACT_CHECK_INVALID_INTEGRATOR_TEST_SCRIPT = scripts/test_check_compare_2link_artifacts_invalid_integrator.sh
COMPARE_2LINK_ARTIFACT_MATRIX_CHECK_SCRIPT = scripts/check_compare_2link_artifact_matrix.sh
COMPARE_2LINK_ARTIFACT_MATRIX_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_matrix.sh
COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_TEST_SCRIPT = scripts/check_compare_2link_artifact_matrix_manifest.py
COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_HEADER_CONTRACT_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_matrix_manifest_header_contract.sh
COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_matrix_manifest_snapshot_count_guard.sh
COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_EXPECTED_INTEGRATORS_TEST_SCRIPT = scripts/test_make_compare_2link_artifact_matrix_manifest_expected_integrators.sh
COMPARE_2LINK_ARTIFACT_MATRIX_INTEGRATORS_TEST_SCRIPT = scripts/test_make_compare_2link_artifact_matrix_integrators.sh
COMPARE_2LINK_ARTIFACT_MATRIX_INVALID_INTEGRATOR_TEST_SCRIPT = scripts/test_check_compare_2link_artifact_matrix_invalid_integrator.sh
B8_GUARD_SCRIPT = scripts/run_b8_guard.sh
B8_GUARD_TEST_SCRIPT = scripts/test_run_b8_guard.sh
B8_GUARD_CONTRACT_SCRIPT = scripts/run_b8_guard_contract.sh
B8_GUARD_CONTRACT_TEST_SCRIPT = scripts/test_run_b8_guard_contract.sh
B8_SYNTAX_CHECK_SCRIPT = scripts/check_b8_scripts_syntax.sh
B8_OUTPUT_TEST_SCRIPT = scripts/test_check_b8_guard_output.sh
COUPLED_INTEGRATOR_SCRIPT = scripts/check_coupled_integrators.sh
DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT = /tmp/fem4c_delayed_cosim_multistep.dat
DELAYED_COSIM_MULTISTEP_SMOKE_LOG = /tmp/fem4c_delayed_cosim_multistep.log
YEAR1_COMPARE_SCHEMA_SCRIPT = scripts/check_year1_compare_schema.sh
COUPLED_RIGID_LIMIT_COMPARE_TEST_SCRIPT = scripts/test_compare_rigid_limit_2link.sh
COUPLED_RIGID_LIMIT_IMPLICIT_TEST_SCRIPT = scripts/test_coupled_rigid_limit_implicit_graceful.sh
COUPLED_RIGID_LIMIT_IMPLICIT_COMPARE_TEST_SCRIPT = scripts/test_compare_rigid_limit_implicit_metrics.sh
COUPLED_RIGID_LIMIT_THRESHOLD_PRINT_SCRIPT = scripts/print_rigid_limit_thresholds.sh
COUPLED_RIGID_LIMIT_THRESHOLD_PRINT_TEST_SCRIPT = scripts/test_print_rigid_limit_thresholds.sh
COUPLED_RIGID_LIMIT_THRESHOLD_DOC_SYNC_TEST_SCRIPT = scripts/test_check_rigid_limit_threshold_docs_sync.sh
COUPLED_COMPARE_CHECKS_SCRIPT = scripts/run_coupled_compare_checks.sh
COUPLED_COMPARE_CHECKS_TEST_SCRIPT = scripts/test_run_coupled_compare_checks.sh
COUPLED_COMPARE_CHECKS_ARTIFACT_MANIFEST_TEST_SCRIPT = scripts/test_run_coupled_compare_checks_artifact_manifest.sh
COUPLED_COMPARE_CHECKS_THRESHOLD_PROVENANCE_TEST_SCRIPT = scripts/test_run_coupled_compare_checks_threshold_provenance.sh
COUPLED_COMPARE_CHECKS_OUT_DIR_TEST_SCRIPT = scripts/test_make_coupled_compare_checks_out_dir.sh
COUPLED_COMPARE_CHECKS_MANIFEST_TEST_SCRIPT = scripts/check_coupled_compare_checks_manifest.py
COUPLED_COMPARE_CHECKS_MANIFEST_OVERRIDE_TEST_SCRIPT = scripts/test_make_coupled_compare_checks_manifest_override.sh
COUPLED_COMPARE_CHECKS_SUBSET_TEST_SCRIPT = scripts/test_make_coupled_compare_checks_subset.sh
COUPLED_COMPARE_CHECKS_FAILFAST_TEST_SCRIPT = scripts/test_run_coupled_compare_checks_failfast.sh
COUPLED_COMPARE_REASON_CODES_TEST_SCRIPT = scripts/test_coupled_compare_reason_codes.sh
COUPLED_COMPARE_CHECKS_MANIFEST_REASON_CODES_TEST_SCRIPT = scripts/test_check_coupled_compare_checks_manifest_reason_codes.sh
COUPLED_COMPARE_CHECKS_MANIFEST_PRINT_REASON_CODES_TEST_SCRIPT = scripts/test_check_coupled_compare_checks_manifest_print_known_result_notes.sh
COUPLED_COMPARE_CHECKS_MANIFEST_PRINT_REASON_CODE_PATTERN_TEST_SCRIPT = scripts/test_check_coupled_compare_checks_manifest_print_known_result_note_pattern.sh
COUPLED_COMPARE_REASON_CODES_PRINT_SCRIPT = scripts/print_coupled_compare_reason_codes.sh
COUPLED_COMPARE_REASON_CODES_PRINT_TEST_SCRIPT = scripts/test_print_coupled_compare_reason_codes.sh
COUPLED_COMPARE_REASON_CODE_DOC_SYNC_SCRIPT = scripts/check_coupled_compare_reason_code_docs_sync.sh
COUPLED_COMPARE_REASON_CODE_DOC_SYNC_TEST_SCRIPT = scripts/test_check_coupled_compare_reason_code_docs_sync.sh
COUPLED_COMPARE_REASON_CODE_CONSISTENCY_SCRIPT = scripts/check_coupled_compare_reason_code_consistency.sh
COUPLED_COMPARE_REASON_CODE_CONSISTENCY_TEST_SCRIPT = scripts/test_check_coupled_compare_reason_code_consistency.sh
COUPLED_COMPARE_REASON_CODE_AUDIT_CACHE_TEST_SCRIPT = ../scripts/test_coupled_compare_reason_code_audit_cache.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_contract_checks.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_STDOUT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report_stdout.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_MISSING_CACHE_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report_missing_cache_log.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_WRONG_TARGET_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_contract_audit_report_wrong_target.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_root_surface_contract_checks.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_checks.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_BUNDLE_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_checks_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_CORE_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_CLI_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_HELP_TEST_SCRIPT = scripts/test_make_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_MISSING_TARGET_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_missing_target.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_COMPONENTS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_expected_components.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_TARGET_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_expected_target.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_OVERRIDE_PAIR_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_expected_override_pair.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_HELP_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_help.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_WRONG_TARGET_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_wrong_target.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_escape.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_contract_checks_wrapper_surface_wrapper_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_STDOUT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_stdout.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_MISSING_CACHE_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_missing_cache_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_WRONG_TARGET_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_audit_report_wrong_target.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_CONTRACT_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_missing_contract_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_NESTED_CONTRACT_KEY_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_missing_nested_contract_key.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISMATCH_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_mismatch.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_escape.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_audit_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_audit_surface.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_SURFACE_MODES_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_audit_surface_modes.sh
COUPLED_COMPARE_REASON_CODE_PM_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_pm_surface_skip_nested_selftests.sh
COUPLED_COMPARE_REASON_CODE_ROOT_MODES_SKIP_NESTED_SELFTESTS_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_modes_skip_nested_selftests.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_skip_nested_selftests.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_report_escape.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_surface_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_surface_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_escape.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_escape.sh
COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_NESTED_MISMATCH_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_nested_mismatch.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_skip_nested_selftests.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_MISSING_KEY_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report_missing_key.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_escape.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_NESTED_MISMATCH_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_nested_mismatch.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_escape.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_NESTED_MISMATCH_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_nested_mismatch.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_default_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT = ../scripts/test_run_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_nested_out_dir.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_missing_log.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_print_required_keys.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_wrong_component.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_ESCAPE_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_escape.sh
COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_NESTED_MISMATCH_TEST_SCRIPT = ../scripts/test_check_coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_nested_mismatch.sh
COUPLED_COMPARE_CONTRACT_TARGETS_TEST_SCRIPT = scripts/test_coupled_compare_contract_targets.sh
COUPLED_2D_ACCEPTANCE_SCRIPT = scripts/run_2d_coupled_acceptance.sh
COUPLED_2D_ACCEPTANCE_TEST_SCRIPT = scripts/test_run_2d_coupled_acceptance.sh
COUPLED_2D_ACCEPTANCE_MANIFEST_TEST_SCRIPT = scripts/check_2d_coupled_acceptance_manifest.py
COUPLED_2D_ACCEPTANCE_THRESHOLD_CONTRACT_TEST_SCRIPT = scripts/test_check_2d_coupled_acceptance_manifest_threshold_contract.sh
COUPLED_2D_ACCEPTANCE_STAGES_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_stages.sh
COUPLED_2D_ACCEPTANCE_STAGE_INTEGRATORS_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_stage_integrators.sh
COUPLED_2D_ACCEPTANCE_COMPARE_STAGE_INTEGRATORS_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_compare_stage_integrators.sh
COUPLED_2D_ACCEPTANCE_COMPARE_STAGE_INTEGRATORS_STALE_BINARY_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_compare_stage_integrators_stale_binary.sh
COUPLED_2D_ACCEPTANCE_INTEGRATORS_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_integrators.sh
COUPLED_2D_ACCEPTANCE_INVALID_STAGE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_invalid_stage.sh
COUPLED_2D_ACCEPTANCE_INVALID_INTEGRATOR_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_invalid_integrator.sh
COUPLED_2D_ACCEPTANCE_CONTRACT_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_contract_checks.sh
COUPLED_2D_ACCEPTANCE_GATE_SCRIPT = scripts/run_2d_coupled_acceptance_gate.sh
COUPLED_2D_ACCEPTANCE_GATE_TEST_SCRIPT = scripts/test_run_2d_coupled_acceptance_gate.sh
COUPLED_2D_ACCEPTANCE_GATE_THRESHOLD_PROVENANCE_TEST_SCRIPT = scripts/test_run_2d_coupled_acceptance_gate_threshold_provenance.sh
COUPLED_2D_ACCEPTANCE_GATE_RESILIENCE_SMOKE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_gate_resilience_smoke.sh
COUPLED_2D_ACCEPTANCE_DOCS_SYNC_TEST_SCRIPT = scripts/test_check_coupled_2d_acceptance_docs_sync.sh
COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACES_TEST_SCRIPT = scripts/test_check_coupled_2d_acceptance_docs_sync_surfaces.sh
COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACES_HELP_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_docs_sync_surfaces_help.sh
COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACE_SMOKE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_docs_sync_surface_smoke.sh
COUPLED_2D_ACCEPTANCE_SURFACE_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_surface_checks.sh
COUPLED_2D_ACCEPTANCE_LIGHTWEIGHT_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_lightweight_checks.sh
COUPLED_SCHEME_SURFACE_CLASSIFICATION_TEST_SCRIPT = scripts/test_coupled_scheme_surface_classification.sh
COUPLED_2D_ACCEPTANCE_WRAPPER_SMOKE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_wrapper_smoke.sh
COUPLED_2D_ACCEPTANCE_RESILIENCE_BUNDLE_TEST_SCRIPT = scripts/test_make_coupled_2d_acceptance_resilience_checks.sh
ENSURE_FEM4C_BINARY_TEST_SCRIPT = scripts/test_ensure_fem4c_binary.sh
MBD_INTEGRATOR_SCRIPT = scripts/check_mbd_integrators.sh
MBD_INTEGRATOR_TEST_SCRIPT = scripts/test_check_mbd_integrators.sh
FEM4C_TEST_LOG_MARKER_SCRIPT = scripts/check_fem4c_test_log_markers.sh
FEM4C_TEST_LOG_MARKER_TEST_SCRIPT = scripts/test_check_fem4c_test_log_markers.sh
MBD_B45_SHA_WATCH_SCRIPT = scripts/run_b45_sha_watch.sh
MBD_B45_ACCEPTANCE_SCRIPT = scripts/run_b45_acceptance.sh
TEAM_ACCEPTANCE_GATE_SCRIPT = scripts/run_team_acceptance_gate.sh
FEM_MODEL_COPY_TEST_SCRIPT = scripts/test_fem_model_copy.sh
Q4_T3_STIFFNESS_ADAPTER_WARNING_TEST_SCRIPT = scripts/test_q4_t3_stiffness_adapter_warnings.sh
FLEX_BC2D_TEST_SCRIPT = scripts/test_flex_bc2d.sh
FLEX_BODY2D_INERTIAL_TEST_SCRIPT = scripts/test_flex_body2d_inertial.sh
FLEX_BODY2D_INTERFACE_CENTER_TEST_SCRIPT = scripts/test_flex_body2d_interface_center.sh
FLEX_BODY2D_MARKER_TEST_SCRIPT = scripts/test_flex_body2d_marker_disp.sh
FLEX_NODESET_TEST_SCRIPT = scripts/test_flex_nodeset.sh
FLEX_SNAPSHOT2D_TEST_SCRIPT = scripts/test_flex_snapshot2d.sh
COUPLED_STEP_COMMON2D_TEST_SCRIPT = scripts/test_coupled_step_common2d.sh
COUPLED_STEP_COMMON2D_SOLVE_TEST_SCRIPT = scripts/test_coupled_step_common2d_solve.sh
FLEX_SOLVER2D_TEST_SCRIPT = scripts/test_flex_solver2d.sh
COUPLED_REASSEMBLY_LOG_TEST_SCRIPT = scripts/test_coupled_reassembly_log.sh
COUPLED_SNAPSHOT_OUTPUT_TEST_SCRIPT = scripts/test_coupled_snapshot_output.sh
COUPLED_IMPLICIT_SNAPSHOT_OUTPUT_TEST_SCRIPT = scripts/test_coupled_implicit_snapshot_output.sh
COUPLED_NODESET_GUARD_TEST_SCRIPT = scripts/test_coupled_nodeset_guard.sh
MBD_SYSTEM2D_SMOKE_INPUT = examples/mbd_two_body_input.dat
MBD_SYSTEM2D_SMOKE_OUTPUT = /tmp/fem4c_mbd_system2d_smoke.out
MBD_EXPLICIT2D_SYSTEM_SMOKE_INPUT = examples/mbd_two_body_input.dat
MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT = /tmp/fem4c_mbd_explicit2d_system_smoke.out
MBD_ASSEMBLER2D_SMOKE_OUTPUT = /tmp/fem4c_mbd_assembler2d_smoke.out
MBD_DENSE_SOLVER_SMOKE_OUTPUT = /tmp/fem4c_mbd_dense_solver_smoke.out
MBD_NEWMARK2D_SYSTEM_SMOKE_INPUT = examples/mbd_single_body_newmark.dat
MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT = /tmp/fem4c_mbd_newmark2d_system_smoke.out
MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT = /tmp/fem4c_mbd_newmark2d_constrained_smoke.out

# Parser target (parser.exe on Windows, parser on POSIX)
PARSER_SRC = $(PARSERDIR)/parser.c
PARSER_TARGET = $(PARSERDIR)/parser$(PARSER_EXE_SUFFIX)
PARSER_CFLAGS = -Wall -Wextra -O3 -std=c99
# If parser needs any project headers, keep INCLUDES here as well.
PARSER_INCLUDES = $(INCLUDES)

# Default target (build solver + parser)
all: $(TARGET) $(PARSER_TARGET)

# Release build
release: CFLAGS += $(OPENMP_FLAGS)
release: $(TARGET) $(PARSER_TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET) $(PARSER_TARGET)

# OpenMP build
openmp: CFLAGS += $(OPENMP_FLAGS)
openmp: $(TARGET) $(PARSER_TARGET)

# Main target
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "Linking $@"
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Build parser executable
$(PARSER_TARGET): $(PARSER_SRC)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) $(PARSER_CFLAGS) $(PARSER_INCLUDES) -o $@ $< -lm

# Build MBD probe harness
$(MBD_PROBE_TARGET): $(MBD_PROBE_SRC) $(MBD_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_PROBE_SRC) $(MBD_PROBE_SRCS) -lm

$(MBD_NEWMARK2D_PROBE_TARGET): $(MBD_NEWMARK2D_PROBE_SRC) $(MBD_NEWMARK2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_NEWMARK2D_PROBE_SRC) $(MBD_NEWMARK2D_PROBE_SRCS) -lm

$(MBD_HHT2D_PROBE_TARGET): $(MBD_HHT2D_PROBE_SRC) $(MBD_HHT2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_HHT2D_PROBE_SRC) $(MBD_HHT2D_PROBE_SRCS) -lm

$(MBD_HHT2D_INVALID_PROBE_TARGET): $(MBD_HHT2D_INVALID_PROBE_SRC) $(MBD_HHT2D_INVALID_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_HHT2D_INVALID_PROBE_SRC) $(MBD_HHT2D_INVALID_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_CLI_TARGET): $(MBD_SYSTEM2D_CLI_SRC) $(MBD_SYSTEM2D_CLI_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_CLI_SRC) $(MBD_SYSTEM2D_CLI_SRCS) -lm

$(MBD_SYSTEM2D_EXPLICIT_PROBE_TARGET): $(MBD_SYSTEM2D_EXPLICIT_PROBE_SRC) $(MBD_SYSTEM2D_EXPLICIT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_EXPLICIT_PROBE_SRC) $(MBD_SYSTEM2D_EXPLICIT_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_NEWMARK_PROBE_TARGET): $(MBD_SYSTEM2D_NEWMARK_PROBE_SRC) $(MBD_SYSTEM2D_NEWMARK_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_NEWMARK_PROBE_SRC) $(MBD_SYSTEM2D_NEWMARK_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_HHT_PROBE_TARGET): $(MBD_SYSTEM2D_HHT_PROBE_SRC) $(MBD_SYSTEM2D_HHT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_HHT_PROBE_SRC) $(MBD_SYSTEM2D_HHT_PROBE_SRCS) -lm

$(MBD_ASSEMBLER2D_PROBE_TARGET): $(MBD_ASSEMBLER2D_PROBE_SRC) $(MBD_ASSEMBLER2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_ASSEMBLER2D_PROBE_SRC) $(MBD_ASSEMBLER2D_PROBE_SRCS) -lm

$(MBD_KINEMATICS2D_PROBE_TARGET): $(MBD_KINEMATICS2D_PROBE_SRC) $(MBD_KINEMATICS2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_KINEMATICS2D_PROBE_SRC) $(MBD_KINEMATICS2D_PROBE_SRCS) -lm

$(MBD_EXPLICIT2D_PROBE_TARGET): $(MBD_EXPLICIT2D_PROBE_SRC) $(MBD_EXPLICIT2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_EXPLICIT2D_PROBE_SRC) $(MBD_EXPLICIT2D_PROBE_SRCS) -lm

$(MBD_BODY2D_REFERENCE_PROBE_TARGET): $(MBD_BODY2D_REFERENCE_PROBE_SRC) $(MBD_BODY2D_REFERENCE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_BODY2D_REFERENCE_PROBE_SRC) $(MBD_BODY2D_REFERENCE_PROBE_SRCS) -lm

$(MBD_CONSTRAINT_RHS_PROBE_TARGET): $(MBD_CONSTRAINT_RHS_PROBE_SRC) $(MBD_CONSTRAINT_RHS_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_CONSTRAINT_RHS_PROBE_SRC) $(MBD_CONSTRAINT_RHS_PROBE_SRCS) -lm

$(MBD_OUTPUT2D_PROBE_TARGET): $(MBD_OUTPUT2D_PROBE_SRC) $(MBD_OUTPUT2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_OUTPUT2D_PROBE_SRC) $(MBD_OUTPUT2D_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_HISTORY_PROBE_TARGET): $(MBD_SYSTEM2D_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_HISTORY_PROBE_SRCS) -lm

$(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_TARGET): $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRC) $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRC) $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_SRCS) -lm

$(MBD_FLEXIBLE_FORCE_PROBE_TARGET): $(MBD_FLEXIBLE_FORCE_PROBE_SRC) $(MBD_FLEXIBLE_FORCE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_FLEXIBLE_FORCE_PROBE_SRC) $(MBD_FLEXIBLE_FORCE_PROBE_SRCS) -lm

$(MBD_DENSE_SOLVER_PROBE_TARGET): $(MBD_DENSE_SOLVER_PROBE_SRC) $(MBD_DENSE_SOLVER_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_DENSE_SOLVER_PROBE_SRC) $(MBD_DENSE_SOLVER_PROBE_SRCS) -lm

$(MBD_DENSE_SOLVER_SINGULAR_PROBE_TARGET): $(MBD_DENSE_SOLVER_SINGULAR_PROBE_SRC) $(MBD_DENSE_SOLVER_SINGULAR_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_DENSE_SOLVER_SINGULAR_PROBE_SRC) $(MBD_DENSE_SOLVER_SINGULAR_PROBE_SRCS) -lm

$(MBD_DENSE_SOLVER_INVALID_PROBE_TARGET): $(MBD_DENSE_SOLVER_INVALID_PROBE_SRC) $(MBD_DENSE_SOLVER_INVALID_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_DENSE_SOLVER_INVALID_PROBE_SRC) $(MBD_DENSE_SOLVER_INVALID_PROBE_SRCS) -lm

$(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_TARGET): $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRC) $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRC) $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_TARGET): $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRC) $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRC) $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_TARGET): $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_TARGET): $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_TARGET): $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET): $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET): $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRC) $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_SRCS) -lm

$(MBD_PROJECTION2D_PROBE_TARGET): $(MBD_PROJECTION2D_PROBE_SRC) $(MBD_PROJECTION2D_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_PROJECTION2D_PROBE_SRC) $(MBD_PROJECTION2D_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET): $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET): $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRC) $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_TARGET): $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_SRCS) -lm

$(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_TARGET): $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRCS)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	$(CC) -Wall -Wextra -O3 -std=c99 -Isrc -o $@ $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRC) $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_SRCS) -lm

# Build + run MBD probe in one command
mbd_probe: $(MBD_PROBE_TARGET)
	@echo "Running $(MBD_PROBE_TARGET)"
	@./$(MBD_PROBE_TARGET)

# B-02 unit smoke: standalone dense KKT assembly contract
mbd_assembler2d_probe_smoke: $(MBD_ASSEMBLER2D_PROBE_TARGET)
	@echo "Running $(MBD_ASSEMBLER2D_PROBE_TARGET)"
	@./$(MBD_ASSEMBLER2D_PROBE_TARGET)

# B-05 smoke: unconstrained single-body Newmark update
mbd_newmark2d_smoke: $(MBD_NEWMARK2D_PROBE_TARGET)
	@echo "Running $(MBD_NEWMARK2D_PROBE_TARGET)"
	@./$(MBD_NEWMARK2D_PROBE_TARGET)

# B-07 unit smoke: HHT coefficient / blend contract
mbd_hht2d_probe_smoke: $(MBD_HHT2D_PROBE_TARGET)
	@echo "Running $(MBD_HHT2D_PROBE_TARGET)"
	@./$(MBD_HHT2D_PROBE_TARGET)

mbd_hht2d_invalid_smoke: $(MBD_HHT2D_INVALID_PROBE_TARGET)
	@echo "Running $(MBD_HHT2D_INVALID_PROBE_TARGET)"
	@./$(MBD_HHT2D_INVALID_PROBE_TARGET)

# B-06 unit smoke: system-owned Newmark free/constrained contract
mbd_system2d_newmark_probe_smoke: $(MBD_SYSTEM2D_NEWMARK_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_NEWMARK_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_NEWMARK_PROBE_TARGET)

# A-05 system smoke: input-driven free explicit single-body path
mbd_system2d_explicit_probe_smoke: $(MBD_SYSTEM2D_EXPLICIT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_EXPLICIT_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_EXPLICIT_PROBE_TARGET)

# B-08 unit smoke: system-owned HHT free/constrained contract
mbd_system2d_hht_probe_smoke: $(MBD_SYSTEM2D_HHT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_HHT_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_HHT_PROBE_TARGET)

# A-04 smoke: local/world marker transform self-check
mbd_kinematics2d_smoke: $(MBD_KINEMATICS2D_PROBE_TARGET)
	@echo "Running $(MBD_KINEMATICS2D_PROBE_TARGET)"
	@./$(MBD_KINEMATICS2D_PROBE_TARGET)

# A-05 smoke: semi-implicit Euler update path
mbd_explicit2d_smoke: $(MBD_EXPLICIT2D_PROBE_TARGET)
	@echo "Running $(MBD_EXPLICIT2D_PROBE_TARGET)"
	@./$(MBD_EXPLICIT2D_PROBE_TARGET)

# A-09 groundwork smoke: body reference frame contract
mbd_body2d_reference_smoke: $(MBD_BODY2D_REFERENCE_PROBE_TARGET)
	@echo "Running $(MBD_BODY2D_REFERENCE_PROBE_TARGET)"
	@./$(MBD_BODY2D_REFERENCE_PROBE_TARGET)

# B-04 unit smoke: residual/gamma Baumgarte contract
mbd_constraint_rhs_probe_smoke: $(MBD_CONSTRAINT_RHS_PROBE_TARGET)
	@echo "Running $(MBD_CONSTRAINT_RHS_PROBE_TARGET)"
	@./$(MBD_CONSTRAINT_RHS_PROBE_TARGET)

# A-07 groundwork smoke: CSV writer header/row contract
mbd_output2d_smoke: $(MBD_OUTPUT2D_PROBE_TARGET)
	@echo "Running $(MBD_OUTPUT2D_PROBE_TARGET)"
	@./$(MBD_OUTPUT2D_PROBE_TARGET)

# A-team compare groundwork: rigid summary CSV schema
mbd_output2d_rigid_compare_smoke: $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_TARGET)
	@echo "Running $(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_TARGET)"
	@./$(MBD_OUTPUT2D_RIGID_COMPARE_PROBE_TARGET)

# A-07 smoke: system2d history CSV sidecar path
mbd_system2d_history_smoke: $(MBD_SYSTEM2D_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_HISTORY_PROBE_TARGET)"
	@cd . && ./$(MBD_SYSTEM2D_HISTORY_PROBE_TARGET)

# A-team compare benchmark smoke: rigid benchmark compare sidecar path
mbd_system2d_rigid_compare_smoke: $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_TARGET)"
	@cd . && ./$(MBD_SYSTEM2D_RIGID_COMPARE_PROBE_TARGET)

# A-team compare geometry smoke: coupled master tip geometry reflected in compare sidecar
mbd_system2d_coupled_geometry_compare_smoke: $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_TARGET)"
	@cd . && ./$(MBD_SYSTEM2D_COUPLED_GEOMETRY_COMPARE_PROBE_TARGET)

# A-08 groundwork smoke: flexible generalized force joins RHS
mbd_flexible_force_smoke: $(MBD_FLEXIBLE_FORCE_PROBE_TARGET)
	@echo "Running $(MBD_FLEXIBLE_FORCE_PROBE_TARGET)"
	@./$(MBD_FLEXIBLE_FORCE_PROBE_TARGET)

# A-13 groundwork smoke: HHT effective generalized force uses system-owned history
mbd_forces2d_hht_effective_smoke: $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_TARGET)
	@echo "Running $(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_TARGET)"
	@./$(MBD_FORCES2D_HHT_EFFECTIVE_PROBE_TARGET)

# A-11 groundwork smoke: body force baseline round-trip via system API
mbd_system2d_force_baseline_smoke: $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_FORCE_BASELINE_PROBE_TARGET)

# A-12 smoke: system-owned current/previous generalized force history
mbd_system2d_force_history_smoke: $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_FORCE_HISTORY_PROBE_TARGET)

# A-12 smoke: Newmark summary emits system-owned current/previous generalized force rows
mbd_system2d_newmark_history_output_smoke: $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_NEWMARK_HISTORY_OUTPUT_PROBE_TARGET)

# A-13 smoke: HHT summary emits system-owned current/previous generalized force rows
mbd_system2d_hht_history_output_smoke: $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_HHT_HISTORY_OUTPUT_PROBE_TARGET)

# B-09 groundwork smoke: one-shot position projection reduces rigid constraint drift
mbd_projection2d_probe_smoke: $(MBD_PROJECTION2D_PROBE_TARGET)
	@echo "Running $(MBD_PROJECTION2D_PROBE_TARGET)"
	@./$(MBD_PROJECTION2D_PROBE_TARGET)

# B-09 probe: multi-step projection reduces drift for explicit/Newmark/HHT
mbd_system2d_projection_compare_smoke: $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET)"
	@./$(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET)

# B-09 hardening smoke: extended projection compare run for drift reduction stability
mbd_system2d_projection_long_compare_smoke: $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET) with FEM4C_MBD_PROJECTION_COMPARE_STEPS=20"
	@FEM4C_MBD_PROJECTION_COMPARE_STEPS=20 ./$(MBD_SYSTEM2D_PROJECTION_COMPARE_PROBE_TARGET)

# B-09 probe: projection-enabled history CSV keeps per-step stop reason/reduction ratio for explicit/Newmark/HHT
mbd_system2d_projection_history_output_smoke: $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) explicit"
	@./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) explicit
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) newmark"
	@./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) newmark
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) hht"
	@./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) hht

# B-09 long-run probe: projection history CSV keeps the same contract through 20 steps
mbd_system2d_projection_long_history_output_smoke: $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) explicit with FEM4C_MBD_PROJECTION_HISTORY_STEPS=20"
	@FEM4C_MBD_PROJECTION_HISTORY_STEPS=20 ./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) explicit
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) newmark with FEM4C_MBD_PROJECTION_HISTORY_STEPS=20"
	@FEM4C_MBD_PROJECTION_HISTORY_STEPS=20 ./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) newmark
	@echo "Running $(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) hht with FEM4C_MBD_PROJECTION_HISTORY_STEPS=20"
	@FEM4C_MBD_PROJECTION_HISTORY_STEPS=20 ./$(MBD_SYSTEM2D_PROJECTION_HISTORY_OUTPUT_PROBE_TARGET) hht

# B-09 acceptance bundle: long-run compare/history plus isolated variants
mbd_system2d_projection_longrun_contract_smoke: mbd_system2d_projection_long_compare_smoke \
                                                mbd_system2d_projection_long_history_output_smoke \
                                                mbd_system2d_projection_long_compare_isolated_smoke \
                                                mbd_system2d_projection_long_history_isolated_smoke
	@true

# Rebuild the local CLI on demand so projection fallback smokes survive stale/missing bin dirs.
mbd_system2d_cli_ready:
	@if [ ! -x ./$(MBD_SYSTEM2D_CLI_TARGET) ]; then \
		$(MAKE) --no-print-directory $(MBD_SYSTEM2D_CLI_TARGET); \
	fi

MBD_SYSTEM2D_HISTORY_BASE_PROBE_BUNDLE = mbd_system2d_history_smoke \
                                         mbd_system2d_explicit_probe_smoke \
                                         mbd_system2d_explicit_console_history_smoke \
                                         mbd_forces2d_hht_effective_smoke \
                                         mbd_system2d_force_history_smoke \
                                         mbd_system2d_projection_history_output_smoke
MBD_SYSTEM2D_HISTORY_IMPLICIT_PROBE_BUNDLE = mbd_system2d_newmark_console_history_smoke \
                                             mbd_system2d_newmark_constrained_console_history_smoke \
                                             mbd_system2d_newmark_history_output_smoke \
                                             mbd_system2d_newmark_constrained_history_output_smoke \
                                             mbd_system2d_hht_console_history_smoke \
                                             mbd_system2d_hht_constrained_console_history_smoke \
                                             mbd_system2d_hht_history_output_smoke \
                                             mbd_system2d_hht_constrained_history_output_smoke
MBD_SYSTEM2D_HISTORY_CLI_TARGETS = mbd_system2d_explicit_smoke \
                                   mbd_system2d_newmark_smoke \
                                   mbd_system2d_newmark_constrained_smoke \
                                   mbd_system2d_hht_smoke \
                                   mbd_system2d_hht_constrained_smoke
MBD_SYSTEM2D_HISTORY_CLI_CONTRACT_BUNDLE = $(MBD_SYSTEM2D_HISTORY_CLI_TARGETS)
MBD_SYSTEM2D_HISTORY_CONTRACT_BUNDLE = mbd_system2d_history_contract_layout_smoke \
                                       mbd_system2d_history_probe_contract_smoke \
                                       mbd_system2d_history_cli_contract_smoke
MBD_A_TEAM_HISTORY_ENTRYPOINT = mbd_system2d_history_contract_smoke
MBD_A_TEAM_HISTORY_HELP_DESC = Run A-team history-only generalized-force contract bundle
MBD_A_TEAM_HISTORY_SURFACE_LABEL = history-only current command surface
MBD_A_TEAM_FOUNDATION_ENTRYPOINT = mbd_a_team_foundation_smoke
MBD_A_TEAM_FOUNDATION_HELP_DESC = Run A-team full rigid MBD foundation bundle
MBD_A_TEAM_FOUNDATION_SURFACE_LABEL = full foundation current command surface
MBD_A_TEAM_CURRENT_COMMAND_DOCS = README.md \
                                  ../docs/06_acceptance_matrix_2d.md
MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE = mbd_output2d_smoke \
                                     mbd_system2d_history_contract_smoke
MBD_A_TEAM_HISTORY_CONSUMER_TARGETS = mbd_implicit_label_review_smoke \
                                      mbd_a_team_foundation_smoke
MBD_A_TEAM_HISTORY_HELP_TARGETS = mbd_a_team_history_contract_layout_smoke \
                                  mbd_a_team_history_header_single_source_smoke \
                                  mbd_a_team_history_audit_smoke \
                                  $(MBD_A_TEAM_HISTORY_ENTRYPOINT)
MBD_A_TEAM_HISTORY_HEADER_HELPER_CONSUMERS = practice/ch09/mbd_output2d_probe.c \
                                             practice/ch09/mbd_system2d_history_probe.c \
                                             practice/ch09/mbd_system2d_newmark_history_output_probe.c \
                                             practice/ch09/mbd_system2d_hht_history_output_probe.c \
                                             practice/ch09/mbd_system2d_constrained_history_output_probe.c \
                                             practice/ch09/mbd_system2d_projection_history_output_probe.c
MBD_A_TEAM_RIGID_COMPARE_HEADER_HELPER_CONSUMERS = practice/ch09/mbd_output2d_rigid_compare_probe.c \
                                                   practice/ch09/mbd_system2d_history_probe.c \
                                                   practice/ch09/mbd_system2d_rigid_compare_probe.c \
                                                   practice/ch09/mbd_system2d_coupled_geometry_compare_probe.c
MBD_A_TEAM_PROBE_UTILS_HELPER_CONSUMERS = practice/ch09/mbd_output2d_probe.c \
                                          practice/ch09/mbd_output2d_rigid_compare_probe.c \
                                          practice/ch09/mbd_system2d_explicit_probe.c \
                                          practice/ch09/mbd_system2d_history_probe.c \
                                          practice/ch09/mbd_system2d_console_history_probe.c \
                                          practice/ch09/mbd_system2d_newmark_history_output_probe.c \
                                          practice/ch09/mbd_system2d_hht_history_output_probe.c \
                                          practice/ch09/mbd_system2d_constrained_history_output_probe.c \
                                          practice/ch09/mbd_system2d_projection_history_output_probe.c \
                                          practice/ch09/mbd_system2d_rigid_compare_probe.c \
                                          practice/ch09/mbd_system2d_coupled_geometry_compare_probe.c
MBD_A_TEAM_FOUNDATION_PROBE_BUNDLE = mbd_kinematics2d_smoke \
                                     mbd_explicit2d_smoke \
                                     mbd_output2d_smoke \
                                     mbd_output2d_rigid_compare_smoke \
                                     mbd_system2d_rigid_compare_smoke \
                                     mbd_system2d_coupled_geometry_compare_smoke \
                                     mbd_flexible_force_smoke \
                                     mbd_system2d_force_baseline_smoke \
                                     mbd_body2d_reference_smoke
MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS = mbd_a_team_foundation_probe_layout_smoke \
                                       mbd_a_team_foundation_contract_layout_smoke
MBD_A_TEAM_FOUNDATION_SURFACE_TARGETS = mbd_a_team_foundation_help_contract_smoke \
                                        mbd_a_team_foundation_surface_smoke
MBD_A_TEAM_FOUNDATION_HELP_TARGETS = $(MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS) \
                                     $(MBD_A_TEAM_FOUNDATION_SURFACE_TARGETS)
MBD_A_TEAM_HISTORY_AUDIT_CORE_BUNDLE = mbd_a_team_history_contract_layout_smoke \
                                       mbd_a_team_history_contract_surface_smoke \
                                       mbd_a_team_history_header_single_source_smoke \
                                       mbd_a_team_rigid_compare_header_single_source_smoke \
                                       mbd_a_team_probe_utils_single_source_smoke
MBD_A_TEAM_HISTORY_AUDIT_BUNDLE = $(MBD_A_TEAM_HISTORY_AUDIT_CORE_BUNDLE) \
                                  $(MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS) \
                                  mbd_a_team_history_help_contract_smoke
MBD_A_TEAM_FOUNDATION_AUDIT_BUNDLE = mbd_a_team_history_audit_smoke \
                                     $(MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS) \
                                     $(MBD_A_TEAM_FOUNDATION_SURFACE_TARGETS)
MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE = mbd_a_team_foundation_probe_smoke \
                                        mbd_a_team_history_contract_smoke
MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE = $(MBD_A_TEAM_FOUNDATION_AUDIT_BUNDLE) \
                                     $(MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE)

# A-team rigid MBD probe smoke pack
mbd_a_team_foundation_probe_smoke: $(MBD_A_TEAM_FOUNDATION_PROBE_BUNDLE)
	@echo "PASS: A-team rigid MBD probe smoke pack"

# A-15 groundwork smoke: explicit/base history probes are grouped behind one bundle
mbd_system2d_history_base_probe_smoke: $(MBD_SYSTEM2D_HISTORY_BASE_PROBE_BUNDLE)
	@echo "PASS: MBD generalized force history base probe smoke pack"

# A-15 groundwork smoke: implicit history probes are grouped behind one bundle
mbd_system2d_history_implicit_probe_smoke: $(MBD_SYSTEM2D_HISTORY_IMPLICIT_PROBE_BUNDLE)
	@echo "PASS: MBD generalized force history implicit probe smoke pack"

# A-15 groundwork smoke: generalized force history probe contract matrix reused by history bundles
mbd_system2d_history_probe_contract_smoke: mbd_system2d_history_base_probe_smoke \
                                           mbd_system2d_history_implicit_probe_smoke
	@echo "PASS: MBD generalized force history probe contract smoke pack"

# A-15 static audit: CLI smoke recipes must not own generalized-force history marker greps
mbd_system2d_history_contract_layout_smoke:
	@set -eu; \
	check_target() { \
		target="$$1"; \
		block="$$(awk -v target="$$target" '\
			$$0 ~ ("^" target ":") {capture=1; print; next} \
			capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
			capture {print} \
		' $(lastword $(MAKEFILE_LIST)) | sed '$$d')"; \
		if printf '%s\n' "$$block" | grep -Eq 'generalized_force_history_valid|generalized_force_current|generalized_force_previous'; then \
			echo "FAIL: $$target still owns generalized-force history marker checks"; \
			exit 1; \
		fi; \
	}; \
	for target in $(MBD_SYSTEM2D_HISTORY_CLI_TARGETS); do \
		check_target "$$target"; \
	done; \
	for consumer in $(MBD_A_TEAM_HISTORY_CONSUMER_TARGETS); do \
		block="$$(awk -v target="$$consumer" '\
			$$0 ~ ("^" target ":") {capture=1; print; next} \
			capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
			capture {print} \
		' $(lastword $(MAKEFILE_LIST)) | sed '$$d')"; \
		if ! printf '%s\n' "$$block" | grep -Eq 'mbd_a_team_history_contract_smoke|MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE|MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE'; then \
			echo "FAIL: $$consumer is no longer wired to the A-team history contract surface"; \
			exit 1; \
		fi; \
	done; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_a_team_history_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_system2d_history_contract_smoke:/ {capture=1; print; next} \
	capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
	capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_SYSTEM2D_HISTORY_CONTRACT_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_SYSTEM2D_HISTORY_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_system2d_history_probe_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_SYSTEM2D_HISTORY_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_system2d_history_cli_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_foundation_probe_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -qv 'mbd_system2d_history_probe_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_history_contract_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE'; \
	echo "PASS: MBD generalized force history layout contract smoke pack"

# A-16 groundwork smoke: CLI/system summary contract matrix layered on top of probe contracts
mbd_system2d_history_cli_contract_smoke: $(MBD_SYSTEM2D_HISTORY_CLI_CONTRACT_BUNDLE)
	@echo "PASS: MBD generalized force history CLI contract smoke pack"

# A-16 contract smoke: A-team history-facing review/foundation entrypoint
mbd_a_team_history_contract_layout_smoke:
	@set -eu; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_history_contract_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_output2d_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_history_contract_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_system2d_history_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_HISTORY_AUDIT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'MBD_A_TEAM_HISTORY_AUDIT_CORE_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_HISTORY_AUDIT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS'; \
	echo "PASS: A-team history contract layout smoke pack"

mbd_a_team_history_contract_surface_smoke:
	@set -eu; \
	help_output="$$( $(MAKE) -s help )"; \
	grep -q 'mbd_a_team_history_contract_layout_smoke' $(lastword $(MAKEFILE_LIST)); \
	printf '%s\n' "$$help_output" | grep -q '^  $(MBD_A_TEAM_HISTORY_ENTRYPOINT) - $(MBD_A_TEAM_HISTORY_HELP_DESC)$$'; \
	printf '%s\n' "$$help_output" | grep -q '^  mbd_a_team_history_contract_layout_smoke - Verify A-team history contract bundle wiring$$'; \
	grep -q '`make -C FEM4C $(MBD_A_TEAM_HISTORY_ENTRYPOINT)` は A-team の $(MBD_A_TEAM_HISTORY_SURFACE_LABEL)' README.md; \
	grep -q '`make -C FEM4C $(MBD_A_TEAM_HISTORY_ENTRYPOINT)` は A-team の $(MBD_A_TEAM_HISTORY_SURFACE_LABEL)' ../docs/06_acceptance_matrix_2d.md; \
	echo "PASS: A-team history contract surface smoke pack"

mbd_a_team_history_header_single_source_smoke:
	@set -eu; \
	for probe in $(MBD_A_TEAM_HISTORY_HEADER_HELPER_CONSUMERS); do \
		if ! grep -Eq 'mbd_output2d_history_header_csv\(\)|#include "mbd_probe_utils.h"' "$$probe"; then \
			echo "FAIL: $$probe is no longer wired to the history header helper surface"; \
			exit 1; \
		fi; \
	done; \
	occurrences="$$(rg -n 'step,time,body_id,x,y,theta,vx,vy,omega,ax,ay,alpha,hht_force_history_mode' src/mbd practice/ch09 -g'*.c' -g'*.h')"; \
	printf '%s\n' "$$occurrences" | grep -q '^src/mbd/output2d.c:'; \
	[ "$$(printf '%s\n' "$$occurrences" | wc -l | tr -d ' ')" = "1" ]; \
	echo "PASS: A-team history header single-source smoke pack"

mbd_a_team_rigid_compare_header_single_source_smoke:
	@set -eu; \
	for probe in $(MBD_A_TEAM_RIGID_COMPARE_HEADER_HELPER_CONSUMERS); do \
		if ! grep -Eq 'mbd_output2d_rigid_compare_header_csv\(\)|#include "mbd_probe_utils.h"' "$$probe"; then \
			echo "FAIL: $$probe is no longer wired to the rigid-compare header helper surface"; \
			exit 1; \
		fi; \
	done; \
	occurrences="$$(rg -n 'time,theta1,theta2,omega1,omega2,tip1_x,tip1_y,tip2_x,tip2_y,root_reaction_x,root_reaction_y,constraint_residual,full_reassembly_count_link1,full_reassembly_count_link2' src/mbd practice/ch09 -g'*.c' -g'*.h')"; \
	printf '%s\n' "$$occurrences" | grep -q '^src/mbd/output2d.c:'; \
	[ "$$(printf '%s\n' "$$occurrences" | wc -l | tr -d ' ')" = "1" ]; \
	echo "PASS: A-team rigid compare header single-source smoke pack"

mbd_a_team_probe_utils_single_source_smoke:
	@set -eu; \
	for probe in $(MBD_A_TEAM_PROBE_UTILS_HELPER_CONSUMERS); do \
		grep -q '#include "mbd_probe_utils.h"' "$$probe"; \
	done; \
	! rg -n 'strcmp\\(line, mbd_output2d_(history|rigid_compare)_header_csv\\(\\)\\)' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'strncpy\((history_path|rigid_compare_path),' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'strlen\\(\"(history|rigid_compare)_csv,\"\\)' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'strstr\\(line, (history_path|rigid_compare_path)\\)' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'char \\*cursor = line;|while \\(column < 14 && cursor\\)' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'parse_cell_or_nan\(|strtod\\(fields\\[[0-9]+\\], NULL\\)' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'static int line_starts_with\(' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'static (inline )?void trim_newline\(' practice/ch09/mbd_*.c > /dev/null; \
	! rg -n 'static (inline )?int split_csv_fields\(' practice/ch09/mbd_*.c > /dev/null; \
	echo "PASS: A-team probe utils single-source smoke pack"

mbd_a_team_foundation_probe_layout_smoke:
	@set -eu; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_foundation_probe_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_FOUNDATION_PROBE_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_PROBE_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_output2d_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_PROBE_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -qv 'mbd_system2d_history_contract_smoke'; \
	echo "PASS: A-team foundation probe layout smoke pack"

mbd_a_team_foundation_contract_layout_smoke:
	@set -eu; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_foundation_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'MBD_A_TEAM_FOUNDATION_AUDIT_BUNDLE'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_a_team_foundation_probe_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_CONTRACT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'mbd_a_team_history_contract_smoke'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_AUDIT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'MBD_A_TEAM_FOUNDATION_LAYOUT_TARGETS'; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^MBD_A_TEAM_FOUNDATION_AUDIT_BUNDLE[[:space:]]*=/ {capture=1; print; next} \
		capture && $$0 !~ /^[[:space:]]+/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)))" | grep -q 'MBD_A_TEAM_FOUNDATION_SURFACE_TARGETS'; \
	echo "PASS: A-team foundation contract layout smoke pack"

mbd_a_team_history_help_contract_smoke:
	@set -eu; \
	help_output="$$( $(MAKE) -s help )"; \
	for target in $(MBD_A_TEAM_HISTORY_HELP_TARGETS); do \
		printf '%s\n' "$$help_output" | grep -q "^  $$target - "; \
	done; \
	echo "PASS: A-team history help contract smoke pack"

mbd_a_team_foundation_help_contract_smoke:
	@set -eu; \
	help_output="$$( $(MAKE) -s help )"; \
	printf '%s\n' "$$help_output" | grep -q '^  $(MBD_A_TEAM_FOUNDATION_ENTRYPOINT) - $(MBD_A_TEAM_FOUNDATION_HELP_DESC)$$'; \
	for target in $(MBD_A_TEAM_FOUNDATION_HELP_TARGETS); do \
		printf '%s\n' "$$help_output" | grep -q "^  $$target - "; \
	done; \
	echo "PASS: A-team foundation help contract smoke pack"

mbd_a_team_foundation_surface_smoke:
	@set -eu; \
	help_output="$$( $(MAKE) -s help )"; \
	printf '%s\n' "$$(awk '\
		$$0 ~ /^mbd_a_team_foundation_smoke:/ {capture=1; print; next} \
		capture && $$0 ~ /^[^[:space:]#].*:/ {exit} \
		capture {print} \
	' $(lastword $(MAKEFILE_LIST)) | sed '$$d')" | grep -q 'MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE'; \
	printf '%s\n' "$$help_output" | grep -q '^  mbd_a_team_foundation_surface_smoke - Verify A-team foundation entrypoint stays bundle-backed$$'; \
	grep -q '`make -C FEM4C $(MBD_A_TEAM_FOUNDATION_ENTRYPOINT)` は A-team の $(MBD_A_TEAM_FOUNDATION_SURFACE_LABEL)' README.md; \
	grep -q '`make -C FEM4C $(MBD_A_TEAM_FOUNDATION_ENTRYPOINT)` は A-team の $(MBD_A_TEAM_FOUNDATION_SURFACE_LABEL)' ../docs/06_acceptance_matrix_2d.md; \
	grep -q '`make -C FEM4C $(MBD_A_TEAM_HISTORY_ENTRYPOINT)` は A-team の $(MBD_A_TEAM_HISTORY_SURFACE_LABEL)' ../docs/06_acceptance_matrix_2d.md; \
	echo "PASS: A-team foundation surface smoke pack"

mbd_a_team_history_audit_smoke: $(MBD_A_TEAM_HISTORY_AUDIT_BUNDLE)
	@echo "PASS: A-team history audit smoke pack"

mbd_a_team_history_contract_smoke: mbd_a_team_history_audit_smoke \
                                   $(MBD_A_TEAM_HISTORY_CONTRACT_BUNDLE)
	@echo "PASS: A-team generalized force history contract smoke pack"

# A-R1 review smoke: neutral implicit_result/implicit_param vocabulary must cover summary, history, and console paths
mbd_implicit_label_review_smoke: mbd_a_team_history_contract_smoke
	@echo "PASS: MBD implicit label review smoke pack"

# A-R2 review smoke: rigid compare sidecar and analytic compare path must stay compare-ready
mbd_rigid_compare_review_smoke: mbd_system2d_rigid_compare_smoke \
                                mbd_output2d_rigid_compare_header_single_source_smoke \
                                mbd_output2d_rigid_compare_field_count_sync_smoke \
                                mbd_rigid_compare_review_columns_sync_smoke \
                                compare_2link_artifact_route_fields_getter_test \
                                compare_2link_artifact_route_fields_sync_test \
                                mbd_system2d_coupled_geometry_compare_smoke \
                                mbd_rigid_analytic_real_test \
                                mbd_rigid_analytic_compare_test \
                                mbd_rigid_analytic_wrapper_test \
                                mbd_rigid_compare_integrator_review_smoke
	@echo "PASS: MBD rigid compare review smoke pack"

# A-team rigid MBD module smoke pack
mbd_a_team_foundation_smoke: $(MBD_A_TEAM_FOUNDATION_SMOKE_BUNDLE)
	@echo "PASS: A-team rigid MBD module smoke pack"

# A-16 groundwork smoke: generalized force history contract matrix reused by foundation pack
mbd_system2d_history_contract_smoke: $(MBD_SYSTEM2D_HISTORY_CONTRACT_BUNDLE)
	@echo "PASS: MBD generalized force history contract smoke pack"

# B-03 unit smoke: standalone dense KKT solve contract
mbd_dense_solver_probe_smoke: $(MBD_DENSE_SOLVER_PROBE_TARGET)
	@echo "Running $(MBD_DENSE_SOLVER_PROBE_TARGET)"
	@./$(MBD_DENSE_SOLVER_PROBE_TARGET)

# B-03 negative smoke: singular dense system must fail cleanly
mbd_dense_solver_singular_smoke: $(MBD_DENSE_SOLVER_SINGULAR_PROBE_TARGET)
	@echo "Running $(MBD_DENSE_SOLVER_SINGULAR_PROBE_TARGET)"
	@./$(MBD_DENSE_SOLVER_SINGULAR_PROBE_TARGET)

# B-03 negative smoke: non-finite dense input must fail cleanly
mbd_dense_solver_invalid_smoke: $(MBD_DENSE_SOLVER_INVALID_PROBE_TARGET)
	@echo "Running $(MBD_DENSE_SOLVER_INVALID_PROBE_TARGET)"
	@./$(MBD_DENSE_SOLVER_INVALID_PROBE_TARGET)

# B-06 groundwork smoke: system2d newmark single-body path (history rows are covered by direct probes)
mbd_system2d_newmark_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_NEWMARK2D_SYSTEM_SMOKE_INPUT) $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^integrator,newmark_beta' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^step_execution_mode,newmark_free' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^steps_executed,1' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_beta,' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_gamma,' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_alpha,' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_beta_source_status,default$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_gamma_source_status,default$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_param_alpha_source_status,default$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@! grep -q '^newmark_beta,' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@! grep -q '^newmark_beta_source_status,' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_max_iterations,1$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_iterations_last,0$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_converged,1$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_scheme,newmark_unconstrained_direct$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_reason,unconstrained_direct$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_num_equations_last,0$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_tolerance_last,0.0000000000000000e+00$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_mode,constraint_residual_l2$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_l2_last,0.0000000000000000e+00$$' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^body,0,0,2.0000000000000000e+00,4.0000000000000000e+00,1.0500000000000001e-01,-2.1000000000000002e-01,5.1250000000000004e-02,1.1000000000000001e+00,-2.2000000000000002e+00,5.2500000000000002e-01,4.0000000000000000e+00,-8.0000000000000000e+00,2.0000000000000000e+00,0' $(MBD_NEWMARK2D_SYSTEM_SMOKE_OUTPUT)

# A-13 console smoke: Newmark stdout exposes generalized force history markers
mbd_system2d_newmark_console_history_smoke: $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) newmark"
	@./$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) newmark

# A-14 groundwork smoke: constrained Newmark stdout exposes both body history markers
mbd_system2d_newmark_constrained_console_history_smoke: $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) newmark_constrained"
	@./$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) newmark_constrained

# A-14 groundwork smoke: constrained Newmark summary file exposes both body history rows via direct probe
mbd_system2d_newmark_constrained_history_output_smoke: $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET) newmark"
	@./$(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET) newmark

# B-06 groundwork smoke: constrained 2-body newmark single-step path (history rows are covered by direct probes)
mbd_system2d_newmark_constrained_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^integrator,newmark_beta' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^step_execution_mode,newmark_kkt' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^steps_executed,1' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^constraint_residual_tol,1.0000000000000001e-01$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_param_beta_source_status,default$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_param_gamma_source_status,default$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_param_alpha_source_status,default$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@! grep -q '^newmark_beta_source_status,' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_max_iterations,1$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_iterations_last,1$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_converged,1$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_scheme,newmark_constrained_single_kkt$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_reason,residual_tolerance$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_num_equations_last,3$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_tolerance_last,1.0000000000000001e-01$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_residual_mode,constraint_residual_l2$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@awk -F, '$$1=="implicit_result_residual_l2_last"{found=1; exit !(($$2+0.0) <= 1.0e-01)} END{exit found?0:1}' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^implicit_result_max_iterations_source_status,cli$$' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-01)} END{exit found?0:1}' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)
	@grep -q '^body,0,0,2.0000000000000000e+00,3.0000000000000000e+00,4.1666666666666960e-08,-2.5808306332653322e-06,-4.2776877755110805e-08,8.3333333333333927e-05,-5.1616612665306644e-03,-8.5553755510221616e-05,' $(MBD_NEWMARK2D_CONSTRAINED_SMOKE_OUTPUT)

# B-09 groundwork smoke: constrained Newmark applies optional position projection
mbd_system2d_newmark_projection_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_POSITION_PROJECTION=1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_enabled,1$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_applied,1$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_source_status,env$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_stop_reason"{reason=$$2; found=1} END{exit (found && reason!="disabled" && reason!="no_constraints" && reason!="not_applied")?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_max_iterations,4$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_max_iterations_source_status,default$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance,9.9999999999999998e-13$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance_source_status,default$$' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_target_reached"{target=$$2+0; found_t=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_a=1} $$1=="position_projection_residual_tolerance"{tol=$$2+0.0; found_tol=1} END{exit (found_t && found_a && found_tol && ((target==1 && after <= tol) || (target==0 && after > tol)))?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_iterations_last"{found=1; exit !(($$2+0) > 0)} END{exit found?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after <= 1.0e-03)?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_reduction_ratio_last"{found=1; exit !(($$2+0.0) > 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_velocity_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after >= 0.0)?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_reduction_ratio_last"{found=1; exit !(($$2+0.0) >= 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-03)} END{exit found?0:1}' /tmp/fem4c_mbd_newmark_projection_smoke.out

# B-09 smoke: projection knob invalid/out-of-range fallback contract
mbd_system2d_projection_option_fallback_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_POSITION_PROJECTION=1 \
	 FEM4C_MBD_POSITION_PROJECTION_MAX_ITERS=banana \
	 FEM4C_MBD_POSITION_PROJECTION_RESIDUAL_TOL=banana \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@grep -q '^position_projection_max_iterations,4$$' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@grep -q '^position_projection_max_iterations_source_status,env_invalid_fallback$$' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@grep -q '^position_projection_residual_tolerance,9.9999999999999998e-13$$' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@grep -q '^position_projection_residual_tolerance_source_status,env_invalid_fallback$$' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@awk -F, '$$1=="position_projection_stop_reason"{reason=$$2; found=1} END{exit (found && reason!="disabled" && reason!="no_constraints" && reason!="not_applied")?0:1}' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@awk -F, '$$1=="position_projection_velocity_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_velocity_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after >= 0.0)?0:1}' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@awk -F, '$$1=="position_projection_velocity_reduction_ratio_last"{found=1; exit !(($$2+0.0) >= 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_projection_option_fallback_invalid.out
	@$(MAKE) --no-print-directory mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_POSITION_PROJECTION=1 \
	 FEM4C_MBD_POSITION_PROJECTION_MAX_ITERS=0 \
	 FEM4C_MBD_POSITION_PROJECTION_RESIDUAL_TOL=0 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_projection_option_fallback_range.out
	@grep -q '^position_projection_max_iterations,4$$' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@grep -q '^position_projection_max_iterations_source_status,env_out_of_range_fallback$$' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@grep -q '^position_projection_residual_tolerance,9.9999999999999998e-13$$' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@grep -q '^position_projection_residual_tolerance_source_status,env_out_of_range_fallback$$' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@awk -F, '$$1=="position_projection_stop_reason"{reason=$$2; found=1} END{exit (found && reason!="disabled" && reason!="no_constraints" && reason!="not_applied")?0:1}' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@awk -F, '$$1=="position_projection_velocity_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_velocity_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after >= 0.0)?0:1}' /tmp/fem4c_mbd_projection_option_fallback_range.out
	@awk -F, '$$1=="position_projection_velocity_reduction_ratio_last"{found=1; exit !(($$2+0.0) >= 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_projection_option_fallback_range.out

# B-09 static contract: every local CLI smoke must depend on mbd_system2d_cli_ready
mbd_system2d_cli_ready_contract_smoke:
	@bash $(MBD_SYSTEM2D_CLI_READY_CONTRACT_TEST_SCRIPT) Makefile

# B-09 static contract: foundation smoke must also pass from fresh local build/output dirs
mbd_b_team_foundation_isolated_smoke:
	@bash $(MBD_B_TEAM_FOUNDATION_ISOLATED_TEST_SCRIPT)

# B-09 static contract: 20-step projection compare must also pass from fresh local build/output dirs
mbd_system2d_projection_long_compare_isolated_smoke:
	@bash $(MBD_SYSTEM2D_PROJECTION_LONG_COMPARE_ISOLATED_TEST_SCRIPT)

# B-09 static contract: 20-step projection history must also pass from fresh local build/output dirs
mbd_system2d_projection_long_history_isolated_smoke:
	@bash $(MBD_SYSTEM2D_PROJECTION_LONG_HISTORY_ISOLATED_TEST_SCRIPT)

# B-09 static contract: history CSV field-count surface must stay synchronized across output/probes
mbd_output2d_history_field_count_sync_smoke:
	@bash $(MBD_OUTPUT2D_HISTORY_FIELD_COUNT_SYNC_TEST_SCRIPT)

# B-11 static contract: rigid-compare CSV header helper must stay single-source
mbd_output2d_rigid_compare_header_single_source_smoke:
	@bash $(MBD_OUTPUT2D_RIGID_COMPARE_HEADER_SINGLE_SOURCE_TEST_SCRIPT)

# B-11 static contract: rigid-compare CSV field-count surface must stay synchronized across output/probes
mbd_output2d_rigid_compare_field_count_sync_smoke:
	@bash $(MBD_OUTPUT2D_RIGID_COMPARE_FIELD_COUNT_SYNC_TEST_SCRIPT)

# B-11 static contract: rigid analytic review wrappers must share the same compare columns helper
mbd_rigid_compare_review_columns_sync_smoke:
	@bash $(MBD_RIGID_REVIEW_COLUMNS_SYNC_TEST_SCRIPT)

compare_2link_artifact_route_fields_getter_test:
	@bash $(COMPARE_2LINK_ARTIFACT_ROUTE_FIELDS_GETTER_TEST_SCRIPT)

compare_2link_artifact_targets_getter_test:
	@bash $(COMPARE_2LINK_ARTIFACT_TARGETS_GETTER_TEST_SCRIPT)

compare_2link_artifact_targets_sync_test:
	@bash $(COMPARE_2LINK_ARTIFACT_TARGETS_SYNC_TEST_SCRIPT)

compare_2link_artifact_integrators_getter_test:
	@bash $(COMPARE_2LINK_ARTIFACT_INTEGRATORS_GETTER_TEST_SCRIPT)

compare_2link_artifact_integrators_sync_test:
	@bash $(COMPARE_2LINK_ARTIFACT_INTEGRATORS_SYNC_TEST_SCRIPT)

compare_2link_artifact_route_fields_sync_test:
	@bash $(COMPARE_2LINK_ARTIFACT_ROUTE_FIELDS_SYNC_TEST_SCRIPT)

# B-06 negative smoke: constrained Newmark must fail when implicit iteration budget is zero
mbd_system2d_newmark_iteration_guard_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@rm -f /tmp/fem4c_mbd_newmark_iter_guard.out /tmp/fem4c_mbd_newmark_iter_guard.err
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=0 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_newmark_iter_guard.out \
	 >/tmp/fem4c_mbd_newmark_iter_guard.stdout 2>/tmp/fem4c_mbd_newmark_iter_guard.err && exit 1 || true
	@grep -q 'newmark implicit step exceeded max iterations 0 before convergence' /tmp/fem4c_mbd_newmark_iter_guard.err

# B-06 smoke: implicit iteration env fallback contract
mbd_system2d_newmark_iteration_fallback_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=banana \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_newmark_iter_fallback_invalid.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_newmark_iter_fallback_invalid.out
	@grep -q '^implicit_result_max_iterations_source_status,env_invalid_fallback$$' /tmp/fem4c_mbd_newmark_iter_fallback_invalid.out
	@$(MAKE) --no-print-directory mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=newmark_beta \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=-1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_newmark_iter_fallback_range.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_newmark_iter_fallback_range.out
	@grep -q '^implicit_result_max_iterations_source_status,env_out_of_range_fallback$$' /tmp/fem4c_mbd_newmark_iter_fallback_range.out

# B-08 groundwork smoke: HHT single-body path (history rows are covered by direct probes)
mbd_system2d_hht_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_NEWMARK2D_SYSTEM_SMOKE_INPUT) /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^integrator,hht_alpha' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^step_execution_mode,hht_free' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^hht_force_history_mode,system_owned_current_previous$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^steps_executed,1' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_beta,' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_gamma,' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_alpha,' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_beta_source_status,default$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_gamma_source_status,default$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_param_alpha_source_status,env$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@! grep -q '^newmark_beta,' /tmp/fem4c_mbd_hht_system_smoke.out
	@! grep -q '^newmark_gamma,' /tmp/fem4c_mbd_hht_system_smoke.out
	@! grep -q '^newmark_beta_source_status,' /tmp/fem4c_mbd_hht_system_smoke.out
	@! grep -q '^newmark_gamma_source_status,' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_iterations_last,0$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_converged,1$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_scheme,hht_unconstrained_direct$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_reason,unconstrained_direct$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_residual_num_equations_last,0$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_residual_tolerance_last,0.0000000000000000e+00$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_residual_mode,hht_effective_residual_l2$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^implicit_result_residual_l2_last,0.0000000000000000e+00$$' /tmp/fem4c_mbd_hht_system_smoke.out
	@grep -q '^body,0,0,2.0000000000000000e+00,4.0000000000000000e+00,1.0551250000000001e-01,-2.1102500000000002e-01,5.1378125000000004e-02,1.1100000000000001e+00,-2.2200000000000002e+00,5.2749999999999997e-01,4.0000000000000000e+00,-8.0000000000000000e+00,2.0000000000000000e+00,0' /tmp/fem4c_mbd_hht_system_smoke.out

# A-13 console smoke: HHT stdout exposes generalized force history markers
mbd_system2d_hht_console_history_smoke: $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) hht"
	@./$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) hht

# A-14 groundwork smoke: constrained HHT stdout exposes both body history markers
mbd_system2d_hht_constrained_console_history_smoke: $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) hht_constrained"
	@./$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) hht_constrained

# A-14 groundwork smoke: constrained HHT summary file exposes both body history rows via direct probe
mbd_system2d_hht_constrained_history_output_smoke: $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET) hht"
	@./$(MBD_SYSTEM2D_CONSTRAINED_HISTORY_OUTPUT_PROBE_TARGET) hht

# B-08 groundwork smoke: HHT constrained 2-body path (history rows are covered by direct probes)
mbd_system2d_hht_constrained_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^integrator,hht_alpha' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^step_execution_mode,hht_kkt' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^hht_force_history_mode,system_owned_current_previous$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^steps_executed,1' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^constraint_residual_tol,1.0000000000000001e-01$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_param_beta_source_status,default$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_param_gamma_source_status,default$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_param_alpha_source_status,env$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@! grep -q '^newmark_beta_source_status,' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@! grep -q '^newmark_gamma_source_status,' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_iterations_last,1$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_converged,1$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_scheme,hht_modified_newton_effective$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_reason,residual_tolerance$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_residual_num_equations_last,3$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_residual_tolerance_last,1.0000000000000001e-01$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_residual_mode,hht_effective_residual_l2$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@awk -F, '$$1=="implicit_result_residual_l2_last"{found=1; exit !(($$2+0.0) <= 1.0e-01)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^implicit_result_max_iterations_source_status,cli$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^gravity_enabled,1$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^gravity_y,-9.8100000000000005e+00$$' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-01)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^body,0,0,2.0000000000000000e+00,3.0000000000000000e+00,4.5937500000000328e-08,-2.8453657731750289e-06,-4.7161507725009666e-08,9.1666666666667318e-05,-5.6778273931837311e-03,-9.4109131061243784e-05,' /tmp/fem4c_mbd_hht_constrained_smoke.out
	@grep -q '^body,1,1,1.0000000000000000e+00,1.0000000000000000e+00,1.0000000459375000e+00,-2.6965372036499422e-06,9.9999655181353231e-02,9.1666666666665367e-05,-5.3808452136325374e-03,-6.8807349016368426e-04,' /tmp/fem4c_mbd_hht_constrained_smoke.out

# B-09 groundwork smoke: constrained HHT applies optional position projection
mbd_system2d_hht_projection_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=1 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_POSITION_PROJECTION=1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_enabled,1$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_applied,1$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_source_status,env$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_stop_reason"{reason=$$2; found=1} END{exit (found && reason!="disabled" && reason!="no_constraints" && reason!="not_applied")?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_max_iterations,4$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_max_iterations_source_status,default$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance,9.9999999999999998e-13$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance_source_status,default$$' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_target_reached"{target=$$2+0; found_t=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_a=1} $$1=="position_projection_residual_tolerance"{tol=$$2+0.0; found_tol=1} END{exit (found_t && found_a && found_tol && ((target==1 && after <= tol) || (target==0 && after > tol)))?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_iterations_last"{found=1; exit !(($$2+0) > 0)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after <= 1.0e-03)?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_reduction_ratio_last"{found=1; exit !(($$2+0.0) > 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_velocity_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after >= 0.0)?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_reduction_ratio_last"{found=1; exit !(($$2+0.0) >= 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-03)} END{exit found?0:1}' /tmp/fem4c_mbd_hht_projection_smoke.out

# B-08 negative smoke: constrained HHT must fail when implicit iteration budget is zero
mbd_system2d_hht_iteration_guard_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@rm -f /tmp/fem4c_mbd_hht_iter_guard.out /tmp/fem4c_mbd_hht_iter_guard.err
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=0 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS_SOURCE=cli \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_hht_iter_guard.out \
	 >/tmp/fem4c_mbd_hht_iter_guard.stdout 2>/tmp/fem4c_mbd_hht_iter_guard.err && exit 1 || true
	@grep -q 'hht implicit step exceeded max iterations 0 before convergence' /tmp/fem4c_mbd_hht_iter_guard.err

# B-08 smoke: implicit iteration env fallback contract for HHT
mbd_system2d_hht_iteration_fallback_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=banana \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_hht_iter_fallback_invalid.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_hht_iter_fallback_invalid.out
	@grep -q '^implicit_result_max_iterations_source_status,env_invalid_fallback$$' /tmp/fem4c_mbd_hht_iter_fallback_invalid.out
	@$(MAKE) --no-print-directory mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=hht_alpha \
	 FEM4C_MBD_HHT_ALPHA=-0.05 \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_IMPLICIT_MAX_ITERS=-1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_hht_iter_fallback_range.out
	@grep -q '^implicit_result_max_iterations,1$$' /tmp/fem4c_mbd_hht_iter_fallback_range.out
	@grep -q '^implicit_result_max_iterations_source_status,env_out_of_range_fallback$$' /tmp/fem4c_mbd_hht_iter_fallback_range.out

# B-09 groundwork smoke: constrained explicit step applies optional position projection
mbd_system2d_explicit_projection_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=explicit \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_POSITION_PROJECTION=1 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_enabled,1$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_applied,1$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_source_status,env$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_stop_reason"{reason=$$2; found=1} END{exit (found && reason!="disabled" && reason!="no_constraints" && reason!="not_applied")?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_max_iterations,4$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_max_iterations_source_status,default$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance,9.9999999999999998e-13$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@grep -q '^position_projection_residual_tolerance_source_status,default$$' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_target_reached"{target=$$2+0; found_t=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_a=1} $$1=="position_projection_residual_tolerance"{tol=$$2+0.0; found_tol=1} END{exit (found_t && found_a && found_tol && ((target==1 && after <= tol) || (target==0 && after > tol)))?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_iterations_last"{found=1; exit !(($$2+0) > 0)} END{exit found?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after <= 1.0e-03)?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_residual_reduction_ratio_last"{found=1; exit !(($$2+0.0) > 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_residual_l2_before"{before=$$2+0.0; found_before=1} $$1=="position_projection_velocity_residual_l2_after"{after=$$2+0.0; found_after=1} END{exit (found_before && found_after && before > after && after >= 0.0)?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="position_projection_velocity_reduction_ratio_last"{found=1; exit !(($$2+0.0) >= 0.0 && ($$2+0.0) < 1.0)} END{exit found?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-03)} END{exit found?0:1}' /tmp/fem4c_mbd_explicit_projection_smoke.out

# A-06 smoke: constrained 2-body explicit step path (history rows are covered by direct probes)
mbd_system2d_explicit_smoke: $(TARGET)
	@FEM4C_ANALYSIS_MODE=mbd \
	 FEM4C_MBD_INTEGRATOR=explicit \
	 FEM4C_MBD_DT=0.001 \
	 FEM4C_MBD_STEPS=1 \
	 ./$(TARGET) $(MBD_EXPLICIT2D_SYSTEM_SMOKE_INPUT) $(MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^integrator,explicit' $(MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^step_execution_mode,explicit_kkt' $(MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^steps_executed,1' $(MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT)
	@grep -q '^gravity_enabled,1' $(MBD_EXPLICIT2D_SYSTEM_SMOKE_OUTPUT)

# A-13 console smoke: explicit stdout keeps history invalid and omits current/previous rows
mbd_system2d_explicit_console_history_smoke: $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET)
	@echo "Running $(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) explicit"
	@./$(MBD_SYSTEM2D_CONSOLE_HISTORY_PROBE_TARGET) explicit

# B-team rigid MBD probe smoke pack
mbd_b_team_foundation_probe_smoke: mbd_assembler2d_probe_smoke \
                                   mbd_dense_solver_probe_smoke \
                                   mbd_dense_solver_singular_smoke \
                                   mbd_dense_solver_invalid_smoke \
                                   mbd_constraint_rhs_probe_smoke \
                                   mbd_projection2d_probe_smoke \
                                   mbd_system2d_projection_compare_smoke \
                                   mbd_system2d_projection_long_compare_smoke \
                                   mbd_system2d_projection_history_output_smoke \
                                   mbd_system2d_projection_long_history_output_smoke \
                                   mbd_output2d_history_field_count_sync_smoke \
                                   mbd_output2d_rigid_compare_header_single_source_smoke \
                                   mbd_output2d_rigid_compare_field_count_sync_smoke \
                                   mbd_system2d_cli_ready_contract_smoke \
                                   mbd_system2d_projection_option_fallback_smoke \
                                   mbd_newmark2d_smoke \
                                   mbd_system2d_newmark_probe_smoke \
                                   mbd_system2d_newmark_iteration_guard_smoke \
                                   mbd_system2d_newmark_iteration_fallback_smoke \
                                   mbd_hht2d_probe_smoke \
                                   mbd_hht2d_invalid_smoke \
                                   mbd_system2d_force_history_smoke \
                                   mbd_system2d_hht_probe_smoke \
                                   mbd_system2d_hht_iteration_guard_smoke \
                                   mbd_system2d_hht_iteration_fallback_smoke
	@echo "PASS: B-team rigid MBD probe smoke pack"

# B-team rigid MBD foundation smoke pack
mbd_b_team_foundation_smoke: mbd_assembler2d_probe_smoke \
                             mbd_assembler2d_smoke \
                             mbd_dense_solver_probe_smoke \
                             mbd_dense_solver_smoke \
                             mbd_dense_solver_singular_smoke \
                             mbd_dense_solver_invalid_smoke \
                             mbd_constraint_rhs_probe_smoke \
                             mbd_projection2d_probe_smoke \
                             mbd_system2d_projection_compare_smoke \
                             mbd_system2d_projection_long_compare_smoke \
                             mbd_system2d_projection_history_output_smoke \
                             mbd_system2d_projection_long_history_output_smoke \
                             mbd_output2d_history_field_count_sync_smoke \
                             mbd_output2d_rigid_compare_header_single_source_smoke \
                             mbd_output2d_rigid_compare_field_count_sync_smoke \
                             mbd_system2d_cli_ready_contract_smoke \
                             mbd_system2d_projection_option_fallback_smoke \
                             mbd_system2d_explicit_projection_smoke \
                             mbd_newmark2d_smoke \
                             mbd_system2d_newmark_probe_smoke \
                             mbd_system2d_newmark_history_output_smoke \
                             mbd_system2d_newmark_smoke \
                             mbd_system2d_newmark_constrained_smoke \
                             mbd_system2d_newmark_projection_smoke \
                             mbd_system2d_newmark_iteration_guard_smoke \
                             mbd_system2d_newmark_iteration_fallback_smoke \
                             mbd_hht2d_probe_smoke \
                             mbd_hht2d_invalid_smoke \
                             mbd_system2d_force_history_smoke \
                             mbd_system2d_hht_history_output_smoke \
                             mbd_system2d_hht_smoke \
                             mbd_system2d_hht_constrained_smoke \
                             mbd_system2d_hht_projection_smoke \
                             mbd_system2d_hht_iteration_guard_smoke \
                             mbd_system2d_hht_iteration_fallback_smoke
	@echo "PASS: B-team rigid MBD foundation smoke pack"

# B-01 smoke: system-owned body/constraint/gravity/time path
mbd_system2d_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) $(MBD_SYSTEM2D_SMOKE_OUTPUT)

# B-02 smoke: dense KKT matrix/RHS output path
mbd_assembler2d_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd FEM4C_MBD_INTEGRATOR=explicit FEM4C_MBD_STEPS=0 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^constraint_row_offset,2,3$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^constraint_residual,1,-2.4979173609871452e-03$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^constraint_phi_dot,1,0.0000000000000000e+00$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^constraint_gamma_rhs,2,-1.2479177080853519e+00$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^constraint_residual_tol,1.0000000000000001e-01$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^kkt_matrix,0,6,-1.0000000000000000e+00$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^kkt_matrix,5,8,4.9750208263901291e-01$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^kkt_rhs,4,-1.0810000000000000e+01$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^kkt_rhs,8,-1.2479177080853519e+00$$' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@awk -F, '$$1=="residual_l2"{found=1; exit !(($$2+0.0) <= 1.0e-01)} END{exit found?0:1}' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^steps_requested,0' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)
	@grep -q '^step_execution_mode,none' $(MBD_ASSEMBLER2D_SMOKE_OUTPUT)

# B-03 smoke: dense KKT solve output path
mbd_dense_solver_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd FEM4C_MBD_INTEGRATOR=explicit FEM4C_MBD_STEPS=0 FEM4C_MBD_SOLVE_DENSE_KKT=1 ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@grep -q '^kkt_solver_status,ok' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@awk -F, 'function abs(x){return x<0?-x:x} $$1=="kkt_solution" && $$2=="0"{found=1; exit !(abs($$3-(1.6666666666666785e-01)) <= 1.0e-12)} END{exit found?0:1}' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@awk -F, 'function abs(x){return x<0?-x:x} $$1=="kkt_solution" && $$2=="6"{found=1; exit !(abs($$3-(-1.9505407723975541e+01)) <= 1.0e-12)} END{exit found?0:1}' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@awk -F, '$$1=="kkt_solve_residual_inf"{found=1; exit !(($$2+0.0) <= 1.0e-12)} END{exit found?0:1}' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@grep -q '^steps_requested,0' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)
	@grep -q '^step_execution_mode,none' $(MBD_DENSE_SOLVER_SMOKE_OUTPUT)

# B-04 groundwork smoke: constraint residual / gamma markers
mbd_constraint_rhs_smoke: $(MBD_SYSTEM2D_CLI_TARGET) mbd_system2d_cli_ready
	@FEM4C_ANALYSIS_MODE=mbd FEM4C_MBD_INTEGRATOR=explicit ./$(MBD_SYSTEM2D_CLI_TARGET) $(MBD_SYSTEM2D_SMOKE_INPUT) /tmp/fem4c_mbd_constraint_rhs_smoke.out
	@grep -q '^constraint_residual,0,' /tmp/fem4c_mbd_constraint_rhs_smoke.out
	@grep -q '^constraint_phi_dot,0,' /tmp/fem4c_mbd_constraint_rhs_smoke.out
	@grep -q '^constraint_gamma_rhs,0,' /tmp/fem4c_mbd_constraint_rhs_smoke.out

# Targeted smoke for globals snapshot clone/restore path
fem_model_copy_test:
	@bash $(FEM_MODEL_COPY_TEST_SCRIPT)

# Targeted smoke for runtime BC override / node-set interpolation path
flex_bc2d_test:
	@bash $(FLEX_BC2D_TEST_SCRIPT)

# Targeted smoke for snapshot inertial-load injection path
flex_body2d_inertial_test:
	@bash $(FLEX_BODY2D_INERTIAL_TEST_SCRIPT)

# Targeted smoke for deformed interface-center helpers
flex_body2d_interface_center_test:
	@bash $(FLEX_BODY2D_INTERFACE_CENTER_TEST_SCRIPT)

# Targeted smoke for root/tip marker displacement path
flex_body2d_marker_test:
	@bash $(FLEX_BODY2D_MARKER_TEST_SCRIPT)

# Targeted smoke for node-set utility module
flex_nodeset_test:
	@bash $(FLEX_NODESET_TEST_SCRIPT)

# Targeted smoke for snapshot filename/world-space export path
flex_snapshot2d_test:
	@bash $(FLEX_SNAPSHOT2D_TEST_SCRIPT)

# Targeted smoke for coupled-step helper extraction
coupled_step_common2d_test:
	@bash $(COUPLED_STEP_COMMON2D_TEST_SCRIPT)

# Targeted smoke for coupled-step shared solve path
coupled_step_common2d_solve_test:
	@bash $(COUPLED_STEP_COMMON2D_SOLVE_TEST_SCRIPT)

# Focused review-spec bundle for C-R1/C-R2 follow-up checks
c_review_spec_smoke_test:
	@$(MAKE) --no-print-directory q4_t3_stiffness_adapter_warning_test
	@$(MAKE) --no-print-directory coupled_step_common2d_test
	@$(MAKE) --no-print-directory coupled_step_common2d_solve_test

# Targeted smoke for model-centric assembly wrapper
flex_solver2d_test:
	@bash $(FLEX_SOLVER2D_TEST_SCRIPT)

# Targeted smoke for coupled output reassembly counter audit
coupled_reassembly_log_test: $(TARGET)
	@bash $(COUPLED_REASSEMBLY_LOG_TEST_SCRIPT)

# Targeted smoke for accepted-step snapshot file output
coupled_snapshot_output_test: $(TARGET)
	@bash $(COUPLED_SNAPSHOT_OUTPUT_TEST_SCRIPT)

# Targeted compile smoke for Q4/T3 stiffness adapters
q4_t3_stiffness_adapter_warning_test:
	@bash $(Q4_T3_STIFFNESS_ADAPTER_WARNING_TEST_SCRIPT)

# Targeted smoke for implicit accepted-step snapshot file output
coupled_implicit_snapshot_output_test: $(TARGET)
	@bash $(COUPLED_IMPLICIT_SNAPSHOT_OUTPUT_TEST_SCRIPT)

# Targeted smoke for duplicate node-set fail-fast guard
coupled_nodeset_guard_test: $(TARGET)
	@bash $(COUPLED_NODESET_GUARD_TEST_SCRIPT)

# One-command regression for MBD mode input adapter path
mbd_regression: $(TARGET)
	@echo "Running MBD mode regression script"
	@bash $(MBD_REGRESSION_SCRIPT)

# Run the top-level M1 rigid acceptance route (rigid main cases + analytic compare)
mbd_m1_rigid_acceptance: $(TARGET)
	@echo "Running M1 rigid acceptance route"
	@MBD_REGRESSION_SCOPE=rigid_main bash $(MBD_REGRESSION_SCRIPT)
	@$(MAKE) mbd_rigid_analytic_compare_test
	@$(MAKE) mbd_rigid_analytic_hht_compare_test

# Verify the top-level M1 rigid acceptance route stays stable
mbd_m1_rigid_acceptance_test:
	@bash $(MBD_M1_RIGID_ACCEPTANCE_TEST_SCRIPT)

# Consistency check between mbd runtime log and probe equation count
mbd_consistency: $(TARGET) $(MBD_PROBE_TARGET)
	@echo "Running MBD equation consistency check"
	@FEM4C_MBD_INTEGRATOR=explicit bash $(MBD_CONSISTENCY_SCRIPT)

# Negative-path check for invalid MBD_* inputs plus accepted body-only unconstrained case
mbd_negative: $(TARGET)
	@echo "Running invalid-input diagnostics check"
	@bash $(MBD_NEGATIVE_SCRIPT)

# Run all MBD checks in one command
mbd_checks: mbd_probe mbd_regression mbd_consistency mbd_negative mbd_integrator_checks
	@echo "PASS: all MBD checks completed"

# Verify Run 1 MBD docs/help stay synchronized with current command-surface roles
mbd_run1_surface_docs_sync_test:
	@bash $(MBD_RUN1_SURFACE_DOCS_SYNC_TEST_SCRIPT)

# Verify the Run 1 docs-sync validator help/inventory surfaces stay stable
mbd_run1_surface_docs_sync_surfaces_test:
	@bash $(MBD_RUN1_SURFACE_DOCS_SYNC_SURFACES_TEST_SCRIPT)

# Verify the Run 1 docs-sync validator help targets stay visible on make help
mbd_run1_surface_docs_sync_surfaces_help_test:
	@bash $(MBD_RUN1_SURFACE_DOCS_SYNC_SURFACES_HELP_TEST_SCRIPT)

# Run the focused Run 1 docs-sync surface smoke pack
mbd_run1_surface_docs_sync_surface_smoke:
	@$(MAKE) mbd_run1_surface_docs_sync_test
	@$(MAKE) mbd_run1_surface_docs_sync_surfaces_help_test
	@$(MAKE) mbd_run1_surface_docs_sync_surfaces_test
	@echo "PASS: mbd_run1_surface_docs_sync_surface_smoke bundle target"

# Verify the focused Run 1 docs-sync surface smoke pack stays stable
mbd_run1_surface_docs_sync_surface_smoke_test:
	@bash $(MBD_RUN1_SURFACE_DOCS_SYNC_SURFACE_SMOKE_TEST_SCRIPT)

# Fetch latest GitHub Actions CI evidence for FEM4C step
mbd_ci_evidence:
	@python3 $(MBD_CI_EVIDENCE_SCRIPT) \
		--repo RyotaMaeno1227/OSS_CAE_FEM_salome \
		--workflow ci.yaml \
		--scan-runs $(if $(SCAN_RUNS),$(SCAN_RUNS),20) \
		$(if $(RUN_ID),--run-id $(RUN_ID),) \
		--strict-acceptance

# Check CI workflow contract locally (no run_id dependency)
mbd_ci_contract:
	@bash $(MBD_CI_CONTRACT_SCRIPT)

# Strict lock_wait_max sync check (for post-conflict final alignment)
mbd_ci_contract_lock_wait_sync:
	@FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC=1 \
	 FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=strict_present \
	 bash $(MBD_CI_CONTRACT_SCRIPT)

# Strict lock_wait_max check-only contract (during test-side edit freeze)
mbd_ci_contract_lock_wait_check_only:
	@FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=compat \
	 bash $(MBD_CI_CONTRACT_SCRIPT)

# One-command B-14 regression (mbd entry integration + contract + self-tests)
mbd_b14_regression:
	@bash $(MBD_B14_REGRESSION_SCRIPT)

# One-command B-8 regression (contract + self-tests + guard with b14 chain)
mbd_b8_regression:
	@B8_MAKE_CMD=$(if $(B8_MAKE_CMD),$(B8_MAKE_CMD),make) \
	 B8_RUN_B14_REGRESSION=$(if $(B8_RUN_B14_REGRESSION),$(B8_RUN_B14_REGRESSION),1) \
	 bash $(MBD_B8_REGRESSION_SCRIPT)

# Self-test for B-8 regression wrapper (pass + expected fail path)
mbd_b8_regression_test:
	@bash $(MBD_B8_REGRESSION_TEST_SCRIPT)

# One-command B-8 full regression from clean rebuild
mbd_b8_regression_full:
	@B8_MAKE_CMD=$(if $(B8_MAKE_CMD),$(B8_MAKE_CMD),make) \
	 B8_RUN_B14_REGRESSION=$(if $(B8_RUN_B14_REGRESSION),$(B8_RUN_B14_REGRESSION),1) \
	 bash $(MBD_B8_REGRESSION_FULL_SCRIPT)

# Self-test for B-8 full regression wrapper (pass + expected fail path)
mbd_b8_regression_full_test:
	@bash $(MBD_B8_REGRESSION_FULL_TEST_SCRIPT)

# B-8 knob matrix test (B8_RUN_B14_REGRESSION and B8_MAKE_CMD)
mbd_b8_knob_matrix_test:
	@bash $(MBD_B8_KNOB_MATRIX_TEST_SCRIPT)

# B-8 knob matrix smoke test (skip full rebuild cases)
mbd_b8_knob_matrix_smoke_test:
	@B8_KNOB_MATRIX_SKIP_FULL=1 bash $(MBD_B8_KNOB_MATRIX_TEST_SCRIPT)

# One-command A-21 regression (MBD time/source static contract + runtime checks)
mbd_a21_regression:
	@bash $(MBD_A21_REGRESSION_SCRIPT)

# Self-test for A-21 regression wrapper (pass + expected fail path)
mbd_a21_regression_test:
	@bash $(MBD_A21_REGRESSION_TEST_SCRIPT)

# One-command A-24 regression (MBD step trace runtime+static contract + self-tests)
mbd_a24_regression:
	@bash $(MBD_A24_REGRESSION_SCRIPT)

# Self-test for A-24 regression wrapper (pass + expected fail path)
mbd_a24_regression_test:
	@bash $(MBD_A24_REGRESSION_TEST_SCRIPT)

# One-command A-24 full regression from clean rebuild
mbd_a24_regression_full:
	@bash $(MBD_A24_REGRESSION_FULL_SCRIPT)

# Self-test for A-24 full regression wrapper (pass + expected fail path)
mbd_a24_regression_full_test:
	@bash $(MBD_A24_REGRESSION_FULL_TEST_SCRIPT)

# One-command A-24 serial batch (regression + tests)
mbd_a24_batch:
	@bash $(MBD_A24_BATCH_SCRIPT)

# Self-test for A-24 batch wrapper (pass + expected fail path)
mbd_a24_batch_test:
	@bash $(MBD_A24_BATCH_TEST_SCRIPT)

# One-command serial acceptance for A-24 wrappers (full_test -> batch_test -> ci_contract_test)
mbd_a24_acceptance_serial:
	@bash $(MBD_A24_ACCEPTANCE_SERIAL_SCRIPT)

# Self-test for A-24 serial acceptance wrapper (pass + expected fail path)
mbd_a24_acceptance_serial_test:
	@bash $(MBD_A24_ACCEPTANCE_SERIAL_TEST_SCRIPT)

# Self-test for CI contract checker (pass + expected fail path)
mbd_ci_contract_test:
	@bash -n $(MBD_CI_CONTRACT_TEST_SCRIPT)
	@bash -n $(MBD_CI_CONTRACT_SCRIPT)
	@FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_REQUIRE_SYNC=1 \
	 FEM4C_CI_CONTRACT_LOCK_WAIT_MAX_MODE=strict_present \
	 bash $(MBD_B45_SHA_WATCH_SCRIPT) FEM4C/scripts/test_check_ci_contract.sh \
	 bash FEM4C/$(MBD_CI_CONTRACT_TEST_SCRIPT)
	@bash $(FEM4C_TEST_LOG_MARKER_TEST_SCRIPT)

# Team acceptance gate (elapsed/timer/evidence/impl-change)
mbd_team_acceptance_gate:
	@cd .. && bash $(TEAM_ACCEPTANCE_GATE_SCRIPT) \
		$(if $(TEAM_STATUS),$(TEAM_STATUS),docs/team_status.md) \
		$(if $(MIN_ELAPSED),$(MIN_ELAPSED),60)

# B-45 helper: run ci_contract_test under sha watch for test_check_ci_contract.sh
mbd_ci_contract_test_sha_watch:
	@bash $(MBD_B45_SHA_WATCH_SCRIPT) FEM4C/scripts/test_check_ci_contract.sh \
	 make -C FEM4C mbd_ci_contract_test

# B-45 acceptance helper: run required four commands with sha trace around them
mbd_b45_acceptance:
	@bash $(MBD_B45_ACCEPTANCE_SCRIPT)

# Verify T3 orientation behavior in default/strict modes
t3_orientation_checks: $(TARGET)
	@bash $(T3_ORIENTATION_SCRIPT)

# Verify legacy parser package + Nastran parser compatibility
parser_compat: $(TARGET)
	@bash $(PARSER_COMPAT_SCRIPT)

# Verify parser compatibility using built-in legacy fallback fixture
parser_compat_fallback: $(TARGET)
	@FEM4C_PARSER_COMPAT_FORCE_FALLBACK=1 bash $(PARSER_COMPAT_SCRIPT)

# Verify legacy no-flex coupled fallback emits contract snapshot and fails as expected
coupled_stub_check: $(TARGET)
	@bash $(COUPLED_STUB_SCRIPT)

# Verify Run 1 coupled docs/help stay synchronized with default-vs-legacy target roles
coupled_run1_surface_docs_sync_test:
	@bash $(COUPLED_RUN1_SURFACE_DOCS_SYNC_TEST_SCRIPT)

# Verify coupled one-way mainline across explicit/newmark/hht integrators
integrator_checks: $(TARGET)
	@bash $(COUPLED_INTEGRATOR_SCRIPT)

# Verify coupled 2-link flexible example inputs + rigid-limit explicit/implicit checks
coupled_example_check: $(TARGET)
	@bash $(COUPLED_EXAMPLE_SCRIPT)

# Verify explicit rigid-limit compare CSV generation against rigid 2-link reference
coupled_rigid_limit_compare_test: $(TARGET)
	@bash $(COUPLED_RIGID_LIMIT_COMPARE_TEST_SCRIPT)

# Verify rigid-limit implicit runs converge without residual abort
coupled_rigid_limit_implicit_test: $(TARGET)
	@bash $(COUPLED_RIGID_LIMIT_IMPLICIT_TEST_SCRIPT)

# Verify rigid-limit implicit compare CSVs stay within integrator-specific thresholds
coupled_rigid_limit_implicit_compare_test: $(TARGET)
	@bash $(COUPLED_RIGID_LIMIT_IMPLICIT_COMPARE_TEST_SCRIPT)

# Print the rigid-limit threshold contract used by compare/helper flows
coupled_rigid_limit_thresholds:
	@bash $(COUPLED_RIGID_LIMIT_THRESHOLD_PRINT_SCRIPT)

# Verify the rigid-limit threshold contract printer stays stable
coupled_rigid_limit_thresholds_test:
	@bash $(COUPLED_RIGID_LIMIT_THRESHOLD_PRINT_TEST_SCRIPT)

# Verify rigid-limit threshold docs stay synchronized with the helper table
coupled_rigid_limit_threshold_docs_sync_test:
	@bash $(COUPLED_RIGID_LIMIT_THRESHOLD_DOC_SYNC_TEST_SCRIPT)

# Verify compare helper consumes snapshot_record manifest before glob fallback
coupled_rigid_limit_manifest_test:
	@bash $(COUPLED_RIGID_LIMIT_MANIFEST_TEST_SCRIPT)

# Verify flexible reference helper consumes snapshot_record manifest before glob fallback
coupled_flex_manifest_test:
	@bash $(COUPLED_FLEX_MANIFEST_TEST_SCRIPT)

# Verify real coupled summary can be normalized into flexible compare schema artifacts
coupled_flex_reference_real_test:
	@bash $(COUPLED_FLEX_REFERENCE_REAL_TEST_SCRIPT)

# Verify compare mode emits compare CSV/PNG plus normalized FEM schema artifacts
coupled_flex_reference_compare_test:
	@bash $(COUPLED_FLEX_REFERENCE_COMPARE_TEST_SCRIPT)

# Verify artifact-only export emits 1-link interface-center and reaction-map CSVs
coupled_flex_reference_artifact_only_test:
	@bash $(COUPLED_FLEX_REFERENCE_ARTIFACT_ONLY_TEST_SCRIPT)

# Verify real MBD summary can be normalized into rigid compare schema artifacts
mbd_rigid_analytic_real_test:
	@bash $(MBD_RIGID_ANALYTIC_REAL_TEST_SCRIPT)

# Verify real MBD summary emits rigid compare CSV/PNG plus normalized schema artifacts
mbd_rigid_analytic_compare_test:
	@bash $(MBD_RIGID_ANALYTIC_COMPARE_TEST_SCRIPT)

# Verify HHT rigid compare emits rigid compare CSV/PNG plus normalized schema artifacts
mbd_rigid_analytic_hht_compare_test:
	@bash $(MBD_RIGID_ANALYTIC_HHT_COMPARE_TEST_SCRIPT)

# Verify rigid compare sidecar stays compare-ready for newmark/hht implicit integrators
mbd_rigid_compare_integrator_review_smoke:
	@bash $(MBD_RIGID_ANALYTIC_INTEGRATOR_COMPARE_READY_TEST_SCRIPT)

# Verify rigid normalization falls back to history geometry when compare sidecar is malformed
mbd_rigid_compare_fallback_review_smoke:
	@bash $(MBD_RIGID_ANALYTIC_FALLBACK_TEST_SCRIPT)

# Verify rigid compare route expectations fail fast when callers ask for the wrong normalization path
mbd_rigid_compare_expect_route_guard_smoke:
	@bash $(MBD_RIGID_ANALYTIC_EXPECT_ROUTE_GUARD_TEST_SCRIPT)

# Verify rigid compare fallback reason expectations fail fast when callers ask for the wrong fallback marker
mbd_rigid_compare_expect_fallback_reason_guard_smoke:
	@bash $(MBD_RIGID_ANALYTIC_EXPECT_FALLBACK_REASON_GUARD_TEST_SCRIPT)

# Verify rigid compare snapshot-count expectations fail fast when callers ask for the wrong route metadata
mbd_rigid_compare_expect_snapshot_count_guard_smoke:
	@bash $(MBD_RIGID_ANALYTIC_EXPECT_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT)

# Verify rigid compare provenance route is preserved through the matrix wrapper surface
mbd_rigid_compare_route_matrix_review_smoke:
	@$(MAKE) compare_2link_artifact_matrix_check_test

# Verify rigid compare provenance contract across wrapper, direct sidecar, and fallback routes
mbd_rigid_compare_route_review_smoke: mbd_rigid_compare_integrator_review_smoke \
                                      mbd_rigid_compare_fallback_review_smoke \
                                      mbd_rigid_compare_expect_route_guard_smoke \
                                      mbd_rigid_compare_expect_fallback_reason_guard_smoke \
                                      mbd_rigid_compare_expect_snapshot_count_guard_smoke \
                                      mbd_rigid_compare_route_matrix_review_smoke \
                                      compare_2link_artifact_route_fields_getter_test \
                                      compare_2link_artifact_targets_getter_test \
                                      compare_2link_artifact_targets_sync_test \
                                      compare_2link_artifact_integrators_getter_test \
                                      compare_2link_artifact_integrators_sync_test \
                                      compare_2link_artifact_route_fields_sync_test \
                                      compare_2link_artifact_manifest_header_contract_test \
                                      compare_2link_artifact_manifest_snapshot_count_guard_test \
                                      compare_2link_artifact_matrix_manifest_header_contract_test \
                                      compare_2link_artifact_matrix_manifest_snapshot_count_guard_test \
                                      mbd_rigid_analytic_wrapper_test \
                                      compare_2link_artifact_check_integrator_test \
                                      compare_2link_artifact_check_invalid_integrator_test
	@echo "PASS: MBD rigid compare route review smoke pack"

# Verify rigid compare supports multiple overlaid reference schema CSVs
mbd_rigid_analytic_multi_reference_test:
	@bash $(MBD_RIGID_ANALYTIC_MULTI_REFERENCE_TEST_SCRIPT)

# Verify rigid normalize/multi wrappers emit stable stdout artifact paths
mbd_rigid_analytic_wrapper_test:
	@bash $(MBD_RIGID_ANALYTIC_WRAPPER_TEST_SCRIPT)

# Verify E-08 compare artifact suite without expanding into full acceptance orchestration
compare_2link_artifact_check:
	@bash $(COMPARE_2LINK_ARTIFACT_CHECK_SCRIPT) $(if $(OUT_DIR),$(OUT_DIR),/tmp/fem4c_compare_2link_artifacts) $(if $(MANIFEST_CSV),$(MANIFEST_CSV),/tmp/fem4c_compare_2link_artifacts/compare_2link_artifacts_manifest.csv) $(if $(INTEGRATOR),$(INTEGRATOR),)

# Verify compare artifact suite stdout contract
compare_2link_artifact_check_test:
	@bash $(COMPARE_2LINK_ARTIFACT_CHECK_TEST_SCRIPT)

# Validate compare artifact suite manifest CSV contract
compare_2link_artifact_manifest_test:
	@manifest_csv="$(MANIFEST_CSV)"; \
	if [ -z "$$manifest_csv" ]; then \
		out_dir="$$(mktemp -d)"; \
		manifest_csv="$$out_dir/compare_2link_artifacts_manifest.csv"; \
		trap 'rm -rf "$$out_dir"' EXIT; \
		bash $(COMPARE_2LINK_ARTIFACT_CHECK_SCRIPT) "$$out_dir" "$$manifest_csv" >/tmp/compare_2link_artifact_manifest_test.log 2>&1; \
	fi; \
	python3 $(COMPARE_2LINK_ARTIFACT_MANIFEST_TEST_SCRIPT) "$$manifest_csv"

# Verify compare artifact manifest validator rejects reordered route header fields
compare_2link_artifact_manifest_header_contract_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MANIFEST_HEADER_CONTRACT_TEST_SCRIPT)

# Verify compare artifact manifest validator rejects mismatched snapshot counts
compare_2link_artifact_manifest_snapshot_count_guard_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MANIFEST_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT)

# Verify compare artifact suite target honors OUT_DIR/MANIFEST_CSV overrides
compare_2link_artifact_check_vars_test:
	@bash $(COMPARE_2LINK_ARTIFACT_CHECK_VARS_TEST_SCRIPT)

# Verify compare artifact suite target honors INTEGRATOR override
compare_2link_artifact_check_integrator_test:
	@bash $(COMPARE_2LINK_ARTIFACT_CHECK_INTEGRATOR_TEST_SCRIPT)

# Verify compare artifact suite fails fast on unsupported integrators
compare_2link_artifact_check_invalid_integrator_test:
	@bash $(COMPARE_2LINK_ARTIFACT_CHECK_INVALID_INTEGRATOR_TEST_SCRIPT)

# Verify compare artifact suite across explicit/newmark/hht in one matrix run
compare_2link_artifact_matrix_check:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_CHECK_SCRIPT) $(if $(OUT_DIR),$(OUT_DIR),/tmp/fem4c_compare_2link_artifact_matrix) $(if $(MANIFEST_CSV),$(MANIFEST_CSV),/tmp/fem4c_compare_2link_artifact_matrix/compare_2link_artifact_matrix_manifest.csv) $(INTEGRATORS)

# Verify compare artifact matrix stdout contract
compare_2link_artifact_matrix_check_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_TEST_SCRIPT)

# Validate compare artifact matrix manifest CSV contract
compare_2link_artifact_matrix_manifest_test:
	@manifest_csv="$(MANIFEST_CSV)"; \
	if [ -z "$$manifest_csv" ]; then \
		out_dir="$$(mktemp -d)"; \
		manifest_csv="$$out_dir/compare_2link_artifact_matrix_manifest.csv"; \
		trap 'rm -rf "$$out_dir"' EXIT; \
		bash $(COMPARE_2LINK_ARTIFACT_MATRIX_CHECK_SCRIPT) "$$out_dir" "$$manifest_csv" >/tmp/compare_2link_artifact_matrix_manifest_test.log 2>&1; \
	fi; \
	python3 $(COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_TEST_SCRIPT) "$$manifest_csv" $(if $(EXPECTED_INTEGRATORS),--expected-integrators $(EXPECTED_INTEGRATORS),)

# Verify compare artifact matrix manifest validator rejects reordered route header fields
compare_2link_artifact_matrix_manifest_header_contract_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_HEADER_CONTRACT_TEST_SCRIPT)

# Verify compare artifact matrix manifest validator rejects mismatched snapshot counts
compare_2link_artifact_matrix_manifest_snapshot_count_guard_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_SNAPSHOT_COUNT_GUARD_TEST_SCRIPT)

# Verify compare artifact matrix manifest target honors EXPECTED_INTEGRATORS overrides
compare_2link_artifact_matrix_manifest_expected_integrators_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_MANIFEST_EXPECTED_INTEGRATORS_TEST_SCRIPT)

# Verify compare artifact matrix target honors INTEGRATORS subset overrides
compare_2link_artifact_matrix_integrators_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_INTEGRATORS_TEST_SCRIPT)

# Verify compare artifact matrix fails fast on unsupported integrators
compare_2link_artifact_matrix_invalid_integrator_test:
	@bash $(COMPARE_2LINK_ARTIFACT_MATRIX_INVALID_INTEGRATOR_TEST_SCRIPT)

# Run the focused compare artifact self-test suite used by C-team
compare_2link_artifact_checks:
	@$(MAKE) compare_2link_artifact_check_test
	@$(MAKE) compare_2link_artifact_manifest_snapshot_count_guard_test
	@$(MAKE) compare_2link_artifact_check_vars_test
	@$(MAKE) compare_2link_artifact_check_integrator_test
	@$(MAKE) compare_2link_artifact_check_invalid_integrator_test
	@$(MAKE) compare_2link_artifact_matrix_check_test
	@$(MAKE) compare_2link_artifact_matrix_manifest_snapshot_count_guard_test
	@$(MAKE) compare_2link_artifact_matrix_manifest_expected_integrators_test
	@$(MAKE) compare_2link_artifact_matrix_integrators_test
	@$(MAKE) compare_2link_artifact_matrix_invalid_integrator_test

# Run the focused coupled compare/manifest suite without broad regression expansion
coupled_compare_checks:
	@bash $(COUPLED_COMPARE_CHECKS_SCRIPT) $(if $(OUT_DIR),$(OUT_DIR),/tmp/fem4c_coupled_compare_checks) $(if $(MANIFEST_CSV),$(MANIFEST_CSV),$(if $(OUT_DIR),$(OUT_DIR)/coupled_compare_checks_manifest.csv,/tmp/fem4c_coupled_compare_checks/coupled_compare_checks_manifest.csv)) $(CHECK_TARGETS)

# Verify coupled compare wrapper emits stable summary rows
coupled_compare_checks_test:
	@bash $(COUPLED_COMPARE_CHECKS_TEST_SCRIPT)

# Verify coupled compare wrapper surfaces compare artifact manifests and interface-center CSVs
coupled_compare_checks_artifact_manifest_test:
	@bash $(COUPLED_COMPARE_CHECKS_ARTIFACT_MANIFEST_TEST_SCRIPT)

# Verify coupled compare wrapper surfaces rigid-limit threshold provenance from coupled_example_check
coupled_compare_checks_threshold_provenance_test:
	@bash $(COUPLED_COMPARE_CHECKS_THRESHOLD_PROVENANCE_TEST_SCRIPT)

# Verify coupled compare wrapper honors OUT_DIR overrides
coupled_compare_checks_out_dir_test:
	@bash $(COUPLED_COMPARE_CHECKS_OUT_DIR_TEST_SCRIPT)

# Validate coupled compare wrapper manifest CSV contract
coupled_compare_checks_manifest_test:
	@python3 $(COUPLED_COMPARE_CHECKS_MANIFEST_TEST_SCRIPT) $(if $(MANIFEST_CSV),"$(MANIFEST_CSV)",/tmp/fem4c_coupled_compare_checks/coupled_compare_checks_manifest.csv) $(if $(EXPECTED_TARGETS),--expected-targets "$(EXPECTED_TARGETS)",)

# Verify coupled compare wrapper + validator honor custom MANIFEST_CSV overrides
coupled_compare_checks_manifest_override_test:
	@bash $(COUPLED_COMPARE_CHECKS_MANIFEST_OVERRIDE_TEST_SCRIPT)

# Verify coupled compare wrapper honors CHECK_TARGETS subset overrides
coupled_compare_checks_subset_test:
	@bash $(COUPLED_COMPARE_CHECKS_SUBSET_TEST_SCRIPT)

# Verify coupled compare wrapper records fail target summaries
coupled_compare_checks_failfast_test:
	@bash $(COUPLED_COMPARE_CHECKS_FAILFAST_TEST_SCRIPT)

# Verify coupled compare reason-code normalization stays within the documented contract
coupled_compare_reason_codes_test:
	@bash $(COUPLED_COMPARE_REASON_CODES_TEST_SCRIPT)

# Print the coupled compare reason-code contract for PM-side audit
coupled_compare_reason_codes:
	@bash $(COUPLED_COMPARE_REASON_CODES_PRINT_SCRIPT)

# Verify the coupled compare reason-code printer stays stable
coupled_compare_reason_codes_print_test:
	@bash $(COUPLED_COMPARE_REASON_CODES_PRINT_TEST_SCRIPT)

# Verify the lightweight coupled_compare test-target helper stays stable
coupled_compare_contract_targets_test:
	@bash $(COUPLED_COMPARE_CONTRACT_TARGETS_TEST_SCRIPT)

# Verify runbook and queue stay synchronized with the coupled compare reason-code contract
coupled_compare_reason_code_docs_sync_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_DOC_SYNC_TEST_SCRIPT)

# Verify shell helper and python validator expose the same reason-code contract
coupled_compare_reason_code_consistency_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_CONSISTENCY_TEST_SCRIPT)

# Verify the shared coupled_compare audit cache helper stays stable
coupled_compare_reason_code_audit_cache_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_AUDIT_CACHE_TEST_SCRIPT)

# Verify the validator can print the known reason-code contract without a manifest path
coupled_compare_checks_manifest_print_reason_codes_test:
	@bash $(COUPLED_COMPARE_CHECKS_MANIFEST_PRINT_REASON_CODES_TEST_SCRIPT)

# Verify the validator can print the known reason-code pattern without a manifest path
coupled_compare_checks_manifest_print_reason_code_pattern_test:
	@bash $(COUPLED_COMPARE_CHECKS_MANIFEST_PRINT_REASON_CODE_PATTERN_TEST_SCRIPT)

# Run the focused coupled_compare reason-code contract suite
coupled_compare_reason_code_contract_checks:
	@$(MAKE) coupled_compare_contract_targets_test
	@$(MAKE) coupled_compare_reason_code_audit_cache_test
	@$(MAKE) coupled_compare_reason_codes_test
	@$(MAKE) coupled_compare_reason_codes_print_test
	@$(MAKE) coupled_compare_reason_code_docs_sync_test
	@$(MAKE) coupled_compare_reason_code_consistency_test
	@$(MAKE) coupled_compare_checks_manifest_print_reason_codes_test
	@$(MAKE) coupled_compare_checks_manifest_print_reason_code_pattern_test
	@$(MAKE) coupled_compare_reason_code_contract_audit_report_test
	@$(MAKE) coupled_compare_checks_manifest_reason_codes_test

# Verify the coupled_compare reason-code contract bundle stays stable
coupled_compare_reason_code_contract_checks_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_BUNDLE_TEST_SCRIPT)

# Verify the focused contract audit report validator stays stable
coupled_compare_reason_code_contract_audit_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_STDOUT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_MISSING_CACHE_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_CONTRACT_AUDIT_REPORT_WRONG_TARGET_TEST_SCRIPT)

# Run the focused root-surface contract suite used by C-team
coupled_compare_reason_code_root_surface_contract_checks:
	@$(MAKE) coupled_compare_reason_code_audit_cache_test
	@$(MAKE) coupled_compare_reason_code_docs_sync_test
	@cd .. && bash ./scripts/test_run_coupled_compare_reason_code_root_surface.sh
	@cd .. && bash ./scripts/test_check_coupled_compare_reason_code_root_surface_report.sh
	@cd .. && bash ./scripts/test_check_coupled_compare_reason_code_root_surface_report_missing_nested_log.sh
	@cd .. && bash ./scripts/test_check_coupled_compare_reason_code_root_surface_report_print_required_keys.sh
	@cd .. && bash ./scripts/test_check_coupled_compare_reason_code_root_surface_report_wrong_component.sh
	@cd .. && bash ./scripts/test_run_coupled_compare_reason_code_root_surface_audit.sh
	@cd .. && bash ./scripts/test_run_coupled_compare_reason_code_root_surface_audit_modes.sh
	@$(MAKE) coupled_compare_reason_code_root_surface_audit_report_test
	@$(MAKE) coupled_compare_reason_code_root_surface_audit_surface_test
	@$(MAKE) coupled_compare_reason_code_root_surface_contract_audit_report_test

# Verify the root-surface contract bundle target stays stable
coupled_compare_reason_code_root_surface_contract_checks_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_TEST_SCRIPT)

# Run the non-recursive skip-nested-selftests contract suite used by wrappers and bundle assembly
coupled_compare_reason_code_skip_nested_selftests_contract_checks_core:
	@$(MAKE) coupled_compare_reason_code_docs_sync_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_report_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_surface_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_surface_report_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_test

# Run the focused skip-nested-selftests contract suite used by C-team
coupled_compare_reason_code_skip_nested_selftests_contract_checks:
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_checks_core
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_test

# Verify the skip-nested-selftests contract bundle target stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_checks_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_BUNDLE_TEST_SCRIPT)

# Verify the skip-nested-selftests contract bundle help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_checks_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_BUNDLE_HELP_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_bundle_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle wrapper validator stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper validator stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface validator stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper validator stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Run the non-recursive focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core:
	@$(MAKE) coupled_compare_reason_code_docs_sync_test
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_ESCAPE_TEST_SCRIPT)

# Run the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks:
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core

# Verify the non-recursive focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks target stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_CORE_TEST_SCRIPT)

# Verify the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks target stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_TEST_SCRIPT)

# Verify the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_HELP_TEST_SCRIPT)

# Verify the focused checks validator CLI bundle help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_CLI_HELP_TEST_SCRIPT)

# Verify the focused checks full smoke surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_test:
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_help_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_help_test
	@$(MAKE) coupled_compare_reason_code_docs_sync_test

# Verify the focused checks full smoke help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_TEST_SCRIPT)

# Verify the repo-root focused checks full smoke help wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root focused checks full smoke help wrapper help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_SMOKE_HELP_WRAPPER_HELP_TEST_SCRIPT)

# Verify the focused checks full help smoke surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_test:
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_test
	@$(MAKE) coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_help_test
	@$(MAKE) coupled_compare_reason_code_docs_sync_test

# Verify the focused checks full help smoke help surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_help_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_HELP_TEST_SCRIPT)

# Verify the repo-root focused checks full help smoke wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_FULL_HELP_SMOKE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root focused skip-nested-selftests contract bundle report wrapper-surface wrapper checks wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root focused skip-nested-selftests contract bundle report wrapper-surface wrapper checks validator stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_MISSING_TARGET_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_COMPONENTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_TARGET_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_OVERRIDE_PAIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_HELP_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_WRONG_TARGET_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_ESCAPE_TEST_SCRIPT)

# Verify the focused checks validator CLI surface stays stable
coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_HELP_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_COMPONENTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_TARGET_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_CONTRACT_REPORT_WRAPPER_SURFACE_WRAPPER_CHECKS_REPORT_EXPECTED_OVERRIDE_PAIR_TEST_SCRIPT)

# Verify the focused root-surface contract audit report validator stays stable
coupled_compare_reason_code_root_surface_contract_audit_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_STDOUT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_MISSING_CACHE_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_AUDIT_REPORT_WRONG_TARGET_TEST_SCRIPT)

# Verify the repo-root root-surface audit report validator stays stable
coupled_compare_reason_code_root_surface_audit_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_CONTRACT_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISSING_NESTED_CONTRACT_KEY_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_MISMATCH_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_ESCAPE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_REPORT_WRONG_COMPONENT_TEST_SCRIPT)

# Verify the repo-root root-surface audited surface wrapper stays stable
coupled_compare_reason_code_root_surface_audit_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_SURFACE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_AUDIT_SURFACE_MODES_TEST_SCRIPT)

# Verify skip-nested-selftests modes stay stable across the root surface stack
coupled_compare_reason_code_skip_nested_selftests_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_PM_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_MODES_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper report validator stays stable
coupled_compare_reason_code_skip_nested_selftests_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_REPORT_ESCAPE_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper/report surface stays stable
coupled_compare_reason_code_skip_nested_selftests_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper/report surface validator stays stable
coupled_compare_reason_code_skip_nested_selftests_surface_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests surface-report wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper surface-report validator stays stable
coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_REPORT_ESCAPE_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper-surface report wrapper stays stable
coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root skip-nested-selftests wrapper-surface wrapper-report validator stays stable
coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_ESCAPE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_SKIP_NESTED_SELFTESTS_WRAPPER_SURFACE_WRAPPER_REPORT_NESTED_MISMATCH_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface wrapper stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface skip-nested-selftests mode stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_skip_nested_selftests_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_SKIP_NESTED_SELFTESTS_TEST_SCRIPT)

# Verify the focused root-surface contract bundle-log validator stays stable
coupled_compare_reason_code_root_surface_contract_bundle_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_MISSING_KEY_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface report validator stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_ESCAPE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_REPORT_NESTED_MISMATCH_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface report wrapper stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface report wrapper validator stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_ESCAPE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_REPORT_NESTED_MISMATCH_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface wrapper-report wrapper stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_DEFAULT_OUT_DIR_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_NESTED_OUT_DIR_TEST_SCRIPT)

# Verify the repo-root root-surface contract bundle surface wrapper-surface report validator stays stable
coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_test:
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_MISSING_LOG_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_PRINT_REQUIRED_KEYS_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_WRONG_COMPONENT_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_ESCAPE_TEST_SCRIPT)
	@bash $(COUPLED_COMPARE_REASON_CODE_ROOT_SURFACE_CONTRACT_BUNDLE_SURFACE_WRAPPER_SURFACE_REPORT_NESTED_MISMATCH_TEST_SCRIPT)

# Verify coupled compare manifest validator rejects undocumented reason codes
coupled_compare_checks_manifest_reason_codes_test:
	@bash $(COUPLED_COMPARE_CHECKS_MANIFEST_REASON_CODES_TEST_SCRIPT)

# Run build + rigid/flex matrix + compare matrix through one coupled acceptance entrypoint
coupled_2d_acceptance:
	@STAGES="$(STAGES)" bash $(COUPLED_2D_ACCEPTANCE_SCRIPT) $(if $(OUT_DIR),$(OUT_DIR),/tmp/fem4c_coupled_2d_acceptance) $(if $(MANIFEST_CSV),$(MANIFEST_CSV),$(if $(OUT_DIR),$(OUT_DIR)/coupled_2d_acceptance_manifest.csv,/tmp/fem4c_coupled_2d_acceptance/coupled_2d_acceptance_manifest.csv)) $(INTEGRATORS)

# Verify coupled acceptance wrapper emits stable summary rows
coupled_2d_acceptance_test:
	@bash $(COUPLED_2D_ACCEPTANCE_TEST_SCRIPT)

# Validate coupled acceptance wrapper manifest CSV contract
coupled_2d_acceptance_manifest_test:
	@python3 $(COUPLED_2D_ACCEPTANCE_MANIFEST_TEST_SCRIPT) $(if $(MANIFEST_CSV),$(MANIFEST_CSV),/tmp/fem4c_coupled_2d_acceptance/coupled_2d_acceptance_manifest.csv) $(if $(EXPECTED_STAGES),--expected-stages $(EXPECTED_STAGES),) $(if $(EXPECTED_INTEGRATORS),--expected-integrators $(EXPECTED_INTEGRATORS),)

# Verify threshold provenance columns remain valid in coupled acceptance manifests
coupled_2d_acceptance_threshold_contract_test:
	@bash $(COUPLED_2D_ACCEPTANCE_THRESHOLD_CONTRACT_TEST_SCRIPT)

# Verify coupled acceptance wrapper honors STAGES subset overrides
coupled_2d_acceptance_stages_test:
	@bash $(COUPLED_2D_ACCEPTANCE_STAGES_TEST_SCRIPT)

# Verify coupled acceptance wrapper honors combined STAGES and INTEGRATORS subset overrides
coupled_2d_acceptance_stage_integrators_test:
	@bash $(COUPLED_2D_ACCEPTANCE_STAGE_INTEGRATORS_TEST_SCRIPT)

# Verify coupled acceptance wrapper honors compare_matrix-only STAGES and INTEGRATORS subset overrides
coupled_2d_acceptance_compare_stage_integrators_test:
	@bash $(COUPLED_2D_ACCEPTANCE_COMPARE_STAGE_INTEGRATORS_TEST_SCRIPT)

# Verify compare-only coupled acceptance self-heals stale fem4c binaries
coupled_2d_acceptance_compare_stage_integrators_stale_binary_test:
	@bash $(COUPLED_2D_ACCEPTANCE_COMPARE_STAGE_INTEGRATORS_STALE_BINARY_TEST_SCRIPT)

# Verify coupled acceptance wrapper honors INTEGRATORS subset overrides
coupled_2d_acceptance_integrators_test:
	@bash $(COUPLED_2D_ACCEPTANCE_INTEGRATORS_TEST_SCRIPT)

# Verify coupled acceptance wrapper rejects unsupported stage subsets before artifact generation
coupled_2d_acceptance_invalid_stage_test:
	@bash $(COUPLED_2D_ACCEPTANCE_INVALID_STAGE_TEST_SCRIPT)

# Verify coupled acceptance wrapper rejects unsupported integrator subsets before artifact generation
coupled_2d_acceptance_invalid_integrator_test:
	@bash $(COUPLED_2D_ACCEPTANCE_INVALID_INTEGRATOR_TEST_SCRIPT)

# Run the focused coupled acceptance subset/invalid contract suite
coupled_2d_acceptance_contract_checks:
	@$(MAKE) coupled_2d_acceptance_threshold_contract_test
	@$(MAKE) coupled_2d_acceptance_stages_test
	@$(MAKE) coupled_2d_acceptance_integrators_test
	@$(MAKE) coupled_2d_acceptance_stage_integrators_test
	@$(MAKE) coupled_2d_acceptance_compare_stage_integrators_test
	@$(MAKE) coupled_2d_acceptance_invalid_stage_test
	@$(MAKE) coupled_2d_acceptance_invalid_integrator_test

# Verify the coupled acceptance contract bundle target stays stable
coupled_2d_acceptance_contract_checks_test:
	@bash $(COUPLED_2D_ACCEPTANCE_CONTRACT_BUNDLE_TEST_SCRIPT)

# Run the full coupled acceptance plus focused contract bundle gate
coupled_2d_acceptance_gate:
	@bash $(COUPLED_2D_ACCEPTANCE_GATE_SCRIPT) $(if $(OUT_DIR),$(OUT_DIR),/tmp/fem4c_coupled_2d_acceptance_gate)

# Verify the coupled acceptance gate target stays stable
coupled_2d_acceptance_gate_test:
	@bash $(COUPLED_2D_ACCEPTANCE_GATE_TEST_SCRIPT)

# Verify the coupled acceptance gate surfaces rigid-limit threshold provenance
coupled_2d_acceptance_gate_threshold_provenance_test:
	@bash $(COUPLED_2D_ACCEPTANCE_GATE_THRESHOLD_PROVENANCE_TEST_SCRIPT)

# Run the focused smoke pack that bundles gate + resilience self-tests
coupled_2d_acceptance_gate_resilience_smoke:
	@$(MAKE) coupled_2d_acceptance_gate_test
	@$(MAKE) coupled_2d_acceptance_resilience_checks_test
	@echo "PASS: coupled_2d_acceptance_gate_resilience_smoke bundle target"

# Verify the focused gate+resilience smoke pack stays stable
coupled_2d_acceptance_gate_resilience_smoke_test:
	@bash $(COUPLED_2D_ACCEPTANCE_GATE_RESILIENCE_SMOKE_TEST_SCRIPT)

# Verify acceptance docs stay synchronized with the current E-team command surface
coupled_2d_acceptance_docs_sync_test:
	@bash $(COUPLED_2D_ACCEPTANCE_DOCS_SYNC_TEST_SCRIPT)

# Verify the docs-sync validator help/inventory surfaces stay stable
coupled_2d_acceptance_docs_sync_surfaces_test:
	@bash $(COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACES_TEST_SCRIPT)

# Verify the docs-sync validator help target stays visible on make help
coupled_2d_acceptance_docs_sync_surfaces_help_test:
	@bash $(COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACES_HELP_TEST_SCRIPT)

# Run the docs-sync surface smoke pack (help contract + validator surfaces)
coupled_2d_acceptance_docs_sync_surface_smoke:
	@$(MAKE) coupled_2d_acceptance_docs_sync_surfaces_help_test
	@$(MAKE) coupled_2d_acceptance_docs_sync_surfaces_test
	@echo "PASS: coupled_2d_acceptance_docs_sync_surface_smoke bundle target"

# Verify the docs-sync surface smoke pack stays stable
coupled_2d_acceptance_docs_sync_surface_smoke_test:
	@bash $(COUPLED_2D_ACCEPTANCE_DOCS_SYNC_SURFACE_SMOKE_TEST_SCRIPT)

# Run the focused acceptance surface bundle (gate wrapper + docs sync)
coupled_2d_acceptance_surface_checks:
	@$(MAKE) coupled_2d_acceptance_docs_sync_surface_smoke_test
	@$(MAKE) coupled_2d_acceptance_docs_sync_test
	@$(MAKE) coupled_2d_acceptance_gate_test
	@$(MAKE) coupled_2d_acceptance_gate_threshold_provenance_test

# Verify the focused acceptance surface bundle target stays stable
coupled_2d_acceptance_surface_checks_test:
	@bash $(COUPLED_2D_ACCEPTANCE_SURFACE_BUNDLE_TEST_SCRIPT)

# Run the lightweight acceptance check pack (contract bundle + surface bundle)
coupled_2d_acceptance_lightweight_checks:
	@$(MAKE) coupled_2d_acceptance_contract_checks_test
	@$(MAKE) coupled_2d_acceptance_surface_checks_test

# Verify the lightweight acceptance check pack stays stable
coupled_2d_acceptance_lightweight_checks_test:
	@bash $(COUPLED_2D_ACCEPTANCE_LIGHTWEIGHT_BUNDLE_TEST_SCRIPT)

# Run the canonical Run 1 default-core acceptance route in one command
run1_default_core_acceptance:
	@$(MAKE) mbd_m1_rigid_acceptance
	@$(MAKE) coupled_flex_reference_compare_test
	@$(MAKE) ensure_fem4c_binary_test
	@$(MAKE) coupled_2d_acceptance_lightweight_checks

# Run the grouped non-default MBD extras for Run 1 review/debug
run1_nondefault_mbd_extras:
	@$(MAKE) mbd_regression
	@$(MAKE) mbd_negative
	@$(MAKE) mbd_run1_surface_docs_sync_test

# Run the grouped non-default coupled extras for Run 1 review/debug
run1_nondefault_coupled_extras:
	@$(MAKE) coupled_2d_acceptance
	@$(MAKE) coupled_2d_acceptance_gate
	@$(MAKE) coupled_2d_acceptance_resilience_checks

# Run the grouped Run 1 non-default review bundle in one command
run1_nondefault_review_bundle:
	@$(MAKE) run1_nondefault_mbd_extras
	@$(MAKE) run1_nondefault_coupled_extras

# Verify the grouped Run 1 non-default bundle surface stays aligned
run1_nondefault_bundle_contract_test:
	@bash scripts/test_run1_nondefault_bundle_contract.sh

# Verify product-adjacent artifact role rows stay aligned in MBD and coupled outputs
product_adjacent_artifact_surface_rows_test:
	@bash scripts/test_product_adjacent_artifact_surface_rows.sh

product_adjacent_artifact_surface_usage_test:
	@bash scripts/test_product_adjacent_artifact_surface_usage.sh

product_adjacent_artifact_preferred_source_contract_test:
	@bash scripts/test_product_adjacent_artifact_preferred_source_contract.sh

rigid_evidence_pack_entrypoint_contract_test:
	@bash scripts/test_rigid_evidence_pack_entrypoint_contract.sh

# Verify coupled-scheme official/experimental surface classification stays stable
coupled_scheme_surface_classification_test:
	@bash $(COUPLED_SCHEME_SURFACE_CLASSIFICATION_TEST_SCRIPT)

# Reconfirm the official one-way baseline evidence surface in one command
oneway_mainline_evidence_test:
	@bash $(COUPLED_INTEGRATOR_SCRIPT)
	@bash $(COUPLED_SCHEME_SURFACE_CLASSIFICATION_TEST_SCRIPT)
	@bash scripts/test_compare_2link_flex_reference_single_body_schema.sh

# Experimental delayed cosim smoke for multi-step sample-hold surface
delayed_cosim_multistep_smoke: $(TARGET)
	@./bin/fem4c --mode=coupled --coupled-scheme=delayed_cosim_v1_5 --coupled-integrator=newmark_beta --mbd-steps=3 examples/coupled_2link_flex_master.dat $(DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT) >$(DELAYED_COSIM_MULTISTEP_SMOKE_LOG) 2>&1
	@grep -q 'coupled_step=1/3' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q 'coupled_step=2/3' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q 'coupled_step=3/3' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q 'sample_hold_active=1' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q 'delayed_snapshot_step=1' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q 'exchange_lag_steps=1' $(DELAYED_COSIM_MULTISTEP_SMOKE_LOG)
	@grep -q '^steps_requested,3$$' $(DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT)
	@grep -q '^steps_executed,3$$' $(DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT)
	@grep -q '^step,2,' $(DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT)
	@grep -q '^step,3,' $(DELAYED_COSIM_MULTISTEP_SMOKE_OUTPUT)
	@echo "PASS: delayed cosim multi-step sample-hold smoke"

# Verify monolithic/delayed year1 compare schema stays aligned
year1_compare_schema_check: $(TARGET)
	@bash $(YEAR1_COMPARE_SCHEMA_SCRIPT)

# Verify compare wrappers rebuild stale fem4c binaries only when needed
ensure_fem4c_binary_test:
	@bash $(ENSURE_FEM4C_BINARY_TEST_SCRIPT)

# Run the focused wrapper smoke pack (lightweight acceptance + stale-binary guard)
coupled_2d_acceptance_wrapper_smoke:
	@$(MAKE) coupled_2d_acceptance_lightweight_checks_test
	@$(MAKE) ensure_fem4c_binary_test

# Verify the focused wrapper smoke pack stays stable
coupled_2d_acceptance_wrapper_smoke_test:
	@bash $(COUPLED_2D_ACCEPTANCE_WRAPPER_SMOKE_TEST_SCRIPT)

# Run the focused resilience pack (wrapper smoke + compare-only stale-binary contract)
coupled_2d_acceptance_resilience_checks:
	@$(MAKE) coupled_2d_acceptance_wrapper_smoke_test
	@$(MAKE) coupled_2d_acceptance_compare_stage_integrators_stale_binary_test

# Verify the focused resilience pack stays stable
coupled_2d_acceptance_resilience_checks_test:
	@bash $(COUPLED_2D_ACCEPTANCE_RESILIENCE_BUNDLE_TEST_SCRIPT)

.PHONY: coupled_2d_acceptance_gate_resilience_smoke coupled_2d_acceptance_gate_resilience_smoke_test

# Verify standalone MBD integrator switching (newmark/hht + invalid fallback + CLI params)
mbd_integrator_checks: $(TARGET)
	@bash $(MBD_INTEGRATOR_SCRIPT)

# Self-test for mbd integrator checker (pass + expected fail path)
mbd_integrator_checks_test:
	@bash $(MBD_INTEGRATOR_TEST_SCRIPT)

# Verify fem4c test log contains required PASS markers (used by CI gate)
fem4c_test_log_markers:
	@bash $(FEM4C_TEST_LOG_MARKER_SCRIPT) $(if $(LOG_FILE),$(LOG_FILE),fem4c_test.log)

# Self-test for FEM4C test log marker checker (pass + expected fail paths)
fem4c_test_log_markers_test:
	@bash $(FEM4C_TEST_LOG_MARKER_TEST_SCRIPT)

# Daily B-8 guard (static guarantee + local regression, optional spot evidence)
mbd_b8_guard:
	@RUN_SPOT=$(if $(RUN_SPOT),$(RUN_SPOT),0) \
	 SPOT_STRICT=$(if $(SPOT_STRICT),$(SPOT_STRICT),0) \
	 RUN_B14_REGRESSION=$(if $(RUN_B14_REGRESSION),$(RUN_B14_REGRESSION),0) \
	 B8_MAKE_CMD=$(if $(B8_MAKE_CMD),$(B8_MAKE_CMD),make) \
	 B8_CONTRACT_TARGET=$(if $(B8_CONTRACT_TARGET),$(B8_CONTRACT_TARGET),mbd_ci_contract) \
	 B8_LOCAL_TARGET=$(if $(B8_LOCAL_TARGET),$(B8_LOCAL_TARGET),mbd_checks) \
	 B8_B14_TARGET=$(if $(B8_B14_TARGET),$(B8_B14_TARGET),mbd_b14_regression) \
	 B8_SPOT_TARGET=$(if $(B8_SPOT_TARGET),$(B8_SPOT_TARGET),mbd_ci_evidence) \
	 RUN_ID=$(RUN_ID) \
	 bash $(B8_GUARD_SCRIPT)

# Self-test for B-8 guard (B-14 chaining + strict/non-strict spot behavior)
mbd_b8_guard_test:
	@bash $(B8_GUARD_TEST_SCRIPT)

# Validate B-8 guard output contract (required keys + pass/fail summary)
mbd_b8_guard_contract:
	@RUN_SPOT=$(if $(RUN_SPOT),$(RUN_SPOT),0) \
	 SPOT_STRICT=$(if $(SPOT_STRICT),$(SPOT_STRICT),0) \
	 RUN_B14_REGRESSION=$(if $(RUN_B14_REGRESSION),$(RUN_B14_REGRESSION),0) \
	 B8_MAKE_CMD=$(if $(B8_MAKE_CMD),$(B8_MAKE_CMD),make) \
	 B8_CONTRACT_TARGET=$(if $(B8_CONTRACT_TARGET),$(B8_CONTRACT_TARGET),mbd_ci_contract) \
	 B8_LOCAL_TARGET=$(if $(B8_LOCAL_TARGET),$(B8_LOCAL_TARGET),mbd_checks) \
	 B8_B14_TARGET=$(if $(B8_B14_TARGET),$(B8_B14_TARGET),mbd_b14_regression) \
	 B8_SPOT_TARGET=$(if $(B8_SPOT_TARGET),$(B8_SPOT_TARGET),mbd_ci_evidence) \
	 RUN_ID=$(RUN_ID) \
	 bash $(B8_GUARD_CONTRACT_SCRIPT)

# Self-test for B-8 guard contract wrapper (pass + expected fail path)
mbd_b8_guard_contract_test:
	@bash $(B8_GUARD_CONTRACT_TEST_SCRIPT)

# Syntax check for B-8 script family
mbd_b8_syntax:
	@bash $(B8_SYNTAX_CHECK_SCRIPT)

# Self-test for B-8 guard output checker
mbd_b8_guard_output_test:
	@bash $(B8_OUTPUT_TEST_SCRIPT)

# Object file compilation
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test targets
test: $(TARGET) $(PARSER_TARGET)
	@echo "Running tests..."
	@if [ -f $(TESTDIR)/run_tests.sh ]; then \
		cd $(TESTDIR) && ./run_tests.sh; \
	else \
		echo "No test script found"; \
	fi
	@echo "Running MBD validation checks (mbd_checks)..."
	@$(MAKE) mbd_checks
	@echo "Running parser compatibility checks (parser_compat)..."
	@$(MAKE) parser_compat
	@echo "Running flex interface-center checks (flex_body2d_interface_center_test)..."
	@$(MAKE) flex_body2d_interface_center_test
	@echo "Running flex marker checks (flex_body2d_marker_test)..."
	@$(MAKE) flex_body2d_marker_test
	@echo "Running coupled integrator checks (integrator_checks)..."
	@$(MAKE) integrator_checks

# Unit test target
unit_test:
	@echo "Building unit tests..."
	@if [ -d $(TESTDIR)/unit ]; then \
		$(MAKE) -C $(TESTDIR)/unit; \
	else \
		echo "No unit test directory found ($(TESTDIR)/unit), skipping"; \
	fi

# Clean targets
# Clean targets
clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	rm -f $(PARSERDIR)/parser $(PARSERDIR)/parser.exe
	@echo "Clean complete"

clean_all: clean
	rm -f $(TESTDIR)/*.out
	@echo "Full clean complete"

# Install target
install: $(TARGET)
	@echo "Installing FEM4C..."
	cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Documentation target
docs:
	@echo "Generating documentation..."
	@if command -v doxygen > /dev/null 2>&1; then \
		doxygen Doxyfile; \
	else \
		echo "Doxygen not found, skipping documentation generation"; \
	fi

# Format code
format:
	@echo "Formatting code..."
	@if command -v clang-format > /dev/null 2>&1; then \
		find $(SRCDIR) -name "*.c" -o -name "*.h" | xargs clang-format -i; \
		echo "Code formatting complete"; \
	else \
		echo "clang-format not found, skipping code formatting"; \
	fi

# Static analysis
analyze:
	@echo "Running static analysis..."
	@if command -v cppcheck > /dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 $(SRCDIR); \
	else \
		echo "cppcheck not found, skipping static analysis"; \
	fi

# Memory check
memcheck: debug
	@echo "Running memory check..."
	@if command -v valgrind > /dev/null 2>&1; then \
		valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(TARGET); \
	else \
		echo "valgrind not found, skipping memory check"; \
	fi

# Performance profiling
profile:
	@echo "Building for profiling..."
	$(MAKE) CFLAGS="$(CFLAGS) -pg" $(TARGET)
	@echo "Run the program and use 'gprof $(TARGET) gmon.out' to view profile"

# Show help
help:
	@echo "FEM4C Build System"
	@echo "=================="
	@echo "Available targets:"
	@echo "  all       - Build the main executable + parser (default)"
	@echo "  release   - Build optimized version with OpenMP (+ parser)"
	@echo "  debug     - Build debug version (+ parser)"
	@echo "  openmp    - Build with OpenMP support (+ parser)"
	@echo "  test      - [non-default Run1] Run repo-wide tests (+ mbd_checks[incl. mbd_integrator_checks] + parser_compat + flex marker/interface + integrator_checks)"
	@echo "  unit_test - Build and run unit tests"
	@echo "  clean     - Remove build files"
	@echo "  clean_all - Remove all generated files"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  docs      - Generate documentation (requires doxygen)"
	@echo "  format    - Format source code (requires clang-format)"
	@echo "  analyze   - Run static analysis (requires cppcheck)"
	@echo "  memcheck  - Run memory leak detection (requires valgrind)"
	@echo "  profile   - Build for performance profiling"
	@echo "  mbd_probe - Build and run MBD constraint probe"
	@echo "  mbd_b_team_foundation_probe_smoke - Run B-team rigid MBD probe/foundation-lite pack with long-run projection checks"
	@echo "  mbd_b_team_foundation_smoke - Run B-team rigid MBD full foundation pack with long-run projection checks"
	@echo "  mbd_system2d_newmark_iteration_fallback_smoke - Verify Newmark implicit iteration env fallback contract"
	@echo "  mbd_system2d_hht_iteration_fallback_smoke - Verify HHT implicit iteration env fallback contract"
	@echo "  mbd_regression - [non-default Run1] Run one-command MBD regression (positive + negative diagnostics aligned with current unconstrained body-only behavior)"
	@echo "  run1_default_core_acceptance - [default-core Run1] Run the canonical Run 1 default acceptance route (M1 rigid + flex compare + binary guard + lightweight coupled checks)"
	@echo "  run1_nondefault_mbd_extras - [non-default Run1] Run the grouped non-default MBD extras (regression + negative + docs-sync)"
	@echo "  run1_nondefault_coupled_extras - [non-default Run1] Run the grouped non-default coupled extras (acceptance + gate + resilience)"
	@echo "  run1_nondefault_review_bundle - [non-default Run1] Run the grouped Run 1 review/debug bundle"
	@echo "  run1_nondefault_bundle_contract_test - [non-default Run1] Verify the grouped non-default umbrella surface"
	@echo "  oneway_mainline_evidence_test - Reconfirm the official one-way baseline evidence bundle (integrators + scheme surface + 1-link schema)"
	@echo "  delayed_cosim_multistep_smoke - Run the experimental delayed_cosim_v1_5 2-link multi-step sample-hold smoke"
	@echo "  year1_compare_schema_check - Verify the shared year1 compare schema across monolithic_strong_v1 and delayed_cosim_v1_5"
	@echo "  product_adjacent_artifact_surface_rows_test - Verify product-adjacent artifact role rows in MBD and coupled outputs"
	@echo "  product_adjacent_artifact_surface_usage_test - Verify compare scripts consume product-adjacent artifact rows"
	@echo "  product_adjacent_artifact_preferred_source_contract_test - Verify preferred compare source contracts and rigid fallback rule"
	@echo "  rigid_evidence_pack_entrypoint_contract_test - Verify OW080 wrapper uses build_rigid_2link_evidence_pack.py"
	@echo "  mbd_m1_rigid_acceptance - [default-core Run1] Run the top-level M1 rigid acceptance route (rigid main cases + analytic compare)"
	@echo "  mbd_m1_rigid_acceptance_test - [non-default Run1] Verify the top-level M1 rigid acceptance route"
	@echo "  mbd_consistency - [non-default Run1] Compare runtime and probe equation counts"
	@echo "  mbd_negative - [non-default Run1] Verify invalid-input failures plus accepted body-only unconstrained case"
	@echo "  mbd_checks - [non-default Run1] Run all MBD checks (probe/regression/consistency/negative/integrator)"
	@echo "  mbd_run1_surface_docs_sync_test - [non-default Run1] Verify Run 1 MBD docs/help stay synchronized with current target roles"
	@echo "  mbd_run1_surface_docs_sync_surfaces_test - [non-default Run1] Verify the Run 1 docs-sync validator help/inventory/count/current-command/doc-source/supported-options audit/schema surfaces"
	@echo "  mbd_run1_surface_docs_sync_surfaces_help_test - [non-default Run1] Verify the Run 1 docs-sync validator help targets stay on make help"
	@echo "  mbd_run1_surface_docs_sync_surface_smoke - [non-default Run1] Run the focused Run 1 docs-sync surface smoke pack"
	@echo "  mbd_run1_surface_docs_sync_surface_smoke_test - [non-default Run1] Verify the focused Run 1 docs-sync surface smoke pack"
	@echo "  mbd_ci_evidence - Fetch CI run evidence with strict acceptance (step+artifact)"
	@echo "  mbd_ci_contract - Validate required FEM4C CI contract in .github/workflows/ci.yaml"
	@echo "  mbd_ci_contract_lock_wait_sync - Enforce strict lock_wait_max sync contract"
	@echo "  mbd_ci_contract_lock_wait_check_only - Run lock_wait_max check-only contract in compat mode"
	@echo "  mbd_b14_regression - Run B-14 one-command regression (contract + self-tests + local test entry)"
	@echo "  mbd_b8_regression - Run B-8 one-command regression (contract + self-tests + guard+b14 chain)"
	@echo "  mbd_b8_regression_test - Self-test for B-8 regression wrapper"
	@echo "  mbd_b8_regression_full - Run B-8 full regression from clean rebuild"
	@echo "  mbd_b8_regression_full_test - Self-test for B-8 full regression wrapper"
	@echo "  mbd_b8_knob_matrix_test - Validate B8_RUN_B14_REGRESSION and B8_MAKE_CMD knob matrix"
	@echo "  mbd_b8_knob_matrix_smoke_test - Validate knob matrix in smoke mode (skip full rebuild)"
	@echo "  mbd_a21_regression - Run A-21 one-command regression (time/source static contract + runtime checks)"
	@echo "  mbd_a21_regression_test - Self-test for A-21 regression wrapper"
	@echo "  mbd_a24_regression - Run A-24 one-command regression (step-trace runtime/static contract + self-tests)"
	@echo "  mbd_a24_regression_test - Self-test for A-24 regression wrapper"
	@echo "  mbd_a24_regression_full - Run A-24 full regression from clean rebuild"
	@echo "  mbd_a24_regression_full_test - Self-test for A-24 full regression wrapper"
	@echo "  mbd_a24_batch - Run A-24 serial batch (regression + tests)"
	@echo "  mbd_a24_batch_test - Self-test for A-24 batch wrapper"
	@echo "  mbd_a24_acceptance_serial - Run A-24 serial acceptance (full_test + batch_test + ci_contract_test)"
	@echo "  mbd_a24_acceptance_serial_test - Self-test for A-24 serial acceptance wrapper"
	@echo "  mbd_ci_contract_test - Verify CI contract checker + log-marker checker pass/fail behavior"
	@echo "  mbd_team_acceptance_gate - Audit latest A/B/C/D/E session gate (elapsed>=60 + impl changes)"
	@echo "  mbd_ci_contract_test_sha_watch - Run mbd_ci_contract_test with test_check_ci_contract.sh sha watchdog"
	@echo "  mbd_b45_acceptance - Run B-45 acceptance (sha pre/post + ci_contract/knob/full/regression tests)"
	@echo "  t3_orientation_checks - Verify default correction + strict expected-fail for clockwise T3"
	@echo "  parser_compat - Verify legacy parser package + Nastran parser compatibility"
	@echo "  parser_compat_fallback - Verify parser compatibility with built-in legacy fallback fixture"
	@echo "  coupled_stub_check - [legacy non-default] Verify the no-flex coupled fallback contract snapshot logs"
	@echo "  coupled_run1_surface_docs_sync_test - [non-default Run1] Verify coupled Run 1 docs/help keep default actual routes ahead of the legacy stub fallback"
	@echo "  integrator_checks - Verify coupled one-way mainline across explicit/newmark/hht integrators"
	@echo "  coupled_example_check - Verify coupled 2-link examples + rigid-limit explicit/implicit compare"
	@echo "  coupled_rigid_limit_compare_test - Verify explicit rigid-limit compare CSV generation"
	@echo "  coupled_rigid_limit_implicit_test - Verify rigid-limit implicit runs converge"
	@echo "  coupled_rigid_limit_implicit_compare_test - Verify rigid-limit implicit compare CSV generation"
	@echo "  coupled_rigid_limit_thresholds - Print the rigid-limit threshold contract"
	@echo "  coupled_rigid_limit_thresholds_test - Verify the rigid-limit threshold contract printer"
	@echo "  coupled_rigid_limit_threshold_docs_sync_test - Verify rigid-limit threshold docs stay synchronized"
	@echo "  fem_model_copy_test - Build and run FEM model deep-copy smoke"
	@echo "  q4_t3_stiffness_adapter_warning_test - Compile-check the Q4/T3 stiffness adapters for warning-free registration"
	@echo "  coupled_step_common2d_test - Build and run coupled-step common-helper smoke"
	@echo "  coupled_step_common2d_solve_test - Build and run coupled-step shared solve-path smoke"
	@echo "  c_review_spec_smoke_test - Run the focused C review-spec smoke bundle (Q4/T3 + coupled-step helper/solve)"
	@echo "  flex_solver2d_test - Build and run model-centric solver smoke"
	@echo "  flex_bc2d_test - Build and run runtime BC override smoke"
	@echo "  flex_body2d_inertial_test - Build and run snapshot inertial-load smoke"
	@echo "  flex_body2d_interface_center_test - Build and run deformed interface-center smoke"
	@echo "  flex_body2d_marker_test - Build and run root/tip marker displacement smoke"
	@echo "  flex_nodeset_test - Build and run nodeset utility smoke"
	@echo "  flex_snapshot2d_test - Build and run snapshot CSV naming/export smoke"
	@echo "  coupled_reassembly_log_test - Build and run coupled reassembly counter smoke"
	@echo "  coupled_snapshot_output_test - Build and run coupled snapshot output smoke"
	@echo "  coupled_implicit_snapshot_output_test - Build and run implicit coupled snapshot output smoke"
	@echo "  coupled_rigid_limit_manifest_test - Verify compare helper prefers summary snapshot manifest"
	@echo "  coupled_flex_manifest_test - [non-default Run1] Verify flexible reference helper prefers summary snapshot manifest"
	@echo "  coupled_flex_reference_real_test - [non-default Run1] Verify real coupled summary normalizes into flex schema artifacts"
	@echo "  coupled_flex_reference_compare_test - [default-core Run1] Verify compare mode emits compare and normalized flex artifacts"
	@echo "  coupled_flex_reference_artifact_only_test - [non-default Run1] Verify 1-link flex artifact-only export emits interface-center and reaction-map CSVs"
	@echo "  coupled_scheme_surface_classification_test - Verify official vs experimental coupled scheme surface keys across all three schemes"
	@echo "  mbd_rigid_analytic_real_test - [non-default Run1] Verify real MBD summary normalizes into rigid schema artifacts"
	@echo "  mbd_rigid_analytic_hht_compare_test - [non-default Run1] Verify HHT rigid compare emits compare and normalized artifacts"
	@echo "  mbd_rigid_analytic_compare_test - [non-default Run1] Verify real MBD summary emits rigid compare and normalized artifacts"
	@echo "  mbd_rigid_compare_integrator_review_smoke - [non-default Run1] Verify rigid compare sidecar stays compare-ready for newmark/hht"
	@echo "  mbd_rigid_compare_fallback_review_smoke - [non-default Run1] Verify rigid compare normalization falls back to history geometry when sidecar tips are malformed"
	@echo "  mbd_rigid_compare_expect_route_guard_smoke - [non-default Run1] Verify rigid compare callers fail fast on wrong normalization-route expectations"
	@echo "  mbd_rigid_compare_expect_fallback_reason_guard_smoke - [non-default Run1] Verify rigid compare callers fail fast on wrong fallback-reason expectations"
	@echo "  mbd_rigid_compare_expect_snapshot_count_guard_smoke - [non-default Run1] Verify rigid compare callers fail fast on wrong snapshot-count route metadata"
	@echo "  mbd_rigid_compare_route_matrix_review_smoke - [non-default Run1] Verify rigid compare provenance route survives matrix wrapper aggregation"
	@echo "  mbd_rigid_compare_route_review_smoke - [non-default Run1] Run the rigid compare provenance route smoke pack"
	@echo "  mbd_rigid_analytic_multi_reference_test - [non-default Run1] Verify rigid compare supports multiple overlaid references"
	@echo "  mbd_rigid_analytic_wrapper_test - [non-default Run1] Verify rigid normalize/multi wrappers emit stable stdout paths"
	@echo "  compare_2link_artifact_check - [non-default Run1] Run the minimal E-08 rigid/flex compare artifact suite"
	@echo "  compare_2link_artifact_check_test - [non-default Run1] Verify compare artifact suite stdout contract"
	@echo "  compare_2link_artifact_manifest_test - [non-default Run1] Validate compare artifact suite manifest CSV contract"
	@echo "  compare_2link_artifact_manifest_header_contract_test - [non-default Run1] Verify compare artifact manifest validator rejects reordered route header fields"
	@echo "  compare_2link_artifact_manifest_snapshot_count_guard_test - [non-default Run1] Verify compare artifact manifest validator rejects mismatched snapshot counts"
	@echo "  compare_2link_artifact_check_vars_test - [non-default Run1] Verify compare artifact suite honors OUT_DIR/MANIFEST_CSV overrides"
	@echo "  compare_2link_artifact_check_integrator_test - [non-default Run1] Verify compare artifact suite honors INTEGRATOR override"
	@echo "  compare_2link_artifact_check_invalid_integrator_test - [non-default Run1] Verify compare artifact suite rejects unsupported integrators"
	@echo "  compare_2link_artifact_matrix_check - [non-default Run1] Run the compare artifact suite across explicit/newmark/hht"
	@echo "  compare_2link_artifact_matrix_check_test - [non-default Run1] Verify compare artifact matrix stdout contract"
	@echo "  compare_2link_artifact_matrix_manifest_test - [non-default Run1] Validate compare artifact matrix manifest CSV contract"
	@echo "  compare_2link_artifact_matrix_manifest_header_contract_test - [non-default Run1] Verify compare artifact matrix manifest validator rejects reordered route header fields"
	@echo "  compare_2link_artifact_matrix_manifest_snapshot_count_guard_test - [non-default Run1] Verify compare artifact matrix manifest validator rejects mismatched snapshot counts"
	@echo "  compare_2link_artifact_matrix_manifest_expected_integrators_test - [non-default Run1] Verify matrix manifest validator honors EXPECTED_INTEGRATORS"
	@echo "  compare_2link_artifact_matrix_integrators_test - [non-default Run1] Verify compare artifact matrix honors INTEGRATORS subset overrides"
	@echo "  compare_2link_artifact_matrix_invalid_integrator_test - [non-default Run1] Verify matrix wrapper rejects unsupported integrators"
	@echo "  compare_2link_artifact_checks - [non-default Run1] Run the focused compare artifact self-test suite"
	@echo "  coupled_compare_checks - Run focused coupled compare/manifest checks"
	@echo "  coupled_compare_checks_test - Verify coupled compare wrapper emits stable summary rows"
	@echo "  coupled_compare_checks_artifact_manifest_test - Verify coupled compare wrapper surfaces compare artifact manifests"
	@echo "  coupled_compare_checks_threshold_provenance_test - Verify coupled compare wrapper surfaces threshold provenance"
	@echo "  coupled_compare_checks_out_dir_test - Verify coupled compare wrapper honors OUT_DIR overrides"
	@echo "  coupled_compare_checks_manifest_test - Validate coupled compare wrapper manifest CSV"
	@echo "  coupled_compare_checks_manifest_override_test - Verify coupled compare wrapper honors MANIFEST_CSV overrides"
	@echo "  coupled_compare_checks_subset_test - Verify coupled compare wrapper honors CHECK_TARGETS subset overrides"
	@echo "  coupled_compare_checks_failfast_test - Verify coupled compare wrapper records fail summaries"
	@echo "  coupled_compare_reason_codes_test - Verify coupled compare reason-code normalization contract"
	@echo "  coupled_compare_reason_codes - Print coupled compare reason-code contract and update points"
	@echo "  coupled_compare_reason_codes_print_test - Verify coupled compare reason-code printer output"
	@echo "  coupled_compare_contract_targets_test - Verify lightweight coupled_compare test targets helper"
	@echo "  coupled_compare_reason_code_docs_sync_test - Verify runbook/queue reason-code docs stay synchronized"
	@echo "  coupled_compare_reason_code_consistency_test - Verify shell/python reason-code contract stays synchronized"
	@echo "  coupled_compare_reason_code_audit_cache_test - Verify the shared coupled_compare audit cache helper"
	@echo "  coupled_compare_checks_manifest_print_reason_codes_test - Verify validator prints known reason codes without a manifest"
	@echo "  coupled_compare_checks_manifest_print_reason_code_pattern_test - Verify validator prints known reason-code pattern without a manifest"
	@echo "  coupled_compare_reason_code_contract_checks - Run the focused coupled_compare reason-code contract suite"
	@echo "  coupled_compare_reason_code_contract_checks_test - Verify the coupled_compare reason-code bundle target"
	@echo "  coupled_compare_reason_code_contract_audit_report_test - Verify the focused contract audit report validator"
	@echo "  coupled_compare_reason_code_root_surface_contract_checks - Run the focused root-surface contract suite"
	@echo "  coupled_compare_reason_code_root_surface_contract_checks_test - Verify the root-surface contract bundle target"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_checks - Run the focused skip-nested-selftests contract suite"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_checks_core - Run the non-recursive skip-nested-selftests contract suite"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_checks_test - Verify the skip-nested-selftests contract bundle target"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_checks_help_test - Verify the skip-nested-selftests contract bundle help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_bundle_test - Verify the repo-root skip-nested-selftests contract bundle wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_test - Verify the repo-root skip-nested-selftests contract bundle wrapper validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_report_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_report_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_report_test - Verify the repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core - Run the non-recursive focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_core_test - Verify the non-recursive focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks target"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks - Run the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_test - Verify the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks target"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_help_test - Verify the focused repo-root skip-nested-selftests contract bundle report wrapper-surface wrapper checks help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_wrapper_test - Verify the repo-root focused skip-nested-selftests contract bundle report wrapper-surface wrapper checks wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_test - Verify the repo-root focused skip-nested-selftests contract bundle report wrapper-surface wrapper checks validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_test - Verify the focused checks validator CLI surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_report_cli_help_test - Verify the focused checks validator CLI bundle help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_test - Verify the focused checks full smoke surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_test - Verify the focused checks full smoke help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_test - Verify the repo-root focused checks full smoke help wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_smoke_help_wrapper_help_test - Verify the repo-root focused checks full smoke help wrapper help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_test - Verify the focused checks full help smoke surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_help_test - Verify the focused checks full help smoke help surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_contract_report_wrapper_surface_wrapper_checks_full_help_smoke_wrapper_test - Verify the repo-root focused checks full help smoke wrapper"
	@echo "  coupled_compare_reason_code_root_surface_contract_audit_report_test - Verify the focused root-surface contract audit report validator"
	@echo "  coupled_compare_reason_code_root_surface_audit_report_test - Verify the repo-root root-surface audit report validator"
	@echo "  coupled_compare_reason_code_root_surface_audit_surface_test - Verify the repo-root root-surface audited surface wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_test - Verify skip-nested-selftests propagation across the root surface stack"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_report_test - Verify the repo-root skip-nested-selftests wrapper report validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_surface_test - Verify the repo-root skip-nested-selftests wrapper/report surface"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_surface_report_test - Verify the repo-root skip-nested-selftests surface report validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_test - Verify the repo-root skip-nested-selftests surface-report wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_report_test - Verify the repo-root skip-nested-selftests wrapper surface-report validator"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_test - Verify the repo-root skip-nested-selftests wrapper-surface report wrapper"
	@echo "  coupled_compare_reason_code_skip_nested_selftests_wrapper_surface_wrapper_report_test - Verify the repo-root skip-nested-selftests wrapper-surface wrapper-report validator"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_test - Verify the repo-root root-surface contract bundle surface wrapper"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_skip_nested_selftests_test - Verify the repo-root root-surface contract bundle surface skip-nested-selftests mode"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_report_test - Verify the focused root-surface contract bundle-log validator"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_report_test - Verify the repo-root root-surface contract bundle surface report validator"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_test - Verify the repo-root root-surface contract bundle surface report wrapper"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_report_test - Verify the repo-root root-surface contract bundle surface report-wrapper validator"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_test - Verify the repo-root root-surface contract bundle surface wrapper-report wrapper"
	@echo "  coupled_compare_reason_code_root_surface_contract_bundle_surface_wrapper_surface_report_test - Verify the repo-root root-surface contract bundle surface wrapper-surface report validator"
	@echo "  coupled_compare_checks_manifest_reason_codes_test - Verify coupled compare manifest validator rejects undocumented reason codes"
	@echo "  coupled_2d_acceptance - [non-default Run1] Run build + rigid/flex/compare 2D acceptance orchestration"
	@echo "  coupled_2d_acceptance_test - Verify coupled acceptance wrapper emits stable summary rows"
	@echo "  coupled_2d_acceptance_manifest_test - Validate coupled acceptance wrapper manifest CSV"
	@echo "  coupled_2d_acceptance_threshold_contract_test - Verify threshold provenance columns in acceptance manifests"
	@echo "  coupled_2d_acceptance_stages_test - Verify coupled acceptance wrapper honors STAGES subset overrides"
	@echo "  coupled_2d_acceptance_stage_integrators_test - Verify coupled acceptance wrapper honors combined STAGES and INTEGRATORS"
	@echo "  coupled_2d_acceptance_compare_stage_integrators_test - Verify coupled acceptance wrapper honors compare_matrix-only STAGES and INTEGRATORS"
	@echo "  coupled_2d_acceptance_compare_stage_integrators_stale_binary_test - Verify compare-only coupled acceptance self-heals stale fem4c binaries"
	@echo "  coupled_2d_acceptance_integrators_test - Verify coupled acceptance wrapper honors INTEGRATORS subset overrides"
	@echo "  coupled_2d_acceptance_invalid_stage_test - Verify coupled acceptance wrapper rejects unsupported STAGES"
	@echo "  coupled_2d_acceptance_invalid_integrator_test - Verify coupled acceptance wrapper rejects unsupported INTEGRATORS"
	@echo "  coupled_2d_acceptance_contract_checks - [non-default Run1] Run the focused coupled acceptance subset/invalid contract suite"
	@echo "  coupled_2d_acceptance_contract_checks_test - [non-default Run1] Verify the coupled acceptance contract bundle target"
	@echo "  coupled_2d_acceptance_gate - [non-default Run1] Run full coupled acceptance plus the focused contract bundle with stable summary rows"
	@echo "  coupled_2d_acceptance_gate_test - [non-default Run1] Verify the coupled acceptance gate wrapper"
	@echo "  coupled_2d_acceptance_gate_threshold_provenance_test - [non-default Run1] Verify the coupled acceptance gate surfaces rigid-limit threshold provenance"
	@echo "  coupled_2d_acceptance_gate_resilience_smoke - [non-default Run1] Run the focused smoke pack that bundles gate and resilience self-tests"
	@echo "  coupled_2d_acceptance_gate_resilience_smoke_test - [non-default Run1] Verify the focused gate+resilience smoke pack"
	@echo "  coupled_2d_acceptance_docs_sync_test - [non-default Run1] Verify acceptance docs stay synchronized with the current E-team command surface"
	@echo "  coupled_2d_acceptance_docs_sync_surfaces_test - [non-default Run1] Verify the docs-sync validator help/inventory surfaces"
	@echo "  coupled_2d_acceptance_docs_sync_surfaces_help_test - [non-default Run1] Verify the docs-sync validator help target stays on make help"
	@echo "  coupled_2d_acceptance_docs_sync_surface_smoke - [non-default Run1] Run the focused docs-sync surface smoke pack"
	@echo "  coupled_2d_acceptance_docs_sync_surface_smoke_test - [non-default Run1] Verify the focused docs-sync surface smoke pack"
	@echo "  coupled_2d_acceptance_surface_checks - [non-default Run1] Run the focused acceptance surface bundle"
	@echo "  coupled_2d_acceptance_surface_checks_test - [non-default Run1] Verify the focused acceptance surface bundle target"
	@echo "  coupled_2d_acceptance_lightweight_checks - [default-core Run1] Run the lightweight acceptance check pack kept on the default path"
	@echo "  coupled_2d_acceptance_lightweight_checks_test - [non-default Run1] Verify the lightweight acceptance check pack"
	@echo "  ensure_fem4c_binary_test - [default-core Run1] Verify compare wrappers rebuild stale fem4c binaries only when needed"
	@echo "  coupled_2d_acceptance_wrapper_smoke - [non-default Run1] Run the focused wrapper smoke pack"
	@echo "  coupled_2d_acceptance_wrapper_smoke_test - [non-default Run1] Verify the focused wrapper smoke pack"
	@echo "  coupled_2d_acceptance_resilience_checks - [non-default Run1] Run the focused resilience pack"
	@echo "  coupled_2d_acceptance_resilience_checks_test - [non-default Run1] Verify the focused resilience pack"
	@echo "  coupled_nodeset_guard_test - Build and run coupled nodeset guard smoke"
	@echo "  mbd_a_team_foundation_probe_smoke - Run A-team rigid MBD probe/foundation-lite smoke pack"
	@echo "  $(MBD_A_TEAM_FOUNDATION_ENTRYPOINT) - $(MBD_A_TEAM_FOUNDATION_HELP_DESC)"
	@echo "  mbd_a_team_history_contract_layout_smoke - Verify A-team history contract bundle wiring"
	@echo "  mbd_a_team_history_header_single_source_smoke - Verify A-team history probes share the output2d header helper"
	@echo "  mbd_a_team_rigid_compare_header_single_source_smoke - Verify rigid-compare probes share the output2d header helper"
	@echo "  mbd_a_team_probe_utils_single_source_smoke - Verify A-team probes share mbd_probe_utils helper logic"
	@echo "  mbd_a_team_foundation_probe_layout_smoke - Verify A-team foundation probe bundle wiring stays history-free"
	@echo "  mbd_a_team_foundation_contract_layout_smoke - Verify A-team foundation bundle wiring stays contract-backed"
	@echo "  mbd_a_team_foundation_help_contract_smoke - Verify A-team foundation help surface entries"
	@echo "  mbd_a_team_foundation_surface_smoke - Verify A-team foundation entrypoint stays bundle-backed"
	@echo "  mbd_a_team_history_help_contract_smoke - Verify A-team history help surface entries"
	@echo "  mbd_a_team_history_audit_smoke - Run A-team history/foundation audit bundle"
	@echo "  $(MBD_A_TEAM_HISTORY_ENTRYPOINT) - $(MBD_A_TEAM_HISTORY_HELP_DESC)"
	@echo "  mbd_system2d_history_base_probe_smoke - Run generalized-force history base probe bundle"
	@echo "  mbd_system2d_history_implicit_probe_smoke - Run generalized-force history implicit probe bundle"
	@echo "  mbd_system2d_history_probe_contract_smoke - Run generalized-force history probe-only contract bundle"
	@echo "  mbd_system2d_history_cli_contract_smoke - Run generalized-force history CLI/system contract bundle"
	@echo "  mbd_implicit_label_review_smoke - Run A-R1 implicit label neutralization smoke pack"
	@echo "  mbd_rigid_compare_review_smoke - Run A-R2 rigid compare readiness smoke pack"
	@echo "  mbd_integrator_checks - Verify standalone MBD integrator switch (newmark/hht + fallback + cli params)"
	@echo "  mbd_integrator_checks_test - Self-test for mbd integrator checker"
	@echo "  fem4c_test_log_markers - Verify required PASS markers in fem4c_test.log"
	@echo "  fem4c_test_log_markers_test - Self-test for fem4c test log marker checker"
	@echo "  mbd_b8_guard - Run B-8 daily guard (contract + local regression + optional b14/spot)"
	@echo "  mbd_b8_guard_test - Self-test for B-8 guard (b14 chain + strict/non-strict spot)"
	@echo "  mbd_b8_guard_contract - Validate B-8 guard output contract (required keys + summary)"
	@echo "  mbd_b8_guard_contract_test - Self-test for B-8 guard contract wrapper"
	@echo "  mbd_b8_syntax - Run bash -n syntax checks for B-8 scripts"
	@echo "  mbd_b8_guard_output_test - Self-test for B-8 guard output checker"
	@echo "  mbd_projection2d_probe_smoke - Run one-shot rigid position projection probe"
	@echo "  mbd_system2d_projection_compare_smoke - Run multi-step position projection compare probe"
	@echo "  mbd_system2d_projection_long_compare_smoke - Run 20-step position projection compare probe"
	@echo "  mbd_system2d_projection_history_output_smoke - Run projection-enabled history CSV probe"
	@echo "  mbd_system2d_projection_long_history_output_smoke - Run 20-step projection-enabled history CSV probe"
	@echo "  mbd_output2d_history_field_count_sync_smoke - Check history CSV field-count sync across output/probes"
	@echo "  mbd_output2d_rigid_compare_header_single_source_smoke - Verify rigid-compare header helper remains single-source"
	@echo "  mbd_output2d_rigid_compare_field_count_sync_smoke - Check rigid-compare CSV field-count sync across output/probes"
	@echo "  mbd_rigid_compare_review_columns_sync_smoke - Check rigid analytic review scripts share one compare-columns helper"
	@echo "  compare_2link_artifact_route_fields_getter_test - Check artifact route field getter stays synchronized with shell/python helpers"
	@echo "  compare_2link_artifact_targets_getter_test - Check artifact target getter stays synchronized with Python validators"
	@echo "  compare_2link_artifact_targets_sync_test - Check artifact target helper stays synchronized across validators"
	@echo "  compare_2link_artifact_integrators_getter_test - Check artifact integrator getter stays synchronized with matrix helpers"
	@echo "  compare_2link_artifact_integrators_sync_test - Check artifact integrator helper stays synchronized across matrix consumers"
	@echo "  compare_2link_artifact_route_fields_sync_test - Check rigid/flex artifact route field helpers stay single-source"
	@echo "  mbd_system2d_projection_longrun_contract_smoke - Run 20-step projection compare/history contract bundle"
	@echo "  mbd_system2d_cli_ready_contract_smoke - Verify every local mbd_system2d_cli smoke depends on mbd_system2d_cli_ready"
	@echo "  mbd_b_team_foundation_isolated_smoke - Run B-team foundation smoke with fresh local BINDIR/BUILDDIR/PARSERDIR and long-run projection checks"
	@echo "  mbd_system2d_projection_long_compare_isolated_smoke - Run 20-step projection compare smoke with fresh local BINDIR/BUILDDIR/PARSERDIR"
	@echo "  mbd_system2d_projection_long_history_isolated_smoke - Run 20-step projection history smoke with fresh local BINDIR/BUILDDIR/PARSERDIR"
	@echo "  mbd_system2d_projection_option_fallback_smoke - Run projection knob fallback contract smoke (rebuilds local mbd_system2d_cli on demand)"
	@echo "  mbd_system2d_explicit_projection_smoke - Run constrained explicit + position projection smoke"
	@echo "  mbd_system2d_newmark_projection_smoke - Run constrained Newmark + position projection smoke"
	@echo "  mbd_system2d_hht_projection_smoke - Run constrained HHT + position projection smoke"
	@echo "  help      - Show this help message"

# Dependencies
$(BUILDDIR)/common/globals.o: $(SRCDIR)/common/globals.c $(SRCDIR)/common/globals.h $(SRCDIR)/common/constants.h $(SRCDIR)/common/types.h
$(BUILDDIR)/common/error.o: $(SRCDIR)/common/error.c $(SRCDIR)/common/error.h $(SRCDIR)/common/types.h

# Phony targets
.PHONY: all release debug openmp test unit_test clean clean_all install docs format analyze memcheck profile mbd_probe mbd_a_team_foundation_probe_smoke mbd_a_team_foundation_smoke mbd_a_team_history_contract_layout_smoke mbd_a_team_history_contract_surface_smoke mbd_a_team_history_header_single_source_smoke mbd_a_team_rigid_compare_header_single_source_smoke mbd_a_team_probe_utils_single_source_smoke mbd_a_team_foundation_probe_layout_smoke mbd_a_team_foundation_contract_layout_smoke mbd_a_team_foundation_help_contract_smoke mbd_a_team_foundation_surface_smoke mbd_a_team_history_help_contract_smoke mbd_a_team_history_audit_smoke mbd_a_team_history_contract_smoke mbd_system2d_history_base_probe_smoke mbd_system2d_history_implicit_probe_smoke mbd_system2d_history_probe_contract_smoke mbd_system2d_history_cli_contract_smoke mbd_system2d_history_contract_smoke mbd_implicit_label_review_smoke mbd_rigid_compare_review_smoke mbd_b_team_foundation_probe_smoke mbd_b_team_foundation_smoke mbd_projection2d_probe_smoke mbd_system2d_projection_compare_smoke mbd_system2d_projection_long_compare_smoke mbd_system2d_projection_history_output_smoke mbd_system2d_projection_long_history_output_smoke mbd_output2d_history_field_count_sync_smoke mbd_output2d_rigid_compare_header_single_source_smoke mbd_output2d_rigid_compare_field_count_sync_smoke mbd_rigid_compare_review_columns_sync_smoke compare_2link_artifact_route_fields_getter_test compare_2link_artifact_targets_getter_test compare_2link_artifact_targets_sync_test compare_2link_artifact_integrators_getter_test compare_2link_artifact_integrators_sync_test compare_2link_artifact_route_fields_sync_test mbd_system2d_projection_longrun_contract_smoke mbd_system2d_cli_ready mbd_system2d_cli_ready_contract_smoke mbd_b_team_foundation_isolated_smoke mbd_system2d_projection_long_compare_isolated_smoke mbd_system2d_projection_long_history_isolated_smoke mbd_system2d_projection_option_fallback_smoke mbd_system2d_explicit_projection_smoke mbd_system2d_newmark_projection_smoke mbd_system2d_hht_projection_smoke mbd_system2d_newmark_iteration_fallback_smoke mbd_system2d_hht_iteration_fallback_smoke mbd_regression mbd_m1_rigid_acceptance mbd_m1_rigid_acceptance_test mbd_consistency mbd_negative mbd_checks mbd_run1_surface_docs_sync_test mbd_run1_surface_docs_sync_surfaces_test mbd_run1_surface_docs_sync_surfaces_help_test mbd_run1_surface_docs_sync_surface_smoke mbd_run1_surface_docs_sync_surface_smoke_test mbd_ci_evidence mbd_ci_contract mbd_ci_contract_lock_wait_sync mbd_ci_contract_lock_wait_check_only mbd_b14_regression mbd_b8_regression mbd_b8_regression_test mbd_b8_regression_full mbd_b8_regression_full_test mbd_b8_knob_matrix_test mbd_b8_knob_matrix_smoke_test mbd_a21_regression mbd_a21_regression_test mbd_a24_regression mbd_a24_regression_test mbd_a24_regression_full mbd_a24_regression_full_test mbd_a24_batch mbd_a24_batch_test mbd_a24_acceptance_serial mbd_a24_acceptance_serial_test mbd_ci_contract_test mbd_team_acceptance_gate mbd_ci_contract_test_sha_watch mbd_b45_acceptance t3_orientation_checks parser_compat parser_compat_fallback coupled_stub_check integrator_checks coupled_example_check coupled_rigid_limit_compare_test coupled_rigid_limit_implicit_test coupled_rigid_limit_implicit_compare_test coupled_rigid_limit_thresholds coupled_rigid_limit_thresholds_test coupled_rigid_limit_threshold_docs_sync_test coupled_rigid_limit_manifest_test coupled_flex_manifest_test coupled_flex_reference_real_test coupled_flex_reference_compare_test coupled_flex_reference_artifact_only_test coupled_compare_checks coupled_compare_checks_test coupled_compare_checks_artifact_manifest_test coupled_compare_checks_threshold_provenance_test coupled_compare_checks_out_dir_test coupled_compare_checks_manifest_test coupled_compare_checks_manifest_override_test coupled_compare_checks_subset_test coupled_compare_checks_failfast_test coupled_compare_reason_codes coupled_compare_reason_codes_test coupled_compare_reason_codes_print_test coupled_compare_contract_targets_test coupled_compare_reason_code_docs_sync_test coupled_compare_reason_code_consistency_test coupled_compare_reason_code_audit_cache_test coupled_compare_checks_manifest_print_reason_codes_test coupled_compare_reason_code_contract_checks coupled_compare_reason_code_contract_checks_test coupled_compare_reason_code_contract_audit_report_test coupled_compare_reason_code_root_surface_contract_checks coupled_compare_reason_code_root_surface_contract_checks_test coupled_compare_reason_code_root_surface_contract_audit_report_test coupled_compare_reason_code_root_surface_audit_report_test coupled_compare_reason_code_root_surface_audit_surface_test coupled_compare_reason_code_root_surface_contract_bundle_report_test coupled_compare_checks_manifest_reason_codes_test coupled_2d_acceptance coupled_2d_acceptance_test coupled_2d_acceptance_manifest_test coupled_2d_acceptance_threshold_contract_test coupled_2d_acceptance_stages_test coupled_2d_acceptance_stage_integrators_test coupled_2d_acceptance_compare_stage_integrators_test coupled_2d_acceptance_compare_stage_integrators_stale_binary_test coupled_2d_acceptance_integrators_test coupled_2d_acceptance_invalid_stage_test coupled_2d_acceptance_invalid_integrator_test coupled_2d_acceptance_contract_checks coupled_2d_acceptance_contract_checks_test coupled_2d_acceptance_gate coupled_2d_acceptance_gate_test coupled_2d_acceptance_gate_threshold_provenance_test coupled_2d_acceptance_docs_sync_test coupled_2d_acceptance_docs_sync_surfaces_test coupled_2d_acceptance_docs_sync_surfaces_help_test coupled_2d_acceptance_docs_sync_surface_smoke coupled_2d_acceptance_docs_sync_surface_smoke_test coupled_2d_acceptance_surface_checks coupled_2d_acceptance_surface_checks_test coupled_2d_acceptance_lightweight_checks coupled_2d_acceptance_lightweight_checks_test ensure_fem4c_binary_test coupled_2d_acceptance_wrapper_smoke coupled_2d_acceptance_wrapper_smoke_test coupled_2d_acceptance_resilience_checks coupled_2d_acceptance_resilience_checks_test mbd_rigid_analytic_real_test mbd_rigid_analytic_compare_test mbd_rigid_compare_integrator_review_smoke mbd_rigid_compare_fallback_review_smoke mbd_rigid_compare_expect_route_guard_smoke mbd_rigid_compare_expect_fallback_reason_guard_smoke mbd_rigid_compare_expect_snapshot_count_guard_smoke mbd_rigid_compare_route_matrix_review_smoke mbd_rigid_compare_route_review_smoke mbd_rigid_analytic_hht_compare_test mbd_rigid_analytic_multi_reference_test mbd_rigid_analytic_wrapper_test compare_2link_artifact_check compare_2link_artifact_check_test compare_2link_artifact_manifest_test compare_2link_artifact_manifest_header_contract_test compare_2link_artifact_manifest_snapshot_count_guard_test compare_2link_artifact_check_vars_test compare_2link_artifact_check_integrator_test compare_2link_artifact_check_invalid_integrator_test compare_2link_artifact_matrix_check compare_2link_artifact_matrix_check_test compare_2link_artifact_matrix_manifest_test compare_2link_artifact_matrix_manifest_header_contract_test compare_2link_artifact_matrix_manifest_snapshot_count_guard_test compare_2link_artifact_matrix_manifest_expected_integrators_test compare_2link_artifact_matrix_integrators_test compare_2link_artifact_matrix_invalid_integrator_test compare_2link_artifact_checks fem_model_copy_test q4_t3_stiffness_adapter_warning_test coupled_step_common2d_test coupled_step_common2d_solve_test c_review_spec_smoke_test flex_solver2d_test flex_bc2d_test flex_body2d_inertial_test flex_body2d_interface_center_test flex_body2d_marker_test flex_nodeset_test flex_snapshot2d_test coupled_reassembly_log_test coupled_snapshot_output_test coupled_implicit_snapshot_output_test coupled_nodeset_guard_test mbd_integrator_checks mbd_integrator_checks_test fem4c_test_log_markers fem4c_test_log_markers_test mbd_b8_guard mbd_b8_guard_test mbd_b8_guard_contract mbd_b8_guard_contract_test mbd_b8_syntax mbd_b8_guard_output_test help

# Print variables for debugging
print-%:
	@echo $* = $($*)
