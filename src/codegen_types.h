#ifndef _LANG_CODEGEN_TYPES_H
#define _LANG_CODEGEN_TYPES_H
#include "ast.h"
#include "llvm_backend.h"
void codegen_struct(AST *ast, LLVMTypeRef structType, Context *ctx);

LLVMTypeRef type_lookup(char *type, Context *ctx);

LLVMTypeRef codegen_type(AST *ast, char *name, Context *ctx);
type_symbol_table *compute_type_metadata(AST *type_expr);
type_symbol_table *get_type_metadata(char *type_name, Context *ctx);

LLVMValueRef struct_instance_with_metadata(AST *expr, LLVMTypeRef type,
                                           type_symbol_table *metadata,
                                           Context *ctx);

int get_member_index(char *member, type_symbol_table *metadata);
#endif
