#ifndef _LANG_TYPES_H
#define _LANG_TYPES_H
#include <stdbool.h>
/*
 * type expression
 * t ::= | 'x
 *       | int
 *       | double
 *       | bool
 *       | str
 *       | t1 * t2  (struct or tuple)
 *       | &t       (ptr to item of type t)
 *       | void
 *       | t1 -> t2 (function type)
 * */

typedef enum {
  T_VAR,    // 'x
  T_INT8,   // int8 - alias char
  T_INT,    // int
  T_NUM,    // double
  T_STR,    // str
  T_BOOL,   // bool
  T_TUPLE,  // struct or tuple
  T_STRUCT, // struct or tuple
  T_VOID,   // void
  T_FN,     // t1 -> t2 -> ... -> return_type
  T_PTR,    // &'x
  T_ARRAY,
} ttype_tag;

typedef struct {
  char *name;
  int index;
} struct_member_metadata;

typedef struct ttype {
  ttype_tag tag;
  union {
    struct T_VAR {
      char name[16];
    } T_VAR;

    struct T_FN {
      int length; // length = num params + 1
      // ie: param_t1 -> param_t2 -> param_t3 -> return_type
      struct ttype *members;
    } T_FN;

    struct T_PTR {
      struct ttype *item;
    } T_PTR;

    struct T_TUPLE {
      int length;
      struct ttype *members;
    } T_TUPLE;

    struct T_ARRAY {
      int length;
      struct ttype *member_type;
    } T_ARRAY;

    struct T_STRUCT {
      int length;
      struct ttype *members;
      struct_member_metadata *struct_metadata;
    } T_STRUCT;

  } as;
} ttype;

ttype _tvar();
ttype tfn(ttype *param_types, int length);
ttype ttuple(ttype *member_types, int length);
ttype tarray(ttype *member_type, int length);
char *_tname();
ttype tvar(char *name);
ttype tvoid();
ttype tint();
ttype tnum();
ttype tbool();
ttype tstr();
ttype tptr(ttype *pointed_to);

ttype tstruct(ttype *member_types, struct_member_metadata *md, int length);

bool is_generic_type(ttype t);

extern int t_counter;

int get_struct_member_index(ttype struct_type, char *name);

bool is_numeric_type(ttype t);
ttype_tag max_type(ttype a, ttype b);

void print_ttype(ttype type);

bool types_equal(ttype *l, ttype *r);

#endif
