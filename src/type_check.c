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

void type_check(AST *ast, TypeCheckContext *ctx) {
  printf("\ntypecheck: \n");
  print_ast(*ast, 0);
  switch (ast->tag) {
  case AST_MAIN: {

    enter_typecheck_scope(ctx);
    type_check(ast->data.AST_MAIN.body, ctx);
    exit_typecheck_scope(ctx);
    return;
  }
  case AST_STATEMENT_LIST: {
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      type_check(ast->data.AST_STATEMENT_LIST.statements[i], ctx);
    }
    return;
  }
  case AST_INTEGER:
  case AST_NUMBER:
  case AST_BOOL:
  case AST_STRING:
  case AST_ADD:
  case AST_SUBTRACT:
  case AST_MUL:
  case AST_DIV:
  case AST_UNOP:
  case AST_BINOP:
  case AST_EXPRESSION:
  case AST_STATEMENT:
  case AST_CALL_EXPRESSION:
  case AST_FN_DECLARATION:
  case AST_ASSIGNMENT:
  case AST_IDENTIFIER:
  case AST_SYMBOL_DECLARATION:
  case AST_FN_PROTOTYPE:
  case AST_CALL:
  case AST_TUPLE:
  case AST_IF_ELSE:
  case AST_MATCH:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ACCESS:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_IMPORT: {
    return;
  }
  default: {
    return;
  }
  }
}

void type_check_pass(AST *ast) {
  TypeCheckContext ctx;
  AstSymbolTable symbol_table;
  symbol_table.current_frame_index = -1;
  ctx.symbol_table = &symbol_table;
  type_check(ast, &ctx);
}
