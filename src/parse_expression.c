#include "parse_expression.h"
#include "lexer.h"
#include "parse.h"
#include "parse_function.h"
#include "parse_statement.h"
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
  case TOKEN_DOT: {
    if (right->tag == AST_ASSIGNMENT) {
      char *member_name = right->data.AST_ASSIGNMENT.identifier;
      // printf("\ntoken dot %s assignment\n", member_name);
      // print_ast(*right, 0);
      return AST_NEW(MEMBER_ASSIGNMENT, prev_expr, strdup(member_name),
                     right->data.AST_ASSIGNMENT.expression);
    }
    char *member_name = right->data.AST_IDENTIFIER.identifier;
    return AST_NEW(MEMBER_ACCESS, prev_expr, strdup(member_name));
  }
  case TOKEN_ASSIGNMENT: {
    char *member_name = prev_expr->data.AST_IDENTIFIER.identifier;
    return AST_NEW(ASSIGNMENT, member_name, NULL, right);
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
static AST *parse_string(bool can_assign) {
  token token = parser.previous;
  AST *str = AST_NEW(STRING, strdup(token.as.vstr), strlen(token.as.vstr));
  return str;
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
    if (check(TOKEN_NL)) {
      advance();
    }
    AST *member = parse_expression();
    ast_tuple_push((AST *)&tuple->data.AST_TUPLE, member);
    if (check(TOKEN_COMMA)) {
      advance();
    }
    if (check(TOKEN_NL)) {
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
    return AST_NEW(ASSIGNMENT, strdup(token.as.vstr), NULL,
                   assignment_expression);
  }
  return AST_NEW(IDENTIFIER, strdup(token.as.vstr));
}

static AST *parse_index_access(bool can_assign, AST *prev_expr) {

  AST *indices = ast_tuple(0);
  while (!match(TOKEN_RIGHT_SQ)) {
    AST *member = parse_expression();
    ast_tuple_push((AST *)&indices->data.AST_TUPLE, member);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }
  if (indices->data.AST_TUPLE.length == 1) {
    indices = indices->data.AST_TUPLE.members[0];
  }
  return AST_NEW(INDEX_ACCESS, prev_expr, indices);
}

static AST *parse_grouping(bool can_assign) {
  AST *tuple = parse_tuple();

  if (tuple->data.AST_TUPLE.length == 1) {
    AST *expr = tuple->data.AST_TUPLE.members[0];
    free_ast(tuple);
    return expr;
  }

  return tuple;
}

static AST *if_expression(bool can_assign) {
  if (!match(TOKEN_LP)) {
    fprintf(stderr, "Error, expected expression after if\n");
    return NULL;
  }
  AST *condition = parse_expression();
  if (!match(TOKEN_RP)) {
    return NULL;
  }
  AST *then_body = parse_fn_body();
  AST *else_body = parse_fn_body();
  return AST_NEW(IF_ELSE, condition, then_body, else_body);
}

AST *ast_match_list(int length, AST *candidate, ...) {

  AST *tuple = AST_NEW(MATCH, candidate, length);
  if (length == 0) {

    AST **list = malloc(sizeof(AST *));
    tuple->data.AST_MATCH.matches = list;
    return tuple;
  }
  // Define a va_list to hold the variable arguments
  va_list args;

  // Initialize the va_list with the variable arguments
  va_start(args, candidate);
  AST **list = malloc(sizeof(AST *) * length);
  for (int i = 0; i < length; i++) {
    AST *arg = va_arg(args, AST *);
    list[i] = arg;
  }
  tuple->data.AST_TUPLE.members = list;

  va_end(args);

  return tuple;
}

void ast_match_push(struct AST_MATCH *tuple, AST *item) {
  if (item) {
    tuple->length++;
    tuple->matches = realloc(tuple->matches, sizeof(AST *) * tuple->length);
    tuple->matches[tuple->length - 1] = item;
  }
}
static AST *parse_match(bool can_assign) {
  AST *match_on = parse_expression();
  AST *result_type = NULL;
  if (match_on->tag == AST_BINOP && match_on->data.AST_BINOP.op == TOKEN_PIPE) {
    result_type = match_on->data.AST_BINOP.right;
    match_on = match_on->data.AST_BINOP.left;
  }

  if (check(TOKEN_NL)) {
    advance();
  }
  AST *match_ast = ast_match_list(0, match_on);
  match_ast->data.AST_MATCH.result_type = result_type;

  AST *match_expr;
  while (match(TOKEN_BAR)) {
    match_expr = parse_expression();
    ast_match_push(&match_ast->data.AST_MATCH, match_expr);
    if (check(TOKEN_NL)) {
      advance();
    }
  }
  if (match_ast->data.AST_MATCH.length == 0) {
    fprintf(stderr, "Error: match expression must contain at least one path");
    return NULL;
  }

  if (match_ast->data.AST_MATCH.length == 1) {
    // just one branch - default branch, don't need to do any comparison
    return match_expr;
  }

  return match_ast;
}
AST *parse_scoped_block(bool can_assign) {
  AST *statements = ast_statement_list(0);

  while (!match(TOKEN_RIGHT_BRACE)) {
    if (check(TOKEN_EOF)) {
      return NULL;
    }
    if (check(TOKEN_NL)) {
      advance();
      continue;
    }
    statements_push(&statements->data.AST_STATEMENT_LIST);
  }
  advance();

  if (statements->data.AST_STATEMENT_LIST.length == 0) {
    free_ast(statements);
    return NULL;
  }
  if (statements->data.AST_STATEMENT_LIST.length == 1) {
    return statements->data.AST_STATEMENT_LIST.statements[0];
  }

  return statements;
}
AST *parse_struct(bool can_assign) {

  AST *proto = ast_fn_prototype(0);
  if (!match(TOKEN_LP)) {
    fprintf(stderr, "Error: expected ( after struct keyword\n");
    return NULL;
  };

  while (!match(TOKEN_RP)) {
    if (check(TOKEN_NL)) {
      advance();
    }
    if (check(TOKEN_RP)) {
      advance();
      break;
    }
    AST *arg = parse_fn_arg();
    arg_list_push(&proto->data.AST_FN_PROTOTYPE, arg);
    if (check(TOKEN_COMMA)) {
      advance();
    }
  }

  proto->tag = AST_STRUCT;
  proto->data.AST_STRUCT.length = proto->data.AST_FN_PROTOTYPE.length;
  proto->data.AST_STRUCT.members = proto->data.AST_FN_PROTOTYPE.parameters;
  return proto;
}

ParseRule rules[] = {
    [TOKEN_LP] = {parse_grouping, parse_call, PREC_CALL},
    [TOKEN_RP] = {NULL, NULL, PREC_NONE},

    [TOKEN_LEFT_SQ] = {NULL, parse_index_access, PREC_CALL},
    [TOKEN_LEFT_BRACE] = {parse_scoped_block, NULL, PREC_NONE},
    // [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, parse_binary, PREC_CALL},
    [TOKEN_MINUS] = {parse_unary, parse_binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_EQUALITY] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_MODULO] = {NULL, parse_binary, PREC_TERM},
    [TOKEN_NL] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, parse_binary, PREC_FACTOR},
    [TOKEN_BANG] = {parse_unary, NULL, PREC_NONE},
    [TOKEN_PIPE] = {NULL, parse_binary, PREC_PIPE},
    /* [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    [TOKEN_ASSIGNMENT] = {NULL, parse_binary, PREC_NONE},
    /* [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY}, */
    /* [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON}, */
    /* [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON}, */
    [TOKEN_LT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GT] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_LTE] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_GTE] = {NULL, parse_binary, PREC_COMPARISON},
    [TOKEN_STRING] = {parse_string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER] = {integer, NULL, PREC_NONE},
    /* [TOKEN_AND] = {NULL, and_, PREC_AND}, */
    /* [TOKEN_CLASS] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_ELSE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_TRUE] = {parse_literal, NULL, PREC_NONE},
    [TOKEN_FALSE] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_FOR] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_FN] = {parse_function, NULL, PREC_NONE},
    [TOKEN_IF] = {if_expression, NULL, PREC_NONE},
    // [TOKEN_NIL] = {parse_literal, NULL, PREC_NONE},
    /* [TOKEN_OR] = {NULL, or_, PREC_OR}, */ /* [TOKEN_PRINT] = {NULL, NULL,
                                                PREC_NONE}, */
    /* [TOKEN_RETURN] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_SUPER] = {super_, NULL, PREC_NONE}, */
    /* [TOKEN_THIS] = {this_, NULL, PREC_NONE}, */
    /* [TOKEN_TRUE] = {literal, NULL, PREC_NONE}, */
    /* [TOKEN_VAR] = {NULL, NULL, PREC_NONE}, */
    /* [TOKEN_WHILE] = {NULL, NULL, PREC_NONE}, */
    [TOKEN_IDENTIFIER] = {identifier, NULL, PREC_ASSIGNMENT},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_MATCH] = {parse_match, NULL, PREC_NONE},
    [TOKEN_STRUCT] = {parse_struct, NULL, PREC_NONE},
    // [TOKEN_PIPE] = {NULL, parse_binary, PREC_NONE}
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
