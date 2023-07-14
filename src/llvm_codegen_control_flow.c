#include "llvm_codegen_control_flow.h"
#include "llvm_codegen.h"
#include <stdlib.h>

LLVMValueRef codegen_if_else(AST *ast, Context *ctx) {
  LLVMValueRef condition = codegen(ast->data.AST_IF_ELSE.condition, ctx);
  if (!condition) {
    return NULL;
  }
  return NULL;
};
