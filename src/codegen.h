#ifndef _LANG_CODEGEN_H
#define _LANG_CODEGEN_H
#include "ast.h"

#include <llvm-c/Core.h>

LLVMValueRef codegen(AST *ast, LLVMModuleRef module, LLVMBuilderRef builder);
#endif /* ifndef _LANG_CODEGEN_H */
