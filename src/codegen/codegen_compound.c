#include "codegen_compound.h"
#include "codegen.h"
#include "codegen_types.h"
#include <stdio.h>

LLVMValueRef codegen_struct(AST *ast, Context *ctx) {
  printf("codegen struct: ");
  print_ttype(ast->type);
  struct AST_STRUCT data = AST_DATA(ast, STRUCT);
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
  for (int i = 0; i < data.length; i++) {
    char *name = data.members[i]->data.AST_SYMBOL_DECLARATION.identifier;
    AST *expr = data.members[i]->data.AST_SYMBOL_DECLARATION.expression;
    print_ast(*data.members[i], 0);

    int idx = get_struct_member_index(ast->type, name);
    members[idx] = codegen(expr, ctx);
  }
  LLVMValueRef tuple_struct = LLVMConstStruct(members, data.length, true);
  return tuple_struct;
};

LLVMValueRef codegen_tuple(AST *ast, Context *ctx) {
  struct AST_TUPLE data = AST_DATA(ast, TUPLE);
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
  for (int i = 0; i < data.length; i++) {
    members[i] = codegen(data.members[i], ctx);
  }
  LLVMValueRef tuple_struct = LLVMConstStruct(members, data.length, true);
  return tuple_struct;
};

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
