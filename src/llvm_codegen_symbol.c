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
    // found symbol
    LLVMValueRef value = sym.data.TYPE_VARIABLE.llvm_value;
    return LLVMBuildLoad2(ctx->builder, sym.data.TYPE_VARIABLE.llvm_type, value,
                          identifier);
  }

  case TYPE_GLOBAL_VARIABLE: {
    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, identifier);
    // LLVMValueRef global = sym.data.TYPE_GLOBAL_VARIABLE.llvm_value;
    return LLVMGetInitializer(global);
  }

  case TYPE_FN_PARAM: {
    return LLVMGetParam(ctx->currentFunction, sym.data.TYPE_FN_PARAM.arg_idx);
  }
  case TYPE_RECURSIVE_REF: {
    // found symbol
    LLVMValueRef value = sym.data.TYPE_RECURSIVE_REF.llvm_value;
    return LLVMBuildLoad2(ctx->builder, sym.data.TYPE_RECURSIVE_REF.llvm_type,
                          value, identifier);
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
static LLVMValueRef declare_global(char *identifier, LLVMValueRef value,
                                   LLVMTypeRef type, Context *ctx) {
  SymbolValue sym;

  // global
  sym.type = TYPE_GLOBAL_VARIABLE;
  sym.data.TYPE_GLOBAL_VARIABLE.llvm_value =
      LLVMAddGlobal(ctx->module, type, identifier);
  sym.data.TYPE_GLOBAL_VARIABLE.llvm_type = type;

  LLVMSetInitializer(sym.data.TYPE_GLOBAL_VARIABLE.llvm_value, value);

  table_insert(ctx->symbol_table, strdup(identifier), sym);
  return sym.data.TYPE_GLOBAL_VARIABLE.llvm_value;
}
static LLVMValueRef assign_global(SymbolValue symbol, char *identifier,
                                  LLVMValueRef value, LLVMTypeRef type,
                                  Context *ctx) {
  printf("assign pre-existing global\n");
  LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, identifier);

  if (symbol.data.TYPE_GLOBAL_VARIABLE.llvm_type != type) {
    fprintf(stderr, "Error assigning value of type %s to variable of type %s",
            LLVMPrintTypeToString(type),
            LLVMPrintTypeToString(symbol.data.TYPE_VARIABLE.llvm_type));
    return NULL;
  }

  LLVMSetInitializer(global, value);
  return global;
}

static LLVMValueRef declare_local(char *identifier, LLVMValueRef value,
                                  LLVMTypeRef type, Context *ctx) {
  SymbolValue sym;
  sym.type = TYPE_VARIABLE;
  LLVMValueRef alloca = LLVMBuildAlloca(ctx->builder, type, identifier);
  LLVMBuildStore(ctx->builder, value, alloca);

  sym.data.TYPE_VARIABLE.llvm_value = alloca;
  sym.data.TYPE_VARIABLE.llvm_type = type;

  table_insert(ctx->symbol_table, strdup(identifier), sym);
  return sym.data.TYPE_VARIABLE.llvm_value;
}

static LLVMValueRef assign_local(SymbolValue symbol, char *identifier,
                                 LLVMValueRef value, LLVMTypeRef type,
                                 Context *ctx) {

  LLVMValueRef alloca = symbol.data.TYPE_VARIABLE.llvm_value;
  LLVMTypeRef symbol_type = symbol.data.TYPE_VARIABLE.llvm_type;

  if (symbol_type != type) {

    fprintf(stderr, "Error assigning value of type %s to variable of type %s",
            LLVMPrintTypeToString(type),
            LLVMPrintTypeToString(symbol.data.TYPE_VARIABLE.llvm_type));
    return NULL;
  }
  LLVMBuildStore(ctx->builder, value, alloca);
  return alloca;
}

LLVMValueRef codegen_symbol(char *identifier, LLVMValueRef value,
                            LLVMTypeRef type, Context *ctx) {

  SymbolValue sym;
  if (table_lookup(ctx->symbol_table, identifier, &sym) != 0) {
    // symbol not found
    if (ctx->symbol_table->current_frame_index == 0) {
      return declare_global(identifier, value, type, ctx);
    } else {
      return declare_local(identifier, value, type, ctx);
    }
  }

  switch (sym.type) {

  case TYPE_GLOBAL_VARIABLE: {
    return assign_global(sym, identifier, value, type, ctx);
  }
  case TYPE_VARIABLE: {
    return assign_local(sym, identifier, value, type, ctx);
  }
  case TYPE_FN_PARAM: {
    // don't reassign fn params for now, TODO:
    // if reassigning to fn param, change type to variable and make a local var
    return NULL;
  }
  }
}

LLVMValueRef codegen_symbol_assignment(AST *ast, Context *ctx) {

  char *identifier = ast->data.AST_ASSIGNMENT.identifier;
  AST *expr = ast->data.AST_ASSIGNMENT.expression;

  LLVMValueRef value = codegen(expr, ctx);

  if (value == NULL) {
    return NULL;
  }

  LLVMTypeRef type = LLVMTypeOf(value);

  return codegen_symbol(identifier, value, type, ctx);
}
