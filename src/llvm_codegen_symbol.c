#include "llvm_codegen_symbol.h"
#include "llvm_codegen.h"
#include <stdio.h>
#include <stdlib.h>

LLVMValueRef codegen_identifier(AST *ast, Context *ctx) {
  char *identifier = ast->data.AST_IDENTIFIER.identifier;
  SymbolValue sym;
  if (table_lookup(ctx->symbol_table, identifier, &sym) != 0) {
    fprintf(stderr, "Error symbol %s not found\n", identifier);
    return NULL;
  }

  switch (sym.type) {
  case TYPE_VARIABLE: {
    if (sym.data.TYPE_VARIABLE.is_global) {
      LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, identifier);
      return LLVMGetInitializer(global);
      // return LLVMBuildLoad2
      // return LLVMLoadInst
      // return LLVMBuildLoad2(ctx->builder, sym.data.TYPE_VARIABLE.llvm_type,
      //                       global, "load_value");
    }

    // found symbol
    LLVMValueRef value = sym.data.TYPE_VARIABLE.llvm_value;
    return value;
  }

  case TYPE_FN_PARAM: {
    return LLVMGetParam(ctx->currentFunction, sym.data.TYPE_FN_PARAM.arg_idx);
  }
  }
}

LLVMValueRef codegen_symbol_declaration(AST *ast, Context *ctx) {
  char *identifier = ast->data.AST_SYMBOL_DECLARATION.identifier;
  char *type = ast->data.AST_SYMBOL_DECLARATION.type;

  SymbolValue sym;
  if (table_lookup(ctx->symbol_table, identifier, &sym) == 0) {
    fprintf(stderr, "Error: symbol %s already exists\n", identifier);
    return NULL;
  }

  // insert uninitialized sym
  table_insert(ctx->symbol_table, identifier, sym);
  return NULL;
}

LLVMValueRef codegen_symbol_assignment(AST *ast, Context *ctx) {

  char *identifier = ast->data.AST_ASSIGNMENT.identifier;
  AST *expr = ast->data.AST_ASSIGNMENT.expression;

  SymbolValue sym;
  if (table_lookup(ctx->symbol_table, identifier, &sym) != 0) {

    // symbol not found
    LLVMValueRef value = codegen(expr, ctx);
    if (value == NULL) {
      return NULL;
    }

    LLVMTypeRef type = LLVMTypeOf(value);

    if (ctx->symbol_table->current_frame_index == 0) {

      // global
      sym.type = TYPE_VARIABLE;
      sym.data.TYPE_VARIABLE.is_global = true;
      sym.data.TYPE_VARIABLE.llvm_value =
          LLVMAddGlobal(ctx->module, type, identifier);
      sym.data.TYPE_VARIABLE.llvm_type = type;

      LLVMSetInitializer(sym.data.TYPE_VARIABLE.llvm_value, value);
    } else {
      return NULL;
    }

    table_insert(ctx->symbol_table, strdup(identifier), sym);
    return sym.data.TYPE_VARIABLE.llvm_value;
  }

  switch (sym.type) {
  case TYPE_VARIABLE: {
    if (sym.data.TYPE_VARIABLE.is_global) {
      LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, identifier);
      LLVMValueRef value = codegen(expr, ctx);

      if (!value) {
        return NULL;
      }

      LLVMTypeRef value_type = LLVMTypeOf(value);

      if (sym.data.TYPE_VARIABLE.llvm_type != value_type) {
        fprintf(stderr,
                "Error assigning value of type %s to variable of type %s",
                LLVMPrintTypeToString(value_type),
                LLVMPrintTypeToString(sym.data.TYPE_VARIABLE.llvm_type));
        return NULL;
      }

      LLVMSetInitializer(global, value);
      return global;
    }
  }
  case TYPE_FN_PARAM: {
    // don't reassign fn params for now, TODO:
    // if reassigning to fn param, change type to variable and make a local var
    return NULL;
  }
  }
}
