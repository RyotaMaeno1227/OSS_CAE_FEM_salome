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
	$(TEST_DIR)/test_circle_collision_materials \
	$(TEST_DIR)/test_constraint_batch_solve

.PHONY: all clean test tests

all: $(TEST_TARGETS)

$(TEST_DIR)/test_%: $(TEST_DIR)/test_%.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

tests: $(TEST_TARGETS)

test: tests
	@set -e; \
	for t in $(TEST_TARGETS); do \
		echo "Running $$t"; \
		"$$t"; \
	done

clean:
	$(RM) $(TEST_TARGETS)
