#ifndef _LANG_MODULE_H
#define _LANG_MODULE_H
#include "ast.h"
#include "generic_symbol_table.h"
typedef struct {
  AST *ast;

} LangModule;

void save_module(char *path, AST *ast);
AST *get_module(char *path);
AST *parse_module(char *path);
#endif
