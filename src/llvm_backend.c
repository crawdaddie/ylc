#include "input.h"
#include "parse.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

#include "llvm_backend.h"
#include "llvm_codegen.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>

int run_value(LLVMExecutionEngineRef engine, LLVMValueRef value) {

  if (value == NULL) {
    fprintf(stderr, "Unable to codegen for node\n");
    return 1;
  }

  void *fp = LLVMGetPointerToGlobal(engine, value);
  int (*FP)() = (int (*)())(intptr_t)fp;
  fprintf(stderr, "Evaluted to %d\n", FP());
  return 0;
}

int init_ctx(Context *ctx) {
  LLVMContextRef context = LLVMGetGlobalContext();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("ylc", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  LLVMExecutionEngineRef engine;

  char *error = NULL;
  if (LLVMCreateJITCompilerForModule(&engine, module, 2, &error) != 0) {
    fprintf(stderr, "Failed to create execution engine: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  ctx->context = context;
  ctx->builder = builder;
  ctx->engine = engine;
  ctx->module = module;
  return 0;
}

static void dump_ast(AST *ast) {
  printf("\n\033[1;35m");
  print_ast(*ast, 0);
  printf("\033[1;0m\n");
}

static void dump_module(LLVMModuleRef module) {
  printf("\n\033[1;36m");
  LLVMDumpModule(module);
  printf("\033[1;0m\n");
}

int reinit_ctx(Context *ctx) {
  LLVMModuleRef module = LLVMCloneModule(ctx->module);
  // LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx->context);
  LLVMExecutionEngineRef engine;

  char *error = NULL;
  if (LLVMCreateJITCompilerForModule(&engine, module, 2, &error) != 0) {
    fprintf(stderr, "Failed to create execution engine: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  LLVMDisposeModule(ctx->module);
  ctx->module = module;
  // ctx->builder = builder;
  ctx->engine = engine;

  init_symbol_table(ctx->symbol_table);
  return 0;
}

void enter_function(Context *ctx, LLVMValueRef function) {
  push_frame(ctx->symbol_table);
  ctx->currentFunction = function;
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(function, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, block);
  ctx->currentBlock = block;
}

void exit_function(Context *ctx, LLVMValueRef parentFunction,
                   LLVMBasicBlockRef prevBlock) {
  pop_frame(ctx->symbol_table);
  ctx->currentFunction = parentFunction;
  ctx->currentBlock = prevBlock;
  LLVMPositionBuilderAtEnd(ctx->builder, prevBlock);
}

int LLVMRuntime(int repl, char *path) {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  Context ctx;
  init_ctx(&ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  ctx.symbol_table = &symbol_table;

  // optimizations
  // LLVMPassManagerRef pass_manager =
  //     LLVMCreateFunctionPassManagerForModule(ctx.module);
  // LLVMAddPromoteMemoryToRegisterPass(pass_manager);
  // LLVMAddInstructionCombiningPass(pass_manager);
  // LLVMAddReassociatePass(pass_manager);
  // LLVMAddGVNPass(pass_manager);
  // LLVMAddCFGSimplificationPass(pass_manager);
  // LLVMInitializeFunctionPassManager(pass_manager);

  if (path) {
    char *filename = path;
    char *input = read_file(path);
    AST *ast = parse(input);
    free(input);

    dump_ast(ast);

    LLVMValueRef value = codegen(ast, &ctx);

    dump_module(ctx.module);
    run_value(ctx.engine, value);

    free_ast(ast);
  }

  if (repl) {
    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    for (;;) {

      repl_input(input, INPUT_BUFSIZE, "> ");

      AST *ast = parse(input);
      dump_ast(ast);

      LLVMValueRef value = codegen(ast, &ctx);
      dump_module(ctx.module);

      run_value(ctx.engine, value);
      free_ast(ast);
      reinit_ctx(&ctx);
    }
  }

  // LLVMDisposePassManager(pass_anager);
  LLVMDisposeBuilder(ctx.builder);
  LLVMDisposeModule(ctx.module);
  // LLVMContextDispose(context);
  return 0;
}
