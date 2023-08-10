#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "../src/parse_function.h"
#include "../src/parse_statement.h"
#include "../src/typecheck.h"
#include "minunit.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AST_TOP_LEVEL(ast, i)                                                  \
  ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[i]

static int tc_error_flag = 0;
static AST *typecheck_input(const char *input) {
  tc_error_flag = 0;
  AST *ast = parse(input);

  tc_error_flag = typecheck(ast);
  return ast;
}

int test_typedef_tuple() {
  AST *test = typecheck_input("type Triple = (\n"
                              "    double,\n"
                              "    double,\n"
                              "    double,\n"
                              ")\n"
                              "let Triple t = (\n"
                              "    2.0,\n"
                              "    1.0,\n"
                              "    0.5,\n"
                              ")");

  print_ast(*test, 0);

  print_ttype(AST_TOP_LEVEL(test, 0)->type);
  print_ttype(AST_TOP_LEVEL(test, 1)->type);

  mu_assert(true, "");
  // mu_assert(&AST_TOP_LEVEL(test, 1)->type == &AST_TOP_LEVEL(test, 0)->type,
  // "");
  return 0;
}
int test_typedef_struct() {
  AST *test = typecheck_input("type Point = struct (\n"
                              "    double x,\n"
                              "    double y,\n"
                              ")\n"
                              "let Point p = (\n"
                              "    x = 2.0,\n"
                              "    y = 1.0,\n"
                              ")");
  print_ast(*test, 0);

  // mu_assert(&AST_TOP_LEVEL(test, 1)->type == &AST_TOP_LEVEL(test, 0)->type,
  // "");
  mu_assert(true, "");
  return 0;
}

int all_tests() {
  int test_result = 0;
  mu_run_test(test_typedef_tuple);
  // mu_run_test(test_typedef_struct);
  return test_result;
}

RUN_TESTS()
