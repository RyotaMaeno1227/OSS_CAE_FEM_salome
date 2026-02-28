CC := gcc
CFLAGS := -std=c99 -O2 -Wall -Wextra -pedantic -fopenmp
INCLUDES := -Iinclude
LDFLAGS := -fopenmp -lm

SRC_DIR := src
TEST_DIR := tests
OBJ_DIR := build/obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c) $(TEST_DIR)/bench_constraints.c
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TEST_TARGET := $(TEST_DIR)/test_coupled_constraint
CONTACT_TEST := $(TEST_DIR)/test_contact_regression
BENCH_TARGET := $(TEST_DIR)/bench_constraints
SCHEMA_TEST := $(TEST_DIR)/test_schema
MINI_TEST := $(TEST_DIR)/test_minicase

RUN_TIMED := python scripts/run_timed.py
MAX_TEST_TIME_SEC ?= 30
MAX_BENCH_TIME_SEC ?= 10
MAX_SCHEMA_TIME_SEC ?= 10

.PHONY: all clean test bench schema minicase

all: $(TEST_TARGET) $(CONTACT_TEST) $(BENCH_TARGET) $(SCHEMA_TEST) $(MINI_TEST)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_TARGET): $(TEST_DIR)/test_coupled_constraint.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

$(CONTACT_TEST): $(TEST_DIR)/test_contact_regression.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

$(BENCH_TARGET): $(TEST_DIR)/bench_constraints.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

$(SCHEMA_TEST): $(TEST_DIR)/test_schema.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

$(MINI_TEST): $(TEST_DIR)/test_minicase.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

test: $(TEST_TARGET) $(CONTACT_TEST) $(MINI_TEST)
	@echo "Running coupled constraint test"
	@mkdir -p artifacts
	@$(RUN_TIMED) --label test_coupled --max-seconds $(MAX_TEST_TIME_SEC) -- \
		$(TEST_TARGET) --descriptor-log artifacts/kkt_descriptor_actions_local.csv
	@echo "Running contact regression test"
	@$(RUN_TIMED) --label test_contact --max-seconds $(MAX_TEST_TIME_SEC) -- \
		$(CONTACT_TEST)
	@echo "Running mini-case test"
	@$(RUN_TIMED) --label test_minicase --max-seconds $(MAX_TEST_TIME_SEC) -- \
		$(MINI_TEST)

bench: $(BENCH_TARGET)
	@mkdir -p artifacts
	@$(RUN_TIMED) --label bench --max-seconds $(MAX_BENCH_TIME_SEC) -- \
		$(BENCH_TARGET) --output artifacts/bench_constraints.csv --warn-only

schema: $(SCHEMA_TEST)
	@echo "Running schema test"
	@$(RUN_TIMED) --label test_schema --max-seconds $(MAX_SCHEMA_TIME_SEC) -- \
		$(SCHEMA_TEST)

minicase: $(MINI_TEST)
	@echo "Running mini-case test"
	@$(RUN_TIMED) --label test_minicase --max-seconds $(MAX_TEST_TIME_SEC) -- \
		$(MINI_TEST)

clean:
	rm -rf $(OBJ_DIR) $(TEST_TARGET) $(CONTACT_TEST) $(BENCH_TARGET) artifacts
