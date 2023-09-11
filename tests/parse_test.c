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

#define TEST_PARSE(SRC, N, ...)                                                \
  mu_assert(test_parse(parse(SRC), N, ##__VA_ARGS__), SRC)

int addition() {
  TEST_PARSE("let a = 1 + 2", 1,
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                             AST_NEW(INTEGER, 2))));
  return 0;
}

int symbol_decl() {
  TEST_PARSE("let a\n"
             "a = 1 + 2",
             2, AST_NEW(SYMBOL_DECLARATION, "a"),
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                             AST_NEW(INTEGER, 2))));
  return 0;
}

int symbol_assignment_to_unop() {
  TEST_PARSE("let a = -1", 1,
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1))));
  return 0;
}
int simple_fn_decl() {
  AST *expected_fn_proto =
      ast_fn_prototype(3, AST_NEW(SYMBOL_DECLARATION, "a", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "b", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "c", "int"));
  expected_fn_proto->data.AST_FN_PROTOTYPE.type = "int";

  TEST_PARSE("let a = fn (int a, int b, int c) int {}", 1,
             AST_NEW(FN_DECLARATION, expected_fn_proto, NULL, "a", false));
  return 0;
}
int literals() {

  /*
   * parse int
   * */
  TEST_PARSE("1", 1, AST_NEW(INTEGER, 1));

  /*
   * parse numbers / literals etc
   * */
  TEST_PARSE("1.0", 1, AST_NEW(NUMBER, 1.0));
  TEST_PARSE("true", 1, AST_NEW(BOOL, true));
  TEST_PARSE("false", 1, AST_NEW(BOOL, false));
  TEST_PARSE("\"hello\"", 1, AST_NEW(STRING, "hello", 5));
  return 0;
}
int binops() {

  /*
   * Binops
   * */
  TEST_PARSE(
      "1 + 2", 1,
      AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 2)));

  TEST_PARSE(
      "1 - 2", 1,
      AST_NEW(BINOP, TOKEN_MINUS, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 2)));

  TEST_PARSE(
      "2 * 8", 1,
      AST_NEW(BINOP, TOKEN_STAR, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 8)));

  TEST_PARSE(
      "2 / 8", 1,
      AST_NEW(BINOP, TOKEN_SLASH, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 8)));

  TEST_PARSE(
      "1 == 1", 1,
      AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 1)));

  TEST_PARSE(
      "8 % 7", 1,
      AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));

  TEST_PARSE(
      "7 < 8", 1,
      AST_NEW(BINOP, TOKEN_LT, AST_NEW(INTEGER, 7), AST_NEW(INTEGER, 8)));

  TEST_PARSE(
      "8 > 7", 1,
      AST_NEW(BINOP, TOKEN_GT, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));

  TEST_PARSE(
      "7 <= 8", 1,
      AST_NEW(BINOP, TOKEN_LTE, AST_NEW(INTEGER, 7), AST_NEW(INTEGER, 8)));

  TEST_PARSE(
      "8 >= 7", 1,
      AST_NEW(BINOP, TOKEN_GTE, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));
  TEST_PARSE("(2 % 9) * 1000", 1,
             AST_NEW(BINOP, TOKEN_STAR,
                     AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 2),
                             AST_NEW(INTEGER, 9)),
                     AST_NEW(INTEGER, 1000)));

  TEST_PARSE("val -> int", 1,
             AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(IDENTIFIER, "val"),
                     AST_NEW(IDENTIFIER, "int")));

  return 0;
}
int member_access_assignment() {

  TEST_PARSE("object.x", 1,
             AST_NEW(MEMBER_ACCESS, AST_NEW(IDENTIFIER, "object"), "x"));

  TEST_PARSE("object.x = 1", 1,
             AST_NEW(MEMBER_ASSIGNMENT, AST_NEW(IDENTIFIER, "object"), "x",
                     AST_NEW(INTEGER, 1)));

  return 0;
}
int conditionals() {

  /*
   * Test if / else
   */
  TEST_PARSE("if (x == 1) {x = 2} else {x = 3}", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                     AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 3))));

  TEST_PARSE("if (x == 1) {x = 2}", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                     NULL));

  /*
   * Test ternary
   */
  TEST_PARSE("(x == 1) ? 2 : 3", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 3)));

  TEST_PARSE("if (x == 1) {x = 2}", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     AST_NEW(ASSIGNMENT, "x", NULL, AST_NEW(INTEGER, 2)),
                     NULL));
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

  TEST_PARSE("match val -> int\n" // type hint for return of match expr
             "| 1 -> 1\n"
             "| 2 -> 2\n"
             "| _ -> 3",
             1,
             AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches,
                     AST_NEW(IDENTIFIER, "int")));

  TEST_PARSE("match val\n" // no type hint
             "| 1 -> 1\n"
             "| 2 -> 2\n"
             "| _ -> 3",
             1, AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches, NULL));
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
  TEST_PARSE("(1,2,3.5) # (tuple)", 1, AST_NEW(TUPLE, 3, tuple_members));
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
  TEST_PARSE("[1,2,3] # (array)", 1, AST_NEW(ARRAY, 3, array_members));
  return 0;
}
int type_declarations() {

  /*
   * Type declarations
   */

  /* aliasing types */
  TEST_PARSE("type NewType = int8", 1,
             AST_NEW(TYPE_DECLARATION, "NewType", AST_NEW(IDENTIFIER, "int8")));

  AST *struct_members[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", AST_NEW(IDENTIFIER, "double")),
      AST_NEW(SYMBOL_DECLARATION, "y", AST_NEW(IDENTIFIER, "double")),
  };
  TEST_PARSE("struct (double x, double y)", 1,
             AST_NEW(STRUCT, 2, struct_members));

  TEST_PARSE("type NewStructType = struct (double x, double y)", 1,
             AST_NEW(TYPE_DECLARATION, "NewStructType",
                     AST_NEW(STRUCT, 2, struct_members)));

  /* ptr types */
  TEST_PARSE(
      "type PtrType = &int8", 1,
      AST_NEW(TYPE_DECLARATION, "PtrType",
              AST_NEW(UNOP, TOKEN_AMPERSAND, AST_NEW(IDENTIFIER, "int8"))));

  /* function types */
  AST *fn_proto_params[] = {
      AST_NEW(SYMBOL_DECLARATION, "a", AST_NEW(IDENTIFIER, "int")),
      AST_NEW(SYMBOL_DECLARATION, "b", AST_NEW(IDENTIFIER, "int")),
  };
  TEST_PARSE("type FnType = fn (int a, int b) int", 1,
             AST_NEW(TYPE_DECLARATION, "FnType",
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params,
                             AST_NEW(IDENTIFIER, "int"))));

  return 0;
}
int function_prototypes() {
  AST *fn_proto_params[] = {
      AST_NEW(SYMBOL_DECLARATION, "a", AST_NEW(IDENTIFIER, "int")),
      AST_NEW(SYMBOL_DECLARATION, "b", AST_NEW(IDENTIFIER, "int")),
  };
  TEST_PARSE("type FnType = fn (int a, int b) int", 1,
             AST_NEW(TYPE_DECLARATION, "FnType",
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params,
                             AST_NEW(IDENTIFIER, "int"))));

  AST *fn_proto_params2[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", AST_NEW(IDENTIFIER, "int")),
      AST_NEW(SYMBOL_DECLARATION, "y", AST_NEW(IDENTIFIER, "int")),
  };
  TEST_PARSE("let h = fn (int x, int y) int {x + y}", 1,
             AST_NEW(FN_DECLARATION,
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2,
                             AST_NEW(IDENTIFIER, "int")),
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(IDENTIFIER, "y")),
                     "h"));

  AST *fn_proto_params3[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", NULL),
      AST_NEW(SYMBOL_DECLARATION, "y", NULL),
  };
  TEST_PARSE("let h = fn (x, y) {x + y} # without optional type "
             "annotations",
             1,
             AST_NEW(FN_DECLARATION,
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2, NULL),
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(IDENTIFIER, "y")),
                     "h"));
  return 0;
}
int type_expr() {
  AST *ast = parse("let double[512] a");

  mu_assert(
      test_parse(ast, 1,
                 AST_NEW(SYMBOL_DECLARATION, "a",
                         AST_NEW(INDEX_ACCESS, AST_NEW(IDENTIFIER, "double"),
                                 AST_NEW(INTEGER, 512)))),
      "let double[512] a");
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
  mu_run_test(member_access_assignment);
  mu_run_test(tuples);
  mu_run_test(type_declarations);
  mu_run_test(function_prototypes);
  mu_run_test(type_expr);
  return test_result;
}

RUN_TESTS()
