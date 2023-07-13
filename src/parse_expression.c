#include "parse_expression.h"
#include "lexer.h"
#include "parse.h"
#include "parse_function.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AST *parse_precedence(Precedence precedence);
static ParseRule *get_rule(enum token_type type);

static AST *parse_unary(bool can_assign) {
  enum token_type op_type = parser.previous.type;
  AST *operand = parse_precedence(PREC_UNARY);
  switch (op_type) {
  case TOKEN_BANG:
  case TOKEN_MINUS: {
    return AST_NEW(UNOP, op_type, operand);
  }
  default:
    return NULL;
  }
}

static AST *parse_binary(bool can_assign, AST *prev_expr) {
  enum token_type op_type = parser.previous.type;
  ParseRule *rule = get_rule(op_type);
  AST *right = parse_precedence(
      (Precedence)(rule->precedence +
                   1)); // go ahead and swallow up the RHS of this expression
  // after returning it will be applied to whatever the parser has seen to the
  // left of the current token

  switch (op_type) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_EQUALITY:
  case TOKEN_MODULO:
  case TOKEN_LT:
  case TOKEN_GT:
  case TOKEN_LTE:
  case TOKEN_GTE:
  case TOKEN_PIPE: {
    AST *binop = malloc(sizeof(AST));
    binop->tag = AST_BINOP;
    binop->data.AST_BINOP.op = op_type;
    binop->data.AST_BINOP.right = right;
    binop->data.AST_BINOP.left = prev_expr;
    return binop;
  }
  default:
    return NULL;
  }
}

static AST *number(bool can_assign) {
  token token = parser.previous;
  return AST_NEW(NUMBER, token.as.vfloat);
}

static AST *integer(bool can_assign) {
  token token = parser.previous;
  return AST_NEW(INTEGER, token.as.vint);
}

static AST *parse_literal(bool can_assign) {
  token token = parser.previous;
  return AST_NEW(BOOL, token.type == TOKEN_TRUE);
}

AST *ast_tuple(int length, ...) {

  AST *tuple = AST_NEW(TUPLE, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    tuple->data.AST_TUPLE.members = list;
    return tuple;
  }
  // Define a va_list to hold the variable arguments
  va_list args;

  // Initialize the va_list with the variable arguments
  va_start(args, length);

  AST **list = malloc(sizeof(AST *) * length);
  for (int i = 0; i < length; i++) {
    AST *arg = va_arg(args, AST *);
    list[i] = arg;
  }
  tuple->data.AST_TUPLE.members = list;

  va_end(args);

  return tuple;
}

typedef struct AST_TUPLE tpl;
void ast_tuple_push(struct AST *tuple, AST *item) {
  tpl *cast_tuple = (tpl *)tuple;
  if (item) {
    cast_tuple->length++;
    cast_tuple->members =
        realloc(cast_tuple->members, sizeof(AST *) * cast_tuple->length);
    cast_tuple->members[cast_tuple->length - 1] = item;
  }
}

static AST *parse_tuple() {
  AST *tuple = ast_tuple(0);
  while (!match(TOKEN_RP)) {
    AST *member = parse_expression();
    ast_tuple_push((AST *)&tuple->data.AST_TUPLE, member);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }
  return tuple;
}

static AST *parse_call(bool can_assign, AST *prev_expr) {
  AST *parameters = parse_tuple();
  return AST_NEW(CALL, prev_expr, parameters);
}
static AST *identifier(bool can_assign) {
  token token = parser.previous;

  if (match(TOKEN_ASSIGNMENT)) {
    AST *assignment_expression = parse_expression();
    return AST_NEW(ASSIGNMENT, strdup(token.as.vstr), assignment_expression);
  }
  return AST_NEW(IDENTIFIER, strdup(token.as.vstr));
}

static AST *parse_grouping(bool can_assign) {
  AST *tuple = parse_tuple();
  AST *expr = tuple->data.AST_TUPLE.members[0];
  free_ast(tuple);
  return expr;
}

ParseRule rules[] = {
    [TOKEN_LP] = {parse_grouping, parse_call, PREC_CALL},
    [TOKEN_RP] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    /* [TOKEN_DOT] = {NULL, dot, PREC_CALL}, */
    [TOKEN_MINUS] = {parse_unary, parse_binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_MODULO] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_NL] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_BANG] = {parse_unary, NULL, PREC_NONE},
    [TOKEN_PIPE] = {NULL, parse_binary, PREC_PIPE},
    /* [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_ASSIGNMENT] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    [TOKEN_LT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_LTE] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GTE] = {NULL, parse_binary, PREC_COMPARISON},
    // [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER] = {integer, NULL, PREC_NONE},
    /* [TOKEN_AND] = {NULL, and_, PREC_AND}, */
    /* [TOKEN_CLASS] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_ELSE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_TRUE] = {parse_literal, NULL, PREC_NONE},
    [TOKEN_FALSE] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_FOR] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_FN] = {parse_function, NULL, PREC_NONE},
    /* [TOKEN_IF] = {NULL, NULL, PREC_NONE}, */
    // [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */ /* [TOKEN_PRINT] = {NULL, NULL,
                                                PREC_NONE}, */
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_IDENTIFIER] = {identifier, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(enum token_type type) { return &(rules[type]); }

static AST *parse_precedence(Precedence precedence) {
  if (check(TOKEN_EOF)) {
    return NULL;
  }

  while (check(TOKEN_NL)) {
    advance();
  }
  advance();

  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    printf("error: Expected expression, found ");
    print_token(parser.previous);
    printf("\n");
    return NULL;
  }

  bool can_assign =
      precedence <=
      PREC_ASSIGNMENT; // guard against expressions like a * b = c + d
  //
  AST *expr = prefix_rule(can_assign);
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();

    ParseFnInfix infix_rule = get_rule(parser.previous.type)->infix;
    AST *tmp = infix_rule(can_assign, expr);
    expr = tmp;
  }

  if (can_assign && match(TOKEN_ASSIGNMENT)) {
    printf("error Invalid assignment target");
    return NULL;
  }
  return expr;
};

AST *parse_expression() { return parse_precedence(PREC_ASSIGNMENT); }
