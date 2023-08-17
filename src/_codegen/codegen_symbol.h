#ifndef _LANG_CODEGEN_SYMBOL_H
#define _LANG_CODEGEN_SYMBOL_H
#include "../ast.h"
#include "../llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_identifier(AST *ast, Context *ctx);
LLVMValueRef codegen_symbol_declaration(AST *ast, Context *ctx);
LLVMValueRef codegen_symbol(char *identifier, LLVMValueRef value,
                            LLVMTypeRef type, Context *ctx);
LLVMValueRef codegen_symbol_assignment(AST *ast, Context *ctx);

LLVMValueRef declare_extern_function(char *identifier, LLVMValueRef value,
                                     LLVMTypeRef type, Context *ctx);

SymbolValue get_value(char *identifier, Context *ctx);
void set_value(char *identifier, Context *ctx, SymbolValue val);

#endif /* ifndef _LANG_CODEGEN_SYMBOL_H */
