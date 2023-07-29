#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "../src/parse_function.h"
#include "../src/parse_statement.h"
#include "../src/typecheck.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
/*
 *
 * wrap list of expressions in a full program, ie:
 *
 * (main:
 *      body:
 *          [...statements]
 **/
static AST *wrap_ast(int length, ...) {

  AST *statements = AST_NEW(STATEMENT_LIST, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    statements->data.AST_STATEMENT_LIST.statements = list;
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
  statements->data.AST_STATEMENT_LIST.statements = list;

  va_end(args);
  return AST_NEW(MAIN, statements);
}

static AST *typecheck_input(const char *input) {
  AST *ast = parse(input);
  typecheck(ast);
  return ast;
}

#define PRINT_TEST_RESULTS                                                     \
  print_ast(*test, 0);                                                         \
  free_ast(test);

int test_untyped_function() {
  int test_result = 0;
  AST *test = typecheck_input("let h = fn (x, y) {x + 1}");
  PRINT_TEST_RESULTS
}

int test_int_casting() {
  AST *test = typecheck_input("let h = fn (double x) {x + 1}\nh(1)");
  PRINT_TEST_RESULTS
}

int main() {
  test_result = 0;
  test_untyped_function();
  test_int_casting();
  return test_result;
}
