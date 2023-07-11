#include "parse_statement.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AST *ast_statement_list(int length, ...) {

  AST *stmt_list = AST_NEW(STATEMENT_LIST, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    stmt_list->data.AST_STATEMENT_LIST.statements = list;
    return stmt_list;
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
  stmt_list->data.AST_STATEMENT_LIST.statements = list;

  va_end(args);

  return stmt_list;
}
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
