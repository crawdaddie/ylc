#include "codegen_conditional.h"

#include "codegen.h"
#include "codegen_types.h"
#include <stddef.h>

/*
 * Check if match_on evaluates to boolean 1
 * */
static LLVMValueRef if_condition(LLVMValueRef match_on, Context *ctx) {
  if (LLVMTypeOf(match_on) == LLVMInt1TypeInContext(ctx->context)) {
    return match_on;
  }
  return LLVMBuildTrunc(ctx->builder, match_on, LLVMInt1Type(), "bool_cast");
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

/*
 * Check if match_on is equal (for some definition of equal) to test
 * should return boolean (LLVMInt1Type)
 * */
static LLVMValueRef match_predicate(LLVMValueRef match_on, LLVMValueRef test,
                                    Context *ctx) {

  return LLVMBuildICmp(ctx->builder, LLVMIntEQ, match_on, test,
                       inst_name("icmp"));
}

LLVMValueRef codegen_match(AST *ast, Context *ctx) {
  LLVMValueRef match_on = codegen(ast->data.AST_MATCH.candidate, ctx);

  // Retrieve function.
  LLVMValueRef func = current_function(ctx);

  int num_cases = ast->data.AST_MATCH.length;
  AST **match_expr_asts = ast->data.AST_MATCH.matches;
  LLVMBasicBlockRef *blocks = malloc(sizeof(LLVMBasicBlockRef) * 2 * num_cases);

  for (int i = 0; i < num_cases; ++i) {
    blocks[(ptrdiff_t)2 * i] =
        LLVMAppendBasicBlock(func, inst_name("match_cond"));
    blocks[2 * i + 1] = LLVMAppendBasicBlock(func, inst_name("match_expr"));
  }
  LLVMBasicBlockRef endBlock = LLVMAppendBasicBlock(func, "end");

  LLVMTypeRef result_type = codegen_type(ast, ctx);

  LLVMValueRef result =
      LLVMBuildAlloca(ctx->builder, result_type, "result_var");
  LLVMBuildBr(ctx->builder, blocks[0]);

  for (int i = 0; i < num_cases; ++i) {
    // FILL POSITION BLOCK
    LLVMPositionBuilderAtEnd(ctx->builder, blocks[2 * i]);
    if (i < num_cases - 1) {
      LLVMValueRef condition = match_predicate(
          match_on, codegen(match_expr_asts[i]->data.AST_BINOP.left, ctx), ctx);

      LLVMBuildCondBr(ctx->builder, condition, blocks[(2 * i) + 1],
                      blocks[(2 * i) + 2]);
    } else {
      LLVMBuildBr(ctx->builder, blocks[(2 * i) + 1]);
    }

    // FILL CORRESPONDING MATCH EXPR
    LLVMPositionBuilderAtEnd(ctx->builder, blocks[2 * i + 1]);
    enter_scope(ctx);
    LLVMValueRef val = codegen(match_expr_asts[i]->data.AST_BINOP.right, ctx);
    LLVMBuildStore(ctx->builder, val, result);
    exit_scope(ctx);
    // once we're done jump out of switch
    LLVMBuildBr(ctx->builder, endBlock);
  }

  LLVMPositionBuilderAtEnd(ctx->builder, endBlock);

  LLVMValueRef result_val =
      LLVMBuildLoad2(ctx->builder, result_type, result, "match_result");

  return result_val;
}
