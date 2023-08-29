#ifndef _LANG_TYPECHECK_UNIFY_H
#define _LANG_TYPECHECK_UNIFY_H
#include "generic_symbol_table.h"
#include "types.h"
INIT_SYM_TABLE_TYPES(ttype);

typedef ttype_StackFrame TypeEnv;

typedef struct {
  ttype *left; // dependent type
  ttype *right;
  int index;
  // AST *ast;
} TypeEquation;

void unify(TypeEquation eq, TypeEnv *env);
void add_type_to_env(TypeEnv *env, ttype *left, ttype *right);

ttype *follow_links(TypeEnv *env, ttype *rtype);
#endif
