#include "codegen.h"
#include <stdio.h>
void codegen(AST *ast) {

  switch (ast->tag) {
  case AST_MAIN: {
    codegen(ast->data.AST_MAIN.body);
    break;
  }

  case AST_INTEGER:

    printf("codegen int\n");
    break;

  case AST_NUMBER:

    printf("codegen number\n");
    break;

  case AST_BINOP: {
    codegen(ast->data.AST_BINOP.left);
    codegen(ast->data.AST_BINOP.right);
    break;
  }

  case AST_UNOP: {
    codegen(ast->data.AST_UNOP.operand);
    break;
  }

  case AST_STATEMENT_LIST: {
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      codegen(ast->data.AST_STATEMENT_LIST.statements[i]);
    }
    break;
  }

  case AST_SYMBOL_DECLARATION: {
    // codegen(ast->data.AST_SYMBOL_DECLARATION.identifier);
    break;
  }

  case AST_ASSIGNMENT: {
    char *identifier = ast->data.AST_ASSIGNMENT.identifier;
    AST *expr = ast->data.AST_ASSIGNMENT.expression;
    codegen(expr);
    break;
  }
  }
}
