#ifndef _LANG_SYM_TABLE_H
#define _LANG_SYM_TABLE_H
#include <llvm-c/Core.h>
#include <stdbool.h>

#define TABLE_SIZE 100
#define STACK_SIZE 100

typedef struct member_type {
  char *name;
  char *type;
} member_type;
typedef struct type_symbol_table {
  int length;
  member_type *member_types;
} type_symbol_table;

typedef struct SymbolValue {
  enum {
    TYPE_FN_PARAM,
    TYPE_VARIABLE,
    TYPE_GLOBAL_VARIABLE,
    TYPE_RECURSIVE_REF,
    TYPE_INT,
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_LIST,
    TYPE_STRUCT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_EXTERN_FN,
    TYPE_TYPE_DECLARATION,
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

    struct TYPE_RECURSIVE_REF {
      LLVMValueRef llvm_value;
      LLVMTypeRef llvm_type;
    } TYPE_RECURSIVE_REF;

    struct TYPE_FN_PARAM {
      int arg_idx;
      LLVMTypeRef type;
    } TYPE_FN_PARAM;

    struct TYPE_EXTERN_FN {
      LLVMValueRef llvm_value;
      LLVMTypeRef llvm_type;
    } TYPE_EXTERN_FN;

    struct TYPE_TYPE_DECLARATION {
      LLVMTypeRef llvm_type;
      type_symbol_table *type_lookups;
    } TYPE_TYPE_DECLARATION;
  } data;
} SymbolValue;

#define VALUE(type_tag, ...)                                                   \
  (SymbolValue) {                                                              \
    TYPE_##type_tag, {                                                         \
      .TYPE_##type_tag = (struct TYPE_##type_tag) { __VA_ARGS__ }              \
    }                                                                          \
  }

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

void print_table(SymbolTable *table);
#endif
