#ifndef _LANG_LLVM_BACKEND_H
#define _LANG_LLVM_BACKEND_H
#include "symbol_table.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <stdbool.h>

#define INPUT_BUFSIZE 2048
int LLVMRuntime(int repl, char *path, char *output);

typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef engine;
  SymbolTable *symbol_table;
  LLVMPassManagerRef pass_manager;

  char *module_path;
} Context;

int init_lang_ctx(Context *ctx);

int reinit_lang_ctx(Context *ctx);

void enter_scope(Context *ctx);
void exit_scope(Context *ctx);
LLVMValueRef current_function(Context *ctx);

#endif /* ifndef _LANG_LLVM_BACKEND_H */
