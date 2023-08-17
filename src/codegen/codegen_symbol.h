#ifndef _LANG_CODEGEN_SYMBOL_H
#define _LANG_CODEGEN_SYMBOL_H
#include "../ast.h"
#include "../llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_symbol(const char *name, ttype type, AST *expr,
                            Context *ctx);
LLVMValueRef codegen_identifier(AST *ast, Context *ctx);
#endif
