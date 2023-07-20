#include "codegen.h"
#include "codegen_arithmetic.h"
#include "codegen_conditionals.h"
#include "codegen_function.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
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
    return codegen_int(ast, ctx);

  case AST_NUMBER: {
    return codegen_number(ast, ctx);
  }

  case AST_STRING: {
    struct AST_STRING data = AST_DATA(ast, STRING);

    const char *str = data.value;
    int len = data.length;
    return LLVMBuildGlobalStringPtr(ctx->builder, str, str);
  }

  case AST_BINOP: {
    struct AST_BINOP data = AST_DATA(ast, BINOP);
    LLVMValueRef left = codegen(data.left, ctx);
    LLVMValueRef right = codegen(data.right, ctx);

    if (value_is_numeric(left) && value_is_numeric(right)) {
      return numerical_binop(data.op, left, right, ctx);
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
    struct AST_FN_DECLARATION data = AST_DATA(ast, FN_DECLARATION);
    if (data.name != NULL && data.is_extern) {
      return codegen_extern_function(ast, ctx);
    }
    return codegen_named_function(ast, ctx, data.name);
  }

  case AST_CALL: {
    return codegen_call(ast, ctx);
  }

  case AST_IDENTIFIER: {
    struct AST_IDENTIFIER data = AST_DATA(ast, IDENTIFIER);
    SymbolValue value = get_value(data.identifier, ctx);
    return codegen_identifier(ast, ctx);
  }

  case AST_SYMBOL_DECLARATION: {
    // symbol declaration [ let a ]- rarely used
    return codegen_symbol_declaration(ast, ctx);
  }

  case AST_ASSIGNMENT: {
    // symbol assignment [ a = x ]
    // also handles immediate declaration + assignment [ let a = x ]
    return codegen_symbol_assignment(ast, ctx);
  }

  case AST_IF_ELSE: {
    return codegen_if_else(ast, ctx);
  }
  case AST_MATCH: {
    return codegen_match(ast, ctx);
  }
  case AST_MEMBER_ACCESS: {
    struct AST_MEMBER_ACCESS data = AST_DATA(ast, MEMBER_ACCESS);
    return NULL;
  }
  case AST_TUPLE: {
    struct AST_TUPLE data = AST_DATA(ast, TUPLE);
    LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * data.length);
    for (int i = 0; i < data.length; i++) {
      members[i] = codegen(data.members[i], ctx);
    }
    LLVMValueRef tuple_struct = LLVMConstStruct(members, data.length, true);
    return tuple_struct;
  }
  case AST_TYPE_DECLARATION: {
    struct AST_TYPE_DECLARATION data = AST_DATA(ast, TYPE_DECLARATION);
    LLVMTypeRef type = codegen_type(data.type_expr, data.name, ctx);

    SymbolValue v = VALUE(TYPE_DECLARATION, type);
    if (table_lookup(ctx->symbol_table, data.name, &v) != 0) {
      table_insert(ctx->symbol_table, data.name, v);
      return NULL;
    }

    return NULL;
  }
  case AST_INDEX_ACCESS: {
    struct AST_INDEX_ACCESS data = AST_DATA(ast, INDEX_ACCESS);
    LLVMValueRef object = codegen(data.object, ctx);
    if (data.index_expr->tag == AST_INTEGER) {
      int index = data.index_expr->data.AST_INTEGER.value;
      return LLVMBuildExtractValue(ctx->builder, object, index, "nth_member");
    }
    LLVMTypeRef object_type = LLVMTypeOf(object);
    LLVMValueRef allocaInst =
        LLVMBuildAlloca(ctx->builder, object_type, "tmp_alloca_for_ptr");
    LLVMTypeRef struct_ptr_type = LLVMPointerType(LLVMTypeOf(object), 0);
    LLVMValueRef struct_ptr = LLVMBuildPointerCast(
        ctx->builder, allocaInst, struct_ptr_type, "object_ptr_cast");

    LLVMValueRef index_val = codegen(data.index_expr, ctx);
    LLVMValueRef member_ptr = LLVMBuildGEP2(
        ctx->builder, struct_ptr_type, struct_ptr, &index_val, 1, "nth_member");

    printf("member access\n");
      LLVMDumpValue(member_ptr);
    // LLVMDumpType( LLVMGetElementType(LLVMTypeOf(member_ptr)));

    return LLVMBuildLoad2(ctx->builder, LLVMInt32TypeInContext(ctx->context),
                          member_ptr, "ptr_deref");
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
