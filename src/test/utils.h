#ifndef _LANG_TEST_UTILS_H
#define _LANG_TEST_UTILS_H
#include "../ast.h"

AST *ast_new(AST ast);

AST *ast_statement_list(int length, ...);

int compare_ast(AST *a, AST *b);

#define assert_ast_compare(a, b, message)                                      \
  do {                                                                         \
    if (!(compare_ast(a, b) == 0)) {                                           \
      printf("❌: %s\n", message);                                             \
      printf("expected:\n");                                                   \
      print_ast(*b, 0);                                                        \
      printf("\n");                                                            \
      printf("actual:\n");                                                     \
      print_ast(*a, 0);                                                        \
    } else {                                                                   \
      printf("✅: %s\n", message);                                             \
    }                                                                          \
  } while (0)

#define AST_NEW(tag, ...)                                                      \
  ast_new((AST){tag, {.tag = (struct tag){__VA_ARGS__}}})
#endif /* ifndef _LANG_TEST_UTILS_H */
