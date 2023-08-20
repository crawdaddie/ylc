#include "codegen_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMTypeRef codegen_ttype(ttype type, Context *ctx) {
  switch (type.tag) {
  case T_INT:
    return LLVMInt32TypeInContext(ctx->context);
  case T_NUM:
    return LLVMDoubleTypeInContext(ctx->context);
  case T_STR:
    return LLVMPointerType(LLVMInt8Type(), 0);
  case T_BOOL:
    return LLVMInt1Type();
  case T_VOID:
    return LLVMVoidTypeInContext(ctx->context);
  case T_INT8:
    return LLVMInt8TypeInContext(ctx->context);

  case T_FN: {
    int len = type.as.T_FN.length;
    LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * len - 1);
    for (int i = 0; i < len - 1; i++) {
      params[i] = codegen_ttype(type.as.T_FN.members[i], ctx);
    }
    LLVMTypeRef ret_type = codegen_ttype(type.as.T_FN.members[len - 1], ctx);
    LLVMTypeRef function_type =
        LLVMFunctionType(ret_type, params, len - 1, false);

    return function_type;
  }

  case T_STRUCT: {
    int len = type.as.T_STRUCT.length;
    LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * len);
    for (int i = 0; i < len; i++) {
      ttype member_type = type.as.T_STRUCT.members[i];
      if (member_type.tag == T_FN) {
        params[i] = LLVMPointerType(codegen_ttype(member_type, ctx), 0);
      } else {
        params[i] = codegen_ttype(member_type, ctx);
      }
    }
    LLVMTypeRef t = LLVMStructType(params, len, true);
    return t;
  }

  case T_TUPLE: {
    int len = type.as.T_TUPLE.length;
    LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * len);
    for (int i = 0; i < len; i++) {
      params[i] = codegen_ttype(type.as.T_TUPLE.members[i], ctx);
    }
    return LLVMStructType(params, len, true);
  }
  case T_PTR: {
    return LLVMPointerType(codegen_ttype(*type.as.T_PTR.item, ctx), 0);
  }
  }
}

LLVMTypeRef codegen_type(AST *ast, Context *ctx) {
  ttype type = ast->type;
  return codegen_ttype(type, ctx);
}
