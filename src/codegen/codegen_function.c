#include "codegen_function.h"
#include "codegen.h"
#include "codegen_symbol.h"
#include "codegen_types.h"
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>

LLVMTypeRef codegen_fn_type(AST *ast, Context *ctx) {
  ttype type = ast->type;
  int param_len =
      ast->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE.length;

  int is_var_args =
      ast->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE
          .parameters[param_len - 1]
          ->tag == AST_VAR_ARG;

  int len = type.as.T_FN.length;

  if (is_var_args) {
    len -= 1;
  }

  LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * len - 1);
  for (int i = 0; i < len - 1; i++) {
    params[i] = codegen_ttype(type.as.T_FN.members[i], ctx);
  }
  LLVMTypeRef ret_type = codegen_ttype(type.as.T_FN.members[len - 1], ctx);
  LLVMTypeRef function_type =
      LLVMFunctionType(ret_type, params, len - 1, is_var_args);

  return function_type;
  return NULL;
}
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

void bind_function(const char *name, LLVMValueRef func, LLVMTypeRef fn_type,
                   ttype type, Context *ctx) {

  SymbolValue sym;
  sym.type = TYPE_FUNCTION;
  sym.data.TYPE_FUNCTION.llvm_value = func;
  sym.data.TYPE_FUNCTION.llvm_type = fn_type;
  sym.data.TYPE_FUNCTION.type = type;
  table_insert(ctx->symbol_table, name, sym);
}

void bind_extern_function(const char *name, LLVMValueRef func,
                          LLVMTypeRef fn_type, ttype type, Context *ctx) {

  SymbolValue sym;
  sym.type = TYPE_EXTERN_FN;
  sym.data.TYPE_FUNCTION.llvm_value = func;
  sym.data.TYPE_FUNCTION.llvm_type = fn_type;
  sym.data.TYPE_FUNCTION.type = type;
  table_insert(ctx->symbol_table, name, sym);
}

void bind_sret_function(const char *name, LLVMValueRef func,
                        LLVMTypeRef fn_type, ttype type, Context *ctx) {

  SymbolValue sym;
  sym.type = TYPE_SRET_FN;
  sym.data.TYPE_SRET_FN.llvm_value = func;
  sym.data.TYPE_SRET_FN.llvm_type = fn_type;
  sym.data.TYPE_SRET_FN.type = type;
  table_insert(ctx->symbol_table, name, sym);
}

LLVMValueRef codegen_function(AST *ast, Context *ctx) {
  char *name = ast->data.AST_FN_DECLARATION.name;
  AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
  AST *body_ast = ast->data.AST_FN_DECLARATION.body;

  ttype type = ast->type;
  LLVMTypeRef fn_type = codegen_fn_type(ast, ctx);
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

LLVMTypeRef codegen_sret_fn_type(ttype type, Context *ctx) {

  int len = type.as.T_FN.length;

  LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * len);

  params[0] =
      LLVMPointerType(codegen_ttype(type.as.T_FN.members[len - 1], ctx), 0);

  for (int i = 0; i < len; i++) {
    params[i + 1] = codegen_ttype(type.as.T_FN.members[i], ctx);
  }

  LLVMTypeRef ret_type = LLVMVoidTypeInContext(ctx->context);
  LLVMTypeRef function_type = LLVMFunctionType(ret_type, params, len, false);

  return function_type;
}

LLVMValueRef codegen_sret_function(AST *ast, Context *ctx) {

  char *name = ast->data.AST_FN_DECLARATION.name;
  AST *prototype = ast->data.AST_FN_DECLARATION.prototype;

  LLVMTypeRef fn_type = codegen_sret_fn_type(ast->type, ctx);
  LLVMValueRef function = LLVMAddFunction(ctx->module, name, fn_type);
  bind_sret_function(name, function, fn_type, ast->type, ctx);

  return function;
}

LLVMValueRef codegen_extern_function(AST *ast, Context *ctx) {
  char *name = ast->data.AST_FN_DECLARATION.name;
  if (!name) {
    fprintf(stderr, "Error: extern function must have a name");
    return NULL;
  }
  ttype type = ast->type;
  ttype ret_type = get_fn_return_type(type);
  if (ret_type.tag == T_TUPLE || ret_type.tag == T_STRUCT ||
      ret_type.tag == T_PTR) {
    // TODO: if return type is not simple, change to void return and add
    // an extra pointer first arg
    return codegen_sret_function(ast, ctx);
  }

  LLVMTypeRef fn_type = codegen_fn_type(ast, ctx);
  LLVMValueRef function = LLVMAddFunction(ctx->module, name, fn_type);
  bind_function(name, function, fn_type, type, ctx);
  return function;
};

static LLVMValueRef call_sret_fn(SymbolValue sym, LLVMValueRef func,
                                 LLVMValueRef *args, unsigned int arg_count,
                                 Context *ctx) {
  ttype sret_fn_type = get_fn_return_type(sym.data.TYPE_SRET_FN.type);
  LLVMTypeRef ret_type_ref = codegen_ttype(sret_fn_type, ctx);

  LLVMValueRef *new_args = malloc(sizeof(LLVMValueRef) * (arg_count + 1));

  new_args[0] = LLVMBuildAlloca(ctx->builder, ret_type_ref, "sret_var");

  for (int i = 0; i < arg_count + 1; i++) {
    new_args[i + 1] = args[i];
  }

  LLVMValueRef val = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(func),
                                    func, new_args, arg_count + 1, "");

  free(args);
  LLVMValueRef alloca_load =
      LLVMBuildLoad2(ctx->builder, ret_type_ref, new_args[0], "sret_var_load");
  free(new_args);
  return alloca_load;
}

LLVMValueRef codegen_call(AST *ast, Context *ctx) {

  char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;

  SymbolValue sym;

  if (table_lookup(ctx->symbol_table, name, &sym) != 0) {
    fprintf(stderr, "ident Error callable %s not found\n", name);
    return NULL;
  }

  LLVMValueRef func;
  if (!(sym.type == TYPE_SRET_FN || sym.type == TYPE_FUNCTION)) {
    fprintf(stderr, "ident Error %s is not callable\n", name);
    return NULL;
  } else {
    func = LLVMGetNamedFunction(ctx->module, name);
    if (!func) {
      func = sym.data.TYPE_FUNCTION.llvm_value;
    }
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
  if (sym.type == TYPE_SRET_FN) {
    return call_sret_fn(sym, func, args, arg_count, ctx);
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
