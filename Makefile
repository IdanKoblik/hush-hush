.DELETE_ON_ERROR:
.PHONY: all compdb test coverage format format-check logs clean

CC           := cc
CLANG_FORMAT := clang-format

LOCAL_LIB_DIR := libs
INC_DIR  := include
SRC_DIR  := src
TEST_DIR := tests
OBJ_DIR  := build
COV_DIR  := coverage
COV_OBJ  := $(COV_DIR)/obj

BIN      := hh
TEST_BIN := $(OBJ_DIR)/test_runner

# Depenencies

SODIUM_CFLAGS := $(shell pkg-config --cflags libsodium)
SODIUM_LIBS   := $(shell pkg-config --libs libsodium)

# Flags

CFLAGS := -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -pthread \
          -I$(INC_DIR) \
          -I$(LOCAL_LIB_DIR) \
          $(SODIUM_CFLAGS)

LDFLAGS := -Wl,-T,linker.ld

LDLIBS := -pthread \
          -lm \
          $(SODIUM_LIBS)

# The test sources include greatest.h from tests/ directly.
TEST_CFLAGS := $(CFLAGS) -I$(TEST_DIR)

# Sources
SRCS := $(sort $(shell find $(SRC_DIR) -name '*.c'))
HDRS := $(shell find $(INC_DIR) -name '*.h')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c')
TEST_HDRS := $(filter-out $(TEST_DIR)/greatest.h,$(shell find $(TEST_DIR) -name '*.h'))
# Link every production object except main.o, which owns its own main().
TEST_OBJS := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

FORMAT_SRCS := $(HDRS) $(SRCS) $(TEST_HDRS) $(TEST_SRCS)

# Build

all: $(BIN) compile_commands.json

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) $(TEST_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(TEST_CFLAGS) $(TEST_SRCS) $(TEST_OBJS) -o $@ $(LDLIBS)

COV_SRCS := $(filter-out $(SRC_DIR)/main.c,$(SRCS)) $(TEST_SRCS)
COV_OBJS := $(COV_SRCS:%.c=$(COV_OBJ)/%.o)

coverage: $(COV_OBJS)
	$(CC) $(LDFLAGS) $(COV_OBJS) --coverage -o $(COV_DIR)/test_runner $(LDLIBS)
	./$(COV_DIR)/test_runner
	lcov --capture --directory $(COV_OBJ) --output-file $(COV_DIR)/coverage.info
	lcov --remove $(COV_DIR)/coverage.info '/usr/*' '*/libs/*' \
		--output-file $(COV_DIR)/coverage.info
	genhtml $(COV_DIR)/coverage.info --output-directory $(COV_DIR)/html
	@echo "Coverage report: $(COV_DIR)/html/index.html"

$(COV_OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) --coverage -c $< -o $@

compdb: compile_commands.json

compile_commands.json: $(SRCS) $(TEST_SRCS) Makefile
	@{ printf '[\n'; sep=""; \
	   for f in $(SRCS); do \
	       printf '%s  {"directory": "%s", "file": "%s", "command": "%s %s -c %s"}\n' \
	           "$$sep" "$(CURDIR)" "$$f" "$(CC)" "$(CFLAGS)" "$$f"; sep=","; \
	   done; \
	   for f in $(TEST_SRCS); do \
	       printf '%s  {"directory": "%s", "file": "%s", "command": "%s %s -c %s"}\n' \
	           "$$sep" "$(CURDIR)" "$$f" "$(CC)" "$(TEST_CFLAGS)" "$$f"; sep=","; \
	   done; \
	   printf ']\n'; } > $@
	@echo "Wrote $@ ($(words $(SRCS) $(TEST_SRCS)) entries)"

format:
	$(CLANG_FORMAT) -i $(FORMAT_SRCS)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SRCS)

logs:
	journalctl -t $(BIN) -o short-iso > $(LOG)
	@echo "Logs written to $(LOG)"

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LOG) $(COV_DIR)

-include $(DEPS)
