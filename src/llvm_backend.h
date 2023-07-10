#ifndef _LANG_LLVM_BACKEND_H
#define _LANG_LLVM_BACKEND_H
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <stdbool.h>

#define INPUT_BUFSIZE 2048
int LLVMRuntime(int repl, char *path);
typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef engine;
} Context;

int init_ctx(Context *ctx);

int reinit_ctx(Context *ctx);

#endif /* ifndef _LANG_LLVM_BACKEND_H */
