#ifndef _LANG_CODEGEN_ADD_H
#define _LANG_CODEGEN_ADD_H
#include "codegen.h"
LLVMValueRef codegen_add(LLVMValueRef left, LLVMValueRef right, Context *ctx);
LLVMValueRef codegen_sub(LLVMValueRef left, LLVMValueRef right, Context *ctx);
LLVMValueRef codegen_neg_unop(LLVMValueRef operand, Context *ctx);

LLVMValueRef numerical_binop(token_type op, LLVMValueRef left,
                             LLVMValueRef right, Context *ctx);

LLVMValueRef numerical_unop(token_type op, LLVMValueRef operand, Context *ctx);

LLVMValueRef get_int(int val, Context *ctx);
LLVMValueRef get_double(double val, Context *ctx);
LLVMValueRef codegen_int(AST *ast, Context *ctx);
LLVMValueRef codegen_number(AST *ast, Context *ctx);
#endif
