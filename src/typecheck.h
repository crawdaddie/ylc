#ifndef _LANG_TYPE_CHECK_H
#define _LANG_TYPE_CHECK_H
#include "ast.h"
#include "generic_symbol_table.h"
#include "unify_types.h"

// typedef AST *ast;
INIT_SYM_TABLE_TYPES(AST);

typedef struct {
  TypeEquation *equations;
  int length;
} TypeEquationsList;

typedef struct {
  AST_SymbolTable *symbol_table;
  TypeEquationsList type_equations;
  TypeEquationsList return_type_equations;
  const char *module_path;
  TypeEnv type_env;
} TypeCheckContext;

int typecheck_in_ctx(AST *ast, const char *module_path, TypeCheckContext *ctx);

void print_ttype(ttype type);
void print_last_entered_type(AST *ast);

bool types_equal(ttype *l, ttype *r);

ttype get_fn_return_type(ttype fn_type);

ttype get_last_entered_type(AST *ast);

ttype get_last_entered_type(AST *ast);

void print_env(TypeEnv *env);

extern int _typecheck_error_flag;
#endif
