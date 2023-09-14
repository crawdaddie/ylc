#ifndef _LANG_CODEGEN_CONDITIONAL_H
#define _LANG_CODEGEN_CONDITIONAL_H

#include "../ast.h"
#include "../runner.h"
#include <llvm-c/Core.h>
LLVMValueRef codegen_if_else(AST *ast, Context *ctx);

LLVMValueRef codegen_match(AST *ast, Context *ctx);
#endif
