#ifndef _LANG_PARSE_STATEMENT_H
#define _LANG_PARSE_STATEMENT_H
#include "../ast.h"

AST *parse_statement();
void statements_push(struct AST_STATEMENT_LIST *list);

AST *ast_statement_list(int length, ...);
#endif /* ifndef _LANG_PARSE_STATEMENT_H */
