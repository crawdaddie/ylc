#ifndef _LANG_TYPE_CHECK_H
#define _LANG_TYPE_CHECK_H
#include "ast.h"

int typecheck(AST *ast);
int _typecheck(AST *ast, int cleanup);

void print_ttype(ttype type);
void print_last_entered_type(AST *ast);
#endif
