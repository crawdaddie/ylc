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

int LLVMRuntime(int repl, char *path) {
  // LLVM stuff
  LLVMModuleRef module = LLVMModuleCreateWithName("ylc");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMExecutionEngineRef engine;

  LLVMInitializeNativeTarget();
  LLVMLinkInMCJIT();

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
    codegen(ast);
    free_ast(ast);
  }

  if (repl) {
    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    for (;;) {

      repl_input(input, INPUT_BUFSIZE, "> ");
      AST *ast = parse(input);
      print_ast(*ast, 0);
      printf("\n");
      codegen(ast);
      free_ast(ast);
    }
  }

  // Dump entire module.
  LLVMDumpModule(module);

  LLVMDisposePassManager(pass_manager);
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(module);
  return 0;
}
