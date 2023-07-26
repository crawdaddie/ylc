#ifndef _LANG_CODEGEN_MODULE_H
#define _LANG_CODEGEN_MODULE_H
#include "llvm_backend.h"
#include <llvm-c/Core.h>

LLVMValueRef codegen_module(char *filename, Context *ctx);
#endif /* ifndef _LANG_CODEGEN_MODULE_H */
