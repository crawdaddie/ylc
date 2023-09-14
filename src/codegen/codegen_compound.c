#include "codegen_compound.h"
#include "codegen.h"
#include "codegen_module.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <stdio.h>

LLVMValueRef codegen_struct(AST *ast, Context *ctx) {
  // printf("codegen struct: ");
  // print_ttype(ast->type);
  //
  struct AST_STRUCT data = AST_DATA(ast, STRUCT);
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
  for (int i = 0; i < data.length; i++) {
    char *name = data.members[i]->data.AST_SYMBOL_DECLARATION.identifier;
    AST *expr = data.members[i]->data.AST_SYMBOL_DECLARATION.expression;

    int idx = get_struct_member_index(ast->type, name);
    members[idx] = codegen(expr, ctx);
  }

  LLVMValueRef tuple_struct = LLVMConstStruct(members, data.length, true);
  return tuple_struct;
};

LLVMValueRef codegen_named_struct(AST *ast, LLVMTypeRef type_ref,
                                  Context *ctx) {
  // printf("codegen struct: ");
  // print_ttype(ast->type);
  //
  struct AST_STRUCT data = AST_DATA(ast, STRUCT);
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
  for (int i = 0; i < data.length; i++) {
    char *name = data.members[i]->data.AST_SYMBOL_DECLARATION.identifier;
    AST *expr = data.members[i]->data.AST_SYMBOL_DECLARATION.expression;

    int idx = get_struct_member_index(ast->type, name);
    members[idx] = codegen(expr, ctx);
  }

  LLVMValueRef tuple_struct =
      LLVMConstNamedStruct(type_ref, members, data.length);

  return tuple_struct;
};

LLVMValueRef codegen_tuple(AST *ast, Context *ctx) {
  struct AST_TUPLE data = AST_DATA(ast, TUPLE);
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
  for (int i = 0; i < data.length; i++) {
    members[i] = codegen(data.members[i], ctx);
  }
  LLVMValueRef tuple_struct = LLVMConstStruct(members, data.length, false);
  return tuple_struct;
};

static void get_compound_object_ptr(SymbolValue sym, char *object_id,
                                    LLVMValueRef *object, ttype *object_type,
                                    Context *ctx) {
  if (sym.type == TYPE_VARIABLE) {
    *object_type = sym.data.TYPE_VARIABLE.type;
    *object = LLVMBuildLoad2(ctx->builder, sym.data.TYPE_VARIABLE.llvm_type,
                             sym.data.TYPE_VARIABLE.llvm_value, "ptr_deref");

  } else if (sym.type == TYPE_GLOBAL_VARIABLE) {

    *object = codegen_global_identifier(sym, object_id, ctx);
    *object_type = sym.data.TYPE_GLOBAL_VARIABLE.type;

  } else {
    fprintf(stderr, "Error: unrecognized variable type");
    return;
    // return NULL;
  }

  if (is_ptr_to_struct(*object_type)) {
    *object_type = *object_type->as.T_PTR.item;
    *object = LLVMBuildLoad2(ctx->builder, codegen_ttype(*object_type, ctx),
                             *object, "ptr_deref");
  }
}

static LLVMValueRef codegen_member_ptr(LLVMValueRef object,
                                       unsigned int member_idx,
                                       ttype object_type, Context *ctx) {

  LLVMValueRef indices[1] = {
      LLVMConstInt(LLVMInt32TypeInContext(ctx->context), member_idx, false)};

  LLVMValueRef member = LLVMBuildGEP2(
      ctx->builder,
      codegen_ttype(object_type.as.T_STRUCT.members[member_idx], ctx), object,
      indices, 1, "get_member_ptr");

  return member;
}

LLVMValueRef codegen_member_access(AST *ast, Context *ctx) {

  char *object_id =
      ast->data.AST_MEMBER_ACCESS.object->data.AST_IDENTIFIER.identifier;

  char *member_name = ast->data.AST_MEMBER_ACCESS.member_name;

  SymbolValue sym;

  if (table_lookup(ctx->symbol_table, object_id, &sym) != 0) {
    fprintf(stderr, "Error symbol %s not found\n", object_id);
    return NULL;
  }
  if (sym.type == TYPE_MODULE) {
    return lookup_module_member(sym, member_name, ctx);
  }

  LLVMValueRef object;
  ttype object_type;

  get_compound_object_ptr(sym, object_id, &object, &object_type, ctx);
  unsigned int member_idx = get_struct_member_index(object_type, member_name);

  LLVMValueRef member_ptr =
      codegen_member_ptr(object, member_idx, object_type, ctx);

  return LLVMBuildLoad2(
      ctx->builder,
      codegen_ttype(object_type.as.T_STRUCT.members[member_idx], ctx),
      member_ptr, "get_member_val");
}

LLVMValueRef codegen_member_assignment(AST *ast, Context *ctx) {

  char *object_id =
      ast->data.AST_MEMBER_ASSIGNMENT.object->data.AST_IDENTIFIER.identifier;
  char *member_name = ast->data.AST_MEMBER_ASSIGNMENT.member_name;

  SymbolValue sym;

  if (table_lookup(ctx->symbol_table, object_id, &sym) != 0) {
    fprintf(stderr, "Error symbol %s not found\n", object_id);
    return NULL;
  }

  if (sym.type == TYPE_MODULE) {
    fprintf(stderr, "Error cannot set module member");
    return NULL;
  }

  LLVMValueRef object;
  ttype object_type;

  get_compound_object_ptr(sym, object_id, &object, &object_type, ctx);

  unsigned int member_idx = get_struct_member_index(object_type, member_name);

  LLVMValueRef member_ptr =
      codegen_member_ptr(object, member_idx, object_type, ctx);

  LLVMValueRef value = codegen(ast->data.AST_MEMBER_ASSIGNMENT.expression, ctx);
  LLVMBuildStore(ctx->builder, value, member_ptr);

  return value;
}

LLVMValueRef codegen_array(AST *ast, Context *ctx) {
  // return NULL;
  int len = ast->data.AST_ARRAY.length;
  LLVMValueRef *vals = malloc(sizeof(LLVMValueRef) * len);
  for (int i = 0; i < len; i++) {
    vals[i] = codegen(ast->data.AST_ARRAY.members[i], ctx);
  }
  LLVMTypeRef type = codegen_ttype(*ast->type.as.T_ARRAY.member_type, ctx);
  LLVMValueRef array = LLVMConstArray(type, vals, len);
  return array;
};

LLVMValueRef codegen_index_access(AST *ast, Context *ctx) {

  AST *object_ast = ast->data.AST_INDEX_ACCESS.object;
  if (object_ast->type.tag != T_ARRAY) {
    return NULL;
  }
  LLVMValueRef object = codegen(object_ast, ctx);
  LLVMValueRef indices[1] = {
      codegen(ast->data.AST_INDEX_ACCESS.index_expr, ctx)};

  LLVMValueRef alloca =
      LLVMBuildAlloca(ctx->builder, LLVMTypeOf(object), "arr_ptr");

  LLVMBuildStore(ctx->builder, object, alloca);

  return LLVMBuildInBoundsGEP2(ctx->builder, LLVMTypeOf(object), alloca,
                               indices, 1, "get_at_index");
}
