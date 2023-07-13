#ifndef _LANG_SYM_TABLE_H
#define _LANG_SYM_TABLE_H
#include <llvm-c/Core.h>
#include <stdbool.h>

#define TABLE_SIZE 100
#define STACK_SIZE 100
// #define TYPES_TABLE_SIZE 31
//
// typedef struct TYPE_INT {
//   LLVMTypeRef cgen_type_ref;
// } Int;
// typedef struct TYPE_NUMBER {
//   LLVMTypeRef cgen_type_ref;
// } Number;
// typedef struct TYPE_BOOL {
//   LLVMTypeRef cgen_type_ref;
// } Bool;
//
// typedef struct TYPE_LIST {
//   LLVMTypeRef cgen_type_ref;
// } List;
//
// typedef struct TYPE_STRUCT {
//   LLVMTypeRef cgen_type_ref;
// } Struct;
//
// typedef struct TYPE_STRING {
//   LLVMTypeRef cgen_type_ref;
// } String;
//
// typedef struct TYPE_CHAR {
//   LLVMTypeRef cgen_type_ref;
// } Char;

typedef struct SymbolValue {
  enum {
    // TYPE_INTEGER,
    // TYPE_NUMBER,
    // TYPE_BOOL,
    // TYPE_TUPLE,
    // TYPE_FUNCTION,
    // TYPE_POINTER,
    // TYPE_STRUCT,
    TYPE_FN_PARAM,
    TYPE_VARIABLE,
    TYPE_GLOBAL_VARIABLE,
    TYPE_INT,
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_LIST,
    TYPE_STRUCT,
    TYPE_CHAR,
    TYPE_STRING,
  } type;

  union {
    struct TYPE_VARIABLE {
      LLVMValueRef llvm_value;
      LLVMTypeRef llvm_type;
    } TYPE_VARIABLE;

    struct TYPE_GLOBAL_VARIABLE {
      LLVMValueRef llvm_value;
      LLVMTypeRef llvm_type;
    } TYPE_GLOBAL_VARIABLE;

    struct TYPE_FN_PARAM {
      int arg_idx;
      // LLVMTypeRef type;
    } TYPE_FN_PARAM;

    // Int TYPE_INT;
    // Number TYPE_NUMBER;
    // Bool TYPE_BOOL;
    // List TYPE_LIST;
    // Struct TYPE_STRUCT;
    // Char TYPE_CHAR;
    // String TYPE_STRING;

  } data;
} SymbolValue;

#define SYMBOL(type)                                                           \
  (SymbolValue) { tag }

// Structure representing a symbol table entry
typedef struct Symbol {
  char *key;
  SymbolValue value;
  struct Symbol *next;
} Symbol;

// Structure representing a stack frame for a scope
typedef struct StackFrame {
  Symbol *entries[TABLE_SIZE];
} StackFrame;

// Structure representing the symbol table
typedef struct SymbolTable {
  StackFrame stack[STACK_SIZE];
  int current_frame_index;
  // Symbol *types[TYPES_TABLE_SIZE];
} SymbolTable;

// Hash function to calculate the index for a given key
// Hash function (djb2)
unsigned int hash(const char *key);

// Function to create a new entry
Symbol *create_entry(const char *key, SymbolValue value);

// Function to push a new stack frame onto the symbol table's stack
void push_frame(SymbolTable *table);

// Function to pop the top stack frame from the symbol table's stack
void pop_frame(SymbolTable *table);

// Function to insert a new entry into the symbol table
void table_insert(SymbolTable *table, const char *key, SymbolValue value);

// Function to lookup a value for a given key in the symbol table
int table_lookup(const SymbolTable *table, const char *key, SymbolValue *value);

// Function to lookup a value for a given key in the symbol table
// int type_lookup(const SymbolTable *table, const char *key, SymbolValue
// *value);

bool in_global_scope(SymbolTable *table);

void init_types(Symbol **types, LLVMContextRef context);
void init_symbol_table(SymbolTable *table);

// int main() {
//   SymbolTable symbolTable;
//   symbolTable.currentFrameIndex = -1;
//
//   // Enter a new scope
//   pushFrame(&symbolTable);
//
//   // Insert some entries into the symbol table
//   insert(&symbolTable, "x", 10);
//   insert(&symbolTable, "y", 20);
//
//   // Lookup values for some keys
//   printf("Value of x: %d\n", lookup(&symbolTable, "x"));
//   printf("Value of y: %d\n", lookup(&symbolTable, "y"));
//
//   // Enter a new scope
//   pushFrame(&symbolTable);
//
//   // Insert a new entry in the inner scope
//   insert(&symbolTable, "x", 30);
//
//   // Lookup values for some keys in the inner scope
//   printf("Value of x in inner scope: %d\n", lookup(&symbolTable, "x"));
//   printf("Value of y in inner scope: %d\n", lookup(&symbolTable, "y"));
//
//   // Exit the inner scope
//   popFrame(&symbolTable);
//
//   // Lookup values for some keys in the outer scope
//   printf("Value of x in outer scope: %d\n", lookup(&symbolTable, "x"));
//   printf("Value of y in outer scope: %d\n", lookup(&symbolTable, "y"));
//
//   // Exit the outer scope
//   popFrame(&symbolTable);
//
//   // Lookup values for some keys after exiting all scopes
//   printf("Value of x after exiting scopes: %d\n", lookup(&symbolTable, "x"));
//   printf("Value of y after exiting scopes: %d\n", lookup(&symbolTable, "y"));
//
//   return 0;
// }
#endif
