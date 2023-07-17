#include "codegen_conditionals.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Check if match_on is equal (for some definition of equal) to matcher
 * should return boolean (LLVMInt1Type)
 * */
static LLVMValueRef compare_eq(LLVMValueRef match_on, LLVMValueRef matcher,
                               Context *ctx) {

  return LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 1, false);
}

/*
 * Check if match_on is equal (for some definition of equal) to test
 * should return boolean (LLVMInt1Type)
 * */
static LLVMValueRef match_predicate(LLVMValueRef match_on, LLVMValueRef test,
                                    Context *ctx) {

  return LLVMBuildICmp(ctx->builder, LLVMIntEQ, match_on, test,
                       inst_name("icmp"));
}

/*
 * Check if match_on evaluates to boolean 1
 * */
static LLVMValueRef if_condition(LLVMValueRef match_on, Context *ctx) {
  if (LLVMTypeOf(match_on) == LLVMInt1TypeInContext(ctx->context)) {
    return match_on;
  }
  return LLVMBuildTrunc(ctx->builder, match_on, LLVMInt1Type(), "bool_cast");

  // LLVMValueRef one =
  //     LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 1, false);
  //
  // return LLVMBuildICmp(ctx->builder, LLVMIntEQ, match_on, one,
  // "if_condition");
}

LLVMValueRef codegen_if_else(AST *ast, Context *ctx) {
  LLVMValueRef condition = codegen(ast->data.AST_IF_ELSE.condition, ctx);
  if (!condition) {
    return NULL;
  }

  condition = if_condition(condition, ctx);

  // Retrieve function.
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

  // Generate true/false expr and merge.
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");

  // Build the branch instruction based on the condition
  LLVMBuildCondBr(ctx->builder, condition, then_block, else_block);

  // Set builder to ifBlock and add instructions
  LLVMPositionBuilderAtEnd(ctx->builder, then_block);

  // Generate 'then' block.
  enter_scope(ctx);
  LLVMValueRef then_value = codegen(ast->data.AST_IF_ELSE.then_body, ctx);
  exit_scope(ctx);
  if (then_value == NULL) {

    return NULL;
  }
  LLVMBuildBr(ctx->builder, merge_block);

  // Set builder to elseBlock and add instructions
  LLVMPositionBuilderAtEnd(ctx->builder, else_block);

  LLVMValueRef else_value;
  if (ast->data.AST_IF_ELSE.else_body != NULL) {
    enter_scope(ctx);
    else_value = codegen(ast->data.AST_IF_ELSE.else_body, ctx);
    exit_scope(ctx);
  }

  LLVMBuildBr(ctx->builder, merge_block);
  LLVMPositionBuilderAtEnd(ctx->builder, merge_block);

  LLVMValueRef phi =
      LLVMBuildPhi(ctx->builder, LLVMTypeOf(then_value), "phi_result");
  LLVMAddIncoming(phi, &then_value, &then_block, 1);
  LLVMAddIncoming(phi, &else_value, &else_block, 1);

  return phi;
};

typedef struct CodegenCase {
  LLVMBasicBlockRef block;
  LLVMValueRef condition;
} CodegenCase;

LLVMValueRef codegen_match(AST *ast, Context *ctx) {
  LLVMValueRef match_on = codegen(ast->data.AST_MATCH.candidate, ctx);
  // Retrieve function.
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

  AST **case_asts = ast->data.AST_MATCH.matches;
  int num_cases = ast->data.AST_MATCH.length;

  CodegenCase *case_codegens = malloc(sizeof(CodegenCase) * num_cases);

  LLVMBasicBlockRef block;
  LLVMValueRef condition;

  for (int i = 0; i < num_cases; i++) {
    AST *match_ast = case_asts[i];
    case_codegens[i].block =
        LLVMAppendBasicBlock(func, inst_name("case_block"));

    if (i < num_cases - 1) {
      case_codegens[i].condition = match_predicate(
          match_on, codegen(match_ast->data.AST_BINOP.left, ctx), ctx);
    }
  }
  LLVMBasicBlockRef mergeBlock = LLVMAppendBasicBlock(func, "merge");

  for (int i = 0; i < num_cases - 1; i++) {
    LLVMBuildCondBr(ctx->builder, case_codegens[i].condition,
                    case_codegens[i].block, case_codegens[i + 1].block);
  }

  LLVMValueRef ret;
  for (int i = 0; i < num_cases; i++) {

    AST *match_ast = case_asts[i];

    LLVMPositionBuilderAtEnd(ctx->builder, case_codegens[i].block);
    ret = codegen(match_ast->data.AST_BINOP.right, ctx);
    LLVMBuildBr(ctx->builder, mergeBlock);
  }
  LLVMPositionBuilderAtEnd(ctx->builder, mergeBlock);
  // LLVMSwitch
  return ret;
}
