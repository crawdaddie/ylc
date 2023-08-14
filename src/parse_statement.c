#include "parse_statement.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "parse_function.h"
#include "paths.h"
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
static AST *return_statement() { return NULL; }
static AST *assignment_statement(char *id, char *type);
static AST *let_statement() {
  advance();
  if (match(TOKEN_IDENTIFIER)) {
    token id_token = parser.previous;
    char *id_str = strdup(id_token.as.vstr);
    char *type = NULL;

    if (match(TOKEN_IDENTIFIER)) {
      // first id was actually a type
      type = id_str;
      id_str = strdup(parser.previous.as.vstr);
    }

    if (check(TOKEN_ASSIGNMENT)) {
      // let <id> = <expr> - declaration immediately followed by assignment
      return assignment_statement(id_str, type);
    }
    return AST_NEW(SYMBOL_DECLARATION, id_str, type);
  } else {
    printf("error missing identifier");
    return NULL;
  }
}
static AST *assignment_statement(char *id, char *type) {
  advance();
  if (match(TOKEN_EXTERN)) {
    return parse_extern_function(id);
  }

  if (check(TOKEN_FN)) {
    advance();
    return parse_named_function(strdup(id));
  }

  AST *expr = AST_NEW(ASSIGNMENT, id, type, parse_expression());
  return expr;
}

static AST *type_declaration() {
  advance();
  if (!match(TOKEN_IDENTIFIER)) {
    fprintf(stderr, "Error: expected name after type keyword");
    return NULL;
  }
  char *name = parser.previous.as.vstr;
  if (!match(TOKEN_ASSIGNMENT)) {
    fprintf(stderr, "Error: expected = after type declaration name");
    return NULL;
  }
  AST *type_expression;
  if (match(TOKEN_FN)) {
    type_expression = parse_fn_prototype();
  } else {
    type_expression = parse_expression();
  }

  return AST_NEW(TYPE_DECLARATION, name, type_expression);
}
static AST *import_module() {
  advance();
  if (!match(TOKEN_STRING)) {
    fprintf(stderr, "expected module path after import keyword\n");
    return NULL;
  }
  token token = parser.previous;
  advance();
  char *module_name = strdup(token.as.vstr);

  if (has_extension(module_name, ".so")) {
    return AST_NEW(IMPORT_LIB, module_name);
  }
  return AST_NEW(IMPORT, module_name);
}
/**
 * match on tokens that begin a statement and call corresponding
 *statement-type constructors
 *
 * TOKEN_LET | TOKEN_WHILE | TOKEN_FN | TOKEN_LEFT_BRACE | TOKEN_LP | TOKEN_IF
 *| TOKEN_RETURN
 **/
AST *parse_statement() {
  char *src_offset = get_scanner_current();
  line_info linfo = get_line_info();
  AST *ast;
  if (!match(TOKEN_NL)) {
    token token = parser.current;
    switch (token.type) {
    case TOKEN_LET: {
      ast = let_statement();
      break;
    }
    // case TOKEN_ASSIGNMENT: {
    //   return assignment_statement();
    // }
    case TOKEN_RETURN: {
      ast = return_statement();
      break;
    }
    case TOKEN_TYPE: {
      ast = type_declaration();
      break;
    }

    case TOKEN_IMPORT: {
      ast = import_module();
      break;
    }
    default: {
      ast = parse_expression();
      break;
    }
    }

    ast->src_offset = src_offset;
    ast->line_info = linfo;
    return ast;
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
