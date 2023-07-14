#include "codegen_control_flow.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

LLVMValueRef codegen_if_else(AST *ast, Context *ctx) {
  LLVMValueRef condition = codegen(ast->data.AST_IF_ELSE.condition, ctx);
  if (!condition) {
    return NULL;
  }
  LLVMValueRef one =
      LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 1, false);

  // Create a comparison instruction (if condition)
  // condition = LLVMBuildIntCast2(ctx->builder, condition,
  //                                LLVMInt32TypeInContext(ctx->context),false,
  //                                "int_cast");

  condition = LLVMBuildICmp(ctx->builder, LLVMIntEQ, condition, one, "ifcond");

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
  LLVMValueRef then_value = codegen(ast->data.AST_IF_ELSE.then_body, ctx);
  // printf("\nthen type\n");
  // LLVMDumpType(LLVMTypeOf(then_value));
  if (then_value == NULL) {
    return NULL;
  }
  LLVMBuildBr(ctx->builder, merge_block);

  // Set builder to elseBlock and add instructions
  LLVMPositionBuilderAtEnd(ctx->builder, else_block);

  LLVMValueRef else_value;
  if (ast->data.AST_IF_ELSE.else_body != NULL) {
    else_value = codegen(ast->data.AST_IF_ELSE.else_body, ctx);
  }

  LLVMBuildBr(ctx->builder, merge_block);
  LLVMPositionBuilderAtEnd(ctx->builder, merge_block);

  LLVMValueRef phi =
      LLVMBuildPhi(ctx->builder, LLVMTypeOf(then_value), "phi_result");
  LLVMAddIncoming(phi, &then_value, &then_block, 1);
  LLVMAddIncoming(phi, &else_value, &else_block, 1);

  return phi;
};
