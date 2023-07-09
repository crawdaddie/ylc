#include "lang_runner.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "llvm_backend.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

int run_value(LLVMExecutionEngineRef engine, LLVMValueRef value) {
  // Wrap in an anonymous function if it's a top-level expression.
  // bool is_top_level = (node->type != KAL_AST_TYPE_FUNCTION &&
  //                      node->type != KAL_AST_TYPE_PROTOTYPE);
  // if (is_top_level) {
  //   kal_ast_node *prototype = kal_ast_prototype_create("", NULL, 0);
  //   node = kal_ast_function_create(prototype, node);
  // }

  if (value == NULL) {
    fprintf(stderr, "Unable to codegen for node\n");
    return 1;
  }

  // Dump IR.
  LLVMDumpValue(value);

  // Run it if it's a top level expression.
  void *fp = LLVMGetPointerToGlobal(engine, value);
  double (*FP)() = (double (*)())(intptr_t)fp;
  fprintf(stderr, "Evaluted to %f\n", FP());
  // If this is a function then optimize it.
  //
  // else if (node->type == KAL_AST_TYPE_FUNCTION) {
  //   LLVMRunFunctionPassManager(pass_manager, value);
  // }
}

int LLVMRuntime(int repl, char *path) {
  // LLVM stuff
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("ylc", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  LLVMExecutionEngineRef engine;

  LLVMInitializeNativeTarget();
  LLVMLinkInMCJIT();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  char *msg;
  if (LLVMCreateExecutionEngineForModule(&engine, module, &msg) == 1) {
    fprintf(stderr, "%s\n", msg);
    LLVMDisposeMessage(msg);
    return 1;
  }

  // optimizations
  LLVMPassManagerRef pass_manager =
      LLVMCreateFunctionPassManagerForModule(module);
  // LLVMAddTargetData(LLVMGetExecutionEngineTargetData(engine), pass_manager);
  LLVMAddPromoteMemoryToRegisterPass(pass_manager);
  LLVMAddInstructionCombiningPass(pass_manager);
  LLVMAddReassociatePass(pass_manager);
  LLVMAddGVNPass(pass_manager);
  LLVMAddCFGSimplificationPass(pass_manager);
  LLVMInitializeFunctionPassManager(pass_manager);

  if (path) {
    char *filename = path;
    AST *ast = parse_file(filename);
    print_ast(*ast, 0);
    printf("\n");
    LLVMValueRef value = codegen(ast, module, builder);
    run_value(engine, value);
    free_ast(ast);
  }

  if (repl) {
    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    for (;;) {

      repl_input(input, INPUT_BUFSIZE, "> ");
      AST *ast = parse(input);
      print_ast(*ast, 0);
      printf("\n");
      LLVMValueRef value = codegen(ast, module, builder);
      run_value(engine, value);
      free_ast(ast);
    }
  }

  // Dump entire module.
  LLVMDumpModule(module);

  LLVMDisposePassManager(pass_manager);
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);
  LLVMContextDispose(context);
  return 0;
}
