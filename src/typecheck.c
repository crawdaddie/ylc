#include "typecheck.h"
#include "generic_symbol_table.h"
#include "string.h"
#include "symbol_table.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

INIT_SYM_TABLE(ttype);

typedef struct {
  ttype_SymbolTable *symbol_table;
} TypeCheckContext;

static void enter_typecheck_scope(TypeCheckContext *ctx) {
  ttype_push_frame(ctx->symbol_table);
}

static void exit_typecheck_scope(TypeCheckContext *ctx) {
  ttype_pop_frame(ctx->symbol_table);
}

static ttype tvoid() { return (ttype){T_VOID}; }
static ttype tint() { return (ttype){T_INT}; }
static ttype tnum() { return (ttype){T_NUM}; }
static ttype tstr() { return (ttype){T_STR}; }
static ttype tbool() { return (ttype){T_BOOL}; }
static ttype tvar(char *name) { return (ttype){T_VAR, {.T_VAR = {name}}}; }
static ttype tfn() { return (ttype){T_FN, {.T_FN = {}}}; }

static ttype typecheck_ast(AST *ast, TypeCheckContext *ctx) {
  switch (ast->tag) {
  case AST_MAIN: {
    enter_typecheck_scope(ctx);
    typecheck_ast(ast->data.AST_MAIN.body, ctx);
    exit_typecheck_scope(ctx);
    return tvoid();
  }
  case AST_STATEMENT_LIST: {
    ttype t;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      AST *stmt_ast = ast->data.AST_STATEMENT_LIST.statements[i];
      t = typecheck_ast(stmt_ast, ctx);
    }
    return t;
  }
  case AST_FN_DECLARATION: {
    char *name = ast->data.AST_FN_DECLARATION.name;
    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
    ttype func_type = tfn();
    func_type.as.T_FN.length = prototype_ast->data.AST_FN_PROTOTYPE.length + 1;
    func_type.as.T_FN.members =
        malloc(sizeof(ttype) * func_type.as.T_FN.length);

    ttype_table_insert(ctx->symbol_table, name, func_type);
    enter_typecheck_scope(ctx);
    typecheck_ast(prototype_ast, ctx);
    ttype ret_type = typecheck_ast(ast->data.AST_FN_DECLARATION.body, ctx);
    // TODO: use any return values in the above step to set the type of
    // of the function
    exit_typecheck_scope(ctx);

    return tvoid();
  }
  case AST_FN_PROTOTYPE: {
    int arg_count = ast->data.AST_FN_PROTOTYPE.length;
    AST **parameters = ast->data.AST_FN_PROTOTYPE.parameters;

    for (int i = 0; i < arg_count; i++) {
      AST *param_ast = parameters[i];
      struct AST_SYMBOL_DECLARATION param_symbol =
          param_ast->data.AST_SYMBOL_DECLARATION;
      // insert param into symbol table for current stack
      ttype_table_insert(ctx->symbol_table, param_symbol.identifier,
                         tvar(param_symbol.identifier));
    }
    return tvoid();
  }
  case AST_CALL: {
    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;
    AST *func_ast;
    ttype func_type;
    if (ttype_table_lookup(ctx->symbol_table, name, &func_type) != 0) {
      fprintf(stderr, "function %s not found in this scope\n", name);
    }
    printf("found callable %s\n", name);

    AST *func_prototype_ast = func_ast->data.AST_FN_DECLARATION.prototype;

    AST *parameters = ast->data.AST_CALL.parameters;
    struct AST_TUPLE parameters_tuple = parameters->data.AST_TUPLE;
    unsigned int arg_count = parameters_tuple.length;

    if (arg_count < func_prototype_ast->data.AST_FN_PROTOTYPE.length) {
      // TODO: replace AST with AST for curried func
    }
    // TODO: replace AST with casts, eg 1 -> 1.0 if param is double

    return tvoid();
  }
  case AST_INTEGER:
    return tint();
  case AST_NUMBER:
    return tnum();
  case AST_BOOL:
    return tbool();
  case AST_STRING:
    return tstr();
  case AST_BINOP:
  case AST_ASSIGNMENT:
  case AST_UNOP:
  case AST_IDENTIFIER:
  case AST_SYMBOL_DECLARATION:
  case AST_TUPLE:
  case AST_IF_ELSE:
  case AST_MATCH:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ACCESS:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_IMPORT:
  default: {
    printf("default %d\n", ast->tag);
    return tvoid();
  }
  }

  return tvoid();
}

void typecheck(AST *ast) {
  TypeCheckContext ctx;
  ttype_SymbolTable symbol_table;
  symbol_table.current_frame_index = -1;
  ctx.symbol_table = &symbol_table;
  typecheck_ast(ast, &ctx);
}
