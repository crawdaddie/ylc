#include "codegen_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMTypeRef type_lookup(char *type, Context *ctx) {
  // TODO: make this more smart / quick
  if (strcmp(type, "int") == 0) {
    return LLVMInt32TypeInContext(ctx->context);
  }

  if (strcmp(type, "double") == 0) {
    return LLVMDoubleTypeInContext(ctx->context);
  }

  if (strcmp(type, "bool") == 0) {
    return LLVMInt1Type();
  }

  if (strcmp(type, "str") == 0) {
    return LLVMPointerType(LLVMInt8Type(), 0);
  }

  SymbolValue v;
  if (table_lookup(ctx->symbol_table, type, &v) == 0) {
    return v.data.TYPE_TYPE_DECLARATION.llvm_type;
  }

  return NULL;
}
void codegen_struct(AST *ast, LLVMTypeRef structType, Context *ctx) {
  int num_members = ast->data.AST_STRUCT.length;
  AST **members = ast->data.AST_STRUCT.members;

  LLVMTypeRef *structFields = malloc(sizeof(LLVMTypeRef) * num_members);
  for (int i = 0; i < num_members; i++) {
    AST *member = members[i];

    structFields[i] =
        type_lookup(member->data.AST_SYMBOL_DECLARATION.type, ctx);
  }
  LLVMStructSetBody(structType, structFields, num_members, 0);
}

LLVMTypeRef codegen_type(AST *ast, char *name, Context *ctx) {
  switch (ast->tag) {
  case AST_STRUCT: {
    struct AST_STRUCT data = AST_DATA(ast, STRUCT);
    LLVMTypeRef *types = malloc(sizeof(LLVMTypeRef) * data.length);
    for (int i = 0; i < data.length; i++) {
      char *type = data.members[i]->data.AST_SYMBOL_DECLARATION.type;
      char *member_name = data.members[i]->data.AST_SYMBOL_DECLARATION.identifier;
      types[i] =
          type_lookup(type, ctx);
    }
    return LLVMStructType(types, data.length, true);
  }

  case AST_TUPLE: {
    struct AST_TUPLE data = AST_DATA(ast, TUPLE);
    LLVMTypeRef *types = malloc(sizeof(LLVMTypeRef) * data.length);
    for (int i = 0; i < data.length; i++) {
      types[i] =
          type_lookup(data.members[i]->data.AST_IDENTIFIER.identifier, ctx);
    }
    return LLVMStructType(types, data.length, true);
  }
  }
}
