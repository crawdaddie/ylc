#include "utils.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int compare_ast(AST *a, AST *b) {
  if (a->tag != b->tag) {
    return 1;
  }

  switch (a->tag) {
  case AST_MAIN: {
    return compare_ast(a->data.AST_MAIN.body, b->data.AST_MAIN.body);
  }

  case AST_INTEGER: {
    if (a->data.AST_INTEGER.value != b->data.AST_INTEGER.value) {
      return 1;
    }
    return 0;
  }

  case AST_NUMBER: {
    if (a->data.AST_NUMBER.value != b->data.AST_NUMBER.value) {
      return 1;
    }
    return 0;
  }

  case AST_BINOP: {
    if (a->data.AST_BINOP.op != b->data.AST_BINOP.op) {
      return 1;
    }
    if (compare_ast(a->data.AST_BINOP.left, b->data.AST_BINOP.left)) {
      return 1;
    }

    if (compare_ast(a->data.AST_BINOP.right, b->data.AST_BINOP.right)) {
      return 1;
    }
    return 0;
  }

  case AST_UNOP: {

    if (a->data.AST_UNOP.op != b->data.AST_UNOP.op) {
      return 1;
    }
    if (compare_ast(a->data.AST_UNOP.operand, b->data.AST_UNOP.operand)) {
      return 1;
    }
    return 0;
  }

  case AST_STATEMENT_LIST: {
    if (a->data.AST_STATEMENT_LIST.length !=
        b->data.AST_STATEMENT_LIST.length) {
      return 1;
    }
    for (int i = 0; i < a->data.AST_STATEMENT_LIST.length; i++) {
      if (compare_ast(a->data.AST_STATEMENT_LIST.statements[i],
                      b->data.AST_STATEMENT_LIST.statements[i])) {
        return 1;
      }
    }
    return 0;
  }

  case AST_SYMBOL_DECLARATION: {
    if (strcmp(a->data.AST_SYMBOL_DECLARATION.identifier,
               b->data.AST_SYMBOL_DECLARATION.identifier) != 0) {
      return 1;
    }
    return 0;
  }

  case AST_ASSIGNMENT: {
    if (strcmp(a->data.AST_ASSIGNMENT.identifier,
               b->data.AST_ASSIGNMENT.identifier) != 0) {
      return 1;
    }
    if (compare_ast(a->data.AST_ASSIGNMENT.expression,
                    b->data.AST_ASSIGNMENT.expression)) {
      return 1;
    }
    return 0;
  }
  }
}
