#include "codegen_symbol.h"
#include "codegen.h"
#include "codegen_types.h"
#include <stdio.h>
#include <stdlib.h>

SymbolValue get_value(char *identifier, Context *ctx) {
  SymbolValue val;
  if (table_lookup(ctx->symbol_table, identifier, &val) != 0) {
    fprintf(stderr, "get value Error symbol %s not found\n", identifier);
  }
  return val;
}
/**
 *
 * */
LLVMValueRef codegen_symbol(const char *name, ttype type, AST *expr,
                            Context *ctx) {

  SymbolValue val;
  if (table_lookup(ctx->symbol_table, name, &val) == 0) {
    if (val.type == TYPE_GLOBAL_VARIABLE) {

      LLVMValueRef variable = val.data.TYPE_GLOBAL_VARIABLE.llvm_value;
      LLVMValueRef value = codegen(expr, ctx);
      LLVMValueRef s = LLVMBuildStore(ctx->builder, value, variable);
      // LLVMDumpValue(s);
      return variable;
    } else if (val.type == TYPE_VARIABLE) {
      LLVMValueRef variable = val.data.TYPE_VARIABLE.llvm_value;
      LLVMValueRef value = codegen(expr, ctx);
      LLVMBuildStore(ctx->builder, value, variable);
      return variable;
    }
  }

  LLVMValueRef value = NULL;
  LLVMTypeRef type_ref = codegen_ttype(type, ctx);

  LLVMValueRef variable = NULL;

  if (ctx->symbol_table->current_frame_index == 0) {
    variable = LLVMAddGlobal(ctx->module, type_ref, name);
    LLVMSetInitializer(variable, LLVMConstNull(type_ref));
    val.type = TYPE_GLOBAL_VARIABLE;
    val.data.TYPE_GLOBAL_VARIABLE.llvm_value = variable;
    val.data.TYPE_GLOBAL_VARIABLE.llvm_type = type_ref;
    val.data.TYPE_GLOBAL_VARIABLE.type = type;

  } else {
    // create local alloca var
    variable = LLVMBuildAlloca(ctx->builder, type_ref, name);
    val.type = TYPE_VARIABLE;
    val.data.TYPE_VARIABLE.llvm_value = variable;
    val.data.TYPE_VARIABLE.llvm_type = type_ref;
    val.data.TYPE_VARIABLE.type = type;
  }

  table_insert(ctx->symbol_table, name, val);

  if (expr) {
    value = codegen(expr, ctx);
    LLVMBuildStore(ctx->builder, value, variable);
    return variable;
  };

  return variable;
}

LLVMValueRef codegen_identifier(AST *ast, Context *ctx) {
  char *name = ast->data.AST_IDENTIFIER.identifier;

  SymbolValue val;

  if (table_lookup(ctx->symbol_table, name, &val) != 0) {
    fprintf(stderr, "ident Error symbol %s not found\n", name);
    return NULL;
  }

  switch (val.type) {
  case TYPE_VARIABLE: {
    LLVMValueRef variable = val.data.TYPE_VARIABLE.llvm_value;
    return LLVMBuildLoad2(
        ctx->builder, LLVMGetElementType(LLVMTypeOf(variable)), variable, "");
  }

  case TYPE_GLOBAL_VARIABLE: {
    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, name);
    // return LLVMGetInitializer(global);
    //
    return LLVMBuildLoad2(ctx->builder, LLVMGetElementType(LLVMTypeOf(global)),
                          global, "");
  }

  case TYPE_FN_PARAM: {
    return LLVMGetParam(current_function(ctx), val.data.TYPE_FN_PARAM.arg_idx);
  }

  case TYPE_FUNCTION: {
    // return val.data.TYPE_FUNCTION.llvm_value;
    return LLVMGetNamedFunction(ctx->module, name);
  }

  case TYPE_RECURSIVE_REF: {
    return LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
  }
  }

  return NULL;
}
