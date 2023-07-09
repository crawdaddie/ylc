#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>

static LLVMValueRef codegen_main(AST *ast, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {

  // Create function type.
  LLVMTypeRef funcType = LLVMFunctionType(LLVMDoubleType(), NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, "entry", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  // Generate body.
  LLVMValueRef body = codegen(ast->data.AST_MAIN.body, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  LLVMBuildRet(builder, body);

  // Verify function.
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(func);
    return NULL;
  }

  return func;
}

static LLVMValueRef codegen_int(AST *ast, LLVMModuleRef module,
                                LLVMBuilderRef builder) {
  return LLVMConstInt(LLVMInt32Type(), ast->data.AST_NUMBER.value, false);
}

static LLVMValueRef codegen_number(AST *ast, LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  return LLVMConstReal(LLVMDoubleType(), ast->data.AST_NUMBER.value);
}

LLVMValueRef codegen(AST *ast, LLVMModuleRef module, LLVMBuilderRef builder) {

  switch (ast->tag) {
  case AST_MAIN: {
    return codegen_main(ast, module, builder);
  }

  case AST_INTEGER:
    return codegen_int(ast, module, builder);

  case AST_NUMBER: {
    return codegen_number(ast, module, builder);
  }

  case AST_BINOP: {
    LLVMValueRef left = codegen(ast->data.AST_BINOP.left, module, builder);
    LLVMValueRef right = codegen(ast->data.AST_BINOP.right, module, builder);
    break;
  }

  case AST_UNOP: {
    LLVMValueRef operand = codegen(ast->data.AST_UNOP.operand, module, builder);
    break;
  }

  case AST_STATEMENT_LIST: {
    LLVMValueRef statement = NULL;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      LLVMValueRef tmp_stmt =
          codegen(ast->data.AST_STATEMENT_LIST.statements[i], module, builder);

      if (tmp_stmt != NULL) {
        statement = tmp_stmt;
      }
    }
    return statement;
  }

  case AST_SYMBOL_DECLARATION: {
    char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;

    // LLVMContextRef context = LLVMGetModuleContext(module);
    // LLVMValueRef global =
    //     LLVMAddGlobal(module, LLVMVoidTypeInContext(context), name);
    return NULL;
  }

  case AST_ASSIGNMENT: {
    char *identifier = ast->data.AST_ASSIGNMENT.identifier;
    AST *expr = ast->data.AST_ASSIGNMENT.expression;
    codegen(expr, module, builder);
    return NULL;
  }
  }
  return NULL;
}
