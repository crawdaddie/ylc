#include "llvm_codegen.h"
#include "llvm_codegen_arithmetic.h"
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
  //
  LLVMTypeRef funcType;
  if (body == NULL) {
    // funcType =
    //     LLVMFunctionType(LLVMVoidTypeInContext(ctx->context), NULL, 0, 0);
    return NULL;
  } else {
    funcType = LLVMFunctionType(LLVMTypeOf(body), NULL, 0, 0);
  }

  // Create function.
  LLVMValueRef func = LLVMAddFunction(ctx->module, "main", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  // Insert body as return vale.
  LLVMBuildRet(ctx->builder, body);

  // Verify function.
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
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
    char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;

    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->value = NULL;
    HASH_ADD_KEYPTR(hh, SymbolTable, sym->name, strlen(sym->name), sym);
    return NULL;
  }

  case AST_ASSIGNMENT: {
    char *identifier = ast->data.AST_ASSIGNMENT.identifier;

    Symbol *sym;
    LLVMValueRef global;

    HASH_FIND_STR(SymbolTable, identifier, sym);
    if (sym) {
      global = LLVMGetNamedGlobal(ctx->module, identifier);

      AST *expr = ast->data.AST_ASSIGNMENT.expression;
      LLVMValueRef value = codegen(expr, ctx);
      LLVMTypeRef value_type = LLVMTypeOf(value);

      if (sym->type != value_type) {
        fprintf(stderr,
                "Error assigning value of type %s to variable of type %s",
                LLVMPrintTypeToString(value_type),
                LLVMPrintTypeToString(sym->type));
        return NULL;
      }

      LLVMSetInitializer(global, value);
      return global;
    }

    AST *expr = ast->data.AST_ASSIGNMENT.expression;
    LLVMValueRef value = codegen(expr, ctx);

    sym = malloc(sizeof(Symbol));
    sym->name = strdup(identifier);
    LLVMTypeRef type = LLVMTypeOf(value);

    global = LLVMAddGlobal(ctx->module, type, identifier);
    LLVMSetInitializer(global, value);
    sym->value = global;
    sym->type = type;

    HASH_ADD_KEYPTR(hh, SymbolTable, sym->name, strlen(sym->name), sym);
    return global;
  }
  }
  return NULL;
}
