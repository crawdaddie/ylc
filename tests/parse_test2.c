
#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse/parse.h"
#include "../src/parse/parse_function.h"
#include "../src/parse/parse_statement.h"
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

/*
 * test tuple
 * */

int main() {
  test_result = 0;
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

  AST *fn_proto_params3[] = {
      AST_NEW(SYMBOL_DECLARATION, "x", NULL),
      AST_NEW(SYMBOL_DECLARATION, "y", NULL),
  };
  test_parse("let h = fn (x, y) {x + y} # without optional type annotations", 1,
             AST_NEW(FN_DECLARATION,
                     AST_NEW(FN_PROTOTYPE, 2, fn_proto_params2, NULL),
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

  test_parse("let h = fn () {1 + 2}\n"
             "h()\n",
             2,
             AST_NEW(FN_DECLARATION, AST_NEW(FN_PROTOTYPE, 0, NULL, NULL),
                     AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                             AST_NEW(INTEGER, 2)),
                     "h"));
  return test_result;
}
