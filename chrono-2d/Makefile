CC := gcc
CFLAGS := -std=c99 -O2 -Wall -Wextra -pedantic -fopenmp
INCLUDES := -Iinclude
LDFLAGS := -fopenmp -lm

SRC_DIR := src
TEST_DIR := tests
OBJ_DIR := build/obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TEST_TARGET := $(TEST_DIR)/test_coupled_constraint
CONTACT_TEST := $(TEST_DIR)/test_contact_regression

.PHONY: all clean test

all: $(TEST_TARGET) $(CONTACT_TEST)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_TARGET): $(TEST_DIR)/test_coupled_constraint.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

$(CONTACT_TEST): $(TEST_DIR)/test_contact_regression.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

test: $(TEST_TARGET) $(CONTACT_TEST)
	@echo "Running coupled constraint test"
	@mkdir -p artifacts
	@$(TEST_TARGET) --descriptor-log artifacts/kkt_descriptor_actions_local.csv
	@echo "Running contact regression test"
	@$(CONTACT_TEST)

clean:
	rm -rf $(OBJ_DIR) $(TEST_TARGET) $(CONTACT_TEST) artifacts
