#include "parse_function.h"
#include "lexer.h"
#include "parse.h"
#include "parse_statement.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AST *ast_fn_prototype(int length, ...) {

  AST *proto = AST_NEW(FN_PROTOTYPE, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    proto->data.AST_FN_PROTOTYPE.parameters = list;
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
  proto->data.AST_FN_PROTOTYPE.parameters = list;

  va_end(args);

  return proto;
}
void arg_list_push(struct AST_FN_PROTOTYPE *proto) {
  AST *arg = parse_fn_arg();
  if (arg) {
    proto->length++;
    proto->parameters =
        realloc(proto->parameters, sizeof(AST *) * proto->length);
    proto->parameters[proto->length - 1] = arg;
  }
}
AST *parse_fn_arg() {
  if (!match(TOKEN_IDENTIFIER)) {
    fprintf(stderr, "Expected param type\n");

    return NULL;
  }
  char *type_str = strdup(parser.previous.as.vstr);
  if (!match(TOKEN_IDENTIFIER)) {
    fprintf(stderr, "Expected param name\n");
  }

  char *id_str = strdup(parser.previous.as.vstr);
  AST *param = AST_NEW(SYMBOL_DECLARATION, id_str, type_str);
  return param;
}

AST *parse_fn_body() {
  if (!match(TOKEN_LEFT_BRACE)) {
    return NULL;
  }

  AST *statements = ast_statement_list(0);

  while (!match(TOKEN_RIGHT_BRACE)) {
    if (check(TOKEN_EOF)) {
      return NULL;
    }
    if (check(TOKEN_NL)) {
      advance();
      continue;
    }
    statements_push(&statements->data.AST_STATEMENT_LIST);
  }
  advance();

  if (statements->data.AST_STATEMENT_LIST.length == 0) {
    free_ast(statements);
    return NULL;
  }

  return statements;
}

AST *parse_fn_prototype() {
  AST *proto = ast_fn_prototype(0);
  if (!match(TOKEN_LP)) {
    fprintf(stderr, "Error: expected ( after fn keyword\n");
    return NULL;
  };

  while (!match(TOKEN_RP)) {
    arg_list_push(&proto->data.AST_FN_PROTOTYPE);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }

  if (!match(TOKEN_IDENTIFIER)) {
    fprintf(stderr, "Error: expected return type for function\n");
    return NULL;
  }
  proto->data.AST_FN_PROTOTYPE.type = strdup(parser.previous.as.vstr);
  return proto;
}

AST *parse_function(bool can_assign) {
  AST *prototype = parse_fn_prototype();
  AST *body = parse_fn_body();
  AST *function_ast = AST_NEW(FN_DECLARATION, prototype, body, NULL);
  return function_ast;
}

AST *parse_named_function(char *name) {
  AST *prototype = parse_fn_prototype();
  AST *body = parse_fn_body();
  AST *function_ast = AST_NEW(FN_DECLARATION, prototype, body, name);
  return function_ast;
}
