#ifndef _LANG_GENERIC_SYM_TABLE_H
#define _LANG_GENERIC_SYM_TABLE_H
#define TABLE_SIZE 100
#define STACK_SIZE 100
#define INIT_SYM_TABLE(VAL_TYPE, Name)                                         \
  typedef struct Name##Symbol {                                                \
    char *key;                                                                 \
    VAL_TYPE value;                                                            \
    struct Name##Symbol *next;                                                 \
  } Name##Symbol;                                                              \
                                                                               \
  typedef struct Name##StackFrame {                                            \
    Name##Symbol *entries[TABLE_SIZE];                                         \
  } Name##StackFrame;                                                          \
                                                                               \
  typedef struct Name##SymbolTable {                                           \
    Name##StackFrame stack[STACK_SIZE];                                        \
    int current_frame_index;                                                   \
  } Name##SymbolTable;                                                         \
                                                                               \
  Name##Symbol *create_##Name##entry(char *key, VAL_TYPE value) {              \
    Name##Symbol *entry = (Name##Symbol *)malloc(sizeof(Name##Symbol));        \
    entry->key = strdup(key);                                                  \
    entry->value = value;                                                      \
    entry->next = NULL;                                                        \
    return entry;                                                              \
  }                                                                            \
  void push_##Name##frame(Name##SymbolTable *table) {                          \
    table->current_frame_index++;                                              \
  }                                                                            \
  void pop_##Name##frame(Name##SymbolTable *table) {                           \
    if (table->current_frame_index > 0) {                                      \
      Name##StackFrame *frame = &table->stack[table->current_frame_index];     \
      for (int i = 0; i < TABLE_SIZE; i++) {                                   \
        Name##Symbol *entry = frame->entries[i];                               \
        while (entry != NULL) {                                                \
          Name##Symbol *nextSymbol = entry->next;                              \
          free(entry->key);                                                    \
          free(entry);                                                         \
          frame->entries[i] = NULL;                                            \
          entry = nextSymbol;                                                  \
        }                                                                      \
      }                                                                        \
      table->current_frame_index--;                                            \
    }                                                                          \
  }                                                                            \
  void table_##Name##insert(Name##SymbolTable *table, char *key,               \
                            VAL_TYPE value) {                                  \
    unsigned int index = hash(key);                                            \
    Name##StackFrame *frame = &table->stack[table->current_frame_index];       \
    Name##Symbol *entry = frame->entries[index];                               \
    Name##Symbol *newSymbol = create_##Name##entry(key, value);                \
    if (entry == NULL) {                                                       \
      frame->entries[index] = newSymbol;                                       \
    } else {                                                                   \
      while (entry->next != NULL) {                                            \
        entry = entry->next;                                                   \
      }                                                                        \
      entry->next = newSymbol;                                                 \
    }                                                                          \
  }                                                                            \
  int table_##Name##lookup(Name##SymbolTable *table, char *key,                \
                           VAL_TYPE *value) {                                  \
    unsigned int index = hash(key);                                            \
                                                                               \
    for (int i = table->current_frame_index; i >= 0; i--) {                    \
      Name##StackFrame *frame = &table->stack[i];                              \
      Name##Symbol *entry = frame->entries[index];                             \
      while (entry != NULL) {                                                  \
        if (strcmp(entry->key, key) == 0) {                                    \
          *value = entry->value;                                               \
          return 0;                                                            \
        }                                                                      \
        entry = entry->next;                                                   \
      }                                                                        \
    }                                                                          \
    return 1;                                                                  \
  }

#endif
