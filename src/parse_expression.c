#include "parse_expression.h"
#include "compiler.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

static AST *parse_precedence(Precedence precedence);
static ParseRule *get_rule(enum token_type type);

static AST *parse_unary(bool can_assign) {
  enum token_type op_type = parser.previous.type;
  AST *operand = parse_precedence(PREC_UNARY);
  switch (op_type) {
  case TOKEN_BANG:
  case TOKEN_MINUS: {
    AST *ast = malloc(sizeof(AST));
    ast->tag = AST_UNOP;
    ast->data.AST_UNOP.op = op_type;
    ast->data.AST_UNOP.operand = operand;
    return ast;
  }
  default:
    return NULL;
  }
}
static AST *parse_binary(bool can_assign) {
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
    return binop;
  }
  default:
    return NULL;
  }
}

static AST *number(bool can_assign) {
  token token = parser.previous;
  AST *expr = malloc(sizeof(AST));
  expr->tag = AST_NUMBER;
  expr->data.AST_NUMBER.value = token.as.vfloat;
  return expr;
}

static AST *integer(bool can_assign) {
  token token = parser.previous;
  AST *expr = malloc(sizeof(AST));
  expr->tag = AST_INTEGER;
  expr->data.AST_INTEGER.value = token.as.vint;
  return expr;
}

static AST *parse_literal(bool can_assign) {
  token token = parser.previous;
  AST *expr = malloc(sizeof(AST));
  expr->tag = AST_BOOL;
  expr->data.AST_BOOL.value = token.type == TOKEN_TRUE;
  return expr;
}

ParseRule rules[] = {
    // [TOKEN_LP] = {grouping, call, PREC_CALL},
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
    // [TOKEN_FN] = {parse_anonymous_function, NULL, PREC_NONE},
    /* [TOKEN_IF] = {NULL, NULL, PREC_NONE}, */
    // [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */
    /* [TOKEN_PRINT] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(enum token_type type) { return &(rules[type]); }

static AST *parse_precedence(Precedence precedence) {
  while (check(TOKEN_NL)) {
    advance();
  }
  advance();

  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    printf("error: Expected expression ");
    print_token(parser.previous);
    return NULL;
  }
  bool can_assign =
      precedence <=
      PREC_ASSIGNMENT; // guard against expressions like a * b = c + d
  //
  AST *expr = prefix_rule(can_assign);
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();

    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    AST *tmp = infix_rule(can_assign);
    tmp->data.AST_BINOP.left = expr;
    expr = tmp;
  }
  if (can_assign && match(TOKEN_ASSIGNMENT)) {
    printf("error Invalid assignment target");
    return NULL;
  }
  return expr;
};

AST *parse_expression() { return parse_precedence(PREC_ASSIGNMENT); }
