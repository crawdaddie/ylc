#include "type_check.h"
#include "generic_sym_table.h"
#include "string.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
typedef AST *ASTPtr;

INIT_SYM_TABLE(ASTPtr, Ast);
typedef struct {
  AstSymbolTable *symbol_table;
} TypeCheckContext;

static void enter_typecheck_scope(TypeCheckContext *ctx) {
  push_Astframe(ctx->symbol_table);
}

static void exit_typecheck_scope(TypeCheckContext *ctx) {
  pop_Astframe(ctx->symbol_table);
}

static TYPES typecheck_ast(AST *ast, TypeCheckContext *ctx) {
  printf("tcheck %d\n", ast->tag);
  switch (ast->tag) {
  case AST_MAIN: {
    enter_typecheck_scope(ctx);
    typecheck_ast(ast->data.AST_MAIN.body, ctx);
    exit_typecheck_scope(ctx);
    return T_VOID;
  }
  case AST_STATEMENT_LIST: {
    TYPES t;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      AST *stmt_ast = ast->data.AST_STATEMENT_LIST.statements[i];
      t = typecheck_ast(stmt_ast, ctx);
    }
    return t;
  }
  case AST_FN_DECLARATION: {
    char *name = ast->data.AST_FN_DECLARATION.name;
    table_Astinsert(ctx->symbol_table, name, ast);
    enter_typecheck_scope(ctx);
    typecheck_ast(ast->data.AST_FN_DECLARATION.prototype, ctx);
    TYPES ret_type = typecheck_ast(ast->data.AST_FN_DECLARATION.body, ctx);
    // TODO: use any return values in the above step to set the type of
    // of the function
    exit_typecheck_scope(ctx);

    return T_COMPOUND;
  }
  case AST_FN_PROTOTYPE: {
    int arg_count = ast->data.AST_FN_PROTOTYPE.length;
    AST **parameters = ast->data.AST_FN_PROTOTYPE.parameters;

    for (int i = 0; i < arg_count; i++) {
      AST *param_ast = parameters[i];
      struct AST_SYMBOL_DECLARATION param_symbol =
          param_ast->data.AST_SYMBOL_DECLARATION;
      // insert param into symbol table for current stack
      table_Astinsert(ctx->symbol_table, param_symbol.identifier, param_ast);
    }
    return T_COMPOUND;
  }
  case AST_CALL: {
    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;
    AST *func_ast;
    if (table_Astlookup(ctx->symbol_table, name, &func_ast) != 0) {
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

    return T_VOID;
  }
  case AST_BINOP:
  case AST_ASSIGNMENT:
  case AST_INTEGER:
  case AST_NUMBER:
  case AST_BOOL:
  case AST_STRING:
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
    return T_VOID;
  }
  }

  return T_VOID;
}

void typecheck(AST *ast) {
  TypeCheckContext ctx;
  AstSymbolTable symbol_table;
  symbol_table.current_frame_index = -1;
  ctx.symbol_table = &symbol_table;
  typecheck_ast(ast, &ctx);
}
