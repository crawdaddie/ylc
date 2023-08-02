#include "typecheck.h"
#include "codegen.h"
#include "generic_symbol_table.h"
#include "string.h"
#include "symbol_table.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
typedef AST *ast;
INIT_SYM_TABLE(ast);

typedef struct {
  ttype left;
  ttype right;
  AST *ast;
} TypeEquation;

void print_ttype(ttype type) {
  switch (type.tag) {
  case T_VOID: {
    printf("Void");
    break;
  }
  case T_INT: {
    printf("Int");
    break;
  }
  case T_NUM: {
    printf("Num");
    break;
  }
  case T_STR: {

    printf("Str");
    break;
  }
  case T_BOOL: {

    printf("Bool");
    break;
  }
  case T_VAR: {
    printf("%s", type.as.T_VAR.name);
    break;
  }
  case T_FN: {
    int length = type.as.T_FN.length;
    printf("Fn (");
    for (int i = 0; i < length; i++) {
      print_ttype(type.as.T_FN.members[i]);
      if (i < length - 1)
        printf(" -> ");
    }
    printf(")");
    break;
  }
  default: {
  }
  }
}
static void print_type_equation(TypeEquation type_equation) {
  print_ttype(type_equation.left);
  printf(" :: ");
  print_ttype(type_equation.right);
  printf(" [ ");
  print_ast(*type_equation.ast, 0);
  printf(" ]");
  printf("\n");
}

typedef struct {
  TypeEquation *equations;
  int length;
} TypeEquationsList;
typedef struct {
  ast_SymbolTable *symbol_table;
  TypeEquationsList type_equations;
} TypeCheckContext;

static int MAX_TEQ_LIST = 100;
void push_type_equation(TypeEquationsList *list, TypeEquation equation) {

  list->length++;
  if (list->length >= MAX_TEQ_LIST) {
    list->equations = realloc(
        list->equations, sizeof(TypeEquation) * list->length + MAX_TEQ_LIST);
    MAX_TEQ_LIST += 8;
  }
  list->equations[list->length - 1] = equation;
}

static void enter_ttype_scope(TypeCheckContext *ctx) {
  ast_push_frame(ctx->symbol_table);
}

static void exit_ttype_scope(TypeCheckContext *ctx) {
  ast_pop_frame(ctx->symbol_table);
}

static ttype tvoid() { return (ttype){T_VOID}; }
static ttype tint() { return (ttype){T_INT}; }
static ttype tnum() { return (ttype){T_NUM}; }
static ttype tstr() { return (ttype){T_STR}; }
static ttype tbool() { return (ttype){T_BOOL}; }
static ttype tvar(char *name) { return (ttype){T_VAR, {.T_VAR = {name}}}; }
static ttype tfn(ttype *param_types, int length) {
  return (ttype){T_FN,
                 {.T_FN = {
                      .length = length,
                      .members = param_types,
                  }}};
}

static int t_counter = 0;

static char *_tname() {
  t_counter++;
  char *result =
      (char *)malloc(32 * sizeof(char)); // Assuming a large enough buffer size
  sprintf(result, "t_%d", t_counter);
  return result;
}
static bool is_boolean_binop(token_type op) {
  return op == TOKEN_EQUALITY || op == TOKEN_LT || op == TOKEN_GT ||
         op == TOKEN_LTE || op == TOKEN_GTE;
}
static void generate_equations(AST *ast, TypeCheckContext *ctx) {

  switch (ast->tag) {

  case AST_INTEGER: {
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, tint(), ast});
    break;
  }
  case AST_NUMBER: {
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, tnum(), ast});
    break;
  }
  case AST_BOOL: {
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, tbool(), ast});
    break;
  }
  case AST_STRING: {
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, tstr(), ast});
    break;
  }
  case AST_BINOP: {
    token_type op = ast->data.AST_BINOP.op;
    AST *left = ast->data.AST_BINOP.left;
    AST *right = ast->data.AST_BINOP.right;
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){left->type, right->type, ast});

    // for now, operands in binops need to be the same type
    // TODO: allow a type-casting hierarchy eg (+ 1 1.0) is allowed and treated
    // as a float

    if (is_boolean_binop(op)) {
      push_type_equation(&ctx->type_equations,
                         (TypeEquation){ast->type, tbool(), ast});
      break;
    }

    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, tint(), ast});
    break;
  }
  case AST_FN_DECLARATION: {
    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
    int length = prototype_ast->data.AST_FN_PROTOTYPE.length + 1;

    ttype *fn_members = malloc(sizeof(ttype) * length);

    for (int i = 0; i < length - 1; i++) {
      AST *param_ast = prototype_ast->data.AST_FN_PROTOTYPE.parameters[i];
      fn_members[i] = param_ast->type;
    }
    fn_members[length - 1] = ast->data.AST_FN_DECLARATION.body->type;

    TypeEquation eq = {ast->type, tfn(fn_members, length), ast};
    print_type_equation(eq);
    push_type_equation(&ctx->type_equations, eq);

    break;
  }
  // case AST_IF_ELSE: {
  //   push_type_equation(
  //       &ctx->type_equations,
  //       (TypeEquation){ast->data.AST_IF_ELSE.condition->type, tbool(), ast});
  //
  //   push_type_equation(
  //       &ctx->type_equations,
  //       (TypeEquation){ast->type, ast->data.AST_IF_ELSE.then_body->type});
  //
  //   if (ast->data.AST_IF_ELSE.else_body != NULL) {
  //     push_type_equation(
  //         &ctx->type_equations,
  //         (TypeEquation){ast->type, ast->data.AST_IF_ELSE.else_body->type});
  //   }
  //   break;
  // }
  case AST_MAIN:
  case AST_UNOP:
  case AST_EXPRESSION:
  case AST_STATEMENT:
  case AST_STATEMENT_LIST:
  case AST_CALL_EXPRESSION:
  case AST_ASSIGNMENT:
  case AST_IDENTIFIER:
  case AST_SYMBOL_DECLARATION:
  case AST_FN_PROTOTYPE:
  case AST_CALL:
  case AST_TUPLE:
  case AST_MATCH:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ACCESS:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_IMPORT:
  case AST_VAR_ARG:
  default:
    break;
  }
}

static void typecheck_ast(AST *ast, TypeCheckContext *ctx) {

  // printf("start tcheck for node %d\n", ast->tag);
  switch (ast->tag) {
  case AST_MAIN: {
    typecheck_ast(ast->data.AST_MAIN.body, ctx);
    break;
  }
  case AST_STATEMENT_LIST: {
    ttype t;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      AST *stmt_ast = ast->data.AST_STATEMENT_LIST.statements[i];
      typecheck_ast(stmt_ast, ctx);
      // t = stmt_ast->type;
    }
    // ast->type = t;
    break;
  }

  case AST_FN_DECLARATION: {
    char *name = ast->data.AST_FN_DECLARATION.name;
    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;

    ast_table_insert(ctx->symbol_table, name, prototype_ast);

    enter_ttype_scope(ctx);
    // process prototype & body within new scope
    typecheck_ast(prototype_ast, ctx);
    typecheck_ast(ast->data.AST_FN_DECLARATION.body, ctx);
    exit_ttype_scope(ctx);

    ast->type = tvar(_tname());
    break;
  }
  case AST_FN_PROTOTYPE: {
    int arg_count = ast->data.AST_FN_PROTOTYPE.length;
    AST **parameters = ast->data.AST_FN_PROTOTYPE.parameters;

    for (int i = 0; i < arg_count; i++) {
      AST *param_ast = parameters[i];
      typecheck_ast(param_ast, ctx);
    }
    // ast->type = tfn();
    ast->type = tvar(_tname());
    break;
  }
  case AST_CALL: {
    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;
    AST *func_ast;
    ttype func_type;

    AST *func_prototype_ast = func_ast->data.AST_FN_DECLARATION.prototype;

    AST *parameters = ast->data.AST_CALL.parameters;
    struct AST_TUPLE parameters_tuple = parameters->data.AST_TUPLE;
    unsigned int arg_count = parameters_tuple.length;

    if (arg_count < func_prototype_ast->data.AST_FN_PROTOTYPE.length) {
      // TODO: replace AST with AST for curried func
    }
    // TODO: replace AST with casts, eg 1 -> 1.0 if param is double

    break;
  }
  case AST_IDENTIFIER: {
    char *name = ast->data.AST_IDENTIFIER.identifier;

    AST *typed_identifier_ast;
    if (ast_table_lookup(ctx->symbol_table, name, &typed_identifier_ast) != 0) {
      fprintf(stderr, "identifier %s not found in this scope\n", name);
      // ast->type = tvar(name);
    } else {
      ast->type = typed_identifier_ast->type;
    };

    break;
  }
  case AST_BINOP: {
    typecheck_ast(ast->data.AST_BINOP.left, ctx);
    typecheck_ast(ast->data.AST_BINOP.right, ctx);
    ast->type = tvar(_tname());
    break;
  }
  case AST_SYMBOL_DECLARATION: {
    // insert param into symbol table for current stack
    char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;
    ast->type = tvar(_tname());
    ast_table_insert(ctx->symbol_table, name, ast);
    break;
  }
  case AST_IF_ELSE: {
    typecheck_ast(ast->data.AST_IF_ELSE.condition, ctx);
    typecheck_ast(ast->data.AST_IF_ELSE.then_body, ctx);

    if (ast->data.AST_IF_ELSE.else_body != NULL) {
      typecheck_ast(ast->data.AST_IF_ELSE.else_body, ctx);
    }

    ast->type = tvar(_tname());
    break;
  }
  case AST_INTEGER:
    ast->type = tint();
    break;
  case AST_NUMBER:
    ast->type = tnum();
    break;
  case AST_BOOL:
    ast->type = tbool();
    break;
  case AST_STRING: {
    ast->type = tstr();
    break;
  }
  case AST_ASSIGNMENT:
  case AST_UNOP:
  case AST_TUPLE:
  case AST_MATCH:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ACCESS:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_IMPORT:
  default: {
    ast->type = tvar(_tname());
  }
  }
  // printf("finish tcheck for node %d\n", ast->tag);
  generate_equations(ast, ctx);
}

void typecheck(AST *ast) {
  TypeCheckContext ctx;
  ast_SymbolTable symbol_table;
  symbol_table.current_frame_index = 0;
  ctx.symbol_table = &symbol_table;
  ctx.type_equations.equations = malloc(sizeof(TypeEquation) * MAX_TEQ_LIST);
  typecheck_ast(ast, &ctx);
  for (int i = 0; i < ctx.type_equations.length; i++) {
    print_type_equation(ctx.type_equations.equations[i]);
  }
}
