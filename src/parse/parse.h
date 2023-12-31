#ifndef _LANG_COMPILER_H
#define _LANG_COMPILER_H
#include "../ast.h"
#include "../lexer.h"
#include <stdbool.h>
AST *parse(const char *source);

typedef struct Parser {
  token previous;
  token current;
} Parser;

extern Parser parser;

void advance();
bool check(enum token_type type);
bool match(enum token_type type);

void print_current();
void print_previous();

typedef struct AST_TUPLE tpl;
void ast_tuple_push(struct AST *tuple, AST *item);
#endif
