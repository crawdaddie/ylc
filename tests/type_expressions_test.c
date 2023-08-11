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

  mu_assert(
      types_equal(&AST_TOP_LEVEL(test, 0)->type, &AST_TOP_LEVEL(test, 1)->type),
      "var t has type Triple");

  free_ast(test);
  return 0;
}

int test_typedef_tuple_error() {
  AST *test = typecheck_input("type Triple = (\n"
                              "    double,\n"
                              "    double,\n"
                              "    double,\n"
                              ")\n"
                              "let Triple t = (\n"
                              "    2.0,\n"
                              "    1.0,\n"
                              "    0.5,\n"
                              "    0.2,\n"
                              ")");

  mu_assert(
      types_equal(&AST_TOP_LEVEL(test, 0)->type, &AST_TOP_LEVEL(test, 1)->type),
      "var t has type Triple");
  mu_assert(tc_error_flag == 1,
            "emits error when assigning tuple with 4 members to variable of "
            "type Triple (tuple with 3 members)");
  free_ast(test);
  return 0;
}

int test_typedef_tuple_error2() {
  AST *test = typecheck_input("type Triple = (\n"
                              "    double,\n"
                              "    double,\n"
                              "    double,\n"
                              ")\n"
                              "let Triple t = (\n"
                              "    2.0,\n"
                              "    1.0,\n"
                              "    \"hello\",\n"
                              ")");

  mu_assert(
      types_equal(&AST_TOP_LEVEL(test, 0)->type, &AST_TOP_LEVEL(test, 1)->type),
      "var t has type Triple");
  mu_assert(tc_error_flag == 1,
            "emits error when assigning tuple with wrong member types");
  free_ast(test);
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

  mu_assert(
      types_equal(&AST_TOP_LEVEL(test, 0)->type, &AST_TOP_LEVEL(test, 1)->type),
      "var p has type Point");

  free_ast(test);
  return 0;
}

int test_typedef_struct_member() {
  AST *test = typecheck_input("type Point = struct (\n"
                              "    double x,\n"
                              "    double y,\n"
                              ")\n"
                              "let Point p = (\n"
                              "    x = 2.0,\n"
                              "    y = 1.0,\n"
                              ")\n"
                              "let px = p.x");

  mu_assert(AST_TOP_LEVEL(test, 2)->type.tag == T_NUM, "var px has type Num");

  free_ast(test);
  return 0;
}

int test_typedef_struct_error() {
  AST *test = typecheck_input("type Point = struct (\n"
                              "    double x,\n"
                              "    double y,\n"
                              ")\n"
                              "let Point p = (\n"
                              "    x = 2.0,\n"
                              "    y = \"some extra nonsense\",\n"
                              ")");

  mu_assert(
      types_equal(&AST_TOP_LEVEL(test, 0)->type, &AST_TOP_LEVEL(test, 1)->type),
      "var p has type Point");

  mu_assert(tc_error_flag == 1,
            "emits error when assigning struct with wrong field types");
  free_ast(test);
  return 0;
}

int all_tests() {
  int test_result = 0;
  mu_run_test(test_typedef_tuple);
  mu_run_test(test_typedef_tuple_error);
  mu_run_test(test_typedef_tuple_error2);
  mu_run_test(test_typedef_struct);
  mu_run_test(test_typedef_struct_member);
  mu_run_test(test_typedef_struct_error);
  return test_result;
}

RUN_TESTS()
