#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();

  LLVMContextRef context = LLVMGetGlobalContext();

  LLVMModuleRef module;
  while (1) {
    // Create an LLVM context and module for each iteration
    module = LLVMModuleCreateWithNameInContext("repl_module", context);

    // Create an execution engine for each iteration
    LLVMExecutionEngineRef engine;
    char *error = NULL;
    if (LLVMCreateJITCompilerForModule(&engine, module, 2, &error) != 0) {
      fprintf(stderr, "Failed to create execution engine: %s\n", error);
      LLVMDisposeMessage(error);
      return 1;
    }

    // Read a number from the user
    printf("Enter a number (or 'q' to quit): ");
    char input[10];
    scanf("%s", input);

    // Check if the user wants to quit
    if (input[0] == 'q' || input[0] == 'Q') {
      LLVMDisposeExecutionEngine(engine);
      LLVMDisposeModule(module);
      break;
    }

    // Convert the input to a double value
    double number = strtod(input, NULL);

    // Create the main function
    LLVMTypeRef returnType = LLVMInt32TypeInContext(context);
    LLVMTypeRef paramTypes[] = {};
    LLVMTypeRef functionType = LLVMFunctionType(returnType, paramTypes, 0, 0);
    LLVMValueRef function = LLVMAddFunction(module, "main", functionType);

    // Create an IR builder
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    LLVMBasicBlockRef block =
        LLVMAppendBasicBlockInContext(context, function, "entry");
    LLVMPositionBuilderAtEnd(builder, block);

    // Create a constant value representing the input number
    LLVMValueRef constant =
        LLVMConstReal(LLVMDoubleTypeInContext(context), number);

    // Build a return instruction with the constant value
    LLVMBuildRet(builder, constant);

    // Print the generated LLVM IR
    // char* irCode;
    // LLVMPrintModuleToString(module, &irCode);
    // printf("Generated LLVM IR:\n%s\n", irCode);
    // LLVMDisposeMessage(irCode);
    //
    LLVMDumpModule(module);

    // Compile the module and execute the function
    // LLVMFinalizeObjectFile(LLVMGetExecutionEngineTargetData(engine),
    // module);
    double (*funcPtr)() = (double (*)())LLVMGetFunctionAddress(engine, "main");
    double result = funcPtr();
    printf("Result: %lf\n", result);
    printf("Res: %d\n", LLVMRunFunctionAsMain(engine, function, 0, NULL, NULL));

    // Clean up the IR builder for each iteration
    LLVMDisposeBuilder(builder);
  }

  return 0;
}
