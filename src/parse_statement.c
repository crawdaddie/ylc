#include "parse_statement.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AST *if_statement() { return NULL; }
static AST *return_statement() { return NULL; }
static AST *assignment_statement();
static AST *let_statement() {
  advance();
  if (match(TOKEN_IDENTIFIER)) {
    if (check(TOKEN_ASSIGNMENT)) {
      // let <id> = <expr> - declaration immediately followed by assignment
      return assignment_statement();
    }
    token id_token = parser.previous;
    char *id_str = id_token.as.vstr;
    return AST_NEW(SYMBOL_DECLARATION, strdup(id_str));
  } else {
    printf("error missing identifier");
    return NULL;
  }
}
static AST *assignment_statement() {
  token id_token = parser.previous;
  char *id_str = id_token.as.vstr;

  advance();
  return AST_NEW(ASSIGNMENT, strdup(id_str), parse_expression());
}
/**
 * match on tokens that begin a statement and call corresponding statement-type
 *constructors
 *
 * TOKEN_LET | TOKEN_WHILE | TOKEN_FN | TOKEN_LEFT_BRACE | TOKEN_LP | TOKEN_IF |
 *TOKEN_RETURN
 **/
AST *parse_statement() {
  if (!match(TOKEN_NL)) {
    token token = parser.current;
    switch (token.type) {
    case TOKEN_LET: {
      return let_statement();
    }
    case TOKEN_IDENTIFIER: {
      advance();
      return parse_statement();
    }
    case TOKEN_ASSIGNMENT: {
      return assignment_statement();
    }
    // case TOKEN_WHILE: {
    // }
    // case TOKEN_FN: {
    // printf("fn decl\n");
    // return function_declaration();
    // }
    // case TOKEN_LEFT_BRACE: {
    // }
    case TOKEN_IF: {
      return if_statement();
    }
    case TOKEN_RETURN: {
      return return_statement();
    }
    default:
      return parse_expression();
    }
  }
  return NULL;
}

void statements_push(struct AST_STATEMENT_LIST *list) {
  AST *stmt = parse_statement();
  if (stmt) {
    list->length++;
    list->statements = realloc(list->statements, sizeof(AST *) * list->length);
    list->statements[list->length - 1] = stmt;
  }
}
