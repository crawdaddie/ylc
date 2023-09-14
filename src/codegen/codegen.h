#ifndef _LANG_CODEGEN_H
#define _LANG_CODEGEN_H
#include "../ast.h"
#include "../runner.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen(AST *ast, Context *ctx);

char *inst_name(const char *cString);

#endif /* ifndef _LANG_CODEGEN_H */
