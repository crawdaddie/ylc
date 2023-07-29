#include "codegen_module.h"
#include "ast.h"
#include "codegen.h"
#include "input.h"
#include "llvm_backend.h"
#include "parse.h"
#include "paths.h"
#include <libgen.h>
#include <llvm-c/Linker.h>
LLVMValueRef codegen_module(char *filename, Context *ctx) {

  char resolved_path[256];
  resolve_path(dirname(ctx->module_path), filename, resolved_path);

  Context this_ctx;
  init_lang_ctx(&this_ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  this_ctx.symbol_table = &symbol_table;

  char *input = read_file(resolved_path);
  AST *ast = parse(input);
  this_ctx.module_path = resolved_path;
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

  free(input);
  free_ast(ast);
  // Return 'something' from import 'module' - TODO: actually create a struct
  // holding global values from the source module to namespace functionality
  return LLVMAddGlobal(ctx->module, LLVMInt32TypeInContext(ctx->context),
                       filename);
}
