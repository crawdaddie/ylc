#ifndef _LANG_CODEGEN_SYMBOL_H
#define _LANG_CODEGEN_SYMBOL_H
#include "ast.h"
#include "llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_identifier(AST *ast, Context *ctx);
LLVMValueRef codegen_symbol_declaration(AST *ast, Context *ctx);
LLVMValueRef codegen_symbol(char *identifier, LLVMValueRef value,
                            LLVMTypeRef type, Context *ctx);
LLVMValueRef codegen_symbol_assignment(AST *ast, Context *ctx);

#endif /* ifndef _LANG_CODEGEN_SYMBOL_H */
