#ifndef _LANG_CODEGEN_COMPOUND_H
#define _LANG_CODEGEN_COMPOUND_H
#include "../ast.h"
#include "../llvm_backend.h"
#include <llvm-c/Core.h>
LLVMValueRef codegen_struct(AST *ast, Context *ctx);
LLVMValueRef codegen_tuple(AST *ast, Context *ctx);
LLVMValueRef codegen_array(AST *ast, Context *ctx);
#endif
