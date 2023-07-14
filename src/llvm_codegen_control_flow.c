#include "llvm_codegen_control_flow.h"
#include "llvm_codegen.h"
#include <stdlib.h>

LLVMValueRef codegen_if_else(AST *ast, Context *ctx) {
  LLVMValueRef condition = codegen(ast->data.AST_IF_ELSE.condition, ctx);
  if (!condition) {
    return NULL;
  }
  LLVMValueRef one = LLVMConstInt(LLVMInt1Type(), 1, false);
  condition = LLVMBuildICmp(ctx->builder, LLVMIntEQ, condition, one, "ifcond");

  // Retrieve function.
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

  // Generate true/false expr and merge.
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");

  return NULL;
};

//
// Returns an LLVM value reference.
//
/*
LLVMValueRef kal_codegen_if_expr(kal_ast_node *node, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {
  // Generate the condition.
  LLVMValueRef condition =
      kal_codegen(node->if_expr.condition, module, builder);
  if (condition == NULL) {
    return NULL;
  }

  // Convert condition to bool.
  LLVMValueRef zero = LLVMConstReal(LLVMDoubleType(), 0);
  condition = LLVMBuildFCmp(builder, LLVMRealONE, condition, zero, "ifcond");

  // Retrieve function.
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

  // Generate true/false expr and merge.
  LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
  LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
  LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");

  LLVMBuildCondBr(builder, condition, then_block, else_block);

  // Generate 'then' block.
  LLVMPositionBuilderAtEnd(builder, then_block);
  LLVMValueRef then_value =
      kal_codegen(node->if_expr.true_expr, module, builder);
  if (then_value == NULL) {
    return NULL;
  }

  LLVMBuildBr(builder, merge_block);
  then_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, else_block);
  LLVMValueRef else_value =
      kal_codegen(node->if_expr.false_expr, module, builder);
  if (else_value == NULL) {
    return NULL;
  }
  LLVMBuildBr(builder, merge_block);
  else_block = LLVMGetInsertBlock(builder);

  LLVMPositionBuilderAtEnd(builder, merge_block);
  LLVMValueRef phi = LLVMBuildPhi(builder, LLVMDoubleType(), "");
  LLVMAddIncoming(phi, &then_value, &then_block, 1);
  LLVMAddIncoming(phi, &else_value, &else_block, 1);

  return phi;
}
*/
