#include "typecheck.h"
#include "codegen/codegen.h"
#include "generic_symbol_table.h"
#include "modules.h"
#include "paths.h"
#include "string.h"
#include "symbol_table.h"
#include "types.h"
#include "unify_types.h"
#include <dlfcn.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#define _TYPECHECK_DBG

INIT_SYM_TABLE(AST);

static void print_type_equation(TypeEquation type_equation) {
  print_ttype(*type_equation.left);
  printf(" :: ");
  print_ttype(*type_equation.right);
  if (type_equation.index != -1) {
    printf(" [%d]", type_equation.index);
  }
  printf("\n");
}
static void generate_equations(AST *ast, TypeCheckContext *ctx);
int _typecheck_error_flag = 0;

static ttype Bool = {T_BOOL};
static ttype Str = {T_STR};
static ttype Int = {T_INT};
static ttype Int8 = {T_INT8};
static ttype Num = {T_NUM};

static int MAX_TEQ_LIST = 32;
void push_type_equation(ttype *dependent_type, ttype *type,
                        TypeCheckContext *ctx) {
  TypeEquation eq = {dependent_type, type, -1};
  unify(eq, &ctx->type_env);
}

void push_to_list(TypeEquationsList *list, TypeEquation eq) {

  list->length++;
  if (list->length >= MAX_TEQ_LIST) {
    list->equations = realloc(
        list->equations, sizeof(TypeEquation) * list->length + MAX_TEQ_LIST);
    MAX_TEQ_LIST += 8;
  }
  list->equations[list->length - 1] = eq;
}

/*
 * say that type l is equal to the nth type of type r (r must be a fn or struct
 * type)
 * */
void push_type_equation_index(ttype *l, ttype *r, int n,
                              TypeCheckContext *ctx) {
  TypeEquation eq = {l, r, n};

  if (n != -1) {
    // TODO: unify indexed type equations, eg l :: fn_type[n]

    // find r type
    r = follow_links(&ctx->type_env, r);
    if (r->tag == T_VAR) {
      printf("defer: ");
      eq.right = r;
      print_type_equation(eq);
      // push equation to deferred typecheck equations list
      return push_to_list(&ctx->type_equations, eq);
    }
  }
  unify(eq, &ctx->type_env);
}

static void enter_ttype_scope(TypeCheckContext *ctx) {
  AST_push_frame(ctx->symbol_table);
}

static void exit_ttype_scope(TypeCheckContext *ctx) {
  AST_pop_frame(ctx->symbol_table);
}

static bool is_boolean_binop(token_type op) {
  return op == TOKEN_EQUALITY || op == TOKEN_LT || op == TOKEN_GT ||
         op == TOKEN_LTE || op == TOKEN_GTE;
}

static bool is_boolean_unop(token_type op) { return op == TOKEN_BANG; }

static ttype *_max_type(ttype *l, ttype *r) {
  if (l->tag >= r->tag) {
    return l;
  }
  return r;
}
static ttype lookup_explicit_type(char *type_identifier,
                                  TypeCheckContext *ctx) {
  if (strcmp(type_identifier, "double") == 0) {
    return Num;
  }

  if (strcmp(type_identifier, "int") == 0) {
    return Int;
  }

  if (strcmp(type_identifier, "str") == 0) {
    return Str;
  }

  if (strcmp(type_identifier, "int8") == 0) {
    return Int8;
  }

  AST *sym;
  if (AST_table_lookup(ctx->symbol_table, type_identifier, sym) == 0) {
    return sym->type;
  }
}

ttype get_fn_return_type(ttype fn_type) {
  int len = fn_type.as.T_FN.length;
  return fn_type.as.T_FN.members[len - 1];
}
/*
 * Transform ast call node with incomplete args to anon fn declaration which
 * is curried version of original fn
 **/
static void typecheck_curry_function(AST *ast, AST *func_ast,
                                     TypeCheckContext *ctx) {

  AST **original_fn_parameters = func_ast->data.AST_FN_DECLARATION.prototype
                                     ->data.AST_FN_PROTOTYPE.parameters;
  char *id = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;

  AST *call_params = ast->data.AST_CALL.parameters;
  int call_params_len = call_params->data.AST_TUPLE.length;
  AST **og_call_list = call_params->data.AST_TUPLE.members;

  int fn_param_len =
      func_ast->data.AST_FN_DECLARATION.prototype->data.AST_FN_PROTOTYPE.length;

  AST **call_list = malloc(sizeof(AST *) * fn_param_len);
  for (int i = 0; i < call_params_len; i++) {
    call_list[i] = og_call_list[i];
  }

  AST *new_prot = AST_NEW(FN_PROTOTYPE, fn_param_len - call_params_len);
  new_prot->data.AST_FN_PROTOTYPE.length = fn_param_len - call_params_len;
  new_prot->data.AST_FN_PROTOTYPE.parameters =
      malloc(sizeof(AST *) * fn_param_len - call_params_len);

  for (int i = 0; i < fn_param_len - call_params_len; i++) {
    new_prot->data.AST_FN_PROTOTYPE.parameters[i] =
        original_fn_parameters[i + call_params_len];

    call_list[call_params_len + i] =
        AST_NEW(IDENTIFIER, new_prot->data.AST_FN_PROTOTYPE.parameters[i]
                                ->data.AST_SYMBOL_DECLARATION.identifier);
  }

  ast->tag = AST_FN_DECLARATION;
  ast->data.AST_FN_DECLARATION.prototype = new_prot;
  ast->data.AST_FN_DECLARATION.body = AST_NEW(
      CALL, AST_NEW(IDENTIFIER, id), AST_NEW(TUPLE, fn_param_len, call_list));
  ast->data.AST_FN_DECLARATION.is_extern = false;
  ast->data.AST_FN_DECLARATION.name = NULL;
  generate_equations(ast, ctx);
}

static int ast_member_lookup(AST *ast, TypeCheckContext *ctx, AST *object) {

  char *obj_identifier =
      ast->data.AST_MEMBER_ACCESS.object->data.AST_IDENTIFIER.identifier;

  if (AST_table_lookup(
          ctx->symbol_table,
          ast->data.AST_MEMBER_ACCESS.object->data.AST_IDENTIFIER.identifier,
          object) == 0) {

    while (object->type.tag == T_VAR) {
      AST_table_lookup(ctx->symbol_table, object->type.as.T_VAR.name, object);
    }
    return 0;
  }
  return 1;
}

static void typecheck_object_member_call(AST *ast, TypeCheckContext *ctx) {

  AST *object = malloc(sizeof(AST));
  if (ast_member_lookup(ast->data.AST_CALL.identifier, ctx, object) == 0) {
    char *member_name =
        ast->data.AST_CALL.identifier->data.AST_MEMBER_ACCESS.member_name;

    unsigned int idx = get_struct_member_index(object->type, member_name);

    ttype member_type = object->type.as.T_STRUCT.members[idx];

    if (member_type.tag != T_FN) {
      fprintf(stderr, "Error: %s not callable", member_name);
      return;
    }

    ttype *return_type = malloc(sizeof(ttype));
    *return_type = get_fn_return_type(member_type);

    push_type_equation(&ast->type, return_type, ctx);
    return;
  };
}

static ttype compute_type_expression(AST *ast, TypeCheckContext *ctx) {
  switch (ast->tag) {

  case AST_TUPLE: {
    int len = ast->data.AST_TUPLE.length;
    ttype *member_types = malloc(sizeof(ttype) * len);

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_TUPLE.members[i];
      ttype t =
          lookup_explicit_type(member_ast->data.AST_IDENTIFIER.identifier, ctx);

      member_types[i] = t;
    }

    ttype tuple_type = ttuple(member_types, len);
    return tuple_type;
  }
  case AST_STRUCT: {

    int len = ast->data.AST_STRUCT.length;
    ttype *member_types = malloc(sizeof(ttype) * len);
    struct_member_metadata *metadata =
        malloc(sizeof(struct_member_metadata) * len);

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_STRUCT.members[i];

      ttype t = lookup_explicit_type(
          member_ast->data.AST_SYMBOL_DECLARATION.type, ctx);

      member_types[i] = t;
      metadata[i] = (struct_member_metadata){
          .name = strdup(member_ast->data.AST_SYMBOL_DECLARATION.identifier),
          .index = i};
    }

    ttype tuple_type = tstruct(member_types, metadata, len);
    return tuple_type;
  }
  }
}

void assign_explicit_type(AST *ast, char *type_identifier,
                          TypeCheckContext *ctx) {
  if (type_identifier == NULL) {
    return;
  }

  ttype explicit_type = lookup_explicit_type(type_identifier, ctx);
  ast->type = explicit_type;
}
static void typecheck_extern_function(AST *ast, TypeCheckContext *ctx) {}

/*
 * if two types 'l & 'r are numeric, but 'l is an int & 'r is a float
 *
 * 'cast' the resulting type to be the 'maximum' of the two types, so for
 * instance
 *
 * in the expression 'l + 'r, the value with type 'l will be treated as
 * a float
 **/
static void generate_equations(AST *ast, TypeCheckContext *ctx) {
  if (ast == NULL) {
    return;
  }
  switch (ast->tag) {

  case AST_BINOP: {
    token_type op = ast->data.AST_BINOP.op;
    AST *left = ast->data.AST_BINOP.left;
    AST *right = ast->data.AST_BINOP.right;

    generate_equations(left, ctx);
    generate_equations(right, ctx);

    // avoid equations like Int :: Num or Num :: Int
    if (left->type.tag == T_VAR) {
      push_type_equation(&left->type, &right->type, ctx);
    } else if (right->type.tag == T_VAR) {
      push_type_equation(&right->type, &left->type, ctx);
    }

    if (is_boolean_binop(op)) {
      push_type_equation(&ast->type, &Bool, ctx);
      break;
    }

    if (op == TOKEN_PIPE) {
      push_type_equation(&ast->type, &right->type, ctx);
      break;
    }

    if (is_numeric_type(left->type) && is_numeric_type(right->type)) {
      push_type_equation(&ast->type, _max_type(&left->type, &right->type), ctx);

    } else {
      push_type_equation(&ast->type, &left->type, ctx);
    }

    break;
  }
  case AST_FN_DECLARATION: {

    if (ast->data.AST_FN_DECLARATION.name != NULL &&
        ast->data.AST_FN_DECLARATION.is_extern) {

      char *name = ast->data.AST_FN_DECLARATION.name;
      AST_table_insert(ctx->symbol_table, name, *ast);

      AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
      enter_ttype_scope(ctx);
      generate_equations(prototype_ast, ctx);
      exit_ttype_scope(ctx);

      int length = prototype_ast->data.AST_FN_PROTOTYPE.length + 1;
      ttype *fn_members = malloc(sizeof(ttype) * length);
      for (int i = 0; i < length - 1; i++) {
        AST *param_ast = prototype_ast->data.AST_FN_PROTOTYPE.parameters[i];
        fn_members[i] = lookup_explicit_type(
            param_ast->data.AST_SYMBOL_DECLARATION.type, ctx);
      }

      fn_members[length - 1] =
          lookup_explicit_type(prototype_ast->data.AST_FN_PROTOTYPE.type, ctx);
      ttype *fn_type = malloc(sizeof(ttype));
      *fn_type = tfn(fn_members, length);

      push_type_equation(&ast->type, fn_type, ctx);
      break;
    }
    bool is_anon = ast->data.AST_FN_DECLARATION.name == NULL;

    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
    if (!is_anon) {
      AST_table_insert(ctx->symbol_table, ast->data.AST_FN_DECLARATION.name,
                       *ast);
    }
    enter_ttype_scope(ctx);
    generate_equations(prototype_ast, ctx);

    if (ast->data.AST_FN_DECLARATION.body != NULL) {
      generate_equations(ast->data.AST_FN_DECLARATION.body, ctx);
    }
    exit_ttype_scope(ctx);

    int length = prototype_ast->data.AST_FN_PROTOTYPE.length + 1;
    ttype *fn_members = malloc(sizeof(ttype) * length);
    for (int i = 0; i < length - 1; i++) {
      AST *param_ast = prototype_ast->data.AST_FN_PROTOTYPE.parameters[i];
      fn_members[i] = param_ast->type;
    }
    fn_members[length - 1] = ast->data.AST_FN_DECLARATION.body->type;
    ttype *fn_type = malloc(sizeof(ttype));
    *fn_type = tfn(fn_members, length);

    push_type_equation(&ast->type, fn_type, ctx);

    break;
  }
  case AST_FN_PROTOTYPE: {
    int length = ast->data.AST_FN_PROTOTYPE.length;
    for (int i = 0; i < length; i++) {
      AST *param_ast = ast->data.AST_FN_PROTOTYPE.parameters[i];
      generate_equations(param_ast, ctx);
    }
    break;
  }

  case AST_CALL: {
    if (ast->data.AST_CALL.identifier->tag == AST_MEMBER_ACCESS) {
      typecheck_object_member_call(ast, ctx);
      break;
    }

    char *name = ast->data.AST_CALL.identifier->data.AST_IDENTIFIER.identifier;

    printf("\n-----\nast call %s", name);
    print_ast(*ast, 0);
    print_ttype(ast->type);
    printf("\n-----\n");

    AST func_ast;
    if (AST_table_lookup(ctx->symbol_table, name, &func_ast) != 0) {
      fprintf(stderr, "Error [typecheck]: function %s not found in scope\n",
              name);

      _typecheck_error_flag = 1;
      break;
    };

    int parameters_len = func_ast.data.AST_FN_DECLARATION.prototype->data
                             .AST_FN_PROTOTYPE.length;

    int args_len = ast->data.AST_CALL.parameters->data.AST_TUPLE.length;

    for (int i = 0; i < args_len; i++) {
      AST *param_ast = ast->data.AST_CALL.parameters->data.AST_TUPLE.members[i];
      generate_equations(param_ast, ctx);
      push_type_equation_index(&param_ast->type, &func_ast.type, i, ctx);
    }

    if (args_len == parameters_len) {
      printf("\nreturn type %s\n", name);
      print_ttype(func_ast.type);
      printf("\n");
      push_type_equation_index(&ast->type, &func_ast.type, parameters_len, ctx);
    }

    print_ttype(ast->type);
    printf("\n");

    break;
  }
  case AST_IF_ELSE: {
    generate_equations(ast->data.AST_IF_ELSE.condition, ctx);
    push_type_equation(&ast->data.AST_IF_ELSE.condition->type, &Bool, ctx);

    generate_equations(ast->data.AST_IF_ELSE.then_body, ctx);
    push_type_equation(&ast->type, &ast->data.AST_IF_ELSE.then_body->type, ctx);

    if (ast->data.AST_IF_ELSE.else_body) {
      generate_equations(ast->data.AST_IF_ELSE.else_body, ctx);
      push_type_equation(&ast->data.AST_IF_ELSE.else_body->type,
                         &ast->data.AST_IF_ELSE.then_body->type, ctx);
    }
    break;
  }

  case AST_MATCH: {
    generate_equations(ast->data.AST_MATCH.candidate, ctx);

    for (int i = 0; i < ast->data.AST_MATCH.length; i++) {
      generate_equations(ast->data.AST_MATCH.matches[i], ctx);

      push_type_equation(&ast->type, &ast->data.AST_MATCH.matches[i]->type,
                         ctx);

      push_type_equation(
          &ast->data.AST_MATCH.candidate->type,
          &ast->data.AST_MATCH.matches[i]->data.AST_BINOP.left->type, ctx);
    }

    break;
  }
  case AST_UNOP: {

    token_type op = ast->data.AST_UNOP.op;
    AST *operand = ast->data.AST_UNOP.operand;

    if (is_boolean_unop(op)) {
      push_type_equation(&operand->type, &Bool, ctx);
      push_type_equation(&ast->type, &Bool, ctx);
      break;
    }
    break;
  }

  case AST_IDENTIFIER: {

    char *name = ast->data.AST_IDENTIFIER.identifier;
    AST sym_ast;

    if (AST_table_lookup(ctx->symbol_table, name, &sym_ast) == 0) {
      // TODO: ew
      ttype *t = malloc(sizeof(ttype));
      *t = sym_ast.type;

      push_type_equation(&ast->type, t, ctx);
    } else {
      fprintf(stderr, "Error [typecheck]: symbol %s not found in this scope\n",
              name);
      // _typecheck_error_flag = 1;
      // return;
    };

    break;
  }

  case AST_SYMBOL_DECLARATION: {
    char *name = ast->data.AST_SYMBOL_DECLARATION.identifier;

    if (ast->data.AST_SYMBOL_DECLARATION.type != NULL) {
      ttype t =
          lookup_explicit_type(ast->data.AST_SYMBOL_DECLARATION.type, ctx);
      push_type_equation(&ast->type, &t, ctx);
    }

    AST_table_insert(ctx->symbol_table, name, *ast);
    break;
  }
  case AST_ASSIGNMENT: {
    char *name = ast->data.AST_ASSIGNMENT.identifier;
    generate_equations(ast->data.AST_ASSIGNMENT.expression, ctx);

    AST sym_ast;
    if (AST_table_lookup(ctx->symbol_table, name, &sym_ast) == 0) {
      // sym already exists - this is a reassignment
      push_type_equation(&(sym_ast.type),
                         &ast->data.AST_ASSIGNMENT.expression->type, ctx);
      break;
    }
    // if (ast->data.AST_ASSIGNMENT.type != NULL) {
    //   ttype t = lookup_explicit_type(ast->data.AST_ASSIGNMENT.type, ctx);
    //   push_type_equation(&ast->type, &t);
    // }
    assign_explicit_type(ast, ast->data.AST_ASSIGNMENT.type, ctx);

    AST_table_insert(ctx->symbol_table, name, *ast);

    ttype t = ast->data.AST_ASSIGNMENT.expression->type;

    push_type_equation(&ast->type, &t, ctx);
    break;
  }
  case AST_TYPE_DECLARATION: {
    char *name = ast->data.AST_TYPE_DECLARATION.name;

    AST sym_ast;
    if (AST_table_lookup(ctx->symbol_table, name, &sym_ast) == 0) {
      // sym already exists - this is a reassignment
      // push_type_equation(&sym_ast->type,
      //                    &ast->data.AST_ASSIGNMENT.expression->type);
      break;
    }

    ttype t =
        compute_type_expression(ast->data.AST_TYPE_DECLARATION.type_expr, ctx);

    ast->type.tag = t.tag;
    ast->type.as = t.as;

    AST_table_insert(ctx->symbol_table, name, *ast);
    break;
  }
  case AST_MAIN: {
    generate_equations(ast->data.AST_MAIN.body, ctx);
    break;
  }
  case AST_STATEMENT_LIST: {
    ttype t;
    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      AST *stmt_ast = ast->data.AST_STATEMENT_LIST.statements[i];
      generate_equations(stmt_ast, ctx);
      t = stmt_ast->type;
    }
    ast->type = t;
    break;
  }
  case AST_TUPLE: {
    int len = ast->data.AST_TUPLE.length;
    ttype *member_types = malloc(sizeof(ttype) * len);
    ttype tuple_type;

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_TUPLE.members[i];
      generate_equations(member_ast, ctx);
      member_types[i] = member_ast->type;
    }
    tuple_type = ttuple(member_types, len);

    push_type_equation(&ast->type, &tuple_type, ctx);
    break;
  }

  case AST_STRUCT: {
    int len = ast->data.AST_STRUCT.length;
    ttype *member_types = malloc(sizeof(ttype) * len);
    ttype *tuple_type = malloc(sizeof(ttype));
    struct_member_metadata *md = malloc(sizeof(struct_member_metadata) * len);

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_STRUCT.members[i];
      generate_equations(member_ast->data.AST_ASSIGNMENT.expression, ctx);

      ttype mem_type = member_ast->data.AST_ASSIGNMENT.expression->type;
      member_types[i] = mem_type;
      md[i] = (struct_member_metadata){
          strdup(member_ast->data.AST_ASSIGNMENT.identifier), i};
    }
    *tuple_type = tstruct(member_types, md, len);
    push_type_equation(&ast->type, tuple_type, ctx);

    break;
  }

  case AST_MEMBER_ACCESS: {
    AST object;
    char *obj_identifier =
        ast->data.AST_MEMBER_ACCESS.object->data.AST_IDENTIFIER.identifier;

    if (AST_table_lookup(
            ctx->symbol_table,
            ast->data.AST_MEMBER_ACCESS.object->data.AST_IDENTIFIER.identifier,
            &object) == 0) {

      while (object.type.tag == T_VAR) {
        AST_table_lookup(ctx->symbol_table, object.type.as.T_VAR.name, &object);
      }

      if (object.type.tag != T_STRUCT) {
        fprintf(stderr,
                "Error [typecheck]: object %s does not have named members",
                obj_identifier);
        break;
      }

      char *member_name = ast->data.AST_MEMBER_ACCESS.member_name;

      ttype obj_type = object.type;
      for (int i = 0; i < object.type.as.T_STRUCT.length; i++) {
        if (strcmp(member_name, obj_type.as.T_STRUCT.struct_metadata[i].name) ==
            0) {
          int member_idx = obj_type.as.T_STRUCT.struct_metadata[i].index;
          ttype *t = malloc(sizeof(ttype));
          *t = obj_type.as.T_STRUCT.members[member_idx];

          push_type_equation(&ast->type, t, ctx);

          break;
        }
      }
    }

    break;
  }

  case AST_IMPORT: {
    const char *module_filename = ast->data.AST_IMPORT.module_name;
    char *mod_name = basename(module_filename);
    if (has_extension(module_filename, ".ylc")) {
      remove_extension(mod_name);
    }

    char resolved_path[PATH_MAX];
    resolve_path(dirname(ctx->module_path), module_filename, resolved_path);
    AST *mod_ast = get_module(resolved_path, ctx);
    ast->data.AST_IMPORT.module_ast = mod_ast;
    ast->type = mod_ast->type;

    if (has_extension(module_filename, ".ylc")) {
      AST_table_insert(ctx->symbol_table, mod_name, *ast);
    }

    break;
  }
  case AST_IMPORT_LIB: {

    break;
  }
  case AST_EXPRESSION:
  case AST_STATEMENT:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_VAR_ARG:
  default:
    break;
  }
  if (_typecheck_error_flag == 1) {
    printf("Typecheck error");
    print_ast(*ast, 0);
    return;
  }
}

void print_env(TypeEnv *env) {
  printf("env: (");
  for (int i = 0; i < TABLE_SIZE; i++) {
    ttype_Symbol *tsym = env->entries[i];
    if (tsym != NULL) {
      printf("%s [", tsym->key);
      print_ttype(tsym->value);
      printf("], ");
    }
  }

  printf(")\n");
}

void update_expression_types(AST *ast, TypeEnv *env) {
  if (ast == NULL) {
    return;
  }

  switch (ast->tag) {
  case AST_BINOP: {
    update_expression_types(ast->data.AST_BINOP.left, env);
    update_expression_types(ast->data.AST_BINOP.right, env);
    break;
  }
  case AST_FN_DECLARATION: {
    update_expression_types(ast->data.AST_FN_DECLARATION.prototype, env);
    update_expression_types(ast->data.AST_FN_DECLARATION.body, env);
    break;
  }
  case AST_FN_PROTOTYPE: {
    for (int i = 0; i < ast->data.AST_FN_PROTOTYPE.length; i++) {
      update_expression_types(ast->data.AST_FN_PROTOTYPE.parameters[i], env);
    }
    break;
  }
  case AST_SYMBOL_DECLARATION: {
    update_expression_types(ast->data.AST_SYMBOL_DECLARATION.expression, env);
    break;
  }
  case AST_STATEMENT_LIST: {

    for (int i = 0; i < ast->data.AST_STATEMENT_LIST.length; i++) {
      update_expression_types(ast->data.AST_STATEMENT_LIST.statements[i], env);
    }
    break;
  }
  case AST_MAIN: {
    update_expression_types(ast->data.AST_MAIN.body, env);
    break;
  }
  case AST_IF_ELSE: {
    update_expression_types(ast->data.AST_IF_ELSE.condition, env);
    update_expression_types(ast->data.AST_IF_ELSE.then_body, env);
    update_expression_types(ast->data.AST_IF_ELSE.else_body, env);
    break;
  }

  case AST_MATCH: {
    update_expression_types(ast->data.AST_MATCH.candidate, env);

    for (int i = 0; i < ast->data.AST_MATCH.length; i++) {
      update_expression_types(ast->data.AST_MATCH.matches[i], env);
    }
    break;
  }
  case AST_CALL: {
    update_expression_types(ast->data.AST_CALL.identifier, env);
    update_expression_types(ast->data.AST_CALL.parameters, env);
    break;
  }
  case AST_TUPLE: {
    for (int i = 0; i < ast->data.AST_TUPLE.length; i++) {
      update_expression_types(ast->data.AST_TUPLE.members[i], env);
    }

    break;
  }
  case AST_ASSIGNMENT: {
    update_expression_types(ast->data.AST_ASSIGNMENT.expression, env);
    break;
  }

  case AST_UNOP: {
    update_expression_types(ast->data.AST_UNOP.operand, env);
    break;
  }
  case AST_MEMBER_ACCESS: {
    update_expression_types(ast->data.AST_MEMBER_ACCESS.object, env);
    break;
  }
  case AST_IDENTIFIER:
  case AST_INTEGER:
  case AST_NUMBER:
  case AST_BOOL:
  case AST_STRING:
  case AST_ADD:
  case AST_SUBTRACT:
  case AST_MUL:
  case AST_DIV:
  case AST_EXPRESSION:
  case AST_STATEMENT:
  case AST_STRUCT:
  case AST_TYPE_DECLARATION:
  case AST_MEMBER_ASSIGNMENT:
  case AST_INDEX_ACCESS:
  case AST_VAR_ARG:
  default:
    break;
  }

  ttype lookup;

  if (ast->type.tag == T_VAR &&
      ttype_env_lookup(env, ast->type.as.T_VAR.name, &lookup) == 0) {
    printf("\nlookup ast call??: %s ", ast->type.as.T_VAR.name);
    print_ttype(lookup);
    ttype *final = follow_links(env, &lookup);
    ast->type = *final;
    if (ast->tag == AST_CALL) {
      printf("update: ");
      print_ast(*ast, 0);
      printf(" :: ");
      print_ttype(ast->type);
      printf("\n");
    }
  }

  return;
}

int typecheck_in_ctx(AST *ast, const char *module_path, TypeCheckContext *ctx) {
  _typecheck_error_flag = 0;
  ctx->module_path = module_path;

  ctx->type_equations.equations = calloc(sizeof(TypeEquation), MAX_TEQ_LIST);
  ctx->type_equations.length = 0;
  generate_equations(ast, ctx);
  for (int i = 0; i < ctx->type_equations.length; i++) {
    unify(ctx->type_equations.equations[i], &ctx->type_env);
  }

  if (_typecheck_error_flag) {
    return 1;
  }

  update_expression_types(ast, &ctx->type_env);
  if (_typecheck_error_flag) {
    return 1;
  }
  return _typecheck_error_flag;
}

ttype get_last_entered_type(AST *ast) {
  int last = ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.length - 1;
  ttype last_type_expr =
      ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[last]->type;

  return last_type_expr;
}
void print_last_entered_type(AST *ast) {
  ttype last_type_expr = get_last_entered_type(ast);
  print_ttype(last_type_expr);
}
