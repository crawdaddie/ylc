#ifndef _LANG_CODEGEN_MODULE_H
#define _LANG_CODEGEN_MODULE_H
#include "../ast.h"
#include "../llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_module(AST *ast, Context *ctx);

LLVMValueRef codegen_so(AST *ast, Context *ctx);

#endif
