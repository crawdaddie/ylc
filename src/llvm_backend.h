#ifndef _LANG_LLVM_BACKEND_H
#define _LANG_LLVM_BACKEND_H
#include "symbol_table.h"
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
  SymbolTable *symbol_table;
  LLVMValueRef currentFunction;
  LLVMBasicBlockRef currentBlock;
} Context;

int init_ctx(Context *ctx);

int reinit_ctx(Context *ctx);

void enter_function(Context *ctx, LLVMValueRef function);
void exit_function(Context *ctx, LLVMValueRef parentFunction);

#endif /* ifndef _LANG_LLVM_BACKEND_H */
