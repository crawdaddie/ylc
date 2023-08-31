#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse/parse.h"
#include "../src/parse/parse_function.h"
#include "../src/parse/parse_statement.h"
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
static int tc_error_flag = 0;
static AST *typecheck_input(const char *input) {
  tc_error_flag = 0;
  AST *ast = parse(input);

  TypeCheckContext tcheck_ctx = {};
  AST_SymbolTable tcheck_symbol_table = {}; // init to zero
  //
  tcheck_symbol_table.current_frame_index = 0;
  tcheck_ctx.symbol_table = &tcheck_symbol_table;

  tc_error_flag = typecheck_in_ctx(ast, "_", &tcheck_ctx);
  // print_env(&tcheck_ctx.type_env);
  return ast;
}

#define CLEAN_TEST_RESULTS                                                     \
  // print_ast(*test, 0);                                                      \
  // free_ast(test);

#define AST_TOP_LEVEL(ast, i)                                                  \
  ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[i]

#define AST_FN_PARAM(i)                                                        \
  data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE.parameters[i]

int test_nothing() {
  AST *test = typecheck_input("1");
  mu_assert(AST_TOP_LEVEL(test, 0)->type.tag == T_INT, "type '1' as Int");
  free_ast(test);
  return 0;
}

int test_binop() {
  AST *test = typecheck_input("1.0 + 1");

  mu_assert(AST_TOP_LEVEL(test, 0)->type.tag == T_NUM, "type '1 + 1.0' as Num");
  // TODO: fix this in unification --- Num :: Int

  free_ast(test);
  return 0;
}

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

int test_fn_call() {
  AST *test = typecheck_input("let h = fn (x) {\n"
                              "  x + 1\n"
                              "}\n"
                              "h(2)");

  AST *fn_call = AST_TOP_LEVEL(test, 1);

  mu_assert(fn_call->type.tag == T_INT,
            "call expr has type of return type of function (Int)");
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

  AST *condition =
      AST_TOP_LEVEL(test, 0)
          ->data.AST_FN_DECLARATION.body->data.AST_IF_ELSE.condition;

  mu_assert(condition->type.tag == T_BOOL,
            "condition expr in If / Else has type Bool");

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

int test_fn_with_type_error() {

  AST *test = typecheck_input("let _h = fn (x) {\n"
                              "  if (x + 1) {\n"
                              "     x + 1\n"
                              "  }\n"
                              "}");

  mu_assert(tc_error_flag == 1, "emits error when typechecking 'if (x + 1)'");
  free_ast(test);
  return 0;
}

int test_fn_with_unop() {

  AST *test = typecheck_input("let _h = fn (x) {\n"
                              "  !x\n"
                              "}");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_VAR &&
                fn_h->type.as.T_FN.members[1].tag == T_BOOL,
            "function has type ('t -> Bool)");

  AST *x_arg = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE
                   .parameters[0];

  mu_assert(x_arg->type.tag == T_VAR, "param x has type Bool");

  free_ast(test);
  return 0;
}

int test_fn_with_match_expr() {

  AST *test = typecheck_input("let m = fn (val) {\n"
                              "  match val\n"
                              "  | 1 -> 1 \n"
                              "  | 2 -> 2\n"
                              "  | _ -> 10\n"
                              "}");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_INT &&
                fn_h->type.as.T_FN.members[1].tag == T_INT,
            "function with matcher inside has type (Int -> Int)");
  free_ast(test);
  return 0;
}

int test_simple_exprs() {
  AST *test = typecheck_input("let x\nx + 1.0");
  mu_assert(AST_TOP_LEVEL(test, 0)->type.tag == T_NUM,
            "let x has type Num (double)");
  return 0;
}

int test_int_casting() {
  AST *test = typecheck_input("let h = fn (double x) {x + 1}\nh(1)");
  return 0;
}

int test_generic_fn() {
  AST *test = typecheck_input("let m = fn (x, y, z, a, b, c) {\n"
                              "  (x, y, z, a, b, c, 2)\n"
                              "}");
  ttype fn_type = AST_TOP_LEVEL(test, 0)->type;

  mu_assert(fn_type.tag == T_FN, "generic fn has fn type");
  mu_assert(fn_type.as.T_FN.length == 7, "fn type has 7 members");
  ttype ret_tuple_type = fn_type.as.T_FN.members[6];
  mu_assert(ret_tuple_type.tag == T_TUPLE, "return has tuple type");

  int each_equal = 1;

  for (int i = 0; i < 6; i++) {
    if (strcmp(ret_tuple_type.as.T_TUPLE.members[0].as.T_VAR.name,
               fn_type.as.T_FN.members[0].as.T_VAR.name) != 0) {
      each_equal = 0;
    }
  }

  mu_assert(
      each_equal == 1,
      "return type of generic function is derived from types of arguments");
  mu_assert(ret_tuple_type.as.T_TUPLE.members[6].tag == T_INT,
            "Final member of tuple is int");
  return 0;
}

int test_partially_explicitly_typed_fn() {
  AST *test = typecheck_input("let h = fn (double x) {\n"
                              "  x + 1\n"
                              "}");

  ttype x_type = AST_TOP_LEVEL(test, 0)->AST_FN_PARAM(0)->type;
  mu_assert(x_type.tag == T_NUM,
            "type of parameter x (Num) is added to AST node for x");

  AST *binop = AST_TOP_LEVEL(test, 0)->data.AST_FN_DECLARATION.body;

  mu_assert(binop->type.tag == T_NUM,
            "binop type is num because of x(num) + 1(int)");

  ttype int_type = binop->data.AST_BINOP.right->type;
  ttype var_type = binop->data.AST_BINOP.left->type;

  mu_assert(var_type.tag == T_NUM, "type of reference to x is Num");

  AST *fn_h = AST_TOP_LEVEL(test, 0);

  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_NUM &&
                fn_h->type.as.T_FN.members[1].tag == T_NUM,
            "function has type (Num -> Num)");

  free_ast(test);
  return 0;
}

int test_fib_fn_decl() {

  AST *test = typecheck_input("let fib = fn (n) {\n"
                              "  match n\n"
                              "  | 0 -> 0\n"
                              "  | 1 -> 1\n"
                              "  | _ -> fib(n - 1) + fib(n - 2)\n"
                              "}");

  print_ast(*test, 0);
  AST *fn_h = AST_TOP_LEVEL(test, 0);
  ttype fn_type = fn_h->type;
  mu_assert(fn_type.tag == T_FN, "fib is a fn");
  mu_assert(fn_type.as.T_FN.members[0].tag == T_INT &&
                fn_type.as.T_FN.members[1].tag == T_INT,
            "fib (Int -> Int)");

  return 0;
}

int test_fib_fn_call() {

  AST *test = typecheck_input("let fib = fn (n) {\n"
                              "  match n\n"
                              "  | 0 -> 0\n"
                              "  | 1 -> 1\n"
                              "  | _ -> fib(n - 1) + fib(n - 2)\n"
                              "}\n"
                              "fib(2)");

  print_ast(*test, 0);
  AST *fn_call = AST_TOP_LEVEL(test, 1);
  ttype call_type = fn_call->type;
  mu_assert(call_type.tag == T_INT, "fib call is an Int");

  AST *ret_l = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.body->data.AST_MATCH.matches[2]
                   ->data.AST_BINOP.right->data.AST_BINOP.left;

  print_ast(*ret_l, 0);
  print_ttype(ret_l->type);
  mu_assert(ret_l->type.tag == T_INT, "recursive call has Int type");

  AST *ret_r = AST_TOP_LEVEL(test, 0)
                   ->data.AST_FN_DECLARATION.body->data.AST_MATCH.matches[2]
                   ->data.AST_BINOP.right->data.AST_BINOP.right;

  print_ast(*ret_r, 0);
  print_ttype(ret_r->type);
  mu_assert(ret_r->type.tag == T_INT, "recursive call has Int type");

  return 0;
}

int test_extern_fn() {
  AST *test = typecheck_input("let randfloat = extern fn (double max) double");

  AST *fn_h = AST_TOP_LEVEL(test, 0);
  mu_assert(fn_h->type.tag == T_FN, "function type is populated");
  mu_assert(fn_h->type.as.T_FN.length == 2,
            "function type has 1 param & 1 return val (length = 2)");

  mu_assert(fn_h->type.as.T_FN.members[0].tag == T_NUM &&
                fn_h->type.as.T_FN.members[1].tag == T_NUM,
            "function has type (Num -> Num)");

  free_ast(test);
  return 0;
}

int test_curried_fn() {

  AST *test = typecheck_input("let f = fn (a,b,c) {\n"
                              "  a + b + c\n"
                              "}\n"
                              "let g = f(1, 2)\n"
                              "let x = g(100)\n");

  AST *g = AST_TOP_LEVEL(test, 1);

  mu_assert(g->data.AST_ASSIGNMENT.expression->tag == AST_CALL,
            "g is a call not value");

  mu_assert(AST_TOP_LEVEL(test, 2)->type.tag == T_INT,
            "final call has int type");

  AST *f = AST_TOP_LEVEL(test, 0);
  mu_assert(get_fn_return_type(g->type).tag == T_INT &&
                get_fn_return_type(f->type).tag == T_INT,
            "g and f have the same return types");
  return 0;
}

#define run_test(TEST)                                                         \
  t_counter = 0;                                                               \
  mu_run_test(TEST);

int all_tests() {
  int test_result = 0;
  run_test(test_nothing);
  run_test(test_binop);
  run_test(test_simple_exprs);
  run_test(test_untyped_function);
  run_test(test_fn_call);
  run_test(test_fn_with_conditionals);
  run_test(test_fn_with_conditionals2);
  run_test(test_fn_with_type_error);
  run_test(test_fn_with_unop);
  run_test(test_fn_with_match_expr);
  run_test(test_generic_fn);
  run_test(test_fib_fn_decl);
  run_test(test_fib_fn_call);
  run_test(test_partially_explicitly_typed_fn);
  run_test(test_int_casting);
  run_test(test_extern_fn);
  run_test(test_curried_fn);
  return test_result;
}

RUN_TESTS()
