#ifndef _LANG_STATIC_ANALYSIS_H
#define _LANG_STATIC_ANALYSIS_H
#include "ast.h"
void init_analysis_ctx();
void type_checker(AST *ast);

#endif /* ifndef _LANG_STATIC_ANALYSIS_H */
