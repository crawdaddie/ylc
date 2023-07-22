#include "codegen_arithmetic.h"
#include "lexer.h"
#include <stdio.h>
static LLVMTypeRef int_type(Context *ctx) {
  return LLVMInt32TypeInContext(ctx->context);
}

static LLVMTypeRef double_type(Context *ctx) {
  return LLVMDoubleTypeInContext(ctx->context);
}
/*
  / Standard Unary Operators /
  LLVMFNeg = 66,

  / Standard Binary Operators/
  LLVMAdd = 8,
  LLVMFAdd = 9,
  LLVMSub = 10,
  LLVMFSub = 11,
  LLVMMul = 12,
  LLVMFMul = 13,
  LLVMUDiv = 14,
  LLVMSDiv = 15,
  LLVMFDiv = 16,
  LLVMURem = 17,
  LLVMSRem = 18,
  LLVMFRem = 19,
 */
LLVMOpcode OperatorMapFloat[] = {
    [TOKEN_PLUS] = LLVMFAdd,   [TOKEN_MINUS] = LLVMFSub,
    [TOKEN_STAR] = LLVMFMul,   [TOKEN_SLASH] = LLVMFDiv,
    [TOKEN_MODULO] = LLVMFRem,
};

LLVMOpcode OperatorMapSInt[] = {
    [TOKEN_PLUS] = LLVMAdd,   [TOKEN_MINUS] = LLVMSub,   [TOKEN_STAR] = LLVMMul,
    [TOKEN_SLASH] = LLVMSDiv, [TOKEN_MODULO] = LLVMSRem,
};

static LLVMOpcode op_map(token_type op, int is_float) {
  if (is_float) {
    return OperatorMapFloat[op];
  }
  return OperatorMapSInt[op];
}

static LLVMValueRef codegen_equality(LLVMValueRef left, LLVMValueRef right,
                                     Context *ctx) {
  // return 1 for equal, 0 for false
  return LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, inst_name("icmp"));
}
LLVMValueRef numerical_binop(token_type op, LLVMValueRef left,
                             LLVMValueRef right, Context *ctx) {

  LLVMTypeRef ltype = LLVMTypeOf(left);
  LLVMTypeRef rtype = LLVMTypeOf(right);
  LLVMTypeRef itype = int_type(ctx);

  if (op == TOKEN_EQUALITY) {
    return codegen_equality(left, right, ctx);
  }

  if (ltype == itype && rtype == itype) {
    return LLVMBuildBinOp(ctx->builder, OperatorMapSInt[op], left, right,
                          inst_name("inst"));
  }

  LLVMTypeRef dbltype = double_type(ctx);
  if (ltype == itype && rtype == dbltype) {
    return LLVMBuildBinOp(
        ctx->builder, OperatorMapFloat[op],
        LLVMBuildSIToFP(ctx->builder, left, dbltype, inst_name("inst")), right,
        inst_name("inst"));
  }

  if (ltype == dbltype && rtype == itype) {
    return LLVMBuildBinOp(
        ctx->builder, OperatorMapFloat[op], left,
        LLVMBuildSIToFP(ctx->builder, right, dbltype, inst_name("inst")),
        inst_name("inst"));
  }

  if (ltype == dbltype && rtype == dbltype) {
    return LLVMBuildBinOp(ctx->builder, OperatorMapFloat[op], left, right,
                          inst_name("inst"));
  }
  return NULL;
}

LLVMValueRef codegen_add(LLVMValueRef left, LLVMValueRef right, Context *ctx) {
  return numerical_binop(TOKEN_PLUS, left, right, ctx);
};

LLVMValueRef codegen_sub(LLVMValueRef left, LLVMValueRef right, Context *ctx) {
  return numerical_binop(TOKEN_MINUS, left, right, ctx);
};

LLVMValueRef codegen_neg_unop(LLVMValueRef operand, Context *ctx) {

  LLVMTypeRef datatype = LLVMTypeOf(operand);
  if (datatype == LLVMInt32TypeInContext(ctx->context)) {
    return LLVMBuildNeg(ctx->builder, operand, inst_name("inst"));
  }

  if (datatype == LLVMDoubleType()) {
    return LLVMBuildFNeg(ctx->builder, operand, inst_name("inst"));
  }
  return NULL;
};

LLVMValueRef get_int(int val, Context *ctx) {
  return LLVMConstInt(LLVMInt32TypeInContext(ctx->context), val, false);
}

LLVMValueRef get_double(double val, Context *ctx) {
  return LLVMConstInt(LLVMFloatTypeInContext(ctx->context), val, false);
}

LLVMValueRef codegen_int(AST *ast, Context *ctx) {
  return get_int(ast->data.AST_INTEGER.value, ctx);
}

LLVMValueRef codegen_number(AST *ast, Context *ctx) {
  return LLVMConstReal(LLVMDoubleTypeInContext(ctx->context),
                       ast->data.AST_NUMBER.value);
}
