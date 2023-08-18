#ifndef _LANG_EXPRESSION_H
#define _LANG_EXPRESSION_H
#include "../ast.h"
#include <stdbool.h>

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_PIPE,       // ->
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef AST *(*ParseFn)(bool can_assign);
typedef AST *(*ParseFnInfix)(bool can_assign, AST *prev_expr);

typedef struct {
  ParseFn prefix;
  ParseFnInfix infix;
  Precedence precedence;
} ParseRule;

AST *parse_expression();

AST *parse_scoped_block(bool can_assign);

#endif /* ifndef _LANG_EXPRESSION_H */
