#ifndef _LANG_TYPES_H
#define _LANG_TYPES_H
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
  T_VAR,      // 'x
  T_INT8,     // int8 - alias char
  T_INT,      // int
  T_NUM,      // double
  T_STR,      // str
  T_BOOL,     // bool
  T_COMPOUND, // struct or tuple
  T_PTR,      // &'x
  T_VOID,     // void
  T_FN,       // t1 -> t2 -> ... -> return_type
} ttype_tag;

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

  } as;
} ttype;

ttype _tvar();
ttype tfn(ttype *param_types, int length);
char *_tname();
ttype tvar(char *name);
ttype tvoid();
ttype tint();
ttype tnum();
ttype tbool();
ttype tstr();
extern int t_counter;
#endif
