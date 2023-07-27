#ifndef _LANG_CODEGEN_FUNCTION_H
#define _LANG_CODEGEN_FUNCTION_H
#include "ast.h"
#include "llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_function(AST *ast, Context *ctx);

LLVMValueRef codegen_call(AST *ast, Context *ctx);

LLVMValueRef codegen_function_prototoype(AST *ast, Context *ctx);

void codegen_prototype(AST *ast, Context *ctx, LLVMValueRef *func,
                       LLVMTypeRef *func_type, LLVMTypeRef *func_return_type,
                       const char *name);

LLVMValueRef codegen_extern_function(AST *ast, Context *ctx);

LLVMValueRef codegen_named_function(AST *ast, Context *ctx, char *name);
// LLVMTypeRef *codegen_function_prototype(AST *ast, Context *ctx);

#endif /* ifndef _LANG_CODEGEN_FUNCTION_H */
