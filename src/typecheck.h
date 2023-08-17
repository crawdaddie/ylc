#ifndef _LANG_TYPE_CHECK_H
#define _LANG_TYPE_CHECK_H
#include "ast.h"
#include "generic_symbol_table.h"

// typedef AST *ast;
int typecheck(AST *ast, const char *module_path);
INIT_SYM_TABLE_TYPES(AST);

typedef struct {
  ttype *left; // dependent type
  ttype *right;
  // AST *ast;
} TypeEquation;

typedef struct {
  TypeEquation *equations;
  int length;
} TypeEquationsList;

INIT_SYM_TABLE_TYPES(ttype);
typedef ttype_StackFrame TypeEnv;
typedef struct {
  AST_SymbolTable *symbol_table;
  TypeEquationsList type_equations;
  const char *module_path;
  TypeEnv type_env;
} TypeCheckContext;

int typecheck_in_ctx(AST *ast, const char *module_path, TypeCheckContext *ctx);

void print_ttype(ttype type);
void print_last_entered_type(AST *ast);

bool types_equal(ttype *l, ttype *r);
#endif
