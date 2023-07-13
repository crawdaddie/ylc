#include "llvm_codegen_function.h"
#include "llvm_codegen.h"
#include "llvm_codegen_symbol.h"
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
static LLVMTypeRef *codegen_function_prototype(AST *prot, Context *ctx) {
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

LLVMValueRef codegen_function(AST *ast, Context *ctx) {
  if (!(ast->data.AST_FN_DECLARATION.body)) {
    return NULL;
  }
  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;

  int arg_count = prototype_ast->data.AST_FN_PROTOTYPE.length;

  LLVMTypeRef *prototype =
      codegen_function_prototype(ast->data.AST_FN_DECLARATION.prototype, ctx);

  LLVMTypeRef ret_type =
      type_lookup(prototype_ast->data.AST_FN_PROTOTYPE.type, ctx);

  LLVMTypeRef function_type =
      LLVMFunctionType(ret_type, prototype, arg_count, 0);

  LLVMValueRef func = LLVMAddFunction(ctx->module, "tmp", function_type);

  // Create basic block.
  LLVMBasicBlockRef prevBlock = LLVMGetInsertBlock(ctx->builder);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);

  LLVMValueRef prevFunc = ctx->currentFunction;
  enter_function(ctx, func);
  store_parameters(ast->data.AST_FN_DECLARATION.prototype, ctx);
  LLVMValueRef body = codegen(ast->data.AST_FN_DECLARATION.body, ctx);
  LLVMBuildRet(ctx->builder, body);

  exit_function(ctx, prevFunc);

  LLVMPositionBuilderAtEnd(ctx->builder, prevBlock);

  if (!(prototype && body)) {
    return NULL;
  }

  // Verify function.
  if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
    fprintf(stderr, "Invalid function");
    LLVMDeleteFunction(func);
    return NULL;
  }
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
