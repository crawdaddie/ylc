#include "codegen.h"
#include "../paths.h"
#include "codegen_compound.h"
#include "codegen_conditional.h"
#include "codegen_function.h"
#include "codegen_module.h"
#include "codegen_op.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Support.h>
#include <stdio.h>
#include <stdlib.h>

static LLVMValueRef codegen_main(AST *ast, Context *ctx) {

  // Create function type.
  LLVMTypeRef funcType =
      LLVMFunctionType(LLVMVoidTypeInContext(ctx->context), NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(ctx->module, "main", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Generate body.
  enter_scope(ctx);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  LLVMValueRef body = codegen(ast->data.AST_MAIN.body, ctx);

  exit_scope(ctx);
  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMPositionBuilderAtEnd(ctx->builder, LLVMGetInsertBlock(ctx->builder));

  LLVMBuildRetVoid(ctx->builder);

  // Verify function.
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
    return LLVMConstInt(LLVMInt32TypeInContext(ctx->context),
                        ast->data.AST_INTEGER.value, false);
  case AST_NUMBER:
    return LLVMConstReal(LLVMDoubleTypeInContext(ctx->context),
                         ast->data.AST_NUMBER.value);

  case AST_BOOL:
    return LLVMConstInt(LLVMInt1TypeInContext(ctx->context),
                        ast->data.AST_BOOL.value, false);
  case AST_STRING: {
    struct AST_STRING data = AST_DATA(ast, STRING);

    const char *str = data.value;
    int len = data.length;
    return LLVMBuildGlobalStringPtr(ctx->builder, str, str);
  }
  case AST_STATEMENT_LIST: {
    struct AST_STATEMENT_LIST data = AST_DATA(ast, STATEMENT_LIST);
    LLVMValueRef statement = NULL;
    for (int i = 0; i < data.length; i++) {
      LLVMValueRef tmp_stmt = codegen(data.statements[i], ctx);
      if (tmp_stmt != NULL) {
        statement = tmp_stmt;
      }
    }
    return statement;
  }
  case AST_FN_DECLARATION: {
    if (ast->data.AST_FN_DECLARATION.is_extern) {
      return codegen_extern_function(ast, ctx);
    }
    return codegen_function(ast, ctx);
  }
  case AST_SYMBOL_DECLARATION: {
    // symbol declaration [ let a ]- rarely used
    const char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;
    ttype type = ast->type;
    AST *expr = ast->data.AST_SYMBOL_DECLARATION.expression; // optional

    return codegen_symbol(name, type, ast->data.AST_SYMBOL_DECLARATION.type,
                          expr, ctx);
  }

  case AST_ASSIGNMENT: {
    // symbol assignment [ a = x ]
    // also handles immediate declaration + assignment [ let a = x ]
    const char *name = ast->data.AST_ASSIGNMENT.identifier;
    ttype type = ast->type;
    AST *expr = ast->data.AST_ASSIGNMENT.expression;

    if (ast->type.tag == T_FN && expr->tag == AST_CALL) {
      // currying a function
      return NULL;
    }
    AST *type_ast = ast->data.AST_ASSIGNMENT.type;

    return codegen_symbol(name, type, type_ast, expr, ctx);
  }
  case AST_IDENTIFIER: {
    return codegen_identifier(ast, ctx);
  }
  case AST_TUPLE:
    return codegen_tuple(ast, ctx);
  case AST_ARRAY:
    return codegen_array(ast, ctx);
  case AST_STRUCT:
    return codegen_struct(ast, ctx);
  case AST_UNOP:
    return codegen_unop(ast, ctx);
  case AST_BINOP: {
    return codegen_binop(ast, ctx);
  }
  case AST_CALL:
    return codegen_call(ast, ctx);
  case AST_IF_ELSE:
    return codegen_if_else(ast, ctx);
  case AST_MATCH:
    return codegen_match(ast, ctx);
  case AST_IMPORT:
    return codegen_module(ast, ctx);
  case AST_IMPORT_LIB: {
    if (has_extension(ast->data.AST_IMPORT_LIB.lib_name, ".so")) {
      return codegen_so(ast, ctx);
    }
    return NULL;
  }
  case AST_MEMBER_ACCESS: {
    // printf("\nmember access\n");
    // print_ast(*ast->data.AST_MEMBER_ACCESS.object, 0);
    // printf(". %s\n", ast->data.AST_MEMBER_ACCESS.member_name);
    // print_ttype(ast->type);
    //
    return codegen_member_access(ast, ctx);
  }
  case AST_MEMBER_ASSIGNMENT: {
    return codegen_member_assignment(ast, ctx);
  }
  case AST_TYPE_DECLARATION: {
    ttype type = ast->type;

    if (type.tag == T_STRUCT) {
      LLVMTypeRef type_ref = codegen_ttype(ast->type, ctx);
      LLVMTypeRef *member_types =
          malloc(sizeof(LLVMTypeRef) * type.as.T_STRUCT.length);

      LLVMGetStructElementTypes(type_ref, member_types);

      LLVMTypeRef named_struct_ty = LLVMStructCreateNamed(
          ctx->context, ast->data.AST_TYPE_DECLARATION.name);

      LLVMStructSetBody(named_struct_ty, member_types, type.as.T_STRUCT.length,
                        true);
      SymbolValue type_symbol;
      type_symbol.type = TYPE_TYPE_DECLARATION;
      type_symbol.data.TYPE_TYPE_DECLARATION.llvm_type = named_struct_ty;
      type_symbol.data.TYPE_TYPE_DECLARATION.type = type;
      table_insert(ctx->symbol_table, ast->data.AST_TYPE_DECLARATION.name,
                   type_symbol);
    }
    break;
  }
  case AST_INDEX_ACCESS: {
    return codegen_index_access(ast, ctx);
  }
  case AST_FN_PROTOTYPE:
  case AST_VAR_ARG:
  default:
    break;
  }
  return NULL;
}

static int counter = 0;
char *inst_name(const char *cString) {

  counter++;
  char *result =
      (char *)malloc(32 * sizeof(char)); // Assuming a large enough buffer size
  sprintf(result, "%s%d", cString, counter);
  return result;
}
