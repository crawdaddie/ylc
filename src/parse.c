#include "parse.h"
#include "ast.h"
#include "lang_runner.h"
#include "lexer.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Parser parser;

void print_current() { print_token(parser.current); }
void print_previous() { print_token(parser.previous); }
static AST *rec_parse(AST *ast); // forward-declaration

/**
 * move to next token
 **/
void advance() {
  parser.previous = parser.current;
  parser.current = scan_token();
}

/**
 * check whether parser's current position is equal to a token type
 **/
bool check(enum token_type type) { return parser.current.type == type; }

/**
 * if next token is of a token type move forward to that token
 **/
bool match(enum token_type type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static AST *rec_parse(AST *ast) {
  if (match(TOKEN_EOF)) {
    return ast;
  }
  switch (ast->tag) {
  case AST_STATEMENT_LIST: {
    statements_push(&ast->data.AST_STATEMENT_LIST);
    return rec_parse(ast);
  }

  case AST_MAIN: {
    AST *body = malloc(sizeof(AST));
    body->tag = AST_STATEMENT_LIST;
    body = rec_parse(body);
    ast->data.AST_MAIN.body = body;
    return ast;
  }
  default:

    advance();
    return rec_parse(ast);
  }
}

AST *parse(char *source) {

  AST *ast = malloc(sizeof(AST));
  ast->tag = AST_MAIN;
  init_scanner(source);
  advance();
  rec_parse(ast);
  return ast;
}

AST *parse_file(const char *path) {
  char *source = read_file(path);
  AST *ast = parse(source);
  free(source);
  return ast;
}
