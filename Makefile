# Define the source files and object files directories
SRC_DIR := src
BUILD_DIR := build

# List all the source files and object files
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

CC=clang
LD=clang

LLVM_CC_FLAGS=`llvm-config --cflags`
LLVM_LINK_FLAGS=`llvm-config --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`
LINK=-lpthread

# Default target
build/lang: $(OBJ)
	$(LD) $(LLVM_LINK_FLAGS) $(C_FLAGS) $(OBJ) $(LINK) -o $@

# Rule to build object files from source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(LLVM_CC_FLAGS) $(C_FLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean target
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Define the test source files and object files directories
TEST_DIR := tests
TEST_SRC := $(filter-out $(SRC_DIR)/main.c, $(SRC))
TEST_SRC += $(TEST_DIR)/utils.c

# List all the test object files
TEST_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(TEST_SRC))
build/parse_test: $(TEST_OBJ) $(BUILD_DIR)/parse_test.o
	$(LD) $(LLVM_LINK_FLAGS) $^ -o $@
	./$@

build/codegen_test: $(TEST_OBJ) $(BUILD_DIR)/codegen_test.o
	$(LD) $(LLVM_LINK_FLAGS) $^ -o $@
	./$@

build/typecheck_test: $(TEST_OBJ) $(BUILD_DIR)/typecheck_test.o
	$(LD) $(LLVM_LINK_FLAGS) $^ -o $@
	./$@

build/symbol_table_test: $(TEST_OBJ) $(BUILD_DIR)/symbol_table_test.o
	$(LD) $(LLVM_LINK_FLAGS) $^ -o $@
	./$@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(LLVM_CC_FLAGS) $(C_FLAGS) -c $< -o $@

.PHONY: debug-lang
debug-lang:
	make && lldb build/lang

.PHONY: repl
repl:
	make && ./build/lang

.PHONY: run
run:
	make && ./build/lang $(input)

.PHONY: yalce-synth 
yalce-synth:
	(cd ~/projects/yalce && make libyalce_synth.so && cp libyalce_synth.so ~/projects/langs/ylc/libs/)

EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC 
.PHONY: libffi
libffi:
	$(CC) -shared -o libffi.so experiments/libffi.c $(EXPORT_COMPILER_OPTIONS)


.PHONY: tests
tests:
	make clean
	make build/parse_test
	make build/codegen_test
	make build/symbol_table_test
	make build/typecheck_test

.PHONY: e2e
e2e:
	make clean
	make
	build/lang examples/if.ylc
	build/lang examples/fib.ylc
	build/lang examples/match.ylc
	build/lang examples/match_rec.ylc
