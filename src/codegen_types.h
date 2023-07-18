#ifndef _LANG_CODEGEN_TYPES_H
#define _LANG_CODEGEN_TYPES_H
#include "ast.h"
#include "llvm_backend.h"
void codegen_struct(AST *ast, LLVMTypeRef structType, Context *ctx);

LLVMTypeRef type_lookup(char *type, Context *ctx);
#endif
