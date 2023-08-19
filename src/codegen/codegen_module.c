#include "codegen_module.h"
#include "../modules.h"
#include "../paths.h"
#include "codegen.h"
#include <libgen.h>
#include <limits.h>
#include <stdio.h>

#include <llvm-c/Linker.h>

static void mangle_name(const char *prefix, LLVMValueRef value) {
  const char *name = LLVMGetValueName(value);

  char mangled_name[PATH_MAX];
  sprintf(mangled_name, "_%s_%s", prefix, name);
  // printf("mangled name '%s'\n", mangled_name);
  LLVMSetValueName2(value, mangled_name, strlen(mangled_name));
}
static LLVMValueRef copy_global(LLVMModuleRef module, LLVMValueRef global,
                                char *name) {

  // Create a new global variable with the same type as the original.
  LLVMValueRef glob_init = LLVMGetInitializer(global);
  LLVMValueRef newGlobalVar =
      LLVMAddGlobal(module, LLVMTypeOf(glob_init), name);

  // Copy the initializer from the original global variable to the new one.
  LLVMSetInitializer(newGlobalVar, glob_init);

  // Optionally, copy attributes from the original global variable to the new
  // one.
  LLVMSetVisibility(newGlobalVar, LLVMGetVisibility(global));
  return newGlobalVar;
}
LLVMValueRef build_module_struct(ttype type, char *module_name, Context *ctx) {
  int len = type.as.T_STRUCT.length;
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * len);
  for (int i = 0; i < len; i++) {
    struct_member_metadata md = type.as.T_STRUCT.struct_metadata[i];
    ttype_tag member_type_tag = type.as.T_STRUCT.members[md.index].tag;

    char mangled_name[PATH_MAX];
    sprintf(mangled_name, "_%s_%s", module_name, md.name);
    if (member_type_tag == T_FN) {
      members[i] = LLVMGetNamedFunction(ctx->module, mangled_name);
    } else {
      members[i] = LLVMGetNamedGlobal(ctx->module, mangled_name);
    }
  }
  LLVMValueRef mod_struct = LLVMConstStruct(members, len, true);
  return mod_struct;
}

LLVMValueRef codegen_module(AST *ast, Context *ctx) {

  char *module_file_name = ast->data.AST_IMPORT.module_name;

  char *module_name = basename(strdup(ast->data.AST_IMPORT.module_name));
  remove_extension(module_name);

  char resolved_path[PATH_MAX];
  resolve_path(dirname(ctx->module_path), module_file_name, resolved_path);
  LangModule *lang_module = lookup_langmod(resolved_path);
  if (!lang_module->is_linked) {

    Context mod_ctx;
    init_lang_ctx(&mod_ctx);
    SymbolTable mod_symbol_table;
    init_symbol_table(&mod_symbol_table);
    mod_ctx.symbol_table = &mod_symbol_table;
    mod_ctx.module_path = resolved_path;
    LLVMSetSourceFileName(mod_ctx.module, resolved_path, strlen(resolved_path));
    LLVMValueRef mod_val = codegen(ast->data.AST_IMPORT.module_ast, &mod_ctx);

    // Iterate over global functions.
    LLVMValueRef function = LLVMGetFirstFunction(mod_ctx.module);
    while (function != NULL) {
      mangle_name(module_name, function);
      function = LLVMGetNextFunction(function);
    }

    // Iterate over global variables.
    LLVMValueRef global = LLVMGetFirstGlobal(mod_ctx.module);
    while (global != NULL) {
      mangle_name(module_name, global);
      global = LLVMGetNextGlobal(global);
    }

    LLVMLinkModules2(ctx->module, mod_ctx.module);

    char module_init_name[PATH_MAX];
    sprintf(module_init_name, "_%s_main", module_name);
    LLVMValueRef module_init =
        LLVMGetNamedFunction(ctx->module, module_init_name);

    LLVMValueRef val =
        LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(module_init),
                       module_init, NULL, 0, "");
    lang_module->is_linked = true;
    save_langmod(resolved_path, lang_module);
  } else {
    printf("already codegen module\n");
  }

  LLVMValueRef module_struct = build_module_struct(ast->type, module_name, ctx);
  return module_struct;
}
