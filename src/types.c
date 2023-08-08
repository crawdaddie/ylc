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
