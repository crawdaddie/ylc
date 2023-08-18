#ifndef _LANG_PARSE_FUNCTION_H
#define _LANG_PARSE_FUNCTION_H
#include "../ast.h"

AST *parse_fn_arg();
AST *parse_fn_body();
AST *parse_fn_prototype();
AST *parse_function(bool can_assign);
AST *parse_named_function(char *name);

AST *ast_fn_prototype(int length, ...);

AST *parse_extern_function(char *name);
void arg_list_push(struct AST_FN_PROTOTYPE *proto, AST *arg);
#endif /* ifndef _LANG_PARSE_FUNCTION_H */
