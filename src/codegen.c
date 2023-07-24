#include "codegen.h"
#include "codegen_arithmetic.h"
#include "codegen_conditionals.h"
#include "codegen_function.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <dlfcn.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Support.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static LLVMValueRef codegen_dynamic_access(LLVMValueRef tuple,
                                           LLVMValueRef index, Context *ctx) {

  LLVMTypeRef object_type = LLVMTypeOf(tuple);
  LLVMValueRef allocaInst =
      LLVMBuildAlloca(ctx->builder, object_type, "tmp_alloca_for_ptr");
  LLVMBuildStore(ctx->builder, tuple, allocaInst);
  LLVMValueRef indices[] = {
      index,
  };

  LLVMValueRef elementPtr = LLVMBuildInBoundsGEP2(
      ctx->builder, object_type, allocaInst, indices, 1, "element_ptr");

  return LLVMBuildLoad2(ctx->builder, LLVMInt32Type(), elementPtr,
                        "load_ptr_val");
}

int hasExtension(const char *str, const char *extension) {
  size_t strLen = strlen(str);
  size_t extensionLen = strlen(extension);

  if (strLen >= extensionLen) {
    const char *endOfStr = str + strLen - extensionLen;
    if (strcmp(endOfStr, extension) == 0) {
      return 1; // The string ends with ".so"
    }
  }

  return 0; // The string does not end with ".so"
}
LLVMValueRef codegen(AST *ast, Context *ctx) {
  switch (ast->tag) {
  case AST_MAIN: {
    return codegen_main(ast, ctx);
  }
  case AST_IMPORT: {
    const char *module_name = ast->data.AST_IMPORT.module_name;
    if (hasExtension(module_name, ".so")) {
      void *libHandle = dlopen(ast->data.AST_IMPORT.module_name, RTLD_LAZY);
      if (!libHandle) {
        fprintf(stderr, "Error loading the shared library: %s\n", dlerror());
      }
      return NULL;
    };

    // TODO: handle native .ylc source modules
    return NULL;
  }

  case AST_INTEGER:
    printf("codegen ast int\n");
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
      LLVMValueRef num_binop = numerical_binop(data.op, left, right, ctx);
      return num_binop;
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

    LLVMValueRef value = codegen(data.object, ctx);
    const char *struct_name = LLVMGetStructName(LLVMTypeOf(value));
    type_symbol_table *metadata = get_type_metadata(struct_name, ctx);
    if (metadata == NULL) {
      return NULL;
    }
    int index = get_member_index(data.member_name, metadata);
    return LLVMBuildExtractValue(ctx->builder, value, index, "nth_member");
  }

  case AST_MEMBER_ASSIGNMENT: {
    struct AST_MEMBER_ASSIGNMENT data = AST_DATA(ast, MEMBER_ASSIGNMENT);

    LLVMValueRef value = codegen(data.object, ctx);
    const char *struct_name = LLVMGetStructName(LLVMTypeOf(value));
    type_symbol_table *metadata = get_type_metadata(struct_name, ctx);
    if (metadata == NULL) {
      return NULL;
    }
    int index = get_member_index(data.member_name, metadata);
    LLVMValueRef assignment_value = codegen(data.expression, ctx);
    return LLVMBuildInsertValue(ctx->builder, value, assignment_value, index,
                                "nth_member");
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
    type_symbol_table *type_metadata = NULL;

    if (data.type_expr->tag == AST_STRUCT) {
      type_metadata = compute_type_metadata(data.type_expr);
    }

    SymbolValue v = VALUE(TYPE_DECLARATION, type);
    v.data.TYPE_TYPE_DECLARATION.type_metadata = type_metadata;

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

    return codegen_dynamic_access(object, codegen(data.index_expr, ctx), ctx);
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
