#include "../src/unify_types.h"
#include "minunit.h"

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

int test_unify_vars() {
  ttype Int = {T_INT};
  TypeEnv env = {};

  ttype l = _tvar();
  ttype r = _tvar();

  add_type_to_env(&env, &r, &Int);
  unify((TypeEquation){&l, &r, -1}, &env);

  ttype test;
  mu_assert(ttype_env_lookup(&env, l.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve type l to Int");

  return 0;
}

int test_unify_chained_vars() {
  ttype Int = {T_INT};
  TypeEnv env = {};

  ttype l = _tvar();
  ttype r0 = _tvar();
  ttype r1 = _tvar();
  ttype r2 = _tvar();
  add_type_to_env(&env, &r2, &Int);
  add_type_to_env(&env, &r1, &r2);
  add_type_to_env(&env, &r0, &r1);

  unify((TypeEquation){&l, &r0, -1}, &env);

  ttype test;

  mu_assert(ttype_env_lookup(&env, l.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve type l to Int");

  return 0;
}

int test_unify_fn() {
  ttype Int = {T_INT};

  TypeEnv env = {};

  ttype param_types[2] = {Int, Int};
  ttype fn = tfn(param_types, 2);

  ttype var0 = _tvar(); // param 0
  ttype var1 = _tvar(); // return
  ttype var_param_types[2] = {var0, var1};
  ttype l = tfn(var_param_types, 2);

  unify((TypeEquation){&l, &fn, -1}, &env);

  ttype test;

  mu_assert(ttype_env_lookup(&env, var0.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 0 to Int");

  mu_assert(ttype_env_lookup(&env, var1.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 1 (return) to Int");
  return 0;
}

int test_unify_fn_var() {
  ttype Int = {T_INT};

  TypeEnv env = {};

  ttype param_types[2] = {Int, Int};
  ttype fn_t = tfn(param_types, 2);
  ttype fn = _tvar();
  add_type_to_env(&env, &fn, &fn_t);

  ttype var0 = _tvar(); // param 0
  ttype var1 = _tvar(); // return
  ttype var_param_types[2] = {var0, var1};
  ttype l = tfn(var_param_types, 2);

  unify((TypeEquation){&l, &fn, -1}, &env);

  ttype test;

  mu_assert(ttype_env_lookup(&env, var0.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 0 to Int");

  mu_assert(ttype_env_lookup(&env, var1.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 1 (return) to Int");
  return 0;
}

int test_unify_fn_index() {
  ttype Int = {T_INT};

  TypeEnv env = {};

  ttype param_types[2] = {Int, Int};
  ttype fn_t = tfn(param_types, 2);
  ttype fn = _tvar();
  add_type_to_env(&env, &fn, &fn_t);

  ttype v0 = _tvar();
  unify((TypeEquation){&v0, &fn, 0}, &env);

  ttype test;

  mu_assert(ttype_env_lookup(&env, v0.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 0 to Int");

  ttype v1 = _tvar();
  unify((TypeEquation){&v1, &fn, 1}, &env);

  mu_assert(ttype_env_lookup(&env, v1.as.T_VAR.name, &test) == 0 &&
                test.tag == T_INT,
            "resolve param 0 to Int");
  return 0;
}

#define run_test(TEST)                                                         \
  t_counter = 0;                                                               \
  mu_run_test(TEST);

int all_tests() {
  int test_result = 0;
  run_test(test_unify_vars);
  run_test(test_unify_chained_vars);
  run_test(test_unify_fn);
  run_test(test_unify_fn_var);
  run_test(test_unify_fn_index);
  return test_result;
}
#undef run_test

RUN_TESTS()
