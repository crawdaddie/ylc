src = $(wildcard src/*.c)

obj = $(src:.c=.o)

CC=clang
LD=clang

LLVM_CC_FLAGS=`llvm-config --cflags`
LLVM_LINK_FLAGS=`llvm-config --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`

build/lang: $(obj)
	$(CC) $(LLVM_CC_FLAGS) -c $(src) $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(obj) -o $@ 

.PHONY: clean
clean:
	rm -f build/src/*.o
	rm -f build/lang


TEST_DIR = src/test
.PHONY: lang_test_suite
lang_test_suite: $(wildcard $(TEST_DIR)/*.test.simple)
	make build/lang
	for file in $^ ; do \
		./test_file.sh $${file} ; \
  done
	
build/HelloWorld: build/hello.o
	ld -macosx_version_min 11.0.0 -o $@ build/hello.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 

build/hello.o: experiments/hello.s
	as -o $@ experiments/hello.s

TEST_SRC = $(filter-out src/main.c, $(src))
TEST_SRC += src/test/utils.c 
TEST_SRC += src/test/parse.c 
TEST_OBJ = $(TEST_SRC:.c=.o)

build/test_parser: $(TEST_OBJ)
	$(CC) $(LLVM_CC_FLAGS) -c $(TEST_SRC)
	$(LD) $(LLVM_LINK_FLAGS) $(TEST_OBJ) -o $@ 
	./build/test_parser

.PHONY: debug-lang
debug-lang:
	make && lldb build/lang
