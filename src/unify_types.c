#include "unify_types.h"
#include "typecheck.h"
#include <stdio.h>

// #define _TYPECHECK_DBG
INIT_SYM_TABLE(ttype);

static void print_type_equation(TypeEquation type_equation) {
  print_ttype(*type_equation.left);
  printf(" :: ");
  print_ttype(*type_equation.right);
  if (type_equation.index != -1) {
    printf(" [%d]", type_equation.index);
  }
  printf("\n");
}
void add_type_to_env(TypeEnv *env, ttype *left, ttype *right) {
  if (left->tag != T_VAR) {
    return;
  }

  ttype_env_insert(env, left->as.T_VAR.name, *right);
}

ttype *follow_links(TypeEnv *env, ttype *rtype) {
  ttype *lookup = rtype;
  while (lookup->tag == T_VAR &&
         ttype_env_lookup(env, lookup->as.T_VAR.name, lookup) == 0) {
  }
  return lookup;
}

static void unify_vars(ttype *l, ttype *r, TypeEnv *env) {

  if (strcmp(l->as.T_VAR.name, r->as.T_VAR.name) == 0) {
    return;
  }
  r = follow_links(env, r);

  add_type_to_env(env, l, r);

  if (r->tag != T_VAR) {
    l->tag = r->tag;
    l->as = r->as;
  }
}

static void unify_tuple(ttype *lvar, ttype *rtuple, TypeEnv *env) {
  for (int i = 0; i < rtuple->as.T_TUPLE.length; i++) {
    ttype member_type = rtuple->as.T_TUPLE.members[i];
    if (member_type.tag == T_VAR) {
      rtuple->as.T_TUPLE.members[i] = *follow_links(env, &member_type);
    }
  }
  add_type_to_env(env, lvar, rtuple);
  lvar->tag = T_TUPLE;
  lvar->as = rtuple->as;
  return;
}

static void unify_fn(ttype *lvar, ttype *rfn, TypeEnv *env) {
  for (int i = 0; i < rfn->as.T_FN.length; i++) {
    ttype member_type = rfn->as.T_FN.members[i];
    if (member_type.tag == T_VAR) {
      rfn->as.T_FN.members[i] = *follow_links(env, &member_type);
    }
  }
  add_type_to_env(env, lvar, rfn);
  // printf("lvar update %s\n", lvar->as.T_VAR.name);
  lvar->tag = T_FN;
  lvar->as = rfn->as;
  return;
}

static void unify_fns(ttype *l, ttype *r, TypeEnv *env) {
  if (l->tag != r->tag) {
    return;
  }
  if (l->as.T_FN.length != r->as.T_FN.length) {
    return;
  }
  for (int i = 0; i < l->as.T_FN.length; i++) {
    ttype u = l->as.T_FN.members[i];
    ttype v = r->as.T_FN.members[i];
    unify((TypeEquation){&u, &v, -1}, env);
  }
}

static void unify_compound_types(ttype *l, ttype *r, TypeEnv *env) {
  if (l->tag != r->tag) {
    return;
  }

  int len;
  ttype *lmembers;
  ttype *rmembers;

  if (l->tag == T_STRUCT) {
    if (l->as.T_STRUCT.length != r->as.T_STRUCT.length) {
      return;
    }
    len = l->as.T_STRUCT.length;
    lmembers = l->as.T_STRUCT.members;

  } else {
    if (l->as.T_TUPLE.length != r->as.T_TUPLE.length) {
      return;
    }
    len = l->as.T_STRUCT.length;
    lmembers = l->as.T_STRUCT.members;
  }

  for (int i = 0; i < len; i++) {
    ttype u = lmembers[i];
    ttype v = rmembers[i];
    unify((TypeEquation){&u, &v, -1}, env);
  }
}
bool check_compound(ttype *l, ttype *r) {
  if (l->tag != r->tag)
    return false;
  if (l->tag == T_STRUCT && l->as.T_STRUCT.length != r->as.T_STRUCT.length) {
    return false;
  }

  if (l->tag == T_TUPLE && l->as.T_TUPLE.length != r->as.T_TUPLE.length) {
    return false;
  }
  return true;
}
void unify(TypeEquation eq, TypeEnv *env) {
#ifdef _TYPECHECK_DBG
  print_type_equation(eq);
#endif
  ttype *l = eq.left;
  ttype *r = eq.right;
  int index = eq.index;

  ttype *lookup = malloc(sizeof(ttype));
  int lookup_exists = 0;
  if (l->tag == T_VAR) {
    lookup_exists = ttype_env_lookup(env, l->as.T_VAR.name, lookup) == 0;
  }

  if (index != -1) {
    // TODO: unify indexed type equations, eg l :: fn_type[n]

    // find r type
    r = follow_links(env, r);
    if (r->tag == T_STRUCT) {
      r = &r->as.T_STRUCT.members[index];
    } else if (r->tag == T_TUPLE) {
      r = &r->as.T_TUPLE.members[index];
    } else if (r->tag == T_FN) {
      r = &r->as.T_FN.members[index];
    } else if (l->tag == T_VAR && r->tag == T_VAR) {
      printf("unify %s with %s[%d]\n", l->as.T_VAR.name, r->as.T_VAR.name,
             index);
      return;
    }

    return unify((TypeEquation){l, r, -1}, env);
  }

  if (r->tag == T_VAR && l->tag != T_VAR) {
    return unify((TypeEquation){r, l, index}, env);
  }

  if (l->tag != T_VAR && r->tag != l->tag) {
    fprintf(stderr,
            "typecheck error: cannot unify types - mismatching tags %d %d\n",
            l->tag, r->tag);
    _typecheck_error_flag = 1;
    return;
  }

  switch (l->tag) {
  case T_FN: {
    if (r->tag == T_VAR) {
      r = follow_links(env, r);
    }
    unify_fns(l, r, env);
    break;
  }
  case T_TUPLE:
  case T_STRUCT: {
    if (!check_compound(l, r)) {
      fprintf(stderr, "typecheck error: cannot unify structs\n");
      _typecheck_error_flag = 1;
      break;
      ;
    }

    if (r->tag == T_VAR) {
      r = follow_links(env, r);
    }
    unify_compound_types(l, r, env);
    break;
  }
  case T_VAR: {

    if (r->tag == T_FN) {
      unify_fn(l, r, env);
      break;
    }

    if (r->tag == T_TUPLE) {

      unify_tuple(l, r, env);
      break;
    }

    unify_vars(l, r, env);
    break;
  }
  case T_INT8:
  case T_INT:
  case T_NUM:
  case T_STR:
  case T_BOOL:
  case T_VOID:
  case T_PTR: {
    if (l->tag != r->tag) {
      fprintf(stderr,
              "typecheck error: cannot unify ptrs - mismatching tags\n");
      _typecheck_error_flag = 1;
      break;
    }
  }
  default:
    break;
  }

#ifdef _TYPECHECK_DBG
  print_env(env);
#endif
  if (lookup_exists) {
    if (lookup->tag == T_VAR) {
      unify((TypeEquation){lookup, l, -1}, env);
    }
  }
};
