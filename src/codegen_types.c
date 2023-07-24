#include "codegen_types.h"
#include "codegen.h"
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

type_symbol_table *compute_type_metadata(AST *type_expr) {

  struct AST_STRUCT struct_def = AST_DATA(type_expr, STRUCT);

  type_symbol_table *type_metadata = malloc(sizeof(type_symbol_table));
  member_type *types = malloc(sizeof(member_type) * struct_def.length);
  for (int i = 0; i < struct_def.length; i++) {
    struct AST_SYMBOL_DECLARATION sym_dec =
        struct_def.members[i]->data.AST_SYMBOL_DECLARATION;
    types[i] = (member_type){sym_dec.identifier, i};
  }
  type_metadata->length = struct_def.length;
  type_metadata->member_types = types;
  return type_metadata;
}

type_symbol_table *get_type_metadata(const char *type_name, Context *ctx) {
  SymbolValue v;
  if (table_lookup(ctx->symbol_table, type_name, &v) == 0) {
    return v.data.TYPE_TYPE_DECLARATION.type_metadata;
  }
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
      char *member_name =
          data.members[i]->data.AST_SYMBOL_DECLARATION.identifier;
      types[i] = type_lookup(type, ctx);
    }
    LLVMTypeRef type = LLVMStructCreateNamed(ctx->context, name);
    LLVMStructSetBody(type, types, data.length, true);

    return type;
    // /LLVMStructType(types, data.length, true);
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

int get_member_index(char *member, type_symbol_table *metadata) {
  for (int i = 0; i < metadata->length; i++) {
    member_type member_type = metadata->member_types[i];
    if (strcmp(member, member_type.name) == 0) {
      return member_type.index;
    }
  }
  return -1;
}

LLVMValueRef struct_instance_with_metadata(AST *expr, LLVMTypeRef type,
                                           type_symbol_table *metadata,
                                           Context *ctx) {
  LLVMValueRef *values = malloc(sizeof(LLVMValueRef) * metadata->length);
  for (int i = 0; i < expr->data.AST_TUPLE.length; i++) {
    AST *mem = expr->data.AST_TUPLE.members[i];
    char *member_name = mem->data.AST_ASSIGNMENT.identifier;
    AST *mem_expression = mem->data.AST_ASSIGNMENT.expression;
    int index = get_member_index(member_name, metadata);
    if (index == -1) {
      fprintf(stderr, "Error: struct does not have a member named %s\n",
              member_name);
      continue;
    }
    values[index] = codegen(mem_expression, ctx);
  }

  return LLVMConstNamedStruct(type, values, metadata->length);
}
