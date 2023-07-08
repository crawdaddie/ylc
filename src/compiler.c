#include "compiler.h"
#include "ast.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_expression.h"

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

static AST *if_statement() {}
static AST *let_statement() {
  advance();
  if (match(TOKEN_IDENTIFIER)) {
    AST *stmt = malloc(sizeof(AST));
    token id_token = parser.previous;
    char *id_str = id_token.as.vstr;

    stmt->tag = AST_SYMBOL_DECLARATION;
    stmt->data.AST_SYMBOL_DECLARATION.identifier = strdup(id_str);
    return stmt;
  } else {
    printf("error missing identifier");
    return NULL;
  }
}
static AST *assignment_statement() {
  AST *stmt = malloc(sizeof(AST));
  token id_token = parser.previous;
  char *id_str = id_token.as.vstr;

  stmt->tag = AST_ASSIGNMENT;
  stmt->data.AST_ASSIGNMENT.identifier = strdup(id_str);
  advance();
  stmt->data.AST_ASSIGNMENT.expression = parse_expression();
  return stmt;
}

/**
 * match on tokens that begin a statement and call corresponding statement-type
 *constructors
 *
 * TOKEN_LET | TOKEN_WHILE | TOKEN_FN | TOKEN_LEFT_BRACE | TOKEN_LP | TOKEN_IF |
 *TOKEN_RETURN
 **/
static AST *parse_statement() {
  while (!match(TOKEN_NL)) {
    token token = parser.current;
    switch (token.type) {
    case TOKEN_LET: {
      return let_statement();
    }
    case TOKEN_ASSIGNMENT: {
      return assignment_statement();
    }
    // case TOKEN_WHILE: {
    // }
    // case TOKEN_FN: {
    // }
    // case TOKEN_LEFT_BRACE: {
    // }
    case TOKEN_IF: {
      return if_statement();
    }
    case TOKEN_RETURN: {
    }
    default:
      advance();
    }
  }
  return NULL;
}

static void statements_push(struct AST_STATEMENT_LIST *list) {
  AST *stmt = parse_statement();
  if (stmt) {
    list->length++;
    list->statements = realloc(list->statements, sizeof(AST *) * list->length);
    list->statements[list->length - 1] = stmt;
  }
}

static AST *rec_parse(AST *ast) {
  if (match(TOKEN_EOF)) {
    return ast;
  }
  switch (ast->tag) {
  case AST_STATEMENT_LIST: {
    if (check(TOKEN_NL)) {
      advance();
      return ast;
    }
    statements_push(&ast->data.AST_STATEMENT_LIST);
    return rec_parse(ast);
  }

  case AST_MAIN: {
    AST *body = malloc(sizeof(AST));
    body->tag = AST_STATEMENT_LIST;

    body = rec_parse(body);
    printf("statements in body: %d\n", body->data.AST_STATEMENT_LIST.length);
    ast->data.AST_MAIN.body = body;
    return ast;
  }
  default:

    advance();
    return rec_parse(ast);
  }
}

void compile(char *source) {
  init_scanner(source);
  printf("[src]: '%s'\n", source);

  AST ast = {AST_MAIN};

  advance();
  rec_parse(&ast);
  printf("\n");
  print_ast(ast, 0);
  codegen(ast);
  free_ast(&ast);
}
