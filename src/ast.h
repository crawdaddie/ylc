#ifndef _LANG_AST_H
#define _LANG_AST_H
#include "lexer.h"
#include <stdbool.h>
typedef struct AST AST;

struct AST {
  enum {
    AST_INTEGER,
    AST_NUMBER,
    AST_BOOL,
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
    AST_CALL_EXPRESSION,
    AST_FN_DECLARATION,
    AST_ASSIGNMENT,
    AST_IDENTIFIER,
    AST_SYMBOL_DECLARATION,
    AST_FN_PROTOTYPE,
    AST_CALL,
    AST_TUPLE,
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
    } AST_FN_DECLARATION;

    struct AST_FN_PROTOTYPE {
      int length;
      AST **identifiers;
    } AST_FN_PROTOTYPE;

    struct AST_ASSIGNMENT {
      char *identifier;
      AST *expression;
    } AST_ASSIGNMENT;

    struct AST_IDENTIFIER {
      char *identifier;
    } AST_IDENTIFIER;

    struct AST_SYMBOL_DECLARATION {
      char *identifier;
      AST *expression;
    } AST_SYMBOL_DECLARATION;

    struct AST_CALL {
      char *identifier;
      AST *parameters;
    } AST_CALL;

    struct AST_TUPLE {
      int length;
      AST **members;
    } AST_TUPLE;

  } data;
};

void print_ast(AST ast, int indent);
struct AST_STATEMENT_LIST *new_ast_stmt_list();

void free_ast(AST *ast);

AST *ast_new(AST ast);
#define AST_NEW(tag, ...)                                                      \
  ast_new((AST){AST_##tag, {.AST_##tag = (struct AST_##tag){__VA_ARGS__}}})

#endif /* end of include guard: _LANG_AST_H */
