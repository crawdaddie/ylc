#include "input.h"
#include "parse.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "llvm_backend.h"
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Support.h>
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
  fprintf(stderr, "(%d)\n", FP());
  return 0;
}

int init_lang_ctx(Context *ctx) {
  LLVMContextRef context = LLVMGetGlobalContext();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("ylc", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  LLVMExecutionEngineRef engine;

  LLVMPassManagerRef pass_manager =
      LLVMCreateFunctionPassManagerForModule(module);

  LLVMAddPromoteMemoryToRegisterPass(pass_manager);
  // LLVMAddInstructionCombiningPass(pass_manager);
  LLVMAddReassociatePass(pass_manager);
  LLVMAddGVNPass(pass_manager);
  LLVMAddCFGSimplificationPass(pass_manager);
  LLVMAddTailCallEliminationPass(pass_manager);

  LLVMInitializeFunctionPassManager(pass_manager);

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
  ctx->pass_manager = pass_manager;
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

int reinit_lang_ctx(Context *ctx) {
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

void enter_scope(Context *ctx) { push_frame(ctx->symbol_table); }

void exit_scope(Context *ctx) { pop_frame(ctx->symbol_table); }

LLVMValueRef current_function(Context *ctx) {
  return LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
}
static int dump_ir(Context *ctx, char *output) {
  LLVMModuleRef module = ctx->module;

  // LLVMPrintModuleToFile(module, output, NULL);
  LLVMWriteBitcodeToFile(module, output);
  return 0;
}
int LLVMRuntime(int repl, char *path, char *output) {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  Context ctx;
  init_lang_ctx(&ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  ctx.symbol_table = &symbol_table;

  printf("\033[1;31m"
         "YLC LANG REPL       \n"
         "--------------------\n"
         "version 0.0.0       \n"
         "\033[1;0m");

  if (path) {
    char *filename = path;
    char *input = read_file(path);
    AST *ast = parse(input);
    free(input);

    if (output) {
      // printf("build exe from %s to %s\n", path, output);
      LLVMValueRef value = codegen(ast, &ctx);
      dump_ir(&ctx, output);

      free_ast(ast);

    } else {
      dump_ast(ast);
      LLVMValueRef value = codegen(ast, &ctx);
      dump_module(ctx.module);
      run_value(ctx.engine, value);
      free_ast(ast);
    }
    if (repl) {
      reinit_lang_ctx(&ctx);
    }
  }

  if (repl) {
    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    for (;;) {

      repl_input(input, INPUT_BUFSIZE, "\033[1;31mλ \033[1;0m"
                 "\033[1;36m"
                 );
      printf("\033[1;0m");

      AST *ast = parse(input);
      dump_ast(ast);

      LLVMValueRef value = codegen(ast, &ctx);
      dump_module(ctx.module);

      run_value(ctx.engine, value);
      free_ast(ast);
      reinit_lang_ctx(&ctx);
    }
  }

  // LLVMDisposePassManager(pass_anager);
  LLVMDisposeBuilder(ctx.builder);
  LLVMDisposeModule(ctx.module);
  // LLVMContextDispose(context);
  return 0;
}
