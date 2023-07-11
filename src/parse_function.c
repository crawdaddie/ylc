#include "parse_function.h"
#include "lexer.h"
#include "parse.h"
#include "parse_statement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  if (check(TOKEN_COMMA)) {
    advance();
  }
  return id;
}

AST *parse_fn_body() {
  match(TOKEN_LEFT_BRACE);
  AST *statements = ast_statement_list(0);
  while (!match(TOKEN_RIGHT_BRACE)) {
    statements_push((struct AST_STATEMENT_LIST *)statements);
  }
  return statements;
}

AST *parse_fn_prototype() {
  AST *proto = ast_fn_prototype(0);
  match(TOKEN_LP);
  while (!match(TOKEN_RP)) {
    arg_list_push((struct AST_FN_PROTOTYPE *)proto);
  }
  parse_fn_body();

  return proto;
}

AST *parse_function(bool can_assign) {
  AST *prototype = parse_fn_prototype();
  AST *body = parse_fn_body();
  AST *function_ast = AST_NEW(FN_DECLARATION, prototype, body);
  return function_ast;
}
