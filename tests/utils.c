#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int test_result;
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

  case AST_BOOL: {
    if (a->data.AST_BOOL.value != b->data.AST_BOOL.value) {
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

  case AST_FN_PROTOTYPE: {
    if (a->data.AST_FN_PROTOTYPE.length != b->data.AST_FN_PROTOTYPE.length) {
      return 1;
    }

    for (int i = 0; i < a->data.AST_FN_PROTOTYPE.length; i++) {
      if (compare_ast(a->data.AST_FN_PROTOTYPE.parameters[i],
                      b->data.AST_FN_PROTOTYPE.parameters[i])) {

        return 1;
      }
    }
    return 0;
  }

  case AST_IDENTIFIER: {
    if (strcmp(a->data.AST_IDENTIFIER.identifier,
               b->data.AST_IDENTIFIER.identifier) != 0) {
      return 1;
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
  case AST_FN_DECLARATION: {
    if (compare_ast(a->data.AST_FN_DECLARATION.prototype,
                    b->data.AST_FN_DECLARATION.prototype) != 0) {
      return 1;
    }

    if (a->data.AST_FN_DECLARATION.body && !b->data.AST_FN_DECLARATION.body) {
      return 1;
    }

    if (!a->data.AST_FN_DECLARATION.body && b->data.AST_FN_DECLARATION.body) {
      return 1;
    }

    if (a->data.AST_FN_DECLARATION.body &&
        compare_ast(a->data.AST_FN_DECLARATION.body,
                    b->data.AST_FN_DECLARATION.body) != 0) {

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
  case AST_STRING: {
    if (strcmp(a->data.AST_STRING.value, b->data.AST_STRING.value) != 0 ||
        a->data.AST_STRING.length != b->data.AST_STRING.length) {
      return 1;
    }
    return 0;
  }
  case AST_MEMBER_ACCESS: {
    if (compare_ast(a->data.AST_MEMBER_ACCESS.object,
                    b->data.AST_MEMBER_ACCESS.object) != 0) {
      return 1;
    }
    if (strcmp(a->data.AST_MEMBER_ACCESS.member_name,
               b->data.AST_MEMBER_ACCESS.member_name) != 0) {
      return 1;
    }
    return 0;
  }

  case AST_MEMBER_ASSIGNMENT: {
    if (compare_ast(a->data.AST_MEMBER_ASSIGNMENT.object,
                    b->data.AST_MEMBER_ASSIGNMENT.object) != 0) {
      return 1;
    }
    if (strcmp(a->data.AST_MEMBER_ASSIGNMENT.member_name,
               b->data.AST_MEMBER_ASSIGNMENT.member_name) != 0) {
      return 1;
    }

    if (compare_ast(a->data.AST_MEMBER_ASSIGNMENT.expression,
                    b->data.AST_MEMBER_ASSIGNMENT.expression) != 0) {
      return 1;
    }
    return 0;
  }

  case AST_IF_ELSE: {
    if ((compare_ast(a->data.AST_IF_ELSE.condition,
                     b->data.AST_IF_ELSE.condition) != 0) ||
        (compare_ast(a->data.AST_IF_ELSE.then_body,
                     b->data.AST_IF_ELSE.then_body) != 0)) {
      return 1;
    }
    if (a->data.AST_IF_ELSE.else_body != NULL &&
        (compare_ast(a->data.AST_IF_ELSE.then_body,
                     b->data.AST_IF_ELSE.then_body) != 0)) {
      return 1;
    }
    return 0;
  }
  case AST_MATCH: {
    if ((compare_ast(a->data.AST_MATCH.candidate,
                     b->data.AST_MATCH.candidate) != 0) ||
        (a->data.AST_MATCH.length != b->data.AST_MATCH.length)) {
      return 1;
    }

    if (a->data.AST_MATCH.result_type != NULL &&
        (compare_ast(a->data.AST_MATCH.result_type,
                     b->data.AST_MATCH.result_type) != 0)) {
      return 1;
    }

    for (int i = 0; i < a->data.AST_MATCH.length; i++) {
      if (compare_ast(a->data.AST_MATCH.matches[i],
                      b->data.AST_MATCH.matches[i]) != 0) {
        return 1;
      }
    }
    return 0;
  }
  case AST_TUPLE: {
    if (a->data.AST_TUPLE.length != b->data.AST_TUPLE.length) {
      return 1;
    }
    for (int i = 0; i < a->data.AST_TUPLE.length; i++) {
      if (compare_ast(a->data.AST_TUPLE.members[i],
                      b->data.AST_TUPLE.members[i]) != 0) {
        return 1;
      }
    }
    return 0;
  }

  case AST_ARRAY: {
    if (a->data.AST_ARRAY.length != b->data.AST_ARRAY.length) {
      return 1;
    }
    for (int i = 0; i < a->data.AST_ARRAY.length; i++) {
      if (compare_ast(a->data.AST_ARRAY.members[i],
                      b->data.AST_ARRAY.members[i]) != 0) {
        return 1;
      }
    }
    return 0;
  }
  case AST_CALL: {
    if (compare_ast(a->data.AST_CALL.identifier, b->data.AST_CALL.identifier) !=
            0 ||
        compare_ast(a->data.AST_CALL.parameters, b->data.AST_CALL.parameters)) {
      return 1;
    }
    return 0;
  }
  case AST_TYPE_DECLARATION: {
    if (strcmp(a->data.AST_TYPE_DECLARATION.name,
               b->data.AST_TYPE_DECLARATION.name) != 0 ||
        compare_ast(a->data.AST_TYPE_DECLARATION.type_expr,
                    b->data.AST_TYPE_DECLARATION.type_expr) != 0) {
      return 1;
    }
    return 0;
  }

  case AST_STRUCT: {
    if (a->data.AST_STRUCT.length != b->data.AST_STRUCT.length) {
      return 1;
    }
    for (int i = 0; i < a->data.AST_STRUCT.length; i++) {
      if (compare_ast(a->data.AST_STRUCT.members[i],
                      b->data.AST_STRUCT.members[i]) != 0) {
        return 1;
      }
    }
    return 0;
  }

  default: {
    printf("missing comparison for ast nodes type: %d\n", a->tag);
  }
  }
}
