#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>

static LLVMValueRef get_int(int val, Context *ctx);

Symbol *SymbolTable = NULL;


LLVMValueRef lookup_symbol(char *name) {
  // Lookup variable reference.
  Symbol *val = NULL;
  HASH_FIND_STR(SymbolTable, name, val);

  if (val != NULL) {
    return val->value;
  } else {
    return NULL;
  }
}

static LLVMValueRef codegen_main(AST *ast, Context *ctx) {
  // Generate body.
  LLVMValueRef body = codegen(ast->data.AST_MAIN.body, ctx);

  // Create function type.
  LLVMTypeRef funcType = LLVMFunctionType(
    LLVMTypeOf(body),
    NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(ctx->module, "main", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);



  if (body == NULL) {
    printf("delete func??");
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  //
  // printf("build %p mod %p cont %p\n", ctx->builder, ctx->module, ctx->context);
  LLVMBuildRet(ctx->builder, body);

  // Verify function.
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(func);
    return NULL;
  }

  return func;
}

static LLVMValueRef codegen_add(LLVMValueRef left, LLVMValueRef right,
                                Context *ctx) {}
static LLVMValueRef get_int(int val, Context *ctx) {
  return LLVMConstInt(LLVMInt32TypeInContext(ctx->context), val, false);
}
static LLVMValueRef codegen_int(AST *ast, Context *ctx) {
  return get_int(ast->data.AST_INTEGER.value, ctx);
}

static LLVMValueRef codegen_number(AST *ast, Context *ctx) {
  return LLVMConstReal(LLVMDoubleTypeInContext(ctx->context), ast->data.AST_NUMBER.value);
}

LLVMValueRef codegen(AST *ast, Context *ctx) {
  // LLVMContextRef context = LLVMGetModuleContext(module);
  //

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
    switch (ast->data.AST_BINOP.op) {
    case TOKEN_PLUS: {
      return codegen_add(left, right, ctx);
    }
    }
    break;
  }

  case AST_UNOP: {
    LLVMValueRef operand = codegen(ast->data.AST_UNOP.operand, ctx);
    LLVMTypeRef datatype = LLVMTypeOf(operand);

    switch (ast->data.AST_UNOP.op) {
    case TOKEN_MINUS: {
      if (datatype == LLVMInt32TypeInContext(ctx->context)) {
        return LLVMBuildNeg(ctx->builder, operand, "tmp_neg");
      }

      if (datatype == LLVMDoubleType()) {
        return LLVMBuildFNeg(ctx->builder, operand, "tmp_neg");
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
          codegen(ast->data.AST_STATEMENT_LIST.statements[i], ctx);

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
      LLVMValueRef value = codegen(expr, ctx);


      Symbol *sym = malloc(sizeof(Symbol));
      sym->name = strdup(identifier);
      sym->value = value;
      HASH_ADD_KEYPTR(hh, SymbolTable, sym->name, strlen(sym->name), sym);

      LLVMTypeRef globalVarType = LLVMTypeOf(value);
      LLVMValueRef globalVar = LLVMAddGlobal(ctx->module, globalVarType, identifier);


      LLVMSetInitializer(globalVar, value);
      return globalVar;
  }
  }
  return NULL;
}
