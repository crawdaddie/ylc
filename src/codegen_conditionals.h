#ifndef _LANG_CODEGEN_CONDITIONALS_H
#define _LANG_CODEGEN_CONDITIONALS_H
#include "ast.h"
#include "llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_if_else(AST *ast, Context *ctx);
LLVMValueRef codegen_match(AST *ast, Context *ctx);
#endif /* ifndef _LANG_CODEGEN_CONTROL_FLOW_H */
