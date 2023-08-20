#include "codegen_module.h"
#include "../modules.h"
#include "../paths.h"
#include "codegen.h"
#include "codegen_types.h"
#include <libgen.h>
#include <limits.h>
#include <stdio.h>

#include <dlfcn.h>
#include <libgen.h>
#include <stdlib.h>

#include <llvm-c/Linker.h>

#define MAX_ID_LEN 64
static char *mname() { return malloc(sizeof(char) * MAX_ID_LEN); }

static void mangle_name(const char *prefix, LLVMValueRef value) {
  const char *name = LLVMGetValueName(value);

  char *mangled_name = mname();
  sprintf(mangled_name, "_%s_%s", prefix, name);
  // printf("mangled name '%s'\n", mangled_name);
  LLVMSetValueName2(value, mangled_name, strlen(mangled_name));
}

LLVMValueRef build_module_struct(ttype type, char *module_name, Context *ctx) {
  int len = type.as.T_STRUCT.length;
  LLVMValueRef *members = malloc(sizeof(LLVMValueRef) * len);
  for (int i = 0; i < len; i++) {
    struct_member_metadata md = type.as.T_STRUCT.struct_metadata[i];
    ttype_tag member_type_tag = type.as.T_STRUCT.members[md.index].tag;

    char *mangled_name = mname();
    sprintf(mangled_name, "_%s_%s", module_name, md.name);
    if (member_type_tag == T_FN) {
      members[i] = LLVMGetNamedFunction(ctx->module, mangled_name);
      // members[i] = LLVMBuildLoad2(ctx->builder, LLVMTypeOf(members[i]),
      // members[i], "");

    } else {
      members[i] = LLVMGetNamedGlobal(ctx->module, mangled_name);
    }
    /*
    printf("\nextract\n");
    LLVMDumpValue(members[i]);
    printf("\n");
    LLVMDumpType(LLVMTypeOf(members[i]));
    printf("\n");
    */
  }
  LLVMTypeRef struct_ty = codegen_ttype(type, ctx);
  LLVMValueRef mod_struct = LLVMConstStruct(members, len, true);
  /*
  printf("\nmod struct type: ");
  LLVMDumpType(LLVMTypeOf(mod_struct));
  */
  return mod_struct;
}

static void mangle_names(LLVMModuleRef module, char *prefix) {

  // Iterate over global functions.
  LLVMValueRef function = LLVMGetFirstFunction(module);
  while (function != NULL) {
    mangle_name(prefix, function);
    function = LLVMGetNextFunction(function);
  }

  // Iterate over global variables.
  LLVMValueRef global = LLVMGetFirstGlobal(module);
  while (global != NULL) {
    mangle_name(prefix, global);
    global = LLVMGetNextGlobal(global);
  }
}

LLVMValueRef codegen_module(AST *ast, Context *ctx) {

  char *module_file_name = ast->data.AST_IMPORT.module_name;

  char *module_name = basename(strdup(ast->data.AST_IMPORT.module_name));
  remove_extension(module_name);

  char *resolved_path = mname();
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
    codegen(ast->data.AST_IMPORT.module_ast, &mod_ctx);

    mangle_names(mod_ctx.module, module_name);

    LLVMLinkModules2(ctx->module, mod_ctx.module);

    char *module_init_name = mname();
    sprintf(module_init_name, "_%s_main", module_name);
    LLVMValueRef module_init =
        LLVMGetNamedFunction(ctx->module, module_init_name);

    LLVMValueRef val =
        LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(module_init),
                       module_init, NULL, 0, "");
    lang_module->is_linked = true;
    save_langmod(resolved_path, lang_module);
  }
  LLVMValueRef module_struct = build_module_struct(ast->type, module_name, ctx);
  return module_struct;
  // return LLVMConstInt(LLVMInt1Type(), 0, 0);
}

LLVMValueRef codegen_so(AST *ast, Context *ctx) {
  char *lib_name = ast->data.AST_IMPORT_LIB.lib_name;

  void *handle = dlopen(lib_name, RTLD_LAZY);

  if (!handle) {
    fprintf(stderr, "Error loading the shared library: %s\n", dlerror());
  }
  return NULL;
}
