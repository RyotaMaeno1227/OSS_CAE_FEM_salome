TBB_INCLUDE_DIR ?=
TBB_LIBS ?=

CC := gcc
CXX := g++
BASE_INCLUDES := -Iinclude
TBB_INCLUDE_FLAGS := $(if $(strip $(TBB_INCLUDE_DIR)),-I$(TBB_INCLUDE_DIR),)
CFLAGS := -std=c99 -Wall -Wextra -pedantic -fopenmp $(BASE_INCLUDES) $(TBB_INCLUDE_FLAGS)
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic $(BASE_INCLUDES) $(TBB_INCLUDE_FLAGS)
LDFLAGS := -lm -fopenmp $(TBB_LIBS) -lstdc++

SRC_DIR := src
TEST_DIR := tests
EXAMPLE_DIR := examples
OBJ_DIR := build/obj

CORE_C_SRCS := $(wildcard $(SRC_DIR)/*.c)
CORE_CPP_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
CORE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.c.o,$(CORE_C_SRCS)) \
	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.cpp.o,$(CORE_CPP_SRCS))
TEST_TARGETS := \
	$(TEST_DIR)/test_distance_constraint_stabilization \
	$(TEST_DIR)/test_distance_constraint_soft \
	$(TEST_DIR)/test_distance_angle_constraint \
	$(TEST_DIR)/test_distance_angle_endurance \
	$(TEST_DIR)/test_coupled_constraint \
	$(TEST_DIR)/test_coupled_logging_integration \
	$(TEST_DIR)/test_coupled_constraint_endurance \
	$(TEST_DIR)/test_circle_collision_regression \
	$(TEST_DIR)/test_circle_collision_oblique \
	$(TEST_DIR)/test_circle_collision_resting \
	$(TEST_DIR)/test_circle_collision_friction \
	$(TEST_DIR)/test_circle_collision_continuous \
	$(TEST_DIR)/test_circle_collision_materials \
	$(TEST_DIR)/test_circle_collision_continuous \
	$(TEST_DIR)/test_island_contact_constraint \
	$(TEST_DIR)/test_island_parallel_contacts \
	$(TEST_DIR)/test_island_parallel_manifold \
	$(TEST_DIR)/test_coupled_contact_combo \
	$(TEST_DIR)/test_polygon_collision \
	$(TEST_DIR)/test_polygon_slope_friction \
	$(TEST_DIR)/test_polygon_spin_collision \
	$(TEST_DIR)/test_polygon_mass_properties \
	$(TEST_DIR)/test_capsule_edge_collision \
	$(TEST_DIR)/test_island_polygon_longrun \
	$(TEST_DIR)/test_island_builder \
	$(TEST_DIR)/test_contact_manager_longrun \
	$(TEST_DIR)/test_revolute_constraint \
	$(TEST_DIR)/test_planar_constraint \
	$(TEST_DIR)/test_planar_constraint_longrun \
	$(TEST_DIR)/test_planar_constraint_endurance \
	$(TEST_DIR)/test_gear_constraint \
	$(TEST_DIR)/test_prismatic_constraint \
	$(TEST_DIR)/test_prismatic_constraint_endurance \
	$(TEST_DIR)/test_distance_constraint_multi \
	$(TEST_DIR)/test_spring_constraint \
	$(TEST_DIR)/test_constraint_batch_solve \
	$(TEST_DIR)/test_constraint_common_abi

BENCH_TARGETS := \
	$(TEST_DIR)/bench_island_solver \
	$(TEST_DIR)/bench_coupled_constraint

EXAMPLE_TARGETS := \
	$(EXAMPLE_DIR)/newton_cradle \
	$(EXAMPLE_DIR)/prismatic_slider \
	$(EXAMPLE_DIR)/planar_constraint_demo \
	$(EXAMPLE_DIR)/logging_integration_demo

.PHONY: all clean test tests examples

all: $(CORE_OBJS) $(TEST_TARGETS) $(EXAMPLE_TARGETS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $< $(CORE_OBJS) $(LDFLAGS) -o $@

$(TEST_DIR)/bench_%: $(TEST_DIR)/bench_%.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $< $(CORE_OBJS) $(LDFLAGS) -o $@

$(EXAMPLE_DIR)/%: $(EXAMPLE_DIR)/%.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $< $(CORE_OBJS) $(LDFLAGS) -o $@

tests: $(TEST_TARGETS)

test: tests
	@set -e; \
	for t in $(TEST_TARGETS); do \
		echo "Running $$t"; \
		"$$t"; \
	done

bench: $(BENCH_TARGETS)
	@set -e; \
	for b in $(BENCH_TARGETS); do \
		echo "Running $$b"; \
		"$$b"; \
	done

examples: $(EXAMPLE_TARGETS)

clean:
	$(RM) $(TEST_TARGETS) $(BENCH_TARGETS) $(EXAMPLE_TARGETS)
	rm -rf $(OBJ_DIR)
