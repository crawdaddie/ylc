#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int t_counter = 0;
ttype _tvar() {
  ttype t = {T_VAR};
  t_counter++;
  sprintf(t.as.T_VAR.name, "'t%d", t_counter);
  return t;
}
ttype tfn(ttype *param_types, int length) {
  return (ttype){T_FN,
                 {.T_FN = {
                      .length = length,
                      .members = param_types,
                  }}};
}

ttype ttuple(ttype *member_types, int length) {
  return (ttype){T_TUPLE,
                 {.T_TUPLE = {
                      .length = length,
                      .members = member_types,
                  }}};
}

ttype tstruct(ttype *member_types, struct_member_metadata *md, int length) {
  return (ttype){T_STRUCT,
                 {.T_STRUCT = {
                      .length = length,
                      .members = member_types,
                      .struct_metadata = md,
                  }}};
}

ttype tptr(ttype *pointed_to) {
  ttype t = {T_PTR};
  t.as.T_PTR.item = pointed_to;
  return t;
}
ttype tarray(ttype *type, int len) {
  return (ttype){T_ARRAY,
                 {.T_ARRAY = {
                      .length = len,
                      .member_type = type,
                  }}};
}

char *_tname() {
  t_counter++;
  char *result =
      (char *)malloc(32 * sizeof(char)); // Assuming a large enough buffer size
  return result;
}

ttype tvar(char *name) {
  ttype t = {T_VAR};
  sprintf(t.as.T_VAR.name, "%s", name);
  return t;
}
ttype tvoid() { return (ttype){T_VOID}; }
ttype tint() { return (ttype){T_INT}; }
ttype tnum() { return (ttype){T_NUM}; }
ttype tstr() { return (ttype){T_STR}; }
ttype tbool() { return (ttype){T_BOOL}; }

bool is_generic_type(ttype t) {
  if (t.tag == T_VAR) {
    return true;
  }

  if (t.tag == T_FN) {
    for (int i = 0; i < t.as.T_FN.length; i++) {
      if (is_generic_type(t.as.T_FN.members[i])) {
        return true;
      }
    }
    return false;
  }

  if (t.tag == T_TUPLE) {
    for (int i = 0; i < t.as.T_TUPLE.length; i++) {
      if (is_generic_type(t.as.T_TUPLE.members[i])) {
        return true;
      }
    }
    return false;
  }

  return false;
};

int get_struct_member_index(ttype struct_type, char *name) {
  struct_member_metadata *md = struct_type.as.T_STRUCT.struct_metadata;
  for (int i = 0; i < struct_type.as.T_STRUCT.length; i++) {
    struct_member_metadata member_type = md[i];

    if (strcmp(name, member_type.name) == 0) {
      return member_type.index;
    }
  }
  return -1;
}

bool is_numeric_type(ttype t) { return t.tag >= T_INT8 && t.tag <= T_NUM; }

ttype_tag max_type(ttype a, ttype b) {
  if (a.tag >= b.tag) {
    return a.tag;
  }
  return b.tag;
}

void print_ttype(ttype type) {
  switch (type.tag) {
  case T_VOID: {
    printf("Void");
    break;
  }
  case T_INT: {
    printf("Int");
    break;
  }
  case T_INT8: {
    printf("Int8");
    break;
  }
  case T_NUM: {
    printf("Num");
    break;
  }
  case T_STR: {
    printf("Str");
    break;
  }
  case T_BOOL: {
    printf("Bool");
    break;
  }
  case T_VAR: {
    printf("%s", type.as.T_VAR.name);
    break;
  }
  case T_FN: {
    int length = type.as.T_FN.length;
    printf("(");
    for (int i = 0; i < length; i++) {
      print_ttype(type.as.T_FN.members[i]);
      if (i < length - 1)
        printf(" -> ");
    }
    printf(")");
    break;
  }

  case T_TUPLE: {
    int length = type.as.T_TUPLE.length;
    printf("(");
    for (int i = 0; i < length; i++) {
      print_ttype(type.as.T_TUPLE.members[i]);
      if (i < length - 1)
        printf(" * ");
    }
    printf(")");
    break;
  }

  case T_STRUCT: {
    int length = type.as.T_STRUCT.length;
    printf("(");
    for (int i = 0; i < length; i++) {
      struct_member_metadata md = type.as.T_STRUCT.struct_metadata[i];
      if (md.index == -1) {
        continue;
      }

      printf("%s=", type.as.T_STRUCT.struct_metadata[i].name);
      print_ttype(type.as.T_STRUCT.members[i]);
      if (i < length - 1)
        printf(" * ");
    }
    printf(")");
    break;
  }
  case T_PTR: {
    printf("*");
    print_ttype(*type.as.T_PTR.item);
    break;
  }

  case T_ARRAY: {
    print_ttype(*type.as.T_ARRAY.member_type);
    printf("[%d]", type.as.T_ARRAY.length);
    break;
  }
  }
}

bool types_equal(ttype *l, ttype *r) {

  if (l == r) {
    return true;
  }

  if (l->tag != r->tag) {
    return false;
  }
  if (l->tag == r->tag) {
    return true;
  }

  if (l->tag == T_TUPLE) {
    for (int i = 0; i < l->as.T_TUPLE.length; i++) {
      if (!types_equal(&l->as.T_TUPLE.members[i], &r->as.T_TUPLE.members[i])) {
        return false;
      }
    }
    return true;
  }

  if (l->tag == T_STRUCT) {
    for (int i = 0; i < l->as.T_STRUCT.length; i++) {
      if (!types_equal(&l->as.T_STRUCT.members[i],
                       &r->as.T_STRUCT.members[i])) {
        return false;
      }
    }
    return true;
  }
  if (l->tag == T_VAR && strcmp(l->as.T_VAR.name, r->as.T_VAR.name) == 0) {
    return true;
  }

  if (l->tag == T_VAR && r->tag == T_VAR &&
      strcmp(l->as.T_VAR.name, r->as.T_VAR.name) != 0) {
    return false;
  }
  return false;
}
