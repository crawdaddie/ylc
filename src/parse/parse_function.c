#include "parse_function.h"
#include "../lexer.h"
#include "parse.h"
#include "parse_expression.h"
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
void arg_list_push(struct AST_FN_PROTOTYPE *proto, AST *arg) {
  if (arg) {
    proto->length++;
    proto->parameters =
        realloc(proto->parameters, sizeof(AST *) * proto->length);
    proto->parameters[proto->length - 1] = arg;
  }
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
  if (statements->data.AST_STATEMENT_LIST.length == 1) {
    return statements->data.AST_STATEMENT_LIST.statements[0];
  }

  return statements;
}

AST *parse_fn_arg() {
  // char *identifiers[2];
  // int i;
  // for (i = 0; match(TOKEN_IDENTIFIER); i++) {
  //   identifiers[i] = strdup(parser.previous.as.vstr);
  // }
  // if (i == 0) {
  //   return NULL;
  // }
  // if (i == 1) {
  //   return AST_NEW(SYMBOL_DECLARATION, identifiers[0], NULL);
  // }
  //
  // return AST_NEW(SYMBOL_DECLARATION, identifiers[1], identifiers[0]);
  return let_statement();
}

AST *parse_fn_prototype() {
  AST *proto = ast_fn_prototype(0);
  if (!match(TOKEN_LP)) {
    fprintf(stderr, "Error: expected ( after fn keyword\n");
    return NULL;
  };

  AST *arg;
  while (!match(TOKEN_RP)) {
    if (match(TOKEN_TRIPLE_DOT)) {
      if (!check(TOKEN_RP)) {
        fprintf(stderr, "Error: ... var args must be last parameter");
        advance();
        return NULL;
      }
      arg = malloc(sizeof(AST));
      arg->tag = AST_VAR_ARG;
      arg->type = _tvar();
    } else {
      arg = parse_fn_arg();
    }

    arg_list_push(&proto->data.AST_FN_PROTOTYPE, arg);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }

  if (match(TOKEN_IDENTIFIER)) {
    proto->data.AST_FN_PROTOTYPE.type =
        AST_NEW(IDENTIFIER, strdup(parser.previous.as.vstr));
  } else if (check(TOKEN_AMPERSAND)) {
    AST *type_expr = parse_expression();
    proto->data.AST_FN_PROTOTYPE.type = type_expr;
  } else {
    proto->data.AST_FN_PROTOTYPE.type = NULL;
  }
  return proto;
}

AST *parse_function(bool can_assign) {
  AST *prototype = parse_fn_prototype();
  if (!prototype) {
    return NULL;
  }
  if (!match(TOKEN_LEFT_BRACE)) {
    fprintf(stderr, "Error: expected { after function prototype\n");
    return NULL;
  }
  AST *body = parse_scoped_block(true);
  AST *function_ast = AST_NEW(FN_DECLARATION, prototype, body, NULL);
  return function_ast;
}

AST *parse_extern_function(char *name) {
  advance();
  AST *prototype = parse_fn_prototype();
  AST *fn_decl = AST_NEW(FN_DECLARATION, prototype);
  fn_decl->data.AST_FN_DECLARATION.name = strdup(name);
  fn_decl->data.AST_FN_DECLARATION.is_extern = true;
  return fn_decl;
}
AST *parse_named_function(char *name) {
  AST *function_ast = parse_function(true);
  function_ast->data.AST_FN_DECLARATION.name = name;
  return function_ast;
}
