#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void perform_cb(int ch, int cc, void (*callback_fn)(int val)) {
  printf("perform %d %d %p\n", ch, cc, callback_fn);
  (*callback_fn)(120);
}
typedef void *(*ThreadFunction)(void *);

pthread_t create_thread(ThreadFunction function, void *arg) {
  pthread_t t;
  pthread_create(&t, NULL, function, arg);
  return t;
}

void kill_thread(pthread_t t) { pthread_cancel(t); }

// math
int randint(int max) { return rand() % max; }
double randfloat(double max) {

  // Generate a random integer in the range [0, RAND_MAX]
  int randomInt = rand();

  // Scale the random integer to the range [0, N)
  return ((double)randomInt / RAND_MAX) * max;
}

struct Point {
  double x;
  double y;
};

// struct Point get_point(double x, double y) {
//   struct Point p;
//   p.x = x;
//   p.y = y;
//   printf("%p (x: %f y: %f)\n", &p, p.x, p.y);
//   return p;
// }

struct Point *get_point(double x, double y) {
  struct Point *p = malloc(sizeof(struct Point));
  p->x = x;
  p->y = y;
  printf("%p (x: %f y: %f)\n", p, p->x, p->y);
  return p;
}
