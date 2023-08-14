#ifndef _LANG_TYPE_CHECK_H
#define _LANG_TYPE_CHECK_H
#include "ast.h"

int typecheck(AST *ast, const char *module_path);

void print_ttype(ttype type);
void print_last_entered_type(AST *ast);

bool types_equal(ttype *l, ttype *r);
#endif
