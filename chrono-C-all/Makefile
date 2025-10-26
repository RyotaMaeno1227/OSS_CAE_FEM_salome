CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Iinclude -fopenmp
LDFLAGS := -lm

SRC_DIR := src
TEST_DIR := tests

CORE_SRCS := $(wildcard $(SRC_DIR)/*.c)
TEST_TARGETS := \
		$(TEST_DIR)/test_distance_constraint_stabilization \
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
	$(TEST_DIR)/test_island_builder \
	$(TEST_DIR)/test_contact_manager_longrun \
	$(TEST_DIR)/test_constraint_batch_solve

BENCH_TARGETS := \
	$(TEST_DIR)/bench_island_solver

.PHONY: all clean test tests

all: $(TEST_TARGETS)

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(TEST_DIR)/bench_%: $(TEST_DIR)/bench_%.c $(CORE_SRCS)
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

clean:
	$(RM) $(TEST_TARGETS) $(BENCH_TARGETS)
