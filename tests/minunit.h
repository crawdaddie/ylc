//==============================================================================
//
// Minunit
//
//==============================================================================
#include <stdio.h>
#define mu_fail(MSG, ...)                                                      \
  do {                                                                         \
    fprintf(stderr, "%s:%d: " MSG "\n", __FILE__, __LINE__, ##__VA_ARGS__);    \
    return 1;                                                                  \
  } while (0)

#define mu_assert(TEST, MSG, ...)                                              \
  do {                                                                         \
    if (!(TEST)) {                                                             \
      fprintf(stderr, "❌: %s:%d: %s " MSG "\n", __FILE__, __LINE__, #TEST,    \
              ##__VA_ARGS__);                                                  \
      return 1;                                                                \
    } else {                                                                   \
      fprintf(stderr, "✅: %s\n", MSG);                                        \
    }                                                                          \
  } while (0)

#define mu_run_test(TEST)                                                      \
  do {                                                                         \
    fprintf(stderr, "\n# %s\n", #TEST);                                        \
    int rc = TEST();                                                           \
    if (rc) {                                                                  \
      fprintf(stderr, "\n  Test Failure: %s()\n", #TEST);                      \
      test_result = 1;                                                         \
    }                                                                          \
  } while (0)

#define RUN_TESTS()                                                            \
  int main() {                                                                 \
    fprintf(stderr, "== %s ==\n", __FILE__);                                   \
    int rc = all_tests();                                                      \
    fprintf(stderr, "\n");                                                     \
    return rc;                                                                 \
  }
