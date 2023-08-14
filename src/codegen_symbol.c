#include "codegen_symbol.h"
#include "codegen.h"
#include "codegen_types.h"
#include "typecheck.h"
#include <stdio.h>
#include <stdlib.h>

SymbolValue get_value(char *identifier, Context *ctx) {
  SymbolValue val;
  if (table_lookup(ctx->symbol_table, identifier, &val) != 0) {
    fprintf(stderr, "get value Error symbol %s not found\n", identifier);
  }
  return val;
}

void set_value(char *identifier, Context *ctx, SymbolValue val) {
  SymbolValue v;
  if (table_lookup(ctx->symbol_table, identifier, &v) != 0) {
    fprintf(stderr, "Error symbol %s not found\n", identifier);
    table_insert(ctx->symbol_table, identifier, val);
  }
  switch (v.type) {
  case TYPE_VARIABLE: {
    LLVMBuildLoad2(ctx->builder, v.data.TYPE_VARIABLE.llvm_type,
                   val.data.TYPE_VARIABLE.llvm_value, identifier);
    break;
  }

  case TYPE_GLOBAL_VARIABLE: {
    LLVMBuildLoad2(ctx->builder, v.data.TYPE_GLOBAL_VARIABLE.llvm_type,
                   val.data.TYPE_GLOBAL_VARIABLE.llvm_value, identifier);
    break;
  }
  }
}

LLVMValueRef codegen_identifier(AST *ast, Context *ctx) {
  struct AST_IDENTIFIER data = AST_DATA(ast, IDENTIFIER);
  char *identifier = data.identifier;
  SymbolValue sym;

  if (table_lookup(ctx->symbol_table, identifier, &sym) != 0) {
    fprintf(stderr, "ident Error symbol %s not found\n", identifier);
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
    return LLVMGetInitializer(global);
  }

  case TYPE_FN_PARAM: {
    return LLVMGetParam(current_function(ctx), sym.data.TYPE_FN_PARAM.arg_idx);
  }

  case TYPE_FUNCTION: {
    return LLVMGetNamedFunction(ctx->module, identifier);
  }

  case TYPE_RECURSIVE_REF: {
    LLVMValueRef func =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    return func;
  }

  case TYPE_EXTERN_FN: {
    return LLVMGetNamedFunction(ctx->module, identifier);
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

  // Create new global & maybe assign to it
  SymbolValue sym;
  LLVMValueRef global = LLVMAddGlobal(ctx->module, type, identifier);
  sym.type = TYPE_GLOBAL_VARIABLE;
  sym.data.TYPE_GLOBAL_VARIABLE.llvm_value = value;
  sym.data.TYPE_GLOBAL_VARIABLE.llvm_type = type;

  if (LLVMIsConstant(value)) {
    LLVMSetInitializer(global, value);
  } else {
    LLVMBuildStore(ctx->builder, value, global);
  }

  table_insert(ctx->symbol_table, identifier, sym);
  return value;
}
static LLVMValueRef assign_global(SymbolValue symbol, char *identifier,
                                  LLVMValueRef value, LLVMTypeRef type,
                                  Context *ctx) {
  LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, identifier);

  LLVMTypeRef global_type = symbol.data.TYPE_GLOBAL_VARIABLE.llvm_type;
  if (symbol.data.TYPE_GLOBAL_VARIABLE.llvm_type != type) {
    fprintf(stderr, "Error assigning value of type %s to variable of type %s",
            LLVMPrintTypeToString(type), LLVMPrintTypeToString(global_type));
    return NULL;
  }

  if (LLVMIsConstant(value)) {
    LLVMSetInitializer(global, value);
  } else {
    LLVMBuildStore(ctx->builder, value, global);
  }

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

  table_insert(ctx->symbol_table, identifier, sym);
  return sym.data.TYPE_VARIABLE.llvm_value;
}
LLVMValueRef declare_extern_function(char *identifier, LLVMValueRef value,
                                     LLVMTypeRef type, Context *ctx) {
  SymbolValue sym;
  sym.type = TYPE_EXTERN_FN;
  sym.data.TYPE_EXTERN_FN.llvm_value = value;
  sym.data.TYPE_EXTERN_FN.llvm_type = type;
  table_insert(ctx->symbol_table, identifier, sym);
  return sym.data.TYPE_EXTERN_FN.llvm_value;
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
  struct AST_ASSIGNMENT data = AST_DATA(ast, ASSIGNMENT);

  char *identifier = data.identifier;
  AST *expr = data.expression;
  LLVMValueRef value;

  if (expr->type.tag == T_STRUCT) {

    value = codegen(expr, ctx);

    LLVMTypeRef type = LLVMTypeOf(value);
    return codegen_symbol(identifier, value, type, ctx);
  }

  if (ast->data.AST_ASSIGNMENT.type != NULL) {

    type_symbol_table *metadata =
        get_type_metadata(ast->data.AST_ASSIGNMENT.type, ctx);

    if (metadata != NULL) {
      LLVMTypeRef llvm_type = type_lookup(ast->data.AST_ASSIGNMENT.type, ctx);

      value = struct_instance_with_metadata(expr, llvm_type, metadata, ctx);
    } else {
      value = codegen(expr, ctx);
    }
  } else {
    value = codegen(expr, ctx);
  }

  if (value == NULL) {
    return NULL;
  }

  LLVMTypeRef type = NULL;

  if (data.type) {
    type = type_lookup(data.type, ctx);
  }

  if (!type) {
    type = LLVMTypeOf(value);
  }
  // bind value to symbol
  return codegen_symbol(identifier, value, type, ctx);
}
