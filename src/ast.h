#ifndef _LANG_AST_H
#define _LANG_AST_H
#include "lexer.h"
#include "types.h"
#include <stdbool.h>
typedef struct AST AST;

struct AST {
  enum {
    AST_INTEGER,
    AST_NUMBER,
    AST_BOOL,
    AST_STRING,
    AST_ADD,
    AST_SUBTRACT,
    AST_MUL,
    AST_DIV,
    AST_MAIN,
    AST_UNOP,
    AST_BINOP,
    AST_EXPRESSION,
    AST_STATEMENT,
    AST_STATEMENT_LIST,
    AST_FN_DECLARATION,
    AST_ASSIGNMENT,
    AST_IDENTIFIER,
    AST_SYMBOL_DECLARATION,
    AST_FN_PROTOTYPE,
    AST_CALL,
    AST_TUPLE,
    AST_IF_ELSE,
    AST_MATCH,
    AST_STRUCT,
    AST_TYPE_DECLARATION,
    AST_MEMBER_ACCESS,
    AST_MEMBER_ASSIGNMENT,
    AST_INDEX_ACCESS,
    AST_IMPORT,
    AST_IMPORT_LIB,
    AST_VAR_ARG,
    AST_CURRIED_FN
  } tag;

  union {

    struct AST_BOOL {
      bool value;
    } AST_BOOL;

    struct AST_INTEGER {
      int value;
    } AST_INTEGER;

    struct AST_NUMBER {
      float value;
    } AST_NUMBER;

    struct AST_STRING {
      const char *value;
      int length;
    } AST_STRING;

    struct AST_ADD {
      AST *left;
      AST *right;
    } AST_ADD;

    struct AST_SUBTRACT {
      AST *left;
      AST *right;
    } AST_SUBTRACT;

    struct AST_MUL {
      AST *left;
      AST *right;
    } AST_MUL;

    struct AST_DIV {
      AST *left;
      AST *right;
    } AST_DIV;

    struct AST_MAIN {
      AST *body;
    } AST_MAIN;

    struct AST_UNOP {
      enum token_type op;
      AST *operand;
    } AST_UNOP;

    struct AST_BINOP {
      enum token_type op;
      AST *left;
      AST *right;
    } AST_BINOP;

    struct AST_EXPRESSION {
    } AST_EXPRESSION;

    struct AST_STATEMENT {
    } AST_STATEMENT;

    struct AST_STATEMENT_LIST {
      int length;
      AST **statements;
    } AST_STATEMENT_LIST;

    struct AST_FN_DECLARATION {
      AST *prototype;
      AST *body;
      char *name;
      bool is_extern;
    } AST_FN_DECLARATION;

    struct AST_FN_PROTOTYPE {
      int length;
      AST **parameters;
      char *type;
    } AST_FN_PROTOTYPE;

    struct AST_ASSIGNMENT {
      char *identifier;
      char *type;
      AST *expression;
    } AST_ASSIGNMENT;

    struct AST_IDENTIFIER {
      char *identifier;
    } AST_IDENTIFIER;

    struct AST_SYMBOL_DECLARATION {
      char *identifier;
      char *type;
      AST *expression;
    } AST_SYMBOL_DECLARATION;

    struct AST_CALL {
      AST *identifier;
      AST *parameters;
    } AST_CALL;

    struct AST_CURRIED_FN {
      AST *identifier;
      AST *parameters;
      int filled_len;
    } AST_CURRIED_FN;

    struct AST_TUPLE {
      int length;
      AST **members;
    } AST_TUPLE;

    struct AST_IF_ELSE {
      AST *condition;
      AST *then_body;
      AST *else_body;
    } AST_IF_ELSE;

    struct AST_MATCH {
      AST *candidate;
      int length;
      AST **matches;
      AST *result_type; // optional
    } AST_MATCH;

    struct AST_STRUCT {
      int length;
      AST **members;
    } AST_STRUCT;

    struct AST_TYPE_DECLARATION {
      char *name;
      AST *type_expr;
    } AST_TYPE_DECLARATION;

    struct AST_MEMBER_ACCESS {
      AST *object;
      char *member_name;
    } AST_MEMBER_ACCESS;

    struct AST_MEMBER_ASSIGNMENT {
      AST *object;
      char *member_name;
      AST *expression;
    } AST_MEMBER_ASSIGNMENT;

    struct AST_INDEX_ACCESS {
      AST *object;
      AST *index_expr;
    } AST_INDEX_ACCESS;

    struct AST_IMPORT {
      char *module_name;
      AST *module_ast;
    } AST_IMPORT;

    struct AST_IMPORT_LIB {
      char *lib_name;
    } AST_IMPORT_LIB;

  } data;

  ttype type; // optional - filled in during typecheck step
  line_info line_info;
  char *src_offset;
};

void print_ast(AST ast, int indent);
struct AST_STATEMENT_LIST *new_ast_stmt_list();

void free_ast(AST *ast);

AST *ast_new(AST ast);
#define AST_NEW(tag, ...)                                                      \
  ast_new((AST){AST_##tag, {.AST_##tag = (struct AST_##tag){__VA_ARGS__}}})

#define AST_DATA(ast, TYPE) (struct AST_##TYPE)(ast->data.AST_##TYPE)
#define FN_PROTOTYPE(fn_decl_ast) fn_decl_ast->data.AST_FN_DECLARATION.prototype
#endif /* end of include guard: _LANG_AST_H */
