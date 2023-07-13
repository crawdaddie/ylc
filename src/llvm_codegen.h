#ifndef _LANG_CODEGEN_H
#define _LANG_CODEGEN_H
#include "ast.h"
#include "llvm_backend.h"
#include "uthash.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen(AST *ast, Context *ctx);

// Used to hold references to arguments by name.
typedef struct Symbol_ {
  const char *name;
  LLVMValueRef value;
  LLVMTypeRef type;
  UT_hash_handle hh;
} Symbol_;

char *inst_name(const char *cString);

#endif /* ifndef _LANG_CODEGEN_H */
