#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  char *test_input = "let a = 1 + 2";
  AST *ast = parse(test_input);
  AST *expected_ast = AST_NEW(
      MAIN, ast_statement_list(
                1, AST_NEW(ASSIGNMENT, "a",
                           AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                   AST_NEW(INTEGER, 2)))));

  assert_ast_compare(ast, expected_ast, test_input);

  test_input = "let a\n"
               "a = 1 + 2";
  ast = parse(test_input);
  expected_ast = AST_NEW(
      MAIN,
      ast_statement_list(2, AST_NEW(SYMBOL_DECLARATION, "a"),
                         AST_NEW(ASSIGNMENT, "a",
                                 AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                         AST_NEW(INTEGER, 2)))));

  assert_ast_compare(ast, expected_ast, test_input);

  test_input = "let a = -1";
  ast = parse(test_input);
  expected_ast = AST_NEW(
      MAIN, ast_statement_list(
                1, AST_NEW(ASSIGNMENT, "a",
                           AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1)))));

  assert_ast_compare(ast, expected_ast, test_input);
}
