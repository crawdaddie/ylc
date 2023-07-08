#include "ast.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

void left_pad(int indent) { printf("%*s", indent, ""); }
void print_ast(AST ast, int indent) {
  switch (ast.tag) {
  case AST_MAIN:
    left_pad(indent);
    printf("main:\n");
    return print_ast(*ast.data.AST_MAIN.body, indent + 1);

  case AST_INTEGER: {
    left_pad(indent);
    printf("%d ", ast.data.AST_INTEGER.value);
    break;
  }

  case AST_NUMBER: {
    left_pad(indent);
    printf("%f ", ast.data.AST_NUMBER.value);
    break;
  }

  case AST_BINOP: {
    left_pad(indent);
    printf("(");
    print_token((token){.type = ast.data.AST_BINOP.op});
    print_ast(*ast.data.AST_BINOP.left, 0);
    print_ast(*ast.data.AST_BINOP.right, 0);
    printf(")");
    break;
  }

  case AST_UNOP: {
    left_pad(indent);
    printf("(");
    print_token((token){.type = ast.data.AST_UNOP.op});
    print_ast(*ast.data.AST_UNOP.operand, 0);
    printf(")");
    break;
  }

  case AST_STATEMENT_LIST: {
    left_pad(indent);
    printf("statements: \n");
    for (int i = 0; i < ast.data.AST_STATEMENT_LIST.length; i++) {
      print_ast(**(ast.data.AST_STATEMENT_LIST.statements + i), indent + 1);
      printf("\n");
    }
    break;
  }

  case AST_SYMBOL_DECLARATION: {
    left_pad(indent);
    printf("symbol decl: %s\n", ast.data.AST_SYMBOL_DECLARATION.identifier);
    break;
  }

  case AST_ASSIGNMENT: {
    left_pad(indent);
    printf("assign to symbol: [%s]\n", ast.data.AST_ASSIGNMENT.identifier);
    print_ast(*ast.data.AST_ASSIGNMENT.expression, indent + 1);
    break;
  }
  }
}
void free_ast(AST *ast) {
  switch (ast->tag) {
  case AST_MAIN:
    free_ast(ast->data.AST_MAIN.body);
    break;

  case AST_INTEGER:
    free(ast);

    break;

  case AST_NUMBER:

    free(ast);
    break;

  case AST_BINOP: {
    free_ast(ast->data.AST_BINOP.left);
    free_ast(ast->data.AST_BINOP.right);
    free(ast);

    break;
  }

  case AST_UNOP: {
    free_ast(ast->data.AST_UNOP.operand);
    free(ast);

    break;
  }

  case AST_STATEMENT_LIST: {

    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      free_ast(ast->data.AST_STATEMENT_LIST.statements[i]);
    }
    free(ast);
    break;
  }

  case AST_SYMBOL_DECLARATION: {

    free(ast->data.AST_SYMBOL_DECLARATION.identifier);
    free(ast);
  }

  case AST_ASSIGNMENT: {
    AST *expr = ast->data.AST_ASSIGNMENT.expression;
    if (expr) {
      free_ast(expr);
      free(ast->data.AST_ASSIGNMENT.identifier);
      free(ast);
    }
    break;
  }
  }
}

struct AST_STATEMENT_LIST *new_ast_stmt_list() {
  AST *body = malloc(sizeof(AST));
  body->tag = AST_STATEMENT_LIST;
  body->data.AST_STATEMENT_LIST.length = 1;
  body->data.AST_STATEMENT_LIST.statements = malloc(sizeof(AST_STATEMENT));
  return body;
};
