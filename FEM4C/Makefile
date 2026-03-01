# FEM4C - High Performance Finite Element Method in C
# Makefile for building the FEM solver (+ parser)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -mcmodel=large
OPENMP_FLAGS = -fopenmp
DEBUG_FLAGS = -g -DDEBUG
INCLUDES = -Isrc/common -Isrc/elements -Isrc/solver -Isrc/io -Isrc/mesh -Isrc/material -Isrc/analysis -Isrc/mbd

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
ELEMENT_SRCS = $(SRCDIR)/elements/element_base.c $(SRCDIR)/elements/elements.c \
               $(SRCDIR)/elements/t6/t6_element.c $(SRCDIR)/elements/t6/t6_stiffness.c \
               $(SRCDIR)/elements/q4/q4_element.c $(SRCDIR)/elements/q4/q4_stiffness.c \
               $(SRCDIR)/elements/t3/t3_element.c
SOLVER_SRCS = $(SRCDIR)/solver/assembly.c $(SRCDIR)/solver/cg_solver.c
ANALYSIS_SRCS = $(SRCDIR)/analysis/static.c $(SRCDIR)/analysis/runner.c
MBD_SRCS = $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/kkt2d.c
MAIN_SRCS = $(SRCDIR)/fem4c.c

# All source files
SRCS = $(COMMON_SRCS) $(IO_SRCS) $(MESH_SRCS) $(MATERIAL_SRCS) $(ELEMENT_SRCS) $(SOLVER_SRCS) $(ANALYSIS_SRCS) $(MBD_SRCS) $(MAIN_SRCS)

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/fem4c
MBD_PROBE_SRC = practice/ch09/mbd_constraint_probe.c
MBD_PROBE_TARGET = $(BINDIR)/mbd_constraint_probe
MBD_PROBE_SRCS = $(SRCDIR)/mbd/constraint2d.c $(SRCDIR)/mbd/kkt2d.c $(SRCDIR)/common/error.c
MBD_REGRESSION_SCRIPT = scripts/run_mbd_regression.sh
MBD_CONSISTENCY_SCRIPT = scripts/check_mbd_equation_consistency.sh
MBD_NEGATIVE_SCRIPT = scripts/check_mbd_invalid_inputs.sh
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
B8_GUARD_SCRIPT = scripts/run_b8_guard.sh
B8_GUARD_TEST_SCRIPT = scripts/test_run_b8_guard.sh
B8_GUARD_CONTRACT_SCRIPT = scripts/run_b8_guard_contract.sh
B8_GUARD_CONTRACT_TEST_SCRIPT = scripts/test_run_b8_guard_contract.sh
B8_SYNTAX_CHECK_SCRIPT = scripts/check_b8_scripts_syntax.sh
B8_OUTPUT_TEST_SCRIPT = scripts/test_check_b8_guard_output.sh
COUPLED_INTEGRATOR_SCRIPT = scripts/check_coupled_integrators.sh
MBD_INTEGRATOR_SCRIPT = scripts/check_mbd_integrators.sh
MBD_INTEGRATOR_TEST_SCRIPT = scripts/test_check_mbd_integrators.sh
FEM4C_TEST_LOG_MARKER_SCRIPT = scripts/check_fem4c_test_log_markers.sh
FEM4C_TEST_LOG_MARKER_TEST_SCRIPT = scripts/test_check_fem4c_test_log_markers.sh

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

# Build + run MBD probe in one command
mbd_probe: $(MBD_PROBE_TARGET)
	@echo "Running $(MBD_PROBE_TARGET)"
	@./$(MBD_PROBE_TARGET)

# One-command regression for MBD mode input adapter path
mbd_regression: $(TARGET)
	@echo "Running MBD mode regression script"
	@bash $(MBD_REGRESSION_SCRIPT)

# Consistency check between mbd runtime log and probe equation count
mbd_consistency: $(TARGET) $(MBD_PROBE_TARGET)
	@echo "Running MBD equation consistency check"
	@bash $(MBD_CONSISTENCY_SCRIPT)

# Negative-path check for invalid MBD_* inputs
mbd_negative: $(TARGET)
	@echo "Running invalid-input diagnostics check"
	@bash $(MBD_NEGATIVE_SCRIPT)

# Run all MBD checks in one command
mbd_checks: mbd_probe mbd_regression mbd_consistency mbd_negative mbd_integrator_checks
	@echo "PASS: all MBD checks completed"

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
	@bash $(MBD_CI_CONTRACT_TEST_SCRIPT)
	@bash $(FEM4C_TEST_LOG_MARKER_TEST_SCRIPT)

# Verify T3 orientation behavior in default/strict modes
t3_orientation_checks: $(TARGET)
	@bash $(T3_ORIENTATION_SCRIPT)

# Verify legacy parser package + Nastran parser compatibility
parser_compat: $(TARGET)
	@bash $(PARSER_COMPAT_SCRIPT)

# Verify parser compatibility using built-in legacy fallback fixture
parser_compat_fallback: $(TARGET)
	@FEM4C_PARSER_COMPAT_FORCE_FALLBACK=1 bash $(PARSER_COMPAT_SCRIPT)

# Verify coupled-mode stub emits contract snapshot and fails as expected
coupled_stub_check: $(TARGET)
	@bash $(COUPLED_STUB_SCRIPT)

# Verify coupled integrator switching (newmark/hht + invalid fallback)
integrator_checks: $(TARGET)
	@bash $(COUPLED_INTEGRATOR_SCRIPT)

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
	@echo "Running coupled stub checks (coupled_stub_check)..."
	@$(MAKE) coupled_stub_check
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
	@echo "  test      - Run tests (+ mbd_checks[incl. mbd_integrator_checks] + parser_compat + coupled_stub_check + integrator_checks)"
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
	@echo "  mbd_regression - Run one-command MBD regression (positive + negative, stable error-code check)"
	@echo "  mbd_consistency - Compare runtime and probe equation counts"
	@echo "  mbd_negative - Verify invalid-input failures with line diagnostics + DIAG_CODES_SEEN"
	@echo "  mbd_checks - Run all MBD checks (probe/regression/consistency/negative/integrator)"
	@echo "  mbd_ci_evidence - Fetch CI run evidence with strict acceptance (step+artifact)"
	@echo "  mbd_ci_contract - Validate required FEM4C CI contract in .github/workflows/ci.yaml"
	@echo "  mbd_ci_contract_lock_wait_sync - Enforce strict lock_wait_max sync contract"
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
	@echo "  t3_orientation_checks - Verify default correction + strict expected-fail for clockwise T3"
	@echo "  parser_compat - Verify legacy parser package + Nastran parser compatibility"
	@echo "  parser_compat_fallback - Verify parser compatibility with built-in legacy fallback fixture"
	@echo "  coupled_stub_check - Verify coupled stub non-zero path + contract snapshot logs"
	@echo "  integrator_checks - Verify coupled integrator switch (newmark/hht + fallback)"
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
	@echo "  help      - Show this help message"

# Dependencies
$(BUILDDIR)/common/globals.o: $(SRCDIR)/common/globals.c $(SRCDIR)/common/globals.h $(SRCDIR)/common/constants.h $(SRCDIR)/common/types.h
$(BUILDDIR)/common/error.o: $(SRCDIR)/common/error.c $(SRCDIR)/common/error.h $(SRCDIR)/common/types.h

# Phony targets
.PHONY: all release debug openmp test unit_test clean clean_all install docs format analyze memcheck profile mbd_probe mbd_regression mbd_consistency mbd_negative mbd_checks mbd_ci_evidence mbd_ci_contract mbd_ci_contract_lock_wait_sync mbd_b14_regression mbd_b8_regression mbd_b8_regression_test mbd_b8_regression_full mbd_b8_regression_full_test mbd_b8_knob_matrix_test mbd_b8_knob_matrix_smoke_test mbd_a21_regression mbd_a21_regression_test mbd_a24_regression mbd_a24_regression_test mbd_a24_regression_full mbd_a24_regression_full_test mbd_a24_batch mbd_a24_batch_test mbd_a24_acceptance_serial mbd_a24_acceptance_serial_test mbd_ci_contract_test t3_orientation_checks parser_compat parser_compat_fallback coupled_stub_check integrator_checks mbd_integrator_checks mbd_integrator_checks_test fem4c_test_log_markers fem4c_test_log_markers_test mbd_b8_guard mbd_b8_guard_test mbd_b8_guard_contract mbd_b8_guard_contract_test mbd_b8_syntax mbd_b8_guard_output_test help

# Print variables for debugging
print-%:
	@echo $* = $($*)
