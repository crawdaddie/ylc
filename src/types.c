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
