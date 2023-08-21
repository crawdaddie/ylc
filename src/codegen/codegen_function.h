#ifndef _LANG_CODEGEN_FUNCTION_H
#define _LANG_CODEGEN_FUNCTION_H
#include "../ast.h"
#include "../llvm_backend.h"
#include <llvm-c/Core.h>
LLVMValueRef codegen_function(AST *ast, Context *ctx);
LLVMValueRef codegen_extern_function(AST *ast, Context *ctx);

LLVMValueRef codegen_call(AST *ast, Context *ctx);

void bind_function(const char *name, LLVMValueRef func, LLVMTypeRef fn_type,
                   ttype type, Context *ctx);
#endif
