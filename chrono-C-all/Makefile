CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Iinclude -fopenmp
LDFLAGS := -lm

SRC_DIR := src
TEST_DIR := tests
EXAMPLE_DIR := examples

CORE_SRCS := $(wildcard $(SRC_DIR)/*.c)
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
	$(TEST_DIR)/test_constraint_batch_solve

BENCH_TARGETS := \
	$(TEST_DIR)/bench_island_solver \
	$(TEST_DIR)/bench_coupled_constraint

EXAMPLE_TARGETS := \
	$(EXAMPLE_DIR)/newton_cradle \
	$(EXAMPLE_DIR)/prismatic_slider \
	$(EXAMPLE_DIR)/planar_constraint_demo \
	$(EXAMPLE_DIR)/logging_integration_demo

.PHONY: all clean test tests examples

all: $(TEST_TARGETS) $(EXAMPLE_TARGETS)

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(TEST_DIR)/bench_%: $(TEST_DIR)/bench_%.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(EXAMPLE_DIR)/%: $(EXAMPLE_DIR)/%.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

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
