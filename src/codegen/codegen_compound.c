#include "codegen_compound.h"
#include "codegen.h"

LLVMValueRef codegen_struct(AST *ast, Context *ctx) {

  // int num_members = ast->data.AST_STRUCT.length;
  // AST **members = ast->data.AST_STRUCT.members;
  //
  // LLVMTypeRef *structFields = malloc(sizeof(LLVMTypeRef) * num_members);
  // for (int i = 0; i < num_members; i++) {
  //   AST *member = members[i];
  //
  //   structFields[i] =
  //       type_lookup(member->data.AST_SYMBOL_DECLARATION.type, ctx);
  // }
  // LLVMStructSetBody(structType, structFields, num_members, 0);

  return NULL;
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
  // LLVMValueRef array = LLVMConstArray
};
