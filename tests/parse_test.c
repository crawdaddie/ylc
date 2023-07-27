#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parse.h"
#include "../src/parse_function.h"
#include "../src/parse_statement.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  char *test_input = "let a = 1 + 2";
  AST *ast = parse(test_input);
  AST *expected_ast = AST_NEW(
      MAIN, ast_statement_list(
                1, AST_NEW(ASSIGNMENT, "a", NULL,
                           AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                   AST_NEW(INTEGER, 2)))));

  assert_ast_compare(ast, expected_ast, test_input);

  test_input = "let a\n"
               "a = 1 + 2";
  ast = parse(test_input);
  expected_ast = AST_NEW(
      MAIN,
      ast_statement_list(2, AST_NEW(SYMBOL_DECLARATION, "a"),
                         AST_NEW(ASSIGNMENT, "a", NULL,
                                 AST_NEW(BINOP, TOKEN_PLUS, AST_NEW(INTEGER, 1),
                                         AST_NEW(INTEGER, 2)))));

  assert_ast_compare(ast, expected_ast, test_input);

  test_input = "let a = -1";
  ast = parse(test_input);
  expected_ast = AST_NEW(
      MAIN, ast_statement_list(
                1, AST_NEW(ASSIGNMENT, "a", NULL,
                           AST_NEW(UNOP, TOKEN_MINUS, AST_NEW(INTEGER, 1)))));

  assert_ast_compare(ast, expected_ast, test_input);

  test_input = "let a = fn (int a, int b, int c) int {}\n";
  ast = parse(test_input);
  AST *expected_fn_proto =
      ast_fn_prototype(3, AST_NEW(SYMBOL_DECLARATION, "a", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "b", "int"),
                       AST_NEW(SYMBOL_DECLARATION, "c", "int"));
  expected_fn_proto->data.AST_FN_PROTOTYPE.type = "int";

  expected_ast = AST_NEW(
      MAIN, ast_statement_list(1, AST_NEW(FN_DECLARATION, expected_fn_proto,
                                          NULL, "a", false)));

  assert_ast_compare(ast, expected_ast, test_input);
}
