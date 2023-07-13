#include "ast.h"
#include "lexer.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void left_pad(int indent) { printf("%*s", indent, ""); }
void print_ast(AST ast, int indent) {
  left_pad(indent);
  switch (ast.tag) {
  case AST_MAIN:
    printf("main:\n");
    return print_ast(*ast.data.AST_MAIN.body, indent + 1);

  case AST_INTEGER: {
    printf(" %d", ast.data.AST_INTEGER.value);
    break;
  }

  case AST_NUMBER: {
    printf(" %f", ast.data.AST_NUMBER.value);
    break;
  }

  case AST_BINOP: {
    printf("(");
    print_token((token){.type = ast.data.AST_BINOP.op});
    print_ast(*ast.data.AST_BINOP.left, 0);
    print_ast(*ast.data.AST_BINOP.right, 0);
    printf(")");
    break;
  }

  case AST_UNOP: {
    printf("(");
    print_token((token){.type = ast.data.AST_UNOP.op});
    print_ast(*ast.data.AST_UNOP.operand, 0);
    printf(")");
    break;
  }

  case AST_STATEMENT_LIST: {
    printf("statements: \n");
    for (int i = 0; i < ast.data.AST_STATEMENT_LIST.length; i++) {
      print_ast(**(ast.data.AST_STATEMENT_LIST.statements + i), indent + 1);
      printf("\n");
    }
    break;
  }

  case AST_IDENTIFIER: {
    printf(" %s ", ast.data.AST_IDENTIFIER.identifier);
    break;
  }

  case AST_SYMBOL_DECLARATION: {
    if (ast.data.AST_SYMBOL_DECLARATION.type != NULL) {
      printf("%s %s", ast.data.AST_SYMBOL_DECLARATION.type,
             ast.data.AST_SYMBOL_DECLARATION.identifier);
    } else {
      printf("%s", ast.data.AST_SYMBOL_DECLARATION.identifier);
    }
    break;
  }

  case AST_FN_PROTOTYPE: {
    if (ast.data.AST_FN_PROTOTYPE.length == 0) {
      break;
    }
    for (int i = 0; i < ast.data.AST_FN_PROTOTYPE.length; i++) {
      printf("arg: ");
      print_ast(*ast.data.AST_FN_PROTOTYPE.parameters[i], 0);
      printf(", ");
    }
    printf("\n");
    left_pad(indent);
    printf("returns %s", ast.data.AST_FN_PROTOTYPE.type);
    break;
  }
  case AST_FN_DECLARATION: {
    printf("fn: \n");
    left_pad(indent + 1);
    printf("fn proto:\n");
    print_ast(*ast.data.AST_FN_DECLARATION.prototype, indent + 2);

    printf("\n");
    left_pad(indent + 1l);
    if (ast.data.AST_FN_DECLARATION.body) {
      printf("fn body:\n");
      print_ast(*ast.data.AST_FN_DECLARATION.body, indent + 2);
    }
    break;
  }

  case AST_ASSIGNMENT: {
    printf("assign to symbol: [%s]\n", ast.data.AST_ASSIGNMENT.identifier);
    print_ast(*ast.data.AST_ASSIGNMENT.expression, indent + 1);
    break;
  }

  case AST_CALL: {
    printf("(%s ",
           ast.data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier);
    print_ast(*ast.data.AST_CALL.parameters, 0);
    printf(")\n");
    break;
  }

  case AST_TUPLE: {
    if (ast.data.AST_TUPLE.length == 0) {
      break;
    }

    printf("(");
    for (int i = 0; i < ast.data.AST_TUPLE.length; i++) {
      print_ast(*ast.data.AST_TUPLE.members[i], 0);
      printf(",");
    }
    printf(")");
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

  case AST_FN_PROTOTYPE: {

    for (int i = 0; i < ast->data.AST_FN_PROTOTYPE.length; i++) {
      free_ast(ast->data.AST_FN_PROTOTYPE.parameters[i]);
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

AST *ast_new(AST ast) {
  AST *ptr = malloc(sizeof(AST));
  if (ptr)
    *ptr = ast;
  return ptr;
}
