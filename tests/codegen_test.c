#include "../src/codegen.h"
#include "minunit.h"
#include "utils.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>

static LLVMValueRef test_codegen(AST *ast) {
  Context ctx;
  init_codegen_ctx(&ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  ctx.symbol_table = &symbol_table;
  enter_scope(&ctx);
  LLVMValueRef result = codegen(ast, &ctx);
  exit_scope(&ctx);
  free_ast(ast);
  return result;
}

//--------------------------------------
// Number
//--------------------------------------

int test_codegen_integer() {

  LLVMValueRef value = test_codegen(AST_NEW(INTEGER, 10));
  LLVMTypeRef type = LLVMTypeOf(value);
  mu_assert(LLVMGetTypeKind(type) == LLVMIntegerTypeKind,
            "AST_INTEGER -> LLVM Integer");
  mu_assert(LLVMIsConstant(value), "AST_INTEGER -> Constant LLVM Value ");
  return 0;
}

int test_codegen_double() {
  LLVMValueRef value = test_codegen(AST_NEW(NUMBER, 10));
  LLVMTypeRef type = LLVMTypeOf(value);
  mu_assert(LLVMGetTypeKind(type) == LLVMDoubleTypeKind,
            "AST_NUMBER -> LLVM Double");
  mu_assert(LLVMIsConstant(value), "AST_INTEGER -> Constant LLVM Value ");
  return 0;
}
int test_codegen_string() {

  LLVMValueRef value = test_codegen(AST_NEW(STRING, "hello", 5));
  LLVMTypeRef type = LLVMTypeOf(value);
  mu_assert(LLVMGetTypeKind(type) == LLVMPointerTypeKind, "");
  return 0;
}

int all_tests() {
  int test_result = 0;
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  mu_run_test(test_codegen_integer);
  mu_run_test(test_codegen_double);
  // mu_run_test(test_codegen_string);
  return test_result;
}

RUN_TESTS()
