#include "codegen.h"
#include "codegen_arithmetic.h"
#include "codegen_control_flow.h"
#include "codegen_function.h"
#include "codegen_symbol.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>

static bool value_is_numeric(LLVMValueRef value) {
  LLVMTypeRef type = LLVMTypeOf(value);
  LLVMTypeKind type_kind = LLVMGetTypeKind(type);

  return type_kind == LLVMIntegerTypeKind || type_kind == LLVMFloatTypeKind ||
         type_kind == LLVMDoubleTypeKind;
}

static LLVMValueRef codegen_main(AST *ast, Context *ctx) {

  // Create function type.
  LLVMTypeRef funcType =
      LLVMFunctionType(LLVMVoidTypeInContext(ctx->context), NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(ctx->module, "main", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);
  // Generate body.
  enter_function(ctx, func);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  LLVMValueRef body = codegen(ast->data.AST_MAIN.body, ctx);

  exit_function(ctx, NULL);
  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  // LLVMBuildRet(ctx->builder, LLVMVoidLLVMVoid);
  LLVMPositionBuilderAtEnd(ctx->builder, LLVMGetInsertBlock(ctx->builder));
  LLVMBuildRetVoid(ctx->builder);

  // Verify function.
  // printf("verify func\n");
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid main function");
    LLVMDeleteFunction(func);
    return NULL;
  }

  return func;
}

LLVMValueRef codegen(AST *ast, Context *ctx) {
  switch (ast->tag) {
  case AST_MAIN: {
    return codegen_main(ast, ctx);
  }

  case AST_INTEGER:
    return codegen_int(ast, ctx);

  case AST_NUMBER: {
    return codegen_number(ast, ctx);
  }

  case AST_BINOP: {
    LLVMValueRef left = codegen(ast->data.AST_BINOP.left, ctx);
    LLVMValueRef right = codegen(ast->data.AST_BINOP.right, ctx);

    if (value_is_numeric(left) && value_is_numeric(right)) {

      return numerical_binop(ast->data.AST_BINOP.op, left, right, ctx);
    }
    return NULL;
  }

  case AST_UNOP: {
    LLVMValueRef operand = codegen(ast->data.AST_UNOP.operand, ctx);
    switch (ast->data.AST_UNOP.op) {

    case TOKEN_MINUS: {
      return codegen_neg_unop(operand, ctx);
    }
    }
    break;
  }

  case AST_STATEMENT_LIST: {
    LLVMValueRef statement = NULL;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      LLVMValueRef tmp_stmt =
          codegen(ast->data.AST_STATEMENT_LIST.statements[i], ctx);

      if (tmp_stmt != NULL) {
        statement = tmp_stmt;
      }
    }
    return statement;
  }

  case AST_SYMBOL_DECLARATION: {
    return codegen_symbol_declaration(ast, ctx);
  }

  case AST_ASSIGNMENT: {
    return codegen_symbol_assignment(ast, ctx);
  }

  case AST_FN_DECLARATION: {
    return codegen_function(ast, ctx);
  }
  case AST_CALL: {
    return codegen_call(ast, ctx);
  }
  case AST_IDENTIFIER: {
    return codegen_identifier(ast, ctx);
  }

  case AST_IF_ELSE: {
    return codegen_if_else(ast, ctx);
  }
  }
  return NULL;
}

static int counter = 0;
char *inst_name(const char *cString) {
  counter++;
  char *result =
      (char *)malloc(100 * sizeof(char)); // Assuming a large enough buffer size
  sprintf(result, "%s%d", cString, counter);
  return result;
}
