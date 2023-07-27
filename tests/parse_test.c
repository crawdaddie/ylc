#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "../src/parse_function.h"
#include "../src/parse_statement.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static void test_parse(char *input, AST *expected) {
  AST *ast = parse(input);
  assert_ast_compare(ast,
                     AST_NEW(MAIN, expected), // wrap statements in 'main' ast
                     input);
}

int main() {
  /*
   * symbol declaration + assignment
   * */
  test_parse("let a = 1 + 2",
             ast_statement_list(
                 1, AST_NEW(ASSIGNMENT, "a", NULL,
                            AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                    AST_NEW(INTEGER, 2)))));

  /*
   * symbol declaration + later assignment
   * */
  test_parse(
      "let a\n"
      "a = 1 + 2",
      ast_statement_list(2, AST_NEW(SYMBOL_DECLARATION, "a"),
                         AST_NEW(ASSIGNMENT, "a", NULL,
                                 AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                         AST_NEW(INTEGER, 2)))));

  /*
   * symbol assignment to unop
   * */
  test_parse("let a = -1",
             ast_statement_list(
                 1, AST_NEW(ASSIGNMENT, "a", NULL,
                            AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1)))));

  /*
   * simple function declaration
   * */
  AST *expected_fn_proto =
      ast_fn_prototype(3, AST_NEW(SYMBOL_DECLARATION, "a", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "b", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "c", "int"));
  expected_fn_proto->data.AST_FN_PROTOTYPE.type = "int";

  test_parse("let a = fn (int a, int b, int c) int {}\n",
             ast_statement_list(1, AST_NEW(FN_DECLARATION, expected_fn_proto,
                                           NULL, "a", false)));
}
