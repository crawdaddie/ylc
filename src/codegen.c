#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>

static int counter = 0;
static LLVMValueRef codegen_main(AST *ast, LLVMModuleRef module,
                                 LLVMBuilderRef builder) {

  // LLVMContextRef context = LLVMGetModuleContext(module);

  // Create function type.
  LLVMTypeRef funcType = LLVMFunctionType(LLVMDoubleType(), NULL, 0, 0);

  // Create function.
  char name[10];
  snprintf(name, sizeof(name), "main_%d", counter);
  counter++;

  LLVMValueRef func = LLVMAddFunction(module, name, funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, name);
  LLVMPositionBuilderAtEnd(builder, block);

  // Generate body.
  LLVMValueRef body = codegen(ast->data.AST_MAIN.body, module, builder);

  if (body == NULL) {
    printf("delete func??");
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
static LLVMValueRef codegen_add(LLVMValueRef left, LLVMValueRef right, LLVMModuleRef module, LLVMBuilderRef builder) {

}

static LLVMValueRef codegen_int(AST *ast, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  // LLVMContextRef context = LLVMGetModuleContext(module);
  return LLVMConstInt(LLVMInt32Type(), ast->data.AST_NUMBER.value, false);
}

static LLVMValueRef codegen_number(AST *ast, LLVMModuleRef module,
                                   LLVMBuilderRef builder) {

  // LLVMContextRef context = LLVMGetModuleContext(module);
  return LLVMConstReal(LLVMDoubleType(), ast->data.AST_NUMBER.value);
}

LLVMValueRef codegen(AST *ast, LLVMModuleRef module, LLVMBuilderRef builder) {
  // LLVMContextRef context = LLVMGetModuleContext(module);

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
    switch (ast->data.AST_BINOP.op) {
        case TOKEN_PLUS: {
          return codegen_add(left, right, module, builder);
        }
      }
    break;
  }

  case AST_UNOP: {
    LLVMValueRef operand = codegen(ast->data.AST_UNOP.operand, module, builder);
      LLVMTypeRef datatype = LLVMTypeOf(operand);

    switch (ast->data.AST_UNOP.op) {
        case TOKEN_MINUS: {
          if (datatype == LLVMInt32Type()) {
            return LLVMBuildNeg(builder, operand, "tmp_neg");
          }

          if (datatype == LLVMDoubleType()) {
            return LLVMBuildFNeg(builder, operand, "tmp_neg");
          }
          return NULL;
        }
    }
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
