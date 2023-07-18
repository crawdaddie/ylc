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

  LLVMTypeRef ctxType = LLVMGetTypeByName2(ctx->context, type);
  if (ctxType) {
    return ctxType;
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
