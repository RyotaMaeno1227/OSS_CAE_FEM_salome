# FEM4C - High Performance Finite Element Method in C
# Makefile for building the FEM solver (+ parser)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -mcmodel=large
OPENMP_FLAGS = -fopenmp
DEBUG_FLAGS = -g -DDEBUG
INCLUDES = -Isrc/common -Isrc/elements -Isrc/solver -Isrc/io -Isrc/mesh -Isrc/material -Isrc/analysis

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
ANALYSIS_SRCS = $(SRCDIR)/analysis/static.c
MAIN_SRCS = $(SRCDIR)/fem4c.c

# All source files
SRCS = $(COMMON_SRCS) $(IO_SRCS) $(MESH_SRCS) $(MATERIAL_SRCS) $(ELEMENT_SRCS) $(SOLVER_SRCS) $(ANALYSIS_SRCS) $(MAIN_SRCS)

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/fem4c

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
	@echo "Linking $@"
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Build parser executable
$(PARSER_TARGET): $(PARSER_SRC)
	@echo "Building $@"
	$(CC) $(PARSER_CFLAGS) $(PARSER_INCLUDES) -o $@ $< -lm

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

# Unit test target
unit_test:
	@echo "Building unit tests..."
	@$(MAKE) -C $(TESTDIR)/unit

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
	@echo "  test      - Run tests"
	@echo "  unit_test - Build and run unit tests"
	@echo "  clean     - Remove build files"
	@echo "  clean_all - Remove all generated files"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  docs      - Generate documentation (requires doxygen)"
	@echo "  format    - Format source code (requires clang-format)"
	@echo "  analyze   - Run static analysis (requires cppcheck)"
	@echo "  memcheck  - Run memory leak detection (requires valgrind)"
	@echo "  profile   - Build for performance profiling"
	@echo "  help      - Show this help message"

# Dependencies
$(BUILDDIR)/common/globals.o: $(SRCDIR)/common/globals.c $(SRCDIR)/common/globals.h $(SRCDIR)/common/constants.h $(SRCDIR)/common/types.h
$(BUILDDIR)/common/error.o: $(SRCDIR)/common/error.c $(SRCDIR)/common/error.h $(SRCDIR)/common/types.h

# Phony targets
.PHONY: all release debug openmp test unit_test clean clean_all install docs format analyze memcheck profile help

# Print variables for debugging
print-%:
	@echo $* = $($*)
