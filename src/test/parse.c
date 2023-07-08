#include "../parse.h"
#include "../ast.h"
#include "../lexer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  char *test_input = "let a = 1 + 2";
  AST *ast = parse(test_input);
  AST *expected_ast = AST_NEW(
      AST_MAIN, ast_statement_list(2, AST_NEW(AST_SYMBOL_DECLARATION, "a"),
                                   AST_NEW(AST_ASSIGNMENT, "a",
                                           AST_NEW(AST_BINOP, TOKEN_PLUS,
                                                   AST_NEW(AST_INTEGER, 1),
                                                   AST_NEW(AST_INTEGER, 2)))));

  assert_ast_compare(ast, expected_ast, test_input);
}
