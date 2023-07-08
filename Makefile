BACKEND=lib/qbe
src = $(wildcard src/*.c)

obj = $(src:.c=.o)

CC=clang
LD=clang

LLVM_CC_FLAGS=`llvm-config --cflags`
LLVM_LINK_FLAGS=`llvm-config --libs --cflags --ldflags core analysis executionengine mcjit interpreter native`

lang: $(obj)
	$(CC) $(LLVM_CC_FLAGS) -c $(src) $(INCLUDES)
	$(LD) $(LLVM_LINK_FLAGS) $(obj) -o $@ 

.PHONY: clean
clean:
	rm -f src/*.o
	rm -f *.o lang


TEST_DIR = src/test
.PHONY: lang_test_suite
lang_test_suite: $(wildcard $(TEST_DIR)/*.test.simple)
	make lang
	for file in $^ ; do \
		./test_file.sh $${file} ; \
  done
	
HelloWorld: hello.o
	ld -macosx_version_min 11.0.0 -o HelloWorld hello.o -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 

hello.o: experiments/hello.s
	as -o hello.o experiments/hello.s
