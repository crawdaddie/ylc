#include "../src/codegen/codegen.h"
#include "../src/codegen/codegen_types.h"
#include "minunit.h"
#include "utils.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <stdlib.h>

static LLVMTypeRef test_codegen_types(ttype type) {
  Context ctx;
  init_lang_ctx(&ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  ctx.symbol_table = &symbol_table;
  enter_scope(&ctx);
  LLVMTypeRef result = codegen_ttype(type, &ctx);
  exit_scope(&ctx);
  return result;
}

static LLVMValueRef test_codegen(AST *ast) {
  Context ctx;
  init_lang_ctx(&ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  ctx.symbol_table = &symbol_table;
  enter_scope(&ctx);
  LLVMValueRef result = codegen(ast, &ctx);
  exit_scope(&ctx);
  return result;
}

//--------------------------------------
// Number
//--------------------------------------

int test_codegen_basic_types() {
  LLVMTypeRef res;
  ttype type = {T_INT};
  res = test_codegen_types(type);
  mu_assert(LLVMGetTypeKind(res) == LLVMIntegerTypeKind,
            "AST_INTEGER -> LLVM Integer");

  type = (ttype){T_NUM};
  res = test_codegen_types(type);
  mu_assert(LLVMGetTypeKind(res) == LLVMDoubleTypeKind,
            "AST_NUMBER -> LLVM Double");

  type = (ttype){T_STR};
  res = test_codegen_types(type);
  mu_assert(LLVMGetTypeKind(res) == LLVMPointerTypeKind,
            "AST_STRING -> LLVM ptr (int8) type");

  type = (ttype){T_BOOL};
  res = test_codegen_types(type);
  mu_assert(LLVMGetTypeKind(res) == LLVMIntegerTypeKind,
            "AST_BOOL -> LLVM int type (int1)");

  return 0;
}
int test_codegen_fn_types() {
  LLVMTypeRef res;
  ttype member_types[] = {
      {T_INT}, {T_NUM}, {T_STR}, {T_BOOL}, {T_NUM},
  };
  ttype fn_type = {T_FN, .as = {.T_FN = {5}}};
  fn_type.as.T_FN.members = member_types;
  res = test_codegen_types(fn_type);

  LLVMTypeRef *params = malloc(sizeof(LLVMTypeRef) * 4);
  LLVMGetParamTypes(res, params);

  mu_assert(LLVMGetTypeKind(params[0]) == LLVMIntegerTypeKind,
            "param 0 is int");

  mu_assert(LLVMGetTypeKind(params[1]) == LLVMDoubleTypeKind,
            "param 1 is double");

  mu_assert(LLVMGetTypeKind(params[2]) == LLVMPointerTypeKind,
            "param 2 is str");

  mu_assert(LLVMGetTypeKind(params[3]) == LLVMIntegerTypeKind,
            "param 3 is bool");

  mu_assert(LLVMGetTypeKind(LLVMGetReturnType(res)) == LLVMDoubleTypeKind,
            "fn return type is double");
  return 0;
}

int test_codegen_struct_type() {

  LLVMTypeRef res;
  ttype member_types[] = {
      {T_INT}, {T_NUM}, {T_STR}, {T_BOOL}, {T_NUM},
  };
  struct_member_metadata md[] = {
      {"a", 0}, {"b", 1}, {"c", 2}, {"d", 3}, {"e", 4},
  };
  ttype struct_type = {T_STRUCT};

  struct_type.as.T_STRUCT.length = 5;
  struct_type.as.T_STRUCT.members = member_types;
  struct_type.as.T_STRUCT.struct_metadata = md;

  res = test_codegen_types(struct_type);

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                res, get_struct_member_index(struct_type, "a"))) ==
                LLVMIntegerTypeKind,
            "member a is int");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                res, get_struct_member_index(struct_type, "b"))) ==
                LLVMDoubleTypeKind,
            "member b is double");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                res, get_struct_member_index(struct_type, "c"))) ==
                LLVMPointerTypeKind,
            "member c is str");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                res, get_struct_member_index(struct_type, "d"))) ==
                LLVMIntegerTypeKind,
            "member d is bool");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                res, get_struct_member_index(struct_type, "e"))) ==
                LLVMDoubleTypeKind,
            "member e is double");

  return 0;
}

int test_codegen_nested_struct_type() {

  LLVMTypeRef res;
  ttype member_types[] = {
      {T_INT}, {T_NUM}, {T_STR}, {T_BOOL}, {T_NUM},
  };
  struct_member_metadata md[] = {
      {"a", 0}, {"b", 1}, {"c", 2}, {"d", 3}, {"e", 4},
  };
  ttype struct_type = {T_STRUCT};
  struct_type.as.T_STRUCT.length = 5;
  struct_type.as.T_STRUCT.members = member_types;
  struct_type.as.T_STRUCT.struct_metadata = md;

  ttype container_type = {T_STRUCT};
  ttype container_member_types[] = {struct_type};
  struct_member_metadata c_md[] = {{"a", 0}};
  container_type.as.T_STRUCT.length = 1;
  container_type.as.T_STRUCT.members = container_member_types;
  container_type.as.T_STRUCT.struct_metadata = c_md;

  res = test_codegen_types(container_type);
  LLVMTypeRef inner_struct = LLVMStructGetTypeAtIndex(res, 0);

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(
                inner_struct, get_struct_member_index(struct_type, "a"))) ==
                LLVMIntegerTypeKind,
            "member a is int");

  return 0;
}

int test_codegen_tuple_type() {

  LLVMTypeRef res;
  ttype member_types[] = {
      {T_INT}, {T_NUM}, {T_STR}, {T_BOOL}, {T_NUM},
  };

  ttype struct_type = {T_TUPLE};

  struct_type.as.T_TUPLE.length = 5;
  struct_type.as.T_TUPLE.members = member_types;

  res = test_codegen_types(struct_type);

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(res, 0)) ==
                LLVMIntegerTypeKind,
            "element 0 is int");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(res, 1)) ==
                LLVMDoubleTypeKind,
            "element 1 is double");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(res, 2)) ==
                LLVMPointerTypeKind,
            "element 2 is str");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(res, 3)) ==
                LLVMIntegerTypeKind,
            "element 3 is bool");

  mu_assert(LLVMGetTypeKind(LLVMStructGetTypeAtIndex(res, 4)) ==
                LLVMDoubleTypeKind,
            "element 4 is double");

  return 0;
}

int test_codegen_type_decl() {
  // AST *ast = AST_NEW(TYPE_DECLARATION, "Point", AST_NEW(STRUCT));
  return 0;
}

int all_tests() {
  int test_result = 0;
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  mu_run_test(test_codegen_basic_types);
  mu_run_test(test_codegen_fn_types);
  mu_run_test(test_codegen_struct_type);
  mu_run_test(test_codegen_nested_struct_type);
  mu_run_test(test_codegen_tuple_type);
  return test_result;
}

RUN_TESTS()
