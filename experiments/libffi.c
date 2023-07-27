#include <stdio.h>

void perform_cb(int ch, int cc, void (*callback_fn)(int val)) {
  printf("perform %d %d %p\n", ch, cc, callback_fn);
  (*callback_fn)(120);
}
