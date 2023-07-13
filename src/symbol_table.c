#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Hash function to calculate the index for a given key
// Hash function (djb2)
unsigned int hash(const char *key) {
  unsigned int hash = 5381;
  int c;

  while ((c = *key++) != '\0') {
    hash = ((hash << 5) + hash) + c; // hash = hash * 33 + c
  }

  return hash % TABLE_SIZE;
}

// Function to create a new entry
Symbol *create_entry(const char *key, SymbolValue value) {
  Symbol *entry = (Symbol *)malloc(sizeof(Symbol));
  entry->key = strdup(key);
  entry->value = value;
  entry->next = NULL;
  return entry;
}

// Function to push a new stack frame onto the symbol table's stack
void push_frame(SymbolTable *table) { table->current_frame_index++; }

// Function to pop the top stack frame from the symbol table's stack
void pop_frame(SymbolTable *table) {
  if (table->current_frame_index > 0) {
    // Free memory allocated for the entries in the popped frame
    StackFrame *frame = &table->stack[table->current_frame_index];
    for (int i = 0; i < TABLE_SIZE; i++) {
      Symbol *entry = frame->entries[i];
      while (entry != NULL) {
        Symbol *nextSymbol = entry->next;
        free(entry->key);
        free(entry);
        entry = nextSymbol;
      }
    }
    table->current_frame_index--;
  }
}

// Function to insert a type into the symbol table
// void insert_type(Symbol **types, const char *key, SymbolValue value) {
//   unsigned int index = hash(key, TYPES_TABLE_SIZE);
//   Symbol *entry = types[index];
//   Symbol *newSymbol = create_entry(key, value);
//
//   // If there are no entries at the calculated index, insert the new entry
//   if (entry == NULL) {
//     types[index] = newSymbol;
//   } else {
//     // Traverse to the end of the linked list at the index and insert the new
//     // entry
//     while (entry->next != NULL) {
//       entry = entry->next;
//     }
//     entry->next = newSymbol;
//   }
// }
// Function to insert a new entry into the symbol table
void table_insert(SymbolTable *table, const char *key, SymbolValue value) {
  unsigned int index = hash(key);
  StackFrame *frame = &table->stack[table->current_frame_index];
  Symbol *entry = frame->entries[index];
  Symbol *newSymbol = create_entry(key, value);

  // If there are no entries at the calculated index, insert the new entry
  if (entry == NULL) {
    frame->entries[index] = newSymbol;
  } else {
    // Traverse to the end of the linked list at the index and insert the new
    // entry
    while (entry->next != NULL) {
      entry = entry->next;
    }
    entry->next = newSymbol;
  }
}

// Function to lookup a value for a given key in the symbol table
int table_lookup(const SymbolTable *table, const char *key,
                 SymbolValue *value) {
  unsigned int index = hash(key);

  // Traverse the stack frames from top to bottom to find the key
  for (int i = table->current_frame_index; i >= 0; i--) {
    // printf("stack %d \n", i);
    StackFrame *frame = &table->stack[i];
    Symbol *entry = frame->entries[index];

    // Traverse the linked list at the calculated index to find the key
    while (entry != NULL) {
      if (strcmp(entry->key, key) == 0) {
        *value = entry->value;
        // value->is_global = i == 0;
        return 0;
      }
      entry = entry->next;
    }
  }

  // Key not found in the symbol table
  return 1;
}

// Function to lookup a type for a given key in the symbol table
// int type_lookup(const SymbolTable *table, const char *key, SymbolValue
// *value) {
//
//   unsigned int index = hash(key, TYPES_TABLE_SIZE);
//
//   Symbol *entry = table->types[index];
//
//   // Traverse the linked list at the calculated index to find the key
//   while (entry != NULL) {
//     if (strcmp(entry->key, key) == 0) {
//       *value = entry->value;
//       // value->is_global = i == 0;
//       return 0;
//     }
//     entry = entry->next;
//   }
//
//   // Key not found in the symbol table
//   return 1;
// }
// void init_types(Symbol **types, LLVMContextRef context) {
//   insert_type(types, "int",
//               (SymbolValue){TYPE_INT, LLVMInt32TypeInContext(context)});
//   insert_type(types, "double",
//               (SymbolValue){TYPE_NUMBER, LLVMDoubleTypeInContext(context)});
//
//   insert_type(types, "bool", (SymbolValue){TYPE_BOOL, LLVMInt1Type()});
//
//   insert_type(types, "list", (SymbolValue){TYPE_LIST, NULL});
//   insert_type(types, "char", (SymbolValue){TYPE_CHAR, LLVMInt8Type()});
//
//   // insert_type(types, "struct", (SymbolValue){TYPE_STRUCT, NULL});
//   // insert_type(types, "string", (SymbolValue){TYPE_STRING, NULL});
// }
void init_symbol_table(SymbolTable *table) { table->current_frame_index = -1; }
