#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "../src/parse_function.h"
#include "../src/parse_statement.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void test_parse(char *input, int length, ...) {
  AST *ast = parse(input);

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
  assert_ast_compare(
      ast, AST_NEW(MAIN, expected_statements), // wrap statements in 'main' ast
      input);
  free_ast(ast);
}

int main() {
  test_result = 0;
  /*
   * symbol declaration + assignment
   * */
  test_parse("let a = 1 + 2", 1,
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                             AST_NEW(INTEGER, 2))));

  /*
   * symbol declaration + later assignment to binop
   * */
  test_parse("let a\n"
             "a = 1 + 2",
             2, AST_NEW(SYMBOL_DECLARATION, "a"),
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                             AST_NEW(INTEGER, 2))));

  /*
   * symbol assignment to unop
   * */
  test_parse("let a = -1", 1,
             AST_NEW(ASSIGNMENT, "a", NULL,
                     AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1))));

  /*
   * simple function declaration
   * */
  AST *expected_fn_proto =
      ast_fn_prototype(3, AST_NEW(SYMBOL_DECLARATION, "a", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "b", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "c", "int"));
  expected_fn_proto->data.AST_FN_PROTOTYPE.type = "int";

  test_parse("let a = fn (int a, int b, int c) int {}", 1,
             AST_NEW(FN_DECLARATION, expected_fn_proto, NULL, "a", false));

  /*
   * parse int
   * */
  test_parse("1", 1, AST_NEW(INTEGER, 1));

  /*
   * parse numbers / literals etc
   * */
  test_parse("1.0", 1, AST_NEW(NUMBER, 1.0));
  test_parse("true", 1, AST_NEW(BOOL, true));
  test_parse("false", 1, AST_NEW(BOOL, false));
  test_parse("\"hello\"", 1, AST_NEW(STRING, "hello", 5));

  /*
   * Binops
   * */
  test_parse(
      "1 + 2", 1,
      AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 2)));

  test_parse(
      "1 - 2", 1,
      AST_NEW(BINOP, TOKEN_MINUS, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 2)));

  test_parse(
      "2 * 8", 1,
      AST_NEW(BINOP, TOKEN_STAR, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 8)));

  test_parse(
      "2 / 8", 1,
      AST_NEW(BINOP, TOKEN_SLASH, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 8)));

  test_parse(
      "1 == 1", 1,
      AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 1)));

  test_parse(
      "8 % 7", 1,
      AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));

  test_parse(
      "7 < 8", 1,
      AST_NEW(BINOP, TOKEN_LT, AST_NEW(INTEGER, 7), AST_NEW(INTEGER, 8)));

  test_parse(
      "8 > 7", 1,
      AST_NEW(BINOP, TOKEN_GT, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));

  test_parse(
      "7 <= 8", 1,
      AST_NEW(BINOP, TOKEN_LTE, AST_NEW(INTEGER, 7), AST_NEW(INTEGER, 8)));

  test_parse(
      "8 >= 7", 1,
      AST_NEW(BINOP, TOKEN_GTE, AST_NEW(INTEGER, 8), AST_NEW(INTEGER, 7)));

  test_parse("(2 % 9) * 1000", 1,
             AST_NEW(BINOP, TOKEN_STAR,
                     AST_NEW(BINOP, TOKEN_MODULO, AST_NEW(INTEGER, 2),
                             AST_NEW(INTEGER, 9)),
                     AST_NEW(INTEGER, 1000)));

  test_parse("val -> int", 1,
             AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(IDENTIFIER, "val"),
                     AST_NEW(IDENTIFIER, "int")));

  test_parse("object.x", 1,
             AST_NEW(MEMBER_ACCESS, AST_NEW(IDENTIFIER, "object"), "x"));

  test_parse("object.x = 1", 1,
             AST_NEW(MEMBER_ASSIGNMENT, AST_NEW(IDENTIFIER, "object"), "x",
                     AST_NEW(INTEGER, 1)));

  /*
   * Test if / else
   */
  test_parse("if (x == 1) {x = 2} else {x = 3}", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     ast_statement_list(1, AST_NEW(ASSIGNMENT, "x", NULL,
                                                   AST_NEW(INTEGER, 2))),
                     ast_statement_list(1, AST_NEW(ASSIGNMENT, "x", NULL,
                                                   AST_NEW(INTEGER, 3)))));

  test_parse("if (x == 1) {x = 2}", 1,
             AST_NEW(IF_ELSE,
                     AST_NEW(BINOP, TOKEN_EQUALITY, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(INTEGER, 1)),
                     ast_statement_list(1, AST_NEW(ASSIGNMENT, "x", NULL,
                                                   AST_NEW(INTEGER, 2))),
                     NULL));

  /*
   * Test pattern matching expressions
   * */
  AST *matches[] = {
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 1)),
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(INTEGER, 2), AST_NEW(INTEGER, 2)),
      AST_NEW(BINOP, TOKEN_PIPE, AST_NEW(IDENTIFIER, "_"), AST_NEW(INTEGER, 3)),
  };

  test_parse("match val -> int\n" // type hint for return of match expr
             "| 1 -> 1\n"
             "| 2 -> 2\n"
             "| _ -> 3",
             1,
             AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches,
                     AST_NEW(IDENTIFIER, "int")));

  test_parse("match val\n" // no type hint
             "| 1 -> 1\n"
             "| 2 -> 2\n"
             "| _ -> 3",
             1, AST_NEW(MATCH, AST_NEW(IDENTIFIER, "val"), 3, matches, NULL));

  /*
   * test tuple
   * */

  AST *tuple_members[] = {
      AST_NEW(INTEGER, 1),
      AST_NEW(INTEGER, 2),
      AST_NEW(NUMBER, 3.5),
  };
  test_parse("(1,2,3.5) # (tuple)", 1, AST_NEW(TUPLE, 3, tuple_members));
  /*
   * test function call expression
   */
  AST *params[] = {AST_NEW(INTEGER, 1), AST_NEW(INTEGER, 2),
                   AST_NEW(NUMBER, 3.0), AST_NEW(IDENTIFIER, "x")};

  test_parse(
      "f(1, 2, 3.0, x) # (function call)", 1,
      AST_NEW(CALL, AST_NEW(IDENTIFIER, "f"), AST_NEW(TUPLE, 4, params)));

  /*
   * Type declarations
   */

  /* aliasing types */
  test_parse("type NewType = int8", 1,
             AST_NEW(TYPE_DECLARATION, "NewType", AST_NEW(IDENTIFIER, "int8")));

  AST *struct_members[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", "double"),
      AST_NEW(SYMBOL_DECLARATION, "y", "double"),
  };
  test_parse("struct (double x, double y)", 1,
             AST_NEW(STRUCT, 2, struct_members));

  test_parse("type NewStructType = struct (double x, double y)", 1,
             AST_NEW(TYPE_DECLARATION, "NewStructType",
                     AST_NEW(STRUCT, 2, struct_members)));

  /* ptr types */
  test_parse(
      "type PtrType = &int8", 1,
      AST_NEW(TYPE_DECLARATION, "PtrType",
              AST_NEW(UNOP, TOKEN_AMPERSAND, AST_NEW(IDENTIFIER, "int8"))));

  /* function types */
  AST *fn_proto_params[] = {
      AST_NEW(SYMBOL_DECLARATION, "a", "int"),
      AST_NEW(SYMBOL_DECLARATION, "b", "int"),
  };
  test_parse("type FnType = fn (int a, int b) int", 1,
             AST_NEW(TYPE_DECLARATION, "FnType",
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params, "int")));

  AST *fn_proto_params2[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", "int"),
      AST_NEW(SYMBOL_DECLARATION, "y", "int"),
  };
  test_parse("let h = fn (int x, int y) int {x + y}", 1,
             AST_NEW(FN_DECLARATION,
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2, "int"),
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(IDENTIFIER, "x"),
                             AST_NEW(IDENTIFIER, "y")),
                     "h"));
  /*
    AST *fn_proto_params3[] = {
        AST_NEW(SYMBOL_DECLARATION, "x_", "int"),
        AST_NEW(SYMBOL_DECLARATION, "y_", "int"),
    };
    test_parse(
        "let h = fn (int x_, int y_) int\n"
        "{let j = 1\nx_ + y_ + j}",
        1,
        AST_NEW(FN_DECLARATION, AST_NEW(FN_PROTOTYPE, 2, fn_proto_params3,
    "int"), ast_statement_list( 2, AST_NEW(ASSIGNMENT, "j", NULL,
    AST_NEW(INTEGER, 1)), AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(BINOP, TOKEN_PLUS,
    AST_NEW(IDENTIFIER, "x_"), AST_NEW(IDENTIFIER, "y_")), AST_NEW(IDENTIFIER,
    "j"))), "h"));
      */
  return test_result;
}
