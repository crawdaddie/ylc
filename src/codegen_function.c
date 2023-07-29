#include "codegen_function.h"
#include "codegen.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>

static LLVMTypeRef *codegen_function_prototype_args(AST *prot, Context *ctx) {
  int arg_count = prot->data.AST_FN_PROTOTYPE.length;

  AST **parameters = prot->data.AST_FN_PROTOTYPE.parameters;

  LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * arg_count);
  for (int i = 0; i < arg_count; i++) {

    AST *param_ast = parameters[i];

    struct AST_SYMBOL_DECLARATION param_symbol =
        param_ast->data.AST_SYMBOL_DECLARATION;

    char *type_str = param_symbol.type;
    params[i] = type_lookup(type_str, ctx);
  }
  return params;
}
static void store_parameters(AST *prot, Context *ctx) {
  int arg_count = prot->data.AST_FN_PROTOTYPE.length;
  AST **parameters = prot->data.AST_FN_PROTOTYPE.parameters;

  for (int i = 0; i < arg_count; i++) {
    AST *param_ast = parameters[i];

    struct AST_SYMBOL_DECLARATION param_symbol =
        param_ast->data.AST_SYMBOL_DECLARATION;

    char *type_str = param_symbol.type;

    // insert param into symbol table for current stack
    table_insert(ctx->symbol_table, param_symbol.identifier,
                 (SymbolValue){TYPE_FN_PARAM, {.TYPE_FN_PARAM = {i}}});
  }
}

static void store_self(char *name, LLVMValueRef function,
                       LLVMTypeRef function_type, Context *ctx) {

  table_insert(
      ctx->symbol_table, name,
      (SymbolValue){TYPE_RECURSIVE_REF,
                    {.TYPE_RECURSIVE_REF = {.llvm_value = function,
                                            .llvm_type = function_type}}});
}

void codegen_prototype(AST *ast, Context *ctx, LLVMValueRef *func,
                       LLVMTypeRef *func_type, LLVMTypeRef *func_return_type,
                       const char *name) {

  struct AST_FN_PROTOTYPE data = AST_DATA(ast, FN_PROTOTYPE);

  int arg_count = data.length;
  LLVMTypeRef *prototype = codegen_function_prototype_args(ast, ctx);
  LLVMTypeRef ret_type = type_lookup(data.type, ctx);
  *func_return_type = ret_type;

  LLVMTypeRef function_type =
      LLVMFunctionType(ret_type, prototype, arg_count, 0);

  *func = LLVMAddFunction(ctx->module, name, function_type);
  *func_type = LLVMTypeOf(*func);
}

static LLVMTypeRef codegen_extern_prototype(AST *extern_ast, Context *ctx) {

  int arg_count = extern_ast->data.AST_FN_PROTOTYPE.length;

  LLVMTypeRef *param_types = codegen_function_prototype_args(extern_ast, ctx);

  LLVMTypeRef ret_type =
      type_lookup(extern_ast->data.AST_FN_PROTOTYPE.type, ctx);

  return LLVMFunctionType(ret_type, param_types, arg_count, 0);
}

LLVMValueRef codegen_extern_function(AST *ast, Context *ctx) {
  char *name = ast->data.AST_FN_DECLARATION.name;

  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;

  LLVMTypeRef func_type = codegen_extern_prototype(prototype_ast, ctx);

  LLVMValueRef func = LLVMAddFunction(ctx->module, name, func_type);

  declare_extern_function(name, func, func_type, ctx);
  return func;
}

LLVMValueRef codegen_named_function(AST *ast, Context *ctx, char *name) {
  LLVMValueRef func;
  LLVMTypeRef func_type;
  LLVMTypeRef ret_type;

  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;

  codegen_prototype(prototype_ast, ctx, &func, &func_type, &ret_type,
                    strdup(name));

  LLVMBasicBlockRef prevBlock = LLVMGetInsertBlock(ctx->builder);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  enter_scope(ctx);

  store_parameters(ast->data.AST_FN_DECLARATION.prototype, ctx);
  if (name != NULL) {
    store_self(name, func, func_type, ctx);
  }

  LLVMValueRef body = codegen(ast->data.AST_FN_DECLARATION.body, ctx);

  // Get the return type of the function
  LLVMTypeRef returnType = LLVMGetReturnType(LLVMGlobalGetValueType(func));
  // int is_void = 0;

  // Check if the return type is void
  if (LLVMGetTypeKind(returnType) == LLVMVoidTypeKind) {
    // is_void = 1;
    LLVMBuildRetVoid(ctx->builder);
  } else {
    LLVMBuildRet(ctx->builder, body);
  }

  exit_scope(ctx);

  LLVMPositionBuilderAtEnd(ctx->builder, prevBlock);

  if (!(func && body)) {
    return NULL;
  }

  // Verify function.
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMRunFunctionPassManager(ctx->pass_manager, func);
  if (name != NULL) {
    // codegen_symbol(name, func, LLVMTypeOf(func), ctx);
    SymbolValue sym;

    sym.type = TYPE_FUNCTION;
    sym.data.TYPE_FUNCTION.llvm_value = func;
    sym.data.TYPE_FUNCTION.llvm_type = func_type;
    // sym.data.TYPE_FUNCTION.ret_type = ret_type;
    table_insert(ctx->symbol_table, name, sym);
  }
  return func;
}

static LLVMValueRef curry_function(struct AST_TUPLE parameters_tuple,
                                   LLVMValueRef func, Context *ctx) {
  fprintf(stderr,
          "NOT YET IMPLEMENTED ERROR: fewer arguments supplied - return a "
          "curried version\n");

  return NULL;
}

LLVMValueRef codegen_call(AST *ast, Context *ctx) {
  char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;

  LLVMValueRef func = codegen_identifier(ast->data.AST_CALL.identifier, ctx);
  if (func == NULL) {
    fprintf(stderr, "Error: function %s not found in this scope\n",
            ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier);
    return NULL;
  }

  // Evaluate arguments.
  AST *parameters = ast->data.AST_CALL.parameters;
  struct AST_TUPLE parameters_tuple = parameters->data.AST_TUPLE;
  unsigned int arg_count = parameters_tuple.length;

  if (arg_count < LLVMCountParams(func)) {
    return curry_function(parameters_tuple, func, ctx);
  }

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
                     arg_count, is_void ? "" : inst_name("call"));

  free(args);
  return val;
}
