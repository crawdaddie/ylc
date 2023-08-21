#include "typecheck.h"
#include "codegen/codegen.h"
#include "generic_symbol_table.h"
#include "modules.h"
#include "paths.h"
#include "string.h"
#include "symbol_table.h"
#include "types.h"
#include <dlfcn.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
// #define _TYPECHECK_DBG

INIT_SYM_TABLE(AST);

static int _typecheck_error_flag = 0;

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
  case T_INT8: {
    printf("Int8");
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

  case T_TUPLE: {
    int length = type.as.T_TUPLE.length;
    printf("(");
    for (int i = 0; i < length; i++) {
      print_ttype(type.as.T_TUPLE.members[i]);
      if (i < length - 1)
        printf(" * ");
    }
    printf(")");
    break;
  }

  case T_STRUCT: {
    int length = type.as.T_STRUCT.length;
    printf("(");
    for (int i = 0; i < length; i++) {
      struct_member_metadata md = type.as.T_STRUCT.struct_metadata[i];
      if (md.index == -1) {
        continue;
      }

      printf("%s=", type.as.T_STRUCT.struct_metadata[i].name);
      print_ttype(type.as.T_STRUCT.members[i]);
      if (i < length - 1)
        printf(" * ");
    }
    printf(")");
    break;
  }
  case T_PTR: {
    printf("*");
    print_ttype(*type.as.T_PTR.item);
    break;
  }
  }
}
static void print_type_equation(TypeEquation type_equation) {
  print_ttype(*type_equation.left);
  printf(" :: ");
  print_ttype(*type_equation.right);
  printf("\n");
}

static ttype Bool = {T_BOOL};
static ttype Str = {T_STR};
static ttype Int = {T_INT};
static ttype Num = {T_NUM};

static int MAX_TEQ_LIST = 32;
void push_type_equation(TypeEquationsList *list, ttype *dependent_type,
                        ttype *type) {
  TypeEquation eq = {dependent_type, type};

  list->length++;
  if (list->length >= MAX_TEQ_LIST) {
    list->equations = realloc(
        list->equations, sizeof(TypeEquation) * list->length + MAX_TEQ_LIST);
    MAX_TEQ_LIST += 8;
  }
  list->equations[list->length - 1] = eq;
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

  AST *sym;
  if (AST_table_lookup(ctx->symbol_table, type_identifier, sym) == 0) {
    return sym->type;
  }
}

ttype get_fn_return_type(ttype fn_type) {
  int len = fn_type.as.T_FN.length;
  return fn_type.as.T_FN.members[len - 1];
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

    push_type_equation(&ctx->type_equations, &ast->type, return_type);
    return;
  };
}

static ttype *compute_type_expression(AST *ast, TypeCheckContext *ctx) {
  switch (ast->tag) {

  case AST_TUPLE: {
    int len = ast->data.AST_TUPLE.length;
    ttype *member_types = malloc(sizeof(ttype) * len);
    ttype *tuple_type = malloc(sizeof(ttype));

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_TUPLE.members[i];
      ttype t =
          lookup_explicit_type(member_ast->data.AST_IDENTIFIER.identifier, ctx);

      member_types[i] = t;
    }
    *tuple_type = ttuple(member_types, len);
    return tuple_type;
  }
  case AST_STRUCT: {

    int len = ast->data.AST_STRUCT.length;
    ttype *member_types = malloc(sizeof(ttype) * len);
    ttype *tuple_type = malloc(sizeof(ttype));
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
    *tuple_type = tstruct(member_types, metadata, len);

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
      push_type_equation(&ctx->type_equations, &left->type, &right->type);
    } else if (right->type.tag == T_VAR) {
      push_type_equation(&ctx->type_equations, &right->type, &left->type);
    }

    if (is_boolean_binop(op)) {
      push_type_equation(&ctx->type_equations, &ast->type, &Bool);
      break;
    }

    if (op == TOKEN_PIPE) {
      push_type_equation(&ctx->type_equations, &ast->type, &right->type);
      break;
    }

    if (is_numeric_type(left->type) && is_numeric_type(right->type)) {
      push_type_equation(&ctx->type_equations, &ast->type,
                         _max_type(&left->type, &right->type));

    } else {
      push_type_equation(&ctx->type_equations, &ast->type, &left->type);
    }

    break;
  }
  case AST_FN_DECLARATION: {
    char *name = ast->data.AST_FN_DECLARATION.name;

    if (name && ast->data.AST_FN_DECLARATION.is_extern) {
      AST_table_insert(ctx->symbol_table, name, *ast);

      AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
      enter_ttype_scope(ctx);
      generate_equations(prototype_ast, ctx);
      exit_ttype_scope(ctx);

      int length = prototype_ast->data.AST_FN_PROTOTYPE.length + 1;
      ttype *fn_members = malloc(sizeof(ttype) * length);
      for (int i = 0; i < length - 1; i++) {
        AST *param_ast = prototype_ast->data.AST_FN_PROTOTYPE.parameters[i];
        fn_members[i] = param_ast->type;
      }

      fn_members[length - 1] =
          lookup_explicit_type(prototype_ast->data.AST_FN_PROTOTYPE.type, ctx);
      ttype *fn_type = malloc(sizeof(ttype));
      *fn_type = tfn(fn_members, length);

      push_type_equation(&ctx->type_equations, &ast->type, fn_type);
      break;
    }

    AST *prototype_ast = ast->data.AST_FN_DECLARATION.prototype;
    AST_table_insert(ctx->symbol_table, name, *ast);
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

    push_type_equation(&ctx->type_equations, &ast->type, fn_type);

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

    AST func_ast;
    if (AST_table_lookup(ctx->symbol_table, name, &func_ast) != 0) {
      fprintf(stderr, "Error [typecheck]: function %s not found in scope\n",
              name);

      _typecheck_error_flag = 1;
      break;
    };

    int fn_type_len = func_ast.data.AST_FN_DECLARATION.prototype->data
                          .AST_FN_PROTOTYPE.length +
                      1;

    ttype *fn_type_list = malloc(sizeof(ttype) * (fn_type_len));

    for (int i = 0; i < fn_type_len - 1; i++) {
      AST *param_ast = ast->data.AST_CALL.parameters->data.AST_TUPLE.members[i];
      generate_equations(param_ast, ctx);
      fn_type_list[i] = param_ast->type;
    }
    fn_type_list[fn_type_len - 1] =
        ast->type; // type of call must be return type
    ttype *implied_fn_type = malloc(sizeof(ttype));
    *implied_fn_type = tfn(fn_type_list, fn_type_len);

    push_type_equation(&ctx->type_equations, &func_ast.type, implied_fn_type);

    break;
  }
  case AST_IF_ELSE: {
    generate_equations(ast->data.AST_IF_ELSE.condition, ctx);
    push_type_equation(&ctx->type_equations,
                       &ast->data.AST_IF_ELSE.condition->type, &Bool);

    generate_equations(ast->data.AST_IF_ELSE.then_body, ctx);
    push_type_equation(&ctx->type_equations, &ast->type,
                       &ast->data.AST_IF_ELSE.then_body->type);

    if (ast->data.AST_IF_ELSE.else_body) {
      generate_equations(ast->data.AST_IF_ELSE.else_body, ctx);
      push_type_equation(&ctx->type_equations,
                         &ast->data.AST_IF_ELSE.else_body->type,
                         &ast->data.AST_IF_ELSE.then_body->type);
    }
    break;
  }

  case AST_MATCH: {
    generate_equations(ast->data.AST_MATCH.candidate, ctx);

    for (int i = 0; i < ast->data.AST_MATCH.length; i++) {
      generate_equations(ast->data.AST_MATCH.matches[i], ctx);

      push_type_equation(&ctx->type_equations, &ast->type,
                         &ast->data.AST_MATCH.matches[i]->type);

      push_type_equation(
          &ctx->type_equations, &ast->data.AST_MATCH.candidate->type,
          &ast->data.AST_MATCH.matches[i]->data.AST_BINOP.left->type);
    }

    break;
  }
  case AST_UNOP: {

    token_type op = ast->data.AST_UNOP.op;
    AST *operand = ast->data.AST_UNOP.operand;

    if (is_boolean_unop(op)) {
      push_type_equation(&ctx->type_equations, &operand->type, &Bool);
      push_type_equation(&ctx->type_equations, &ast->type, &Bool);
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

      push_type_equation(&ctx->type_equations, &ast->type, t);
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
      push_type_equation(&ctx->type_equations, &ast->type, &t);
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
      push_type_equation(&ctx->type_equations, &(sym_ast.type),
                         &ast->data.AST_ASSIGNMENT.expression->type);
      break;
    }
    // if (ast->data.AST_ASSIGNMENT.type != NULL) {
    //   ttype t = lookup_explicit_type(ast->data.AST_ASSIGNMENT.type, ctx);
    //   push_type_equation(&ctx->type_equations, &ast->type, &t);
    // }
    assign_explicit_type(ast, ast->data.AST_ASSIGNMENT.type, ctx);

    AST_table_insert(ctx->symbol_table, name, *ast);
    ttype *t = malloc(sizeof(ttype));
    *t = ast->data.AST_ASSIGNMENT.expression->type;

    push_type_equation(&ctx->type_equations, &ast->type, t);
    break;
  }
  case AST_TYPE_DECLARATION: {
    char *name = ast->data.AST_TYPE_DECLARATION.name;

    AST sym_ast;
    if (AST_table_lookup(ctx->symbol_table, name, &sym_ast) == 0) {
      // sym already exists - this is a reassignment
      // push_type_equation(&ctx->type_equations, &sym_ast->type,
      //                    &ast->data.AST_ASSIGNMENT.expression->type);
      break;
    }
    ttype *t =
        compute_type_expression(ast->data.AST_TYPE_DECLARATION.type_expr, ctx);

    // sprintf(ast->type.as.T_VAR.name, "%s", name);
    ast->type = *t;

    push_type_equation(&ctx->type_equations, &ast->type, t);

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
    ttype *tuple_type = malloc(sizeof(ttype));

    for (int i = 0; i < len; i++) {
      AST *member_ast = ast->data.AST_TUPLE.members[i];
      generate_equations(member_ast, ctx);
      member_types[i] = member_ast->type;
    }
    *tuple_type = ttuple(member_types, len);
    push_type_equation(&ctx->type_equations, &ast->type, tuple_type);

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
    push_type_equation(&ctx->type_equations, &ast->type, tuple_type);

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

          push_type_equation(&ctx->type_equations, &ast->type, t);

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
    AST *mod_ast = get_module(resolved_path);
    ast->data.AST_IMPORT.module_ast = mod_ast;
    ast->type = mod_ast->type;

    // assign_explicit_type(ast, ast->data.AST_ASSIGNMENT.type, ctx);

    if (has_extension(module_filename, ".ylc")) {
      // remove_extension(mod_name);

      // struct AST_IMPORT import = ast->data.AST_IMPORT;
      // import.module_ast = mod_ast;
      // AST *import_ast = malloc(sizeof(AST));
      // import_ast->tag = AST_IMPORT;
      // import_ast->data.AST_IMPORT = import;
      // import_ast->type = mod_ast->type;

      AST_table_insert(ctx->symbol_table, mod_name, *ast);
      // printf("table insert mod [%s] mod_name\n");
      // ast->tag = AST_ASSIGNMENT;
      // ast->data.AST_ASSIGNMENT = (struct AST_ASSIGNMENT){
      //     .identifier = mod_name, .expression = ast};

      // push_type_equation(&ctx->type_equations, &ast->type, &mod_ast->type);
      // push_type_equation(&ctx->type_equations,
      //                    &ast->data.AST_ASSIGNMENT.expression->type,
      //                    &mod_ast->type);
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
}
INIT_SYM_TABLE(ttype);

static void add_type_to_env(TypeEnv *env, ttype *left, ttype *right) {
  if (left->tag != T_VAR) {
    return;
  }
  ttype_env_insert(env, left->as.T_VAR.name, *right);
}

static void print_env(TypeEnv *env) {
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
ttype *follow_links(TypeEnv *env, ttype *rtype) {
  ttype *lookup = rtype;
  while (lookup->tag == T_VAR &&
         ttype_env_lookup(env, lookup->as.T_VAR.name, lookup) == 0) {
  }
  return lookup;
}
/*
 * Unification
 *
 * step through the list of type equations and apply substitutions where
 * possible
 **/
void unify(ttype *left, ttype *right, TypeEnv *env);
/*
 * Does the type var left occur anywhere inside right?
 *
 * Variables in r are looked up in env and the check is applied
 * recursively.
 * */
bool occurs_check(ttype *left, ttype *right, TypeEnv *env) {
  if (left == right) {
    return true;
  }

  ttype lookup;

  if (right->tag == T_VAR &&
      ttype_env_lookup(env, right->as.T_VAR.name, &lookup) == 0) {
    return occurs_check(left, &lookup, env);
  }

  if (right->tag == T_FN) {
    for (int i = 0; i < right->as.T_FN.length; i++) {
      if (occurs_check(left, right->as.T_FN.members + i, env)) {
        return true;
      }
    }
  }
  return false;
}

/*
 * Unifies type variable left with type var right, using env to find
 * substitutions.
 *
 * updates env or returns early.
 * */
void unify_variable(ttype *left, ttype *right, TypeEnv *env) {

  ttype lookup;
  if (ttype_env_lookup(env, left->as.T_VAR.name, &lookup) == 0) {
    return unify(&lookup, right, env);
  }

  if (right->tag == T_VAR &&
      ttype_env_lookup(env, right->as.T_VAR.name, &lookup) == 0) {
    return unify(left, &lookup, env);
  }

  if (occurs_check(left, right, env)) {
    return;
  }
  return add_type_to_env(env, left, right);
}

bool types_equal(ttype *l, ttype *r) {

  if (l == r) {
    return true;
  }

  if (l->tag != r->tag) {
    return false;
  }
  if (l->tag == r->tag) {
    return true;
  }

  if (l->tag == T_TUPLE) {
    for (int i = 0; i < l->as.T_TUPLE.length; i++) {
      if (!types_equal(&l->as.T_TUPLE.members[i], &r->as.T_TUPLE.members[i])) {
        return false;
      }
    }
    return true;
  }

  if (l->tag == T_STRUCT) {
    for (int i = 0; i < l->as.T_STRUCT.length; i++) {
      if (!types_equal(&l->as.T_STRUCT.members[i],
                       &r->as.T_STRUCT.members[i])) {
        return false;
      }
    }
    return true;
  }
  if (l->tag == T_VAR && strcmp(l->as.T_VAR.name, r->as.T_VAR.name) == 0) {
    return true;
  }

  if (l->tag == T_VAR && r->tag == T_VAR &&
      strcmp(l->as.T_VAR.name, r->as.T_VAR.name) != 0) {
    return false;
  }
  return false;
}
void unify_functions(ttype *left, ttype *right, TypeEnv *env) {

  if (left->as.T_FN.length != right->as.T_FN.length) {
    printf("unify funcs\n");
    print_type_equation((TypeEquation){left, right});

    return;
  }

  for (int i = 0; i < left->as.T_FN.length; i++) {
    ttype *l_fn_mem = left->as.T_FN.members + i;
    ttype *r_fn_mem = right->as.T_FN.members + i;

    if (types_equal(l_fn_mem, r_fn_mem)) {
      continue;
    }

    ttype mem_lookup;

    if (l_fn_mem->tag == T_VAR &&
        ttype_env_lookup(env, l_fn_mem->as.T_VAR.name, &mem_lookup) != 0) {
      add_type_to_env(env, l_fn_mem, r_fn_mem);
    }

    if (r_fn_mem->tag == T_VAR) {
      printf("r type\n");
      print_ttype(*r_fn_mem);
    }
    left->as.T_FN.members[i] = right->as.T_FN.members[i];
    unify(l_fn_mem, r_fn_mem, env);
  }

  return;
}
void unify_structs(ttype *left, ttype *right, TypeEnv *env) {
  // print_type_equation((TypeEquation){left, right});

  if (left->as.T_STRUCT.length != right->as.T_STRUCT.length) {

    fprintf(stderr,
            "Error [typecheck] type contradiction cannot set tuple with length "
            "%d to a tuple type with length %d\n",
            left->as.T_STRUCT.length, right->as.T_STRUCT.length);
    _typecheck_error_flag = 1;
    return;
  }

  struct_member_metadata lmd;
  struct_member_metadata rmd;

  for (int i = 0; i < left->as.T_STRUCT.length; i++) {
    lmd = left->as.T_STRUCT.struct_metadata[i];
    rmd = right->as.T_STRUCT.struct_metadata[i];

    if (strcmp(lmd.name, rmd.name) != 0 || lmd.index != rmd.index) {
      return;
    }
  }

  for (int i = 0; i < left->as.T_STRUCT.length; i++) {
    ttype *l_fn_mem = left->as.T_STRUCT.members + i;
    ttype *r_fn_mem = right->as.T_STRUCT.members + i;

    if (types_equal(l_fn_mem, r_fn_mem)) {
      continue;
    } else {
      fprintf(stderr,
              "Error [typecheck] type contradiction cannot set struct type tag "
              "%d to "
              "type tag %d\n",
              l_fn_mem->tag, r_fn_mem->tag);
      _typecheck_error_flag = 1;
      return;
    }

    ttype mem_lookup;

    if (l_fn_mem->tag == T_VAR &&
        ttype_env_lookup(env, l_fn_mem->as.T_VAR.name, &mem_lookup) != 0) {
      add_type_to_env(env, l_fn_mem, r_fn_mem);
    }

    left->as.T_STRUCT.members[i] = right->as.T_FN.members[i];

    unify(l_fn_mem, r_fn_mem, env);
  }

  return;
}
void unify_tuples(ttype *left, ttype *right, TypeEnv *env) {

  if (left->as.T_TUPLE.length != right->as.T_TUPLE.length) {

    fprintf(stderr,
            "Error [typecheck] type contradiction cannot set tuple with length "
            "%d to a tuple type with length %d\n",
            left->as.T_TUPLE.length, right->as.T_TUPLE.length);
    _typecheck_error_flag = 1;
    return;
  }

  for (int i = 0; i < left->as.T_TUPLE.length; i++) {
    ttype *l_fn_mem = left->as.T_TUPLE.members + i;
    ttype *r_fn_mem = right->as.T_TUPLE.members + i;

    if (types_equal(l_fn_mem, r_fn_mem)) {

      continue;
    } else {
      // printf("---");
      fprintf(stderr,
              "Error [typecheck] type contradiction cannot unify tuples\n"),
          _typecheck_error_flag = 1;
      return;
    }

    ttype mem_lookup;

    if (l_fn_mem->tag == T_VAR &&
        ttype_env_lookup(env, l_fn_mem->as.T_VAR.name, &mem_lookup) != 0) {
      add_type_to_env(env, l_fn_mem, r_fn_mem);
    }

    left->as.T_TUPLE.members[i] = right->as.T_FN.members[i];

    unify(l_fn_mem, r_fn_mem, env);
  }

  return;
}

void unify_compound_types(struct T_TUPLE left, struct T_TUPLE right,
                          TypeEnv *env) {

  if (left.length != right.length) {

    fprintf(stderr,
            "Error [typecheck] type contradiction cannot set tuple / fn / "
            "struct with length "
            "%d to a tuple / fn / struct type with length %d\n",
            left.length, right.length);
    _typecheck_error_flag = 1;
    return;
  }

  for (int i = 0; i < left.length; i++) {
    ttype *l_fn_mem = left.members + i;
    ttype *r_fn_mem = right.members + i;

    if (types_equal(l_fn_mem, r_fn_mem)) {
      continue;
    } else {
      fprintf(stderr,
              "Error [typecheck] type contradiction cannot set type tag %d to "
              "type tag %d\n",
              l_fn_mem->tag, r_fn_mem->tag);
      _typecheck_error_flag = 1;
      return;
    }

    ttype mem_lookup;

    if (l_fn_mem->tag == T_VAR &&
        ttype_env_lookup(env, l_fn_mem->as.T_VAR.name, &mem_lookup) != 0) {
      add_type_to_env(env, l_fn_mem, r_fn_mem);
    }

    left.members[i] = right.members[i];

    unify(l_fn_mem, r_fn_mem, env);
  }

  return;
}

/*
 * Unify two types left and left, with substitution environment.
 **/
void unify(ttype *left, ttype *right, TypeEnv *env) {

  if (left->tag == T_VAR && right->tag == T_VAR) {
    return unify_variable(left, right, env);
  }

  if (left->tag == T_FN && right->tag == T_FN) {
    return unify_functions(left, right, env);
  }

  if (left->tag == T_TUPLE && right->tag == T_TUPLE) {
    return unify_tuples(left, right, env);
  }

  if (left->tag == T_STRUCT && right->tag == T_STRUCT) {
    return unify_structs(left, right, env);
  }

  if (types_equal(left, right)) {
    return;
  }

  // coerce numeric types??
  ttype lookup;
  if (left->tag == T_VAR &&
      ttype_env_lookup(env, left->as.T_VAR.name, &lookup) == 0) {

    if (is_numeric_type(lookup) && is_numeric_type(*right) &&
        right->tag <= lookup.tag) {
      // ignore implicit rules that cast Float down to Int /
      // Int8
      return;
    }
  }

  if (left->tag != T_VAR && right->tag != T_VAR && left->tag != right->tag) {
    fprintf(stderr,
            "Error [typecheck] type contradiction type tag %d != type tag %d\n",
            left->tag, right->tag);
    _typecheck_error_flag = 1;

    return;
  }

  if (left->tag == T_VAR && right->tag != T_VAR && right->tag != T_FN) {

    ttype lookup;
    if (ttype_env_lookup(env, left->as.T_VAR.name, &lookup) == 0) {
      unify(&lookup, right, env);
    }
    add_type_to_env(env, left, right);
  }

  if (left->tag == T_VAR && right->tag == T_FN) {
    ttype existing_func_lookup;
    if (ttype_env_lookup(env, left->as.T_VAR.name, &existing_func_lookup) ==
        0) {
      unify(right, &existing_func_lookup, env);
    }
    for (int i = 0; i < right->as.T_FN.length; i++) {
      ttype *fn_member = right->as.T_FN.members + i;
      ttype *fn_member_lookup = fn_member;

      while (fn_member_lookup->tag == T_VAR &&
             ttype_env_lookup(env, fn_member_lookup->as.T_VAR.name,
                              fn_member_lookup) == 0) {
      }
      right->as.T_FN.members[i] = *fn_member_lookup;
    }
  }

  if (left->tag == T_VAR && right->tag == T_TUPLE) {
    ttype existing_tuple;

    if (ttype_env_lookup(env, left->as.T_VAR.name, &existing_tuple) == 0) {
      unify(right, &existing_tuple, env);
    }

    for (int i = 0; i < right->as.T_TUPLE.length; i++) {
      ttype *tuple_member = right->as.T_TUPLE.members + i;
      ttype *tuple_member_lookup = tuple_member;

      while (tuple_member_lookup->tag == T_VAR &&
             ttype_env_lookup(env, tuple_member_lookup->as.T_VAR.name,
                              tuple_member_lookup) == 0) {
      }
      right->as.T_TUPLE.members[i] = *tuple_member_lookup;
    }
  }

  if (left->tag == T_VAR) {
    ttype lookup;
    if (ttype_env_lookup(env, left->as.T_VAR.name, &lookup) == 0) {
      unify(&lookup, right, env);
    }
    return unify_variable(left, right, env);
  }

  if (right->tag == T_VAR) {
    ttype lookup;
    if (ttype_env_lookup(env, right->as.T_VAR.name, &lookup) == 0) {
      unify(&lookup, left, env);
    }
    return unify_variable(right, left, env);
  }
}

void unify_equations(TypeEquation *equations, int len, TypeEnv *env) {
  if (len == 0) {
    return;
  }
  TypeEquation eq = *equations;
#ifdef _TYPECHECK_DBG
  print_type_equation(eq);
#endif
  unify(eq.left, eq.right, env);

  if (_typecheck_error_flag == 1) {
    return;
  }

#ifdef _TYPECHECK_DBG
  printf("\033[1;31m");
  print_env(env);
  printf("\033[1;0m");
#endif

  if (len > 1) {
    unify_equations(equations + 1, len - 1, env);
  }
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
    ttype *final = follow_links(env, &lookup);
    ast->type = *final;
  }
  return;
}

int typecheck(AST *ast, const char *module_path) {
  _typecheck_error_flag = 0;
  TypeCheckContext ctx;
  ctx.module_path = module_path;

  AST_SymbolTable symbol_table = {}; // init to zero
  symbol_table.current_frame_index = 0;
  ctx.symbol_table = &symbol_table;

  ctx.type_equations.equations = calloc(sizeof(TypeEquation), MAX_TEQ_LIST);
  ctx.type_equations.length = 0;
  generate_equations(ast, &ctx);

  if (_typecheck_error_flag) {
    return 1;
  }
#ifdef _TYPECHECK_DBG
  // printf("eqs---\n");
  // for (int i = 0; i < ctx.type_equations.length; i++) {
  //   print_type_equation(ctx.type_equations.equations[i]);
  // }
  // printf("---\n");
  printf("typecheck %s\n", module_path);
#endif

  TypeEnv env = {};
  TypeEquation *eqs =
      ctx.type_equations.equations; // new pointer to eqs we can manipulate
  unify_equations(eqs, ctx.type_equations.length, &env);

  if (_typecheck_error_flag) {
    return 1;
  }

  update_expression_types(ast, &env);

  if (_typecheck_error_flag) {
    return 1;
  }

  free(ctx.type_equations.equations); // free original pointer to eqs

  return _typecheck_error_flag;
}

int typecheck_in_ctx(AST *ast, const char *module_path, TypeCheckContext *ctx) {
  _typecheck_error_flag = 0;
  ctx->module_path = module_path;

  ctx->type_equations.equations = calloc(sizeof(TypeEquation), MAX_TEQ_LIST);
  ctx->type_equations.length = 0;
  generate_equations(ast, ctx);

  if (_typecheck_error_flag) {
    return 1;
  }
#ifdef _TYPECHECK_DBG
  printf("eqs---\n");
  for (int i = 0; i < ctx->type_equations.length; i++) {
    print_type_equation(ctx->type_equations.equations[i]);
  }
  printf("---\n");
  printf("typecheck %s\n", module_path);
#endif

  TypeEquation *eqs =
      ctx->type_equations.equations; // new pointer to eqs we can manipulate
  unify_equations(eqs, ctx->type_equations.length, &ctx->type_env);

  if (_typecheck_error_flag) {
    return 1;
  }

  update_expression_types(ast, &ctx->type_env);

#ifdef _TYPECHECK_DBG
  // printf("eqs---\n");
  // for (int i = 0; i < ctx.type_equations.length; i++) {
  //   print_type_equation(ctx.type_equations.equations[i]);
  // }
  // printf("---\n");
  // print_env(&ctx->type_env);
#endif
  if (_typecheck_error_flag) {
    return 1;
  }

  return _typecheck_error_flag;
}

void print_last_entered_type(AST *ast) {
  int last = ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.length - 1;
  ttype last_type_expr =
      ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements[last]->type;

  print_ttype(last_type_expr);
}
