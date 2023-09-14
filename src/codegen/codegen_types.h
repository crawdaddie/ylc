#ifndef _LANG_CODEGEN_TYPES_H
#define _LANG_CODEGEN_TYPES_H
#include "../ast.h"
#include "../runner.h"

LLVMTypeRef codegen_type(AST *ast, Context *ctx);

LLVMTypeRef codegen_ttype(ttype type, Context *ctx);

#endif
