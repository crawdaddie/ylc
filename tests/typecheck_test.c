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

#define CLEAN_TEST_RESULTS                                                     \
  // print_ast(*test, 0);                                                         \
  // free_ast(test);

#define AST_TOP_LEVEL(ast, i)                                                  \
  ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[i]

#define AST_FN_PARAM(i)                                                        \
  data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE.parameters[i]

#define AST_FN_BODY(i)
int test_untyped_function() {
  AST *test = typecheck_input("let h = fn (x) {\n"
                              "  x + 1\n"
                              "}");

  ttype x_type = AST_TOP_LEVEL(test, 0)->AST_FN_PARAM(0)->type;
  mu_assert(x_type.tag == T_INT,
            "type of parameter x is added to AST node for x");

  AST *binop = AST_TOP_LEVEL(test, 0)->data.AST_FN_DECLARATION.body;

  ttype int_type = binop->data.AST_BINOP.right->type;
  ttype var_type = binop->data.AST_BINOP.left->type;

  mu_assert(int_type.tag == T_INT, "type of AST node for 1 is Int");
  mu_assert(var_type.tag == T_INT, "type of AST node for 1 is Int");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_INT &&
                fn_h->type.as.T_FN.members[1].tag == T_INT,
            "function has type (Int -> Int)");

  free_ast(test);
  return 0;
}

int test_fn_with_conditionals() {

  AST *test = typecheck_input("let _h = fn (x) {\n"
                              "  if (x == 1) {\n"
                              "     x + 1\n"
                              "  } else {\n"
                              "     x + 2\n"
                              "  }\n"
                              "}");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_INT &&
                fn_h->type.as.T_FN.members[1].tag == T_INT,
            "function has type (Int -> Int)");

  AST *x_arg = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE
                   .parameters[0];

  mu_assert(x_arg->type.tag == T_INT, "param x has type Int");

  AST *x_ref = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.body->data.AST_IF_ELSE.condition
                   ->data.AST_BINOP.left;

  mu_assert(x_ref->type.tag == T_INT, "reference to x has type Int");
  free_ast(test);
  return 0;
}
int test_fn_with_conditionals2() {

  AST *test = typecheck_input("let _h = fn (x) {\n"
                              "  if (x == 1) {\n"
                              "     x + 1\n"
                              "  }\n"
                              "}");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_INT &&
                fn_h->type.as.T_FN.members[1].tag == T_INT,
            "function has type (Int -> Int)");

  AST *x_arg = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE
                   .parameters[0];

  mu_assert(x_arg->type.tag == T_INT, "param x has type Int");

  AST *x_ref = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.body->data.AST_IF_ELSE.condition
                   ->data.AST_BINOP.left;

  mu_assert(x_ref->type.tag == T_INT, "reference to x has type Int");
  free_ast(test);
  return 0;
}

int test_simple_expr() {
  AST *test = typecheck_input("x + 1");
  mu_assert(true, "");
  return 0;
}

int test_int_casting() {
  AST *test = typecheck_input("let h = fn (double x) {x + 1}\nh(1)");
  return 0;
}

int all_tests() {
  int test_result = 0;
  mu_run_test(test_untyped_function);
  // mu_run_test(test_simple_expr);
  mu_run_test(test_fn_with_conditionals);
  mu_run_test(test_fn_with_conditionals2);
  // mu_run_test(test_int_casting);
  return test_result;
}

RUN_TESTS()
