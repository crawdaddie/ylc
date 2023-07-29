#ifndef _LANG_TYPE_CHECK_H
#define _LANG_TYPE_CHECK_H
#include "ast.h"

typedef enum {
  T_INT,
  T_NUM,
  T_STR,
  T_BOOL,
  T_COMPOUND,
  T_PTR,
  T_VOID,
} TYPES;

void typecheck(AST *ast);
#endif
