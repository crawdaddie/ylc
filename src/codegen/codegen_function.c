#include "codegen_function.h"
#include "codegen.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>

static bool function_is_void(LLVMValueRef func) {
  // Get the return type of the function
  LLVMTypeRef returnType = LLVMGetReturnType(LLVMGlobalGetValueType(func));

  // Check if the return type is void
  return LLVMGetTypeKind(returnType) == LLVMVoidTypeKind;
}

static void store_parameters(AST *prot, Context *ctx) {
  int arg_count = prot->data.AST_FN_PROTOTYPE.length;
  AST **parameters = prot->data.AST_FN_PROTOTYPE.parameters;

  for (int i = 0; i < arg_count; i++) {
    AST *param_ast = parameters[i];

    struct AST_SYMBOL_DECLARATION param_symbol =
        param_ast->data.AST_SYMBOL_DECLARATION;

    char *type_str = param_symbol.type;
    if (type_str == NULL) {
      // TODO: fallback to inferred type
    }

    // insert param into symbol table for current stack
    table_insert(ctx->symbol_table, param_symbol.identifier,
                 (SymbolValue){TYPE_FN_PARAM, {.TYPE_FN_PARAM = {i}}});
  }
}

static void store_self(char *name, LLVMValueRef function,
                       LLVMTypeRef function_type, ttype type, Context *ctx) {

  table_insert(ctx->symbol_table, name,
               (SymbolValue){TYPE_RECURSIVE_REF,
                             {.TYPE_RECURSIVE_REF = {.llvm_value = function,
                                                     .llvm_type = function_type,
                                                     .type = type}}});
}

static void bind_function(const char *name, LLVMValueRef func,
                          LLVMTypeRef fn_type, ttype type, Context *ctx) {

  SymbolValue sym;
  sym.type = TYPE_FUNCTION;
  sym.data.TYPE_FUNCTION.llvm_value = func;
  sym.data.TYPE_FUNCTION.llvm_type = fn_type;
  sym.data.TYPE_FUNCTION.type = type;
  table_insert(ctx->symbol_table, name, sym);
}

LLVMValueRef codegen_function(AST *ast, Context *ctx) {
  char *name = ast->data.AST_FN_DECLARATION.name;
  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
  AST *body_ast = ast->data.AST_FN_DECLARATION.body;
  ttype type = ast->type;

  LLVMTypeRef fn_type = codegen_type(ast, ctx);
  LLVMValueRef function =
      LLVMAddFunction(ctx->module, name ? name : "", fn_type);

  LLVMBasicBlockRef prev_block = LLVMGetInsertBlock(ctx->builder);
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(function, "entry");

  enter_scope(ctx);
  store_parameters(prototype_ast, ctx);
  store_self(name ? name : "this_fn", function, fn_type, type, ctx);

  LLVMPositionBuilderAtEnd(ctx->builder, block);
  LLVMValueRef body = codegen(body_ast, ctx);

  // Check if the return type is void
  if (function_is_void(function)) {
    LLVMBuildRetVoid(ctx->builder);
  } else {
    LLVMBuildRet(ctx->builder, body);
  }

  exit_scope(ctx);

  LLVMPositionBuilderAtEnd(ctx->builder, prev_block);

  // Verify function.
  if (LLVMVerifyFunction(function, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(function);
    return NULL;
  }

  LLVMRunFunctionPassManager(ctx->pass_manager, function);

  if (name) {
    bind_function(name, function, fn_type, type, ctx);
  }

  return function;
};

LLVMValueRef codegen_extern_function(AST *ast, Context *ctx) {
  char *name = ast->data.AST_FN_DECLARATION.name;
  if (!name) {
    fprintf(stderr, "Error: extern function must have a name");
    return NULL;
  }
  AST *prototype = ast->data.AST_FN_DECLARATION.prototype;
  ttype type = ast->type;
  LLVMTypeRef fn_type = codegen_type(ast, ctx);
  LLVMValueRef function = LLVMAddFunction(ctx->module, name, fn_type);

  bind_function(name, function, fn_type, type, ctx);

  return function;
};
LLVMValueRef codegen_call(AST *ast, Context *ctx) {
  char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;
  LLVMValueRef func = codegen_identifier(ast->data.AST_CALL.identifier, ctx);

  if (func == NULL) {
    fprintf(stderr, "Error: function %s not found in this scope (%d)\n",
            ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier,
            ctx->symbol_table->current_frame_index);
    return NULL;
  }

  // Evaluate arguments.
  AST *parameters = ast->data.AST_CALL.parameters;
  struct AST_TUPLE parameters_tuple = parameters->data.AST_TUPLE;
  unsigned int arg_count = parameters_tuple.length;

  unsigned int fn_param_count = LLVMCountParams(func);

  LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * arg_count);
  unsigned int i;
  for (i = 0; i < arg_count; i++) {
    args[i] = codegen(parameters_tuple.members[i], ctx);
  }

  // Get the return type of the function
  LLVMTypeRef returnType = LLVMGetReturnType(LLVMGlobalGetValueType(func));
  int is_void = 0;

  // Check if the return type is void
  if (LLVMGetTypeKind(returnType) == LLVMVoidTypeKind) {
    is_void = 1;
  }
  LLVMValueRef val =
      LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(func), func, args,
                     arg_count, "");
  LLVMDumpValue(val);

  free(args);
  return val;
}
