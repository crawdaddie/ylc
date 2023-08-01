#include "codegen_module.h"
#include "ast.h"
#include "codegen.h"
#include "input.h"
#include "llvm_backend.h"
#include "parse.h"
#include "paths.h"
#include <libgen.h>
#include <limits.h>
#include <llvm-c/Linker.h>

static void extract_type(LLVMTypeRef *types, int index, Symbol *sym) {
  switch (sym->value.type) {

  case TYPE_GLOBAL_VARIABLE: {
    types[index] = sym->value.data.TYPE_GLOBAL_VARIABLE.llvm_type;
    break;
  }

  case TYPE_FUNCTION: {
    types[index] = sym->value.data.TYPE_FUNCTION.llvm_type;
    break;
  }

  case TYPE_TYPE_DECLARATION: {
    break;
  }
  }
}
static void extract_value(LLVMValueRef *values, int index, Symbol *sym) {
  switch (sym->value.type) {

  case TYPE_GLOBAL_VARIABLE: {

    values[index] = sym->value.data.TYPE_GLOBAL_VARIABLE.llvm_value;
    break;
  }

  case TYPE_FUNCTION: {
    values[index] = sym->value.data.TYPE_FUNCTION.llvm_value;
    break;
  }

  case TYPE_TYPE_DECLARATION: {
    break;
  }
  }
}

LLVMValueRef codegen_module(char *filename, Context *ctx) {

  char resolved_path[PATH_MAX];
  resolve_path(dirname(ctx->module_path), filename, resolved_path);

  Context this_ctx;
  init_lang_ctx(&this_ctx);
  SymbolTable symbol_table;
  init_symbol_table(&symbol_table);
  this_ctx.symbol_table = &symbol_table;

  char *input = read_file(resolved_path);
  AST *ast = parse(input);
  this_ctx.module_path = resolved_path;

  LLVMSetSourceFileName(this_ctx.module, resolved_path, strlen(resolved_path));
  codegen(ast, &this_ctx);
  /*
    int num_top_level_declarations =
        this_ctx.symbol_table->stack[0].allocated_entries;

    LLVMTypeRef *types = malloc(sizeof(LLVMTypeRef) *
    num_top_level_declarations); LLVMValueRef *values =
        malloc(sizeof(LLVMValueRef) * num_top_level_declarations);

    int top_level_decl_idx = 0;
    */

  for (int i = 0; i < TABLE_SIZE; i++) {
    Symbol *sym = this_ctx.symbol_table->stack[0]
                      .entries[i]; // iterate over top-level symbols

    while (sym) {
      printf("copy from %s - key: %s to scope %d\n", resolved_path, sym->key,
             ctx->symbol_table->current_frame_index);

      table_insert(ctx->symbol_table, sym->key,
                   sym->value); // Copy top-level symbols from loaded module
                                // to loading module
      /*
      extract_type(types, top_level_decl_idx, sym);
      extract_value(values, top_level_decl_idx, sym);

      top_level_decl_idx++;
      */

      sym = sym->next;
    }
  }

  /*
  LLVMTypeRef type = LLVMStructCreateNamed(ctx->context, filename);
  LLVMStructSetBody(type, types, num_top_level_declarations, true);
  LLVMValueRef module_struct =
      LLVMConstNamedStruct(type, values, num_top_level_declarations);
*/
  if (!LLVMLinkModules2(ctx->module, this_ctx.module)) {
    fprintf(stderr, "Error linking module %s into %s\n", this_ctx.module_path,
            ctx->module_path);
  };

  free(input);
  free_ast(ast);
  // Return 'something' from import 'module' - TODO: actually create a struct
  // holding global values from the source module to namespace functionality
  return LLVMAddGlobal(ctx->module, LLVMInt32TypeInContext(ctx->context),
                       filename);
}
