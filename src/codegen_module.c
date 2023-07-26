#include "codegen_module.h"
#include "ast.h"
#include "codegen.h"
#include "input.h"
#include "llvm_backend.h"
#include "parse.h"
#include <llvm-c/Linker.h>

LLVMValueRef codegen_module(char *filename, Context *ctx) {
  Context this_ctx;
  init_lang_ctx(&this_ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  this_ctx.symbol_table = &symbol_table;

  char *input = read_file(filename);
  AST *ast = parse(input);
  free(input);
  codegen(ast, &this_ctx);

  for (int i = 0; i < TABLE_SIZE; i++) {
    Symbol *sym = this_ctx.symbol_table->stack[0].entries[i];
    while (sym) {
      table_insert(ctx->symbol_table, sym->key,
                   sym->value); // Copy top-level symbols from loaded module
                                // to loading module
      sym = sym->next;
    }
  }
  LLVMLinkModules2(ctx->module, this_ctx.module);

  return LLVMAddGlobal(ctx->module, LLVMInt32TypeInContext(ctx->context),
                       filename);
}
