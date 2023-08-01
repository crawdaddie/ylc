#include "../src/generic_symbol_table.h"
#include "minunit.h"

typedef struct {
  int value;
} ttype_;

INIT_SYM_TABLE(ttype_)

int test_insert_value() {
  ttype__SymbolTable table;
  table.current_frame_index = 0;

  ttype__table_insert(&table, "x", (ttype_){1});

  ttype_ result;
  ttype__table_lookup(&table, "x", &result);
  mu_assert(result.value == 1, "");
  return 0;
}

int test_update_value() {
  ttype__SymbolTable table;
  table.current_frame_index = 0;

  ttype__table_insert(&table, "x", (ttype_){1});
  ttype__table_insert(&table, "x", (ttype_){2});
  ttype_ result;
  ttype__table_lookup(&table, "x", &result);
  mu_assert(result.value == 2, "");
  return 0;
}

int test_get_value_from_closest_scope() {
  ttype__SymbolTable table;
  table.current_frame_index = 0;
  ttype__table_insert(&table, "x", (ttype_){1});
  table.current_frame_index++;
  ttype__table_insert(&table, "x", (ttype_){2});

  ttype_ result;
  ttype__table_lookup(&table, "x", &result);
  mu_assert(result.value == 2, "");
  return 0;
}

int all_tests() {
  int test_result = 0;
  mu_run_test(test_insert_value);
  // mu_run_test(test_update_value);
  mu_run_test(test_get_value_from_closest_scope);
  return test_result;
}

RUN_TESTS()
