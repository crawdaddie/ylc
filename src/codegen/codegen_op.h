#ifndef _LANG_CODEGEN_BINOP_H
#define _LANG_CODEGEN_BINOP_H
#include "../ast.h"
#include "../llvm_backend.h"

LLVMValueRef codegen_binop(AST *ast, Context *ctx);
LLVMValueRef codegen_unop(AST *ast, Context *ctx);
#endif
