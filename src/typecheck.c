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

static int _typecheck_error_flag = 0;
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
    printf("(");
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
  printf("\t\t[ ");
  print_ast(*type_equation.ast, 0);
  // printf("ast %d", type_equation.ast->tag);
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

static int MAX_TEQ_LIST = 32;
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

static char *_tname();
static ttype _tvar() { return (ttype){T_VAR, {.T_VAR = {_tname()}}}; }
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
  sprintf(result, "'t%d", t_counter);
  return result;
}
static bool is_boolean_binop(token_type op) {
  return op == TOKEN_EQUALITY || op == TOKEN_LT || op == TOKEN_GT ||
         op == TOKEN_LTE || op == TOKEN_GTE;
}

static bool is_boolean_unop(token_type op) { return op == TOKEN_BANG; }

static void generate_equations(AST *ast, TypeCheckContext *ctx);

static void typecheck_ast(AST *ast, TypeCheckContext *ctx) {

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
      t = stmt_ast->type;
    }
    ast->type = t;
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
    ast->type = _tvar();
    break;
  }
  case AST_FN_DECLARATION: {
    char *name = ast->data.AST_FN_DECLARATION.name;

    if (name != NULL && ast->data.AST_FN_DECLARATION.is_extern) {
      ast->type = _tvar();
      ast_table_insert(ctx->symbol_table, name, ast);
      return;
    }

    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
    ast->type = _tvar();
    ast_table_insert(ctx->symbol_table, name, ast);

    enter_ttype_scope(ctx);
    // process prototype & body within new scope
    typecheck_ast(prototype_ast, ctx);
    typecheck_ast(ast->data.AST_FN_DECLARATION.body, ctx);
    exit_ttype_scope(ctx);

    break;
  }
  case AST_CALL: {
    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;
    AST *func_ast;
    if (ast_table_lookup(ctx->symbol_table, name, &func_ast) != 0) {
      fprintf(stderr, "Error [typecheck]: function %s not found in scope\n",
              name);
      break;
    };

    AST *func_prototype_ast = func_ast->data.AST_FN_DECLARATION.prototype;

    AST *parameters = ast->data.AST_CALL.parameters;
    struct AST_TUPLE parameters_tuple = parameters->data.AST_TUPLE;
    unsigned int arg_count = parameters_tuple.length;

    if (arg_count < func_prototype_ast->data.AST_FN_PROTOTYPE.length) {
      // TODO: replace AST with AST for curried func
    }
    // TODO: replace AST with casts, eg 1 -> 1.0 if param is double
    for (int i = 0; i < arg_count; i++) {
      typecheck_ast(parameters_tuple.members[i], ctx);
    }

    ast->type = _tvar();
    break;
  }
  case AST_IDENTIFIER: {
    char *name = ast->data.AST_IDENTIFIER.identifier;

    AST *typed_identifier_ast;
    if (ast_table_lookup(ctx->symbol_table, name, &typed_identifier_ast) != 0) {

      if (strcmp(name, "_") == 0) {
        ast->type = _tvar(); // placeholder var
        break;
      }
      fprintf(stderr, "Error [typecheck]: identifier %s not found in scope\n",
              name);
    } else {
      ast->type = typed_identifier_ast->type;
    };

    break;
  }
  case AST_BINOP: {
    typecheck_ast(ast->data.AST_BINOP.left, ctx);
    typecheck_ast(ast->data.AST_BINOP.right, ctx);
    ast->type = _tvar();
    break;
  }

  case AST_UNOP: {
    typecheck_ast(ast->data.AST_UNOP.operand, ctx);
    ast->type = _tvar();
    break;
  }
  case AST_SYMBOL_DECLARATION: {
    // insert param into symbol table for current stack
    char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;
    ast->type = _tvar();
    ast_table_insert(ctx->symbol_table, name, ast);
    break;
  }
  case AST_ASSIGNMENT: {
    char *name = ast->data.AST_ASSIGNMENT.identifier;
    AST *sym_ast;
    if (ast_table_lookup(ctx->symbol_table, name, &sym_ast) == 0) {
      // sym already exists - this is a reassignment
    }

    typecheck_ast(ast->data.AST_ASSIGNMENT.expression, ctx);
    ast->type = _tvar();
    ast_table_insert(ctx->symbol_table, name, ast);
    break;
  }
  case AST_IF_ELSE: {
    typecheck_ast(ast->data.AST_IF_ELSE.condition, ctx);
    typecheck_ast(ast->data.AST_IF_ELSE.then_body, ctx);
    if (ast->data.AST_IF_ELSE.else_body) {
      typecheck_ast(ast->data.AST_IF_ELSE.else_body, ctx);
    }

    ast->type = _tvar();
    break;
  }
  case AST_MATCH: {
    typecheck_ast(ast->data.AST_MATCH.candidate, ctx);
    for (int i = 0; i < ast->data.AST_MATCH.length; i++) {
      typecheck_ast(ast->data.AST_MATCH.matches[i], ctx);
    }

    ast->type = _tvar();
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
  case AST_TUPLE:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ACCESS:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_IMPORT:
  default: {
    ast->type = _tvar();
  }
  }
  generate_equations(ast, ctx);
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
                       (TypeEquation){left->type, right->type, left});

    // for now, operands in binops need to be the same type
    // TODO: allow a type-casting hierarchy eg (+ 1 1.0) is allowed and treated
    // as a float
    if (is_boolean_binop(op)) {
      push_type_equation(&ctx->type_equations,
                         (TypeEquation){ast->type, tbool(), ast});
      break;
    }
    if (op == TOKEN_PIPE) {
      // printf("push type equation for matcher expr\n");
      push_type_equation(
          &ctx->type_equations,
          (TypeEquation){ast->type, ast->data.AST_BINOP.right->type, ast});
      break;
    }

    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->type, left->type, ast});
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

    TypeEquation eq = (TypeEquation){ast->type, tfn(fn_members, length), ast};

    push_type_equation(&ctx->type_equations, eq);

    break;
  }
  case AST_CALL: {
    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;

    AST *func_ast;
    if (ast_table_lookup(ctx->symbol_table, name, &func_ast) != 0) {
      fprintf(stderr, "Error [typecheck]: function %s not found in scope\n",
              name);
      break;
    };

    ttype fn_type_var = func_ast->type;

    AST *params_ast = ast->data.AST_CALL.parameters;
    int arg_len = params_ast->data.AST_TUPLE.length;
    ttype *fn_param_types = malloc(sizeof(ttype) * (arg_len + 1));

    for (int i = 0; i < arg_len; i++) {
      AST *param_ast = params_ast->data.AST_TUPLE.members[i];
      fn_param_types[i] = param_ast->type;
    }

    fn_param_types[arg_len] = ast->type;

    ttype implied_fn_type = tfn(fn_param_types, arg_len + 1);
    push_type_equation(&ctx->type_equations, (TypeEquation){
                                                 fn_type_var,
                                                 implied_fn_type,
                                                 ast,
                                             });
    break;
  }
  case AST_IF_ELSE: {
    push_type_equation(&ctx->type_equations,
                       (TypeEquation){ast->data.AST_IF_ELSE.condition->type,
                                      tbool(),
                                      ast->data.AST_IF_ELSE.condition});

    push_type_equation(
        &ctx->type_equations,
        (TypeEquation){ast->type, ast->data.AST_IF_ELSE.then_body->type, ast});

    if (ast->data.AST_IF_ELSE.else_body) {
      // push_type_equation(&ctx->type_equations,
      //                    (TypeEquation){ast->data.AST_IF_ELSE.else_body->type,
      //                                   ast->data.AST_IF_ELSE.then_body->type});
    }
    break;
  }
  case AST_MATCH: {
    // push_type_equation
    push_type_equation(
        &ctx->type_equations,
        (TypeEquation){
            ast->type,
            ast->data.AST_MATCH.matches[0]->data.AST_BINOP.right->type, ast});

    push_type_equation(
        &ctx->type_equations,
        (TypeEquation){
            ast->data.AST_MATCH.candidate->type,
            ast->data.AST_MATCH.matches[0]->data.AST_BINOP.left->type, ast});
    break;
  }
  case AST_UNOP: {

    token_type op = ast->data.AST_UNOP.op;
    AST *operand = ast->data.AST_UNOP.operand;

    if (is_boolean_unop(op)) {
      push_type_equation(&ctx->type_equations,
                         (TypeEquation){operand->type, tbool(), operand});

      push_type_equation(&ctx->type_equations,
                         (TypeEquation){ast->type, tbool(), ast});
      break;
    }
    break;
  }
  case AST_MAIN:
  case AST_EXPRESSION:
  case AST_STATEMENT:
  case AST_STATEMENT_LIST:
  case AST_CALL_EXPRESSION:
  case AST_ASSIGNMENT:
  case AST_IDENTIFIER:
  case AST_SYMBOL_DECLARATION:
  case AST_FN_PROTOTYPE:
  case AST_TUPLE:
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

// TYPE UNIFICATION

INIT_SYM_TABLE(ttype);
typedef ttype_StackFrame TypeEnv;

void unify(TypeEquationsList *list, TypeEnv *env) {

  TypeEquation eq = *list->equations;

  do {
    if (eq.left.tag != T_VAR) {
      break;
    }
    if (eq.left.as.T_VAR.name == NULL) {
      break;
    }

    char *lname = eq.left.as.T_VAR.name;

#ifdef _TYPECHECK_DBG
    print_ttype(eq.left);
    printf(" :: ");
#endif

    ttype right = eq.right;

    if (right.tag == T_VAR) {
      char *rname = right.as.T_VAR.name;
      while (right.tag == T_VAR && ttype_env_lookup(env, rname, &right) == 0) {
        rname = right.as.T_VAR.name;
      }
    } else if (right.tag == T_FN && eq.ast->tag == AST_FN_DECLARATION) {

      AST *prototype_ast = FN_PROTOTYPE(eq.ast);

      for (int i = 0; i < right.as.T_FN.length; i++) {
        // iterate over fn arg types - if arg is 'tn and found in the env
        // [lookedup_member] we can substitute 'tn for whatever's found in the

        ttype fn_member = right.as.T_FN.members[i];
        ttype lookedup_member;

        if (ttype_env_lookup(env, fn_member.as.T_VAR.name, &lookedup_member) ==
            0) {
          right.as.T_FN.members[i] = lookedup_member;

          if (i < right.as.T_FN.length - 1) {
            prototype_ast->data.AST_FN_PROTOTYPE.parameters[i]->type =
                lookedup_member;
          }
        }
      }
    } else if (right.tag == T_FN && eq.ast->tag == AST_CALL) {

      ttype lookedup_fn;
      if (ttype_env_lookup(env, lname, &lookedup_fn) == 0) {
        int fn_len = lookedup_fn.as.T_FN.length;
        if (fn_len == 1) {
          eq.ast->type = lookedup_fn.as.T_FN.members[0];
          break;
        }

        ttype ret_type = lookedup_fn.as.T_FN.members[fn_len - 1];
        char *ret_name = ret_type.as.T_VAR.name;
        while (ret_type.tag == T_VAR &&
               ttype_env_lookup(env, ret_name, &ret_type) == 0) {
          ret_name = right.as.T_VAR.name;
        }
        eq.ast->type = ret_type;
        break;
      }
    }
#ifdef _TYPECHECK_DBG
    print_ttype(right);
    printf("\n");
#endif

    ttype existing;
    if (ttype_env_lookup(env, lname, &existing) == 0 &&
        existing.tag != right.tag) {
      printf("\033[1;31m");
      printf("Error: type error at %s", eq.ast->src_offset);
      printf(" found ");
      print_ttype(existing);
      printf(" - expected ");
      print_ttype(right);
      printf("\033[1;0m");
      printf("\n");
      _typecheck_error_flag = 1;
    }
    ttype_env_insert(env, lname, right);

    eq.ast->type = right;

  } while (0);

  if (list->length > 1) {
    list->length--;
    list->equations++;
    unify(list, env);
  }
}
int typecheck(AST *ast) {
  _typecheck_error_flag = 0;
  TypeCheckContext ctx;
  ast_SymbolTable symbol_table;
  symbol_table.current_frame_index = 0;
  ctx.symbol_table = &symbol_table;
  t_counter = 0;

  ctx.type_equations.equations = calloc(sizeof(TypeEquation), MAX_TEQ_LIST);
  ctx.type_equations.length = 0;
  typecheck_ast(ast, &ctx);

  if (_typecheck_error_flag) {
    return 1;
  }

#ifdef _TYPECHECK_DBG

  printf("type equations\n-----\n");
  for (int i = 0; i < ctx.type_equations.length; i++) {
    print_type_equation(ctx.type_equations.equations[i]);
  }

#endif

  TypeEnv env;

  unify(&ctx.type_equations, &env);

  for (int i = 0; i < TABLE_SIZE; i++) {
    // TODO: wtf?? if I do typecheck
    // twice, it seems the same stack
    // slots are reused, sometimes there
    // are values left over :(
    env.entries[i] = NULL;
  }
  // free(ctx.type_equations.equations);

  return _typecheck_error_flag;
}

void print_last_entered_type(AST *ast) {
  int last = ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.length - 1;
  ttype last_type_expr =
      ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[last]->type;
  print_ttype(last_type_expr);
}
