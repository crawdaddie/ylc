#include "codegen_function.h"
#include "codegen.h"
#include "codegen_symbol.h"
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
static LLVMTypeRef type_lookup(char *type, Context *ctx) {
  // TODO: make this more smart
  if (strcmp(type, "int") == 0) {
    return LLVMInt32TypeInContext(ctx->context);
  }

  if (strcmp(type, "double") == 0) {
    return LLVMDoubleTypeInContext(ctx->context);
  }

  if (strcmp(type, "bool") == 0) {
    return LLVMInt1Type();
  }
  return NULL;
}
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

    // // insert param into symbol table for current stack
    // table_insert(
    //     ctx->symbol_table, param_symbol.identifier,
    //     (SymbolValue){TYPE_FN_PARAM, {.TYPE_FN_PARAM = {i, params[i]}}});
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

    // // insert param into symbol table for current stack
    table_insert(ctx->symbol_table, param_symbol.identifier,
                 (SymbolValue){TYPE_FN_PARAM, {.TYPE_FN_PARAM = {i}}});
  }
}

static void store_self(char *name, LLVMValueRef function, LLVMTypeRef func_type,
                       Context *ctx) {
  table_insert(ctx->symbol_table, name,
               (SymbolValue){TYPE_RECURSIVE_REF,
                             {.TYPE_RECURSIVE_REF = {.llvm_value = function,
                                                     .llvm_type = func_type}}});
}

// LLVMValueRef codegen_function_prototype(AST *ast, Context *ctx) {
//   AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
//   bool recursive = ast->data.AST_FN_DECLARATION.name != NULL;
//
//   int arg_count = prototype_ast->data.AST_FN_PROTOTYPE.length;
//
//   LLVMTypeRef *prototype =
//       codegen_function_type(ast->data.AST_FN_DECLARATION.prototype, ctx);
//
//   LLVMTypeRef ret_type =
//       type_lookup(prototype_ast->data.AST_FN_PROTOTYPE.type, ctx);
//
//   LLVMTypeRef function_type =
//       LLVMFunctionType(ret_type, prototype, arg_count, 0);
//
//   LLVMValueRef func = LLVMAddFunction(ctx->module, "tmp", function_type);
//   return func;
// }

void codegen_prototype(AST *ast, Context *ctx, LLVMValueRef *func,
                       LLVMTypeRef *func_type) {
  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;

  int arg_count = ast->data.AST_FN_PROTOTYPE.length;

  LLVMTypeRef *prototype = codegen_function_prototype_args(ast, ctx);

  LLVMTypeRef ret_type = type_lookup(ast->data.AST_FN_PROTOTYPE.type, ctx);

  LLVMTypeRef function_type =
      LLVMFunctionType(ret_type, prototype, arg_count, 0);

  *func = LLVMAddFunction(ctx->module, "tmp", function_type);
  *func_type = LLVMTypeOf(func);
}

static LLVMValueRef codegen_named_function(AST *ast, Context *ctx) {
  LLVMValueRef func;
  LLVMTypeRef func_type;

  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
  codegen_prototype(prototype_ast, ctx, &func, &func_type);

  char *name = ast->data.AST_FN_DECLARATION.name;

  LLVMBasicBlockRef prevBlock = LLVMGetInsertBlock(ctx->builder);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  LLVMValueRef prevFunc = ctx->currentFunction;
  enter_function(ctx, func);
  store_parameters(ast->data.AST_FN_DECLARATION.prototype, ctx);
  store_self(name, func, func_type, ctx);
  LLVMValueRef body = codegen(ast->data.AST_FN_DECLARATION.body, ctx);
  LLVMBuildRet(ctx->builder, body);

  exit_function(ctx, prevFunc);

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
  codegen_symbol(name, func, LLVMTypeOf(func), ctx);
  return func;
}

LLVMValueRef codegen_function(AST *ast, Context *ctx) {
  if (!(ast->data.AST_FN_DECLARATION.body)) {
    return NULL;
  }
  LLVMValueRef func;
  LLVMTypeRef func_type;
  if (ast->data.AST_FN_DECLARATION.name != NULL) {
    return codegen_named_function(ast, ctx);
  }

  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
  codegen_prototype(prototype_ast, ctx, &func, &func_type);

  LLVMBasicBlockRef prevBlock = LLVMGetInsertBlock(ctx->builder);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  LLVMValueRef prevFunc = ctx->currentFunction;
  enter_function(ctx, func);
  store_parameters(ast->data.AST_FN_DECLARATION.prototype, ctx);
  store_self(ast->data.AST_FN_DECLARATION.name, func, func_type, ctx);
  LLVMValueRef body = codegen(ast->data.AST_FN_DECLARATION.body, ctx);
  LLVMBuildRet(ctx->builder, body);

  exit_function(ctx, prevFunc);

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
  printf("return anon func\n");
  return func;
}
static LLVMValueRef curry_function(struct AST_TUPLE parameters_tuple,
                                   LLVMValueRef func, Context *ctx) {
  fprintf(stderr,
          "NOT YET IMPLEMENTED ERROR: fewer arguments supplied - return a "
          "curried version\n");

  return NULL;
}
static LLVMValueRef recursive_call(AST *ast, Context *ctx) {

  // Retrieve function.
  LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
  return func;
}
LLVMValueRef codegen_call(AST *ast, Context *ctx) {

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

  // Create call instruction.

  LLVMValueRef val = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(func),
                                    func, args, arg_count, inst_name("call"));
  free(args);
  return val;
}
