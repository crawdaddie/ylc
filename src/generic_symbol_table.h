#ifndef _LANG_GENERIC_SYMBOL_TABLE_H
#define _LANG_GENERIC_SYMBOL_TABLE_H
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

#define INIT_SYM_TABLE_TYPES(entry_type)                                       \
  typedef struct entry_type##_Symbol {                                         \
    char *key;                                                                 \
    entry_type value;                                                          \
    struct entry_type##_Symbol *next;                                          \
  } entry_type##_Symbol;                                                       \
                                                                               \
  typedef struct entry_type##_StackFrame {                                     \
    entry_type##_Symbol *entries[TABLE_SIZE];                                  \
    int allocated_entries;                                                     \
  } entry_type##_StackFrame;                                                   \
                                                                               \
  typedef struct entry_type##_SymbolTable {                                    \
    entry_type##_StackFrame stack[STACK_SIZE];                                 \
    int current_frame_index;                                                   \
  } entry_type##_SymbolTable;                                                  \
  int entry_type##_env_lookup(entry_type##_StackFrame *frame, const char *key, \
                              entry_type *value);                              \
                                                                               \
  static entry_type##_Symbol *entry_type##_create_entry(const char *key,       \
                                                        entry_type value) {    \
    entry_type##_Symbol *entry =                                               \
        (entry_type##_Symbol *)malloc(sizeof(entry_type##_Symbol));            \
    entry->key = strdup(key);                                                  \
    entry->value = value;                                                      \
    entry->next = NULL;                                                        \
    return entry;                                                              \
  }

#define INIT_SYM_TABLE(entry_type)                                             \
  void entry_type##_push_frame(entry_type##_SymbolTable *table) {              \
    table->current_frame_index++;                                              \
    table->stack[table->current_frame_index].allocated_entries = 0;            \
  }                                                                            \
                                                                               \
  void entry_type##_pop_frame(entry_type##_SymbolTable *table) {               \
    if (table->current_frame_index > 0) {                                      \
      entry_type##_StackFrame *frame =                                         \
          &table->stack[table->current_frame_index];                           \
      for (int i = 0; i < TABLE_SIZE; i++) {                                   \
        entry_type##_Symbol *entry = frame->entries[i];                        \
        while (entry != NULL && entry != 0x1) {                                \
          entry_type##_Symbol *nextSymbol = entry->next;                       \
          free(entry->key);                                                    \
          free(entry);                                                         \
          frame->entries[i] = NULL;                                            \
          entry = nextSymbol;                                                  \
        }                                                                      \
      }                                                                        \
      table->current_frame_index--;                                            \
    }                                                                          \
  }                                                                            \
                                                                               \
  int entry_type##_env_lookup_idx(entry_type##_StackFrame *frame,              \
                                  const char *key, unsigned int index,         \
                                  entry_type *value) {                         \
    entry_type##_Symbol *entry = frame->entries[index];                        \
    while (entry != NULL) {                                                    \
      if (strcmp(entry->key, key) == 0) {                                      \
        *value = entry->value;                                                 \
        return 0;                                                              \
      }                                                                        \
      entry = entry->next;                                                     \
    }                                                                          \
    return 1;                                                                  \
  }                                                                            \
  int entry_type##_env_lookup(entry_type##_StackFrame *frame, const char *key, \
                              entry_type *value) {                             \
    unsigned int index = hash(key);                                            \
    return entry_type##_env_lookup_idx(frame, key, index, value);              \
  }                                                                            \
                                                                               \
  void entry_type##_env_insert(entry_type##_StackFrame *frame,                 \
                               const char *key, entry_type value) {            \
    unsigned int index = hash(key);                                            \
    entry_type##_Symbol *entry = frame->entries[index];                        \
                                                                               \
    entry_type##_Symbol *newSymbol = entry_type##_create_entry(key, value);    \
    if (entry == NULL) {                                                       \
      frame->entries[index] = newSymbol;                                       \
    } else {                                                                   \
      if (strcmp(entry->key, key) == 0) {                                      \
        entry->value = value;                                                  \
      } else {                                                                 \
        while (entry->next != NULL) {                                          \
          entry = entry->next;                                                 \
        }                                                                      \
        entry->next = newSymbol;                                               \
      }                                                                        \
    }                                                                          \
    frame->allocated_entries++;                                                \
  }                                                                            \
                                                                               \
  void entry_type##_table_insert(entry_type##_SymbolTable *table,              \
                                 const char *key, entry_type value) {          \
    entry_type##_StackFrame *frame =                                           \
        &table->stack[table->current_frame_index];                             \
    entry_type##_env_insert(frame, key, value);                                \
  }                                                                            \
                                                                               \
  int entry_type##_table_lookup(entry_type##_SymbolTable *table,               \
                                const char *key, entry_type *value) {          \
    unsigned int index = hash(key);                                            \
                                                                               \
    for (int i = table->current_frame_index; i >= 0; i--) {                    \
      entry_type##_StackFrame *frame = &table->stack[i];                       \
      if (entry_type##_env_lookup_idx(frame, key, index, value) == 0) {        \
        return 0;                                                              \
      };                                                                       \
    }                                                                          \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  void init_##entry_type##_symbol_table(entry_type##_SymbolTable *table) {     \
    table->current_frame_index = -1;                                           \
  }

#endif
