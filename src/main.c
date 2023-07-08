#include "lang_runner.h"
#include <stdio.h>
#include <stdlib.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

#include "llvm_backend.h"

int main(int argc, char **argv) {
  setbuf(stdout, NULL); // turn off line buffering for stdout
  int repl = 0;
  repl = argc == 1;
  return LLVMRuntime(repl, argc == 2 ? argv[1] : NULL);
}
