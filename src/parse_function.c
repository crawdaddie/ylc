#include "parse_function.h"
#include "lexer.h"
#include "parse.h"
#include "parse_statement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

AST *ast_fn_prototype(int length, ...) {

  AST *proto = AST_NEW(FN_PROTOTYPE, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    proto->data.AST_FN_PROTOTYPE.identifiers = list;
    return proto;

  }
  // Define a va_list to hold the variable arguments
  va_list args;

  // Initialize the va_list with the variable arguments
  va_start(args, length);

  AST **list = malloc(sizeof(AST *) * length);
  for (int i = 0; i < length; i++) {
    AST *arg = va_arg(args, AST *);
    list[i] = arg;
  }
  proto->data.AST_FN_PROTOTYPE.identifiers = list;

  va_end(args);

  return proto;
}
void arg_list_push(struct AST_FN_PROTOTYPE *proto) {
  AST *arg = parse_fn_arg();
  if (arg) {
    proto->length++;
    proto->identifiers =
        realloc(proto->identifiers, sizeof(AST *) * proto->length);
    proto->identifiers[proto->length - 1] = arg;
  }
}
AST *parse_fn_arg() {
  AST *id = AST_NEW(IDENTIFIER, strdup(parser.current.as.vstr));
  advance();
  return id;
}

AST *parse_fn_body() {
  if (!match(TOKEN_LEFT_BRACE)) {
    printf("expected Function body ({) - found ");
    print_current();
    printf("\n");
    return NULL;
  }

  AST *statements = ast_statement_list(0);

  while (!match(TOKEN_RIGHT_BRACE)) {
    if (check(TOKEN_EOF)) {
      return NULL;
    }
    if (check(TOKEN_NL)) {advance(); continue;}
    statements_push(&statements->data.AST_STATEMENT_LIST);
  }
  advance();

  return statements;
}

AST *parse_fn_prototype() {
  AST *proto = ast_fn_prototype(0);
  if (!match(TOKEN_LP)) {
    return NULL;
  };

  while (!match(TOKEN_RP)) {
    print_current();
    arg_list_push(&proto->data.AST_FN_PROTOTYPE);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }

  return proto;
}

AST *parse_function(bool can_assign) {
  AST *prototype = parse_fn_prototype();
  AST *body = parse_fn_body();
  AST *function_ast = AST_NEW(FN_DECLARATION, prototype, body);
  return function_ast;
}
