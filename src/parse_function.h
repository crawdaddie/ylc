#ifndef _LANG_PARSE_FUNCTION_H
#define _LANG_PARSE_FUNCTION_H
#include "ast.h"

AST *parse_fn_arg();
AST *parse_fn_body();
AST *parse_fn_prototype();
AST *parse_function(bool can_assign);
#endif /* ifndef _LANG_PARSE_FUNCTION_H */
