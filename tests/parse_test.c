#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse/parse.h"
#include "../src/parse/parse_function.h"
#include "../src/parse/parse_statement.h"
#include "minunit.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int test_parse(AST *ast, int length, ...) {

  AST *expected_statements = AST_NEW(STATEMENT_LIST, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    expected_statements->data.AST_STATEMENT_LIST.statements = list;
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
  expected_statements->data.AST_STATEMENT_LIST.statements = list;

  va_end(args);

  AST *expected =
      AST_NEW(MAIN, expected_statements); // wrap statements in 'main' ast
  //
  int res = compare_ast(ast, expected) == 0;

  if (!res) {
    printf("expected:\n");
    print_ast(*expected, 0);
    printf("\n");
    printf("actual:\n");
    print_ast(*ast, 0);
  }
  free_ast(ast);
  // free_ast(expected);
  return res;
}

int addition() {
  char *src = "let a = 1 + 2";
  mu_assert(test_parse(parse(src), 1,
                       AST_NEW(ASSIGNMENT, "a", NULL,
                               AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                       AST_NEW(INTEGER, 2)))),
            "let a = 1 + 2");
  return 0;
}

int symbol_decl() {
  mu_assert(test_parse(parse("let a\n"
                             "a = 1 + 2"),
                       2, AST_NEW(SYMBOL_DECLARATION, "a"),
                       AST_NEW(ASSIGNMENT, "a", NULL,
                               AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                       AST_NEW(INTEGER, 2)))),
            "let a\n"
            "a = 1 + 2");
  return 0;
}

int symbol_assignment_to_unop() {
  mu_assert(
      test_parse(parse("let a = -1"), 1,
                 AST_NEW(ASSIGNMENT, "a", NULL,
                         AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1)))),
      "let a = -1");
  return 0;
}
int simple_fn_decl() {
  AST *expected_fn_proto =
      ast_fn_prototype(3, AST_NEW(SYMBOL_DECLARATION, "a", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "b", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "c", "int"));
  expected_fn_proto->data.AST_FN_PROTOTYPE.type = "int";

  mu_assert(
      test_parse(parse("let a = fn (int a, int b, int c) int {}"), 1,
                 AST_NEW(FN_DECLARATION, expected_fn_proto, NULL, "a", false)),
      "simple function declaration");
  return 0;
}
int literals() {

  /*
   * parse int
   * */
  mu_assert(test_parse(parse("1"), 1, AST_NEW(INTEGER, 1)), "1 - Int");

  /*
   * parse numbers / literals etc
   * */
  mu_assert(test_parse(parse("1.0"), 1, AST_NEW(NUMBER, 1.0)), "1.0 - Num");
  mu_assert(test_parse(parse("true"), 1, AST_NEW(BOOL, true)), "true - True");
  mu_assert(test_parse(parse("false"), 1, AST_NEW(BOOL, false)),
            "false - False");
  mu_assert(test_parse(parse("\"hello\""), 1, AST_NEW(STRING, "hello", 5)),
            "hello string");
  return 0;
}
int binops() {

  /*
   * Binops
   * */
  mu_assert(test_parse(parse("1 + 2"), 1,
                       AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                               AST_NEW(INTEGER, 2))),
            "");

  mu_assert(test_parse(parse("1 - 2"), 1,
                       AST_NEW(BINOP, TOKEN_MINUS, AST_NEW(INTEGER, 1),
                               AST_NEW(INTEGER, 2))),
            "");

  mu_assert(test_parse(parse("2 * 8"), 1,
                       AST_NEW(BINOP, TOKEN_STAR, AST_NEW(INTEGER, 2),
                               AST_NEW(INTEGER, 8))),
            "");

  mu_assert(test_parse(parse("2 / 8"), 1,
                       AST_NEW(BINOP, TOKEN_SLASH, AST_NEW(INTEGER, 2),
                               AST_NEW(INTEGER, 8))),
            "");

  mu_assert(test_parse(parse("1 == 1"), 1,
                       AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(INTEGER, 1),
                               AST_NEW(INTEGER, 1))),
            "");

  mu_assert(test_parse(parse("8 % 7"), 1,
                       AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 8),
                               AST_NEW(INTEGER, 7))),
            "");

  mu_assert(test_parse(parse("7 < 8"), 1,
                       AST_NEW(BINOP, TOKEN_LT, AST_NEW(INTEGER, 7),
                               AST_NEW(INTEGER, 8))),
            "");

  mu_assert(test_parse(parse("8 > 7"), 1,
                       AST_NEW(BINOP, TOKEN_GT, AST_NEW(INTEGER, 8),
                               AST_NEW(INTEGER, 7))),
            "");

  mu_assert(test_parse(parse("7 <= 8"), 1,
                       AST_NEW(BINOP, TOKEN_LTE, AST_NEW(INTEGER, 7),
                               AST_NEW(INTEGER, 8))),
            "");

  mu_assert(test_parse(parse("8 >= 7"), 1,
                       AST_NEW(BINOP, TOKEN_GTE, AST_NEW(INTEGER, 8),
                               AST_NEW(INTEGER, 7))),
            "");

  mu_assert(test_parse(parse("(2 % 9) * 1000"), 1,
                       AST_NEW(BINOP, TOKEN_STAR,
                               AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 2),
                                       AST_NEW(INTEGER, 9)),
                               AST_NEW(INTEGER, 1000))),
            "");

  mu_assert(test_parse(parse("val -> int"), 1,
                       AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(IDENTIFIER, "val"),
                               AST_NEW(IDENTIFIER, "int"))),
            "");

  mu_assert(
      test_parse(parse("object.x"), 1,
                 AST_NEW(MEMBER_ACCESS, AST_NEW(IDENTIFIER, "object"), "x")),
      "");

  mu_assert(test_parse(parse("object.x = 1"), 1,
                       AST_NEW(MEMBER_ASSIGNMENT, AST_NEW(IDENTIFIER, "object"),
                               "x", AST_NEW(INTEGER, 1))),
            "");

  return 0;
}
int conditionals() {

  /*
   * Test if / else
   */
  mu_assert(
      test_parse(parse("if (x == 1) {x = 2} else {x = 3}"), 1,
                 AST_NEW(IF_ELSE,
                         AST_NEW(BINOP, TOKEN_EQUALITY,
                                 AST_NEW(IDENTIFIER, "x"), AST_NEW(INTEGER, 1)),
                         AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                         AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 3)))),
      "");

  mu_assert(
      test_parse(parse("if (x == 1) {x = 2}"), 1,
                 AST_NEW(IF_ELSE,
                         AST_NEW(BINOP, TOKEN_EQUALITY,
                                 AST_NEW(IDENTIFIER, "x"), AST_NEW(INTEGER, 1)),
                         AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                         NULL)),
      "");

  /*
   * Test ternary
   */
  mu_assert(
      test_parse(parse("(x == 1) ? 2 : 3"), 1,
                 AST_NEW(IF_ELSE,
                         AST_NEW(BINOP, TOKEN_EQUALITY,
                                 AST_NEW(IDENTIFIER, "x"), AST_NEW(INTEGER, 1)),
                         AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 3))),
      "");

  mu_assert(
      test_parse(parse("if (x == 1) {x = 2}"), 1,
                 AST_NEW(IF_ELSE,
                         AST_NEW(BINOP, TOKEN_EQUALITY,
                                 AST_NEW(IDENTIFIER, "x"), AST_NEW(INTEGER, 1)),
                         AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                         NULL)),
      "");
  return 0;
}

int pattern_matching() {

  /*
   * Test pattern matching expressions
   * */
  AST *matches[] = {
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 1)),
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 2)),
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(IDENTIFIER, "_"), AST_NEW(INTEGER, 3)),
  };

  mu_assert(test_parse(
                parse("match val -> int\n" // type hint for return of match expr
                      "| 1 -> 1\n"
                      "| 2 -> 2\n"
                      "| _ -> 3"),
                1,
                AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches,
                        AST_NEW(IDENTIFIER, "int"))),
            "");

  mu_assert(
      test_parse(parse("match val\n" // no type hint
                       "| 1 -> 1\n"
                       "| 2 -> 2\n"
                       "| _ -> 3"),
                 1,
                 AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches, NULL)),
      "");
  return 0;
}
int tuples() {

  /*
   * test tuple
   * */

  AST *tuple_members[] = {
      AST_NEW(INTEGER, 1),
      AST_NEW(INTEGER, 2),
      AST_NEW(NUMBER, 3.5),
  };
  mu_assert(test_parse(parse("(1,2,3.5) # (tuple)"), 1,
                       AST_NEW(TUPLE, 3, tuple_members)),
            "");
  return 0;
}
int arrays() {
  /*
   * test array
   * */

  AST *array_members[] = {
      AST_NEW(INTEGER, 1),
      AST_NEW(INTEGER, 2),
      AST_NEW(INTEGER, 3),
  };
  mu_assert(test_parse(parse("[1,2,3] # (array)"), 1,
                       AST_NEW(ARRAY, 3, array_members)),
            "");
  return 0;
}
int type_declarations() {

  /*
   * Type declarations
   */

  /* aliasing types */
  mu_assert(test_parse(parse("type NewType = int8"), 1,
                       AST_NEW(TYPE_DECLARATION, "NewType",
                               AST_NEW(IDENTIFIER, "int8"))),
            "");

  AST *struct_members[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", "double"),
      AST_NEW(SYMBOL_DECLARATION, "y", "double"),
  };
  mu_assert(test_parse(parse("struct (double x, double y)"), 1,
                       AST_NEW(STRUCT, 2, struct_members)),
            "");

  mu_assert(
      test_parse(parse("type NewStructType = struct (double x, double y)"), 1,
                 AST_NEW(TYPE_DECLARATION, "NewStructType",
                         AST_NEW(STRUCT, 2, struct_members))),
      "");

  /* ptr types */
  mu_assert(test_parse(parse("type PtrType = &int8"), 1,
                       AST_NEW(TYPE_DECLARATION, "PtrType",
                               AST_NEW(UNOP, TOKEN_AMPERSAND,
                                       AST_NEW(IDENTIFIER, "int8")))),
            "");

  /* function types */
  AST *fn_proto_params[] = {
      AST_NEW(SYMBOL_DECLARATION, "a", "int"),
      AST_NEW(SYMBOL_DECLARATION, "b", "int"),
  };
  mu_assert(
      test_parse(parse("type FnType = fn (int a, int b) int"), 1,
                 AST_NEW(TYPE_DECLARATION, "FnType",
                         AST_NEW(FN_PROTOTYPE, 2, fn_proto_params, "int"))),
      "");

  return 0;
}
int function_prototypes() {
  AST *fn_proto_params[] = {
      AST_NEW(SYMBOL_DECLARATION, "a", "int"),
      AST_NEW(SYMBOL_DECLARATION, "b", "int"),
  };
  mu_assert(
      test_parse(parse("type FnType = fn (int a, int b) int"), 1,
                 AST_NEW(TYPE_DECLARATION, "FnType",
                         AST_NEW(FN_PROTOTYPE, 2, fn_proto_params, "int"))),
      "");

  AST *fn_proto_params2[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", "int"),
      AST_NEW(SYMBOL_DECLARATION, "y", "int"),
  };
  mu_assert(
      test_parse(parse("let h = fn (int x, int y) int {x + y}"), 1,
                 AST_NEW(FN_DECLARATION,
                         AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2, "int"),
                         AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(IDENTIFIER, "x"),
                                 AST_NEW(IDENTIFIER, "y")),
                         "h")),
      "");

  AST *fn_proto_params3[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", NULL),
      AST_NEW(SYMBOL_DECLARATION, "y", NULL),
  };
  mu_assert(
      test_parse(
          parse(
              "let h = fn (x, y) {x + y} # without optional type annotations"),
          1,
          AST_NEW(FN_DECLARATION,
                  AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2, NULL),
                  AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(IDENTIFIER, "x"),
                          AST_NEW(IDENTIFIER, "y")),
                  "h")),
      "");
  return 0;
}

int all_tests() {
  int test_result = 0;
  mu_run_test(addition);
  mu_run_test(symbol_decl);
  mu_run_test(symbol_assignment_to_unop);
  mu_run_test(simple_fn_decl);
  mu_run_test(literals);
  mu_run_test(binops);
  mu_run_test(tuples);
  mu_run_test(type_declarations);
  mu_run_test(function_prototypes);
  return test_result;
}

RUN_TESTS()
