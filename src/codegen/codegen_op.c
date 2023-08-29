#include "codegen_op.h"
#include "../lexer.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

LLVMOpcode OperatorMapFloat[] = {
    [TOKEN_PLUS] = LLVMFAdd,   [TOKEN_MINUS] = LLVMFSub,
    [TOKEN_STAR] = LLVMFMul,   [TOKEN_SLASH] = LLVMFDiv,
    [TOKEN_MODULO] = LLVMFRem,

    // [TOKEN_EQUALITY] = LLVMRealUEQ,            /**< True if unordered or
    // equal */ [TOKEN_EQUALITY] = LLVMF
};

LLVMOpcode OperatorMapSInt[] = {
    [TOKEN_PLUS] = LLVMAdd,   [TOKEN_MINUS] = LLVMSub,   [TOKEN_STAR] = LLVMMul,
    [TOKEN_SLASH] = LLVMSDiv, [TOKEN_MODULO] = LLVMSRem,

    // [TOKEN_EQUALITY] = LLVMIntEQ,            /**< True if unordered or equal
    // */
};

static LLVMValueRef numeric_binop(AST *left, AST *right, token_type op,
                                  Context *ctx) {
  ttype_tag max_tag_type = max_type(left->type, right->type);
  LLVMValueRef l = codegen(left, ctx);
  LLVMValueRef r = codegen(right, ctx);
  if (left->type.tag < right->type.tag) {
    // cast left val up to match right
    l = LLVMBuildSIToFP(ctx->builder, l, LLVMDoubleTypeInContext(ctx->context),
                        "cast");
  } else if (right->type.tag < left->type.tag) {
    // cast right val up to match left
    r = LLVMBuildSIToFP(ctx->builder, r, LLVMDoubleTypeInContext(ctx->context),
                        "cast");
  } else if (left->type.tag == right->type.tag) {
  }

  if (op == TOKEN_EQUALITY) {
    if (max_tag_type == T_NUM) {
      return LLVMBuildFCmp(ctx->builder, LLVMRealUEQ, l, r, "dbl_eq");
    }
    if (max_tag_type == T_INT) {
      return LLVMBuildICmp(ctx->builder, LLVMIntEQ, l, r, "int_eq");
    }
    return NULL;
  }

  if (op == TOKEN_NOT_EQUAL) {
    if (max_tag_type == T_NUM) {
      return LLVMBuildFCmp(ctx->builder, LLVMRealUNE, l, r, "dbl_eq");
    }
    if (max_tag_type == T_INT) {
      return LLVMBuildICmp(ctx->builder, LLVMIntNE, l, r, "int_eq");
    }
    return NULL;
  }

  if (max_tag_type == T_NUM) {
    // build float binop
    // printf("float binop\n");
    return LLVMBuildBinOp(ctx->builder, OperatorMapFloat[op], l, r,
                          inst_name("float_binop"));
  }
  if (max_tag_type == T_INT) {
    // build int binop
    return LLVMBuildBinOp(ctx->builder, OperatorMapSInt[op], l, r,
                          inst_name("int_binop"));
  }
  return NULL;
}

LLVMValueRef codegen_binop(AST *ast, Context *ctx) {

  AST *left = ast->data.AST_BINOP.left;
  AST *right = ast->data.AST_BINOP.right;
  token_type op = ast->data.AST_BINOP.op;

  if (is_numeric_type(left->type) && is_numeric_type(right->type)) {
    return numeric_binop(left, right, op, ctx);
  }
  return NULL;
}

LLVMValueRef codegen_unop(AST *ast, Context *ctx) {
  LLVMValueRef operand = codegen(ast->data.AST_UNOP.operand, ctx);
  switch (ast->data.AST_UNOP.op) {
  case TOKEN_MINUS: {
    if (ast->data.AST_UNOP.operand->type.tag == T_INT) {
      return LLVMBuildNeg(ctx->builder, operand, inst_name("neg"));
    }
    if (ast->data.AST_UNOP.operand->type.tag == T_NUM) {
      return LLVMBuildFNeg(ctx->builder, operand, inst_name("neg"));
    }
    return NULL;
  }
  case TOKEN_BANG: {
    return LLVMBuildNot(ctx->builder, operand, inst_name("not"));
  }
  }
  return NULL;
}
