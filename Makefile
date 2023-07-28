src = $(wildcard src/*.c)

obj = $(src:.c=.o)

CC=clang
LD=clang

LLVM_CC_FLAGS=`llvm-config --cflags`
LLVM_LINK_FLAGS=`llvm-config --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`
LINK=-lpthread

build/lang: $(obj)
	$(CC) $(LLVM_CC_FLAGS) -c $(src) $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(obj) $(LINK) -o $@ 

.PHONY: clean
clean:
	rm -f build/src/*.o
	rm -f build/lang

.PHONY: wasm
wasm:


TEST_DIR = src/test
.PHONY: lang_test_suite
lang_test_suite: $(wildcard $(TEST_DIR)/*.test.simple)
	make build/lang
	for file in $^ ; do \
		./test_file.sh $${file} ; \
  done
	
TEST_SRC = $(filter-out src/main.c, $(src))
TEST_SRC += tests/utils.c 
# TEST_SRC += tests/parse_test.c 
TEST_OBJ = $(TEST_SRC:.c=.o)

build/test_parser: $(TEST_OBJ)
	mkdir -p build
	$(CC) $(LLVM_CC_FLAGS) -c $(TEST_SRC) tests/parse_test.c $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(TEST_OBJ) parse_test.o -o $@
	./build/test_parser


build/test_codegen: $(TEST_OBJ)
	mkdir -p build
	$(CC) $(LLVM_CC_FLAGS) -c $(TEST_SRC) tests/codegen_test.c $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(TEST_OBJ) codegen_test.o -o $@
	./build/test_codegen

build/test_typecheck: $(TEST_OBJ)
	mkdir -p build
	$(CC) $(LLVM_CC_FLAGS) -c $(TEST_SRC) tests/typecheck_test.c $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(TEST_OBJ) typecheck_test.o -o $@
	./build/test_typecheck


.PHONY: debug-lang
debug-lang:
	make && lldb build/lang

.PHONY: repl
repl:
	make && ./build/lang

.PHONY: test
test:
	make && ./build/lang examples/test.ylc

.PHONY: yalce-synth 
yalce-synth:
	(cd ~/projects/yalce && make libyalce_synth.so && cp libyalce_synth.so ~/projects/langs/ylc/libs/)

EXPORT_COMPILER_OPTIONS = -Werror -Wall -Wextra -fPIC 
.PHONY: libffi
libffi:
	$(CC) -shared -o libffi.so experiments/libffi.c $(EXPORT_COMPILER_OPTIONS)
