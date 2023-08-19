#include "modules.h"
#include "input.h"
#include "parse/parse.h"
#include "typecheck.h"

INIT_SYM_TABLE_TYPES(LangModule);
INIT_SYM_TABLE(LangModule);

typedef LangModule_StackFrame ModuleEnv;
static ModuleEnv module_env = {};

void save_module(char *path, AST *ast) {
  LangModule mod = {ast};
  LangModule_env_insert(&module_env, path, mod);
}

void save_langmod(char *path, LangModule *mod) {
  LangModule_env_insert(&module_env, path, *mod);
}

void writeToFile(FILE *file, const void *data, size_t size) {
  fwrite(data, size, 1, file);
}
static void writeTypeToFile(FILE *file, ttype type) {
  int len = type.as.T_STRUCT.length;
  writeToFile(file, &len, sizeof(int));
  writeToFile(file, &type.as.T_STRUCT.members, sizeof(struct ttype) * len);

  writeToFile(file, &type.as.T_STRUCT.struct_metadata,
              sizeof(struct_member_metadata) * len);
}

static void save_type_bin(ttype mod_type) {
  FILE *write_ptr;
  write_ptr = fopen("./output.bin", "wb"); // w for write, b for binary

  writeTypeToFile(write_ptr, mod_type);
  fclose(write_ptr);
  // write 10 bytes from our buffer
}

ttype compute_module_type(AST *ast) {}

AST *parse_module(char *path) {
  char *input = read_file(path);
  AST *ast = parse(input);
  free(input);
  typecheck(ast, path);

  int len = ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.length;

  AST *module_struct_ast = malloc(sizeof(AST));

  module_struct_ast->tag = AST_STRUCT;

  module_struct_ast->data.AST_STRUCT.length = len;
  module_struct_ast->data.AST_STRUCT.members =
      ast->data.AST_MAIN.body->data.AST_STATEMENT_LIST.statements;

  ttype *member_types = malloc(sizeof(ttype) * len);
  struct_member_metadata *md = malloc(sizeof(struct_member_metadata) * len);

  for (int i = 0; i < len; i++) {
    AST *stmt = module_struct_ast->data.AST_STRUCT.members[i];
    member_types[i] = stmt->type;
    switch (stmt->tag) {
    case AST_TYPE_DECLARATION: {

      md[i] = (struct_member_metadata){
          .name = stmt->data.AST_TYPE_DECLARATION.name, i};

      break;
    }
    case AST_SYMBOL_DECLARATION: {

      md[i] = (struct_member_metadata){
          .name = stmt->data.AST_SYMBOL_DECLARATION.identifier, i};

      break;
    }
    case AST_ASSIGNMENT: {

      md[i] = (struct_member_metadata){
          .name = stmt->data.AST_ASSIGNMENT.identifier, i};

      break;
    }
    case AST_FN_DECLARATION: {

      md[i] = (struct_member_metadata){
          .name = stmt->data.AST_FN_DECLARATION.name, i};

      break;
    }
    default: {

      md[i] = (struct_member_metadata){.name = "", -1};
      break;
    }
    }
  }

  // print_ast(*module_struct_ast, 0);
  ttype module_type = module_struct_ast->type;
  module_type.tag = T_STRUCT;
  module_type.as.T_STRUCT.length = len;
  module_type.as.T_STRUCT.members = member_types;
  module_type.as.T_STRUCT.struct_metadata = md;

  free(module_struct_ast);

  ast->type = module_type;

  return ast;
}

/*
 * Returns the body of the module's Main node
 * ie either a list of statements or a single statement
 * */
AST *get_module(char *path) {
  LangModule mod;
  if (LangModule_env_lookup(&module_env, path, &mod) == 0) {
    return mod.ast;
  }
  AST *module = parse_module(path);
  save_module(path, module);
  return module;
}

LangModule *lookup_langmod(char *path) {
  LangModule mod;
  if (LangModule_env_lookup(&module_env, path, &mod) == 0) {
    return &mod;
  }
  return NULL;
}
