#include "llvm_backend.h"
#include <stdio.h>

int main(int argc, char **argv) {
  setbuf(stdout, NULL); // turn off line buffering for stdout
  int repl = 0;
  repl = argc == 1;
  return LLVMRuntime(repl, argc == 2 ? argv[1] : NULL);
}
