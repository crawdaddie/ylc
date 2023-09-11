#include "ast.h"
#include "lexer.h"
#include "types.h"
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

  case AST_STRING: {
    printf(" '%s'", ast.data.AST_STRING.value);
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
      print_ast(*ast.data.AST_SYMBOL_DECLARATION.type, 0);
    }

    printf("%s", ast.data.AST_SYMBOL_DECLARATION.identifier);
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
    if (ast.data.AST_FN_PROTOTYPE.type) {
      printf("\n");
      left_pad(indent);
      printf("returns: ");
      print_ast(*ast.data.AST_FN_PROTOTYPE.type, 0);
    }
    break;
  }
  case AST_FN_DECLARATION: {
    char *name = ast.data.AST_FN_DECLARATION.name;
    if (name == NULL) {
      printf("fn: \n");
    } else if (ast.data.AST_FN_DECLARATION.is_extern) {
      printf("extern %s = fn: \n", name);
    } else {
      printf("%s = fn: \n", name);
    }

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

    if (ast.data.AST_ASSIGNMENT.type != NULL) {
      printf("%s %s", ast.data.AST_ASSIGNMENT.type,
             ast.data.AST_ASSIGNMENT.identifier);
    } else {
      printf("%s", ast.data.AST_ASSIGNMENT.identifier);
    }
    printf(" = ");
    print_ast(*ast.data.AST_ASSIGNMENT.expression, 0);
    break;
  }

  case AST_CALL: {
    if (ast.data.AST_CALL.identifier->tag == AST_IDENTIFIER) {
      printf("(%s ",
             ast.data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier);
    } else if (ast.data.AST_CALL.identifier->tag == AST_MEMBER_ACCESS) {
      printf("( ");
      print_ast(*ast.data.AST_CALL.identifier, 0);
    } else {
      printf("(anon func ");
    }
    print_ast(*ast.data.AST_CALL.parameters, 0);
    printf(")");
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

  case AST_ARRAY: {
    if (ast.data.AST_ARRAY.length == 0) {
      printf("[]");
      break;
    }

    printf("[");
    for (int i = 0; i < ast.data.AST_ARRAY.length; i++) {
      print_ast(*ast.data.AST_ARRAY.members[i], 0);
      printf(",");
    }
    printf("]");
    break;
  }
  case AST_IF_ELSE: {
    printf("if:");
    print_ast(*ast.data.AST_IF_ELSE.condition, 0);
    printf("\n");
    print_ast(*ast.data.AST_IF_ELSE.then_body, indent + 1);
    if (ast.data.AST_IF_ELSE.else_body) {
      left_pad(indent);
      printf("else:");
      printf("\n");
      print_ast(*ast.data.AST_IF_ELSE.else_body, indent + 1);
    }
    break;
  }

  case AST_MATCH: {
    if (ast.data.AST_MATCH.length == 0) {
      break;
    }
    printf("match on :");
    print_ast(*ast.data.AST_MATCH.candidate, 0);
    if (ast.data.AST_MATCH.result_type != NULL) {
      printf("( returning ");
      print_ast(*ast.data.AST_MATCH.result_type, 0);
      printf(")");
    }

    printf("\n");
    for (int i = 0; i < ast.data.AST_MATCH.length; i++) {
      print_ast(*ast.data.AST_MATCH.matches[i], indent + 1);
      printf("\n");
    }
    break;
  }
  case AST_STRUCT: {
    printf("struct\n");
    for (int i = 0; i < ast.data.AST_STRUCT.length; i++) {
      print_ast(*ast.data.AST_STRUCT.members[i], indent + 1);
      printf("\n");
    }

    break;
  }

  case AST_TYPE_DECLARATION: {
    printf("type %s\n", ast.data.AST_TYPE_DECLARATION.name);
    if (ast.data.AST_TYPE_DECLARATION.type_expr == NULL) {
      break;
    }
    print_ast(*ast.data.AST_TYPE_DECLARATION.type_expr, indent + 1);
    break;
  }

  case AST_MEMBER_ACCESS: {
    print_ast(*ast.data.AST_MEMBER_ACCESS.object, indent);
    printf(".%s", ast.data.AST_MEMBER_ACCESS.member_name);
    break;
  }

  case AST_MEMBER_ASSIGNMENT: {
    print_ast(*ast.data.AST_MEMBER_ASSIGNMENT.object, indent);
    printf(" . %s", ast.data.AST_MEMBER_ASSIGNMENT.member_name);
    printf(" = ");
    print_ast(*ast.data.AST_MEMBER_ASSIGNMENT.expression, 0);
    break;
  }

  case AST_INDEX_ACCESS: {
    print_ast(*ast.data.AST_INDEX_ACCESS.object, indent);
    printf("[");
    print_ast(*ast.data.AST_INDEX_ACCESS.index_expr, 0);
    printf("] ");
    break;
  }

  case AST_IMPORT: {
    printf("import %s\n", ast.data.AST_IMPORT.module_name);
    if (ast.data.AST_IMPORT.module_ast) {
      print_ast(*ast.data.AST_IMPORT.module_ast, indent + 1);
    }
    break;
  }
  case AST_VAR_ARG: {
    printf("<...>");
    break;
  }
  default:
    printf("ast node %d\n", ast.tag);

    break;
  }
}

static void _free(void *ptr) {
  if (ptr == NULL) {
    return;
  }
  free(ptr);
}

void free_ast(AST *ast) {
  if (ast == NULL) {
    return;
  }

  if (ast->tag == AST_CALL && ast->type.tag == T_FN) {
    // defer this til all other references freed?
    return;
  }

  switch (ast->tag) {
  case AST_MAIN:
    free_ast(ast->data.AST_MAIN.body);
    break;

  case AST_BINOP: {
    free_ast(ast->data.AST_BINOP.left);
    free_ast(ast->data.AST_BINOP.right);
    break;
  }

  case AST_UNOP: {
    free_ast(ast->data.AST_UNOP.operand);
    break;
  }

  case AST_STATEMENT_LIST: {
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      free_ast(ast->data.AST_STATEMENT_LIST.statements[i]);
    }
    break;
  }

  case AST_FN_PROTOTYPE: {
    for (int i = 0; i < ast->data.AST_FN_PROTOTYPE.length; i++) {
      free_ast(ast->data.AST_FN_PROTOTYPE.parameters[i]);
    }
    break;
  }

  case AST_SYMBOL_DECLARATION: {
    free(ast->data.AST_SYMBOL_DECLARATION.identifier);
    _free(ast->data.AST_SYMBOL_DECLARATION.type);
    free_ast(ast->data.AST_SYMBOL_DECLARATION.expression);

    break;
  }

  case AST_ASSIGNMENT: {
    free(ast->data.AST_ASSIGNMENT.identifier);
    _free(ast->data.AST_ASSIGNMENT.type);
    AST *expr = ast->data.AST_ASSIGNMENT.expression;
    free_ast(expr);
    break;
  }

  case AST_CALL: {
    free_ast(ast->data.AST_CALL.identifier);
    free_ast(ast->data.AST_CALL.parameters);
    break;
  }
  case AST_TUPLE: {
    for (int i = 0; i < ast->data.AST_TUPLE.length; i++) {
      free_ast(ast->data.AST_TUPLE.members[i]);
    }
    break;
  }

  case AST_ARRAY: {
    for (int i = 0; i < ast->data.AST_ARRAY.length; i++) {
      free_ast(ast->data.AST_ARRAY.members[i]);
    }
    break;
  }
  case AST_IF_ELSE: {
    free_ast(ast->data.AST_IF_ELSE.condition);
    free_ast(ast->data.AST_IF_ELSE.then_body);
    free_ast(ast->data.AST_IF_ELSE.else_body);
    break;
  }

  case AST_MATCH: {
    free_ast(ast->data.AST_MATCH.candidate);
    for (int i = 0; i < ast->data.AST_MATCH.length; i++) {
      free_ast(ast->data.AST_MATCH.matches[i]);
    }
    free_ast(ast->data.AST_MATCH.result_type);
    break;
  }
  case AST_TYPE_DECLARATION: {
    free(ast->data.AST_TYPE_DECLARATION.name);
    free_ast(ast->data.AST_TYPE_DECLARATION.type_expr);
    break;
  }
  case AST_MEMBER_ASSIGNMENT: {
    free_ast(ast->data.AST_MEMBER_ASSIGNMENT.object);
    free(ast->data.AST_MEMBER_ASSIGNMENT.member_name);
    free_ast(ast->data.AST_MEMBER_ASSIGNMENT.expression);
    break;
  }
  case AST_MEMBER_ACCESS: {

    free_ast(ast->data.AST_MEMBER_ACCESS.object);
    free(ast->data.AST_MEMBER_ACCESS.member_name);
    break;
  }
  case AST_INDEX_ACCESS: {

    free_ast(ast->data.AST_INDEX_ACCESS.object);
    free_ast(ast->data.AST_INDEX_ACCESS.index_expr);
    break;
  }
  case AST_IMPORT: {
    free(ast->data.AST_IMPORT.module_name);
    break;
  }
  }
  free(ast);
}

AST *ast_new(AST ast) {
  AST *ptr = malloc(sizeof(AST));
  if (ptr)
    *ptr = ast;
  ptr->type = _tvar();
  return ptr;
}

AST *get_final_expression(AST *ast) {
  int last_idx = ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.length - 1;
  return ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[last_idx];
}
