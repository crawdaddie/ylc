#include "paths.h"
#include <libgen.h>
#include <stdio.h>
#include <string.h>

void resolve_path(const char *dir, const char *rel, char *result) {

  // Step 2: Process the relative path
  if (strncmp(rel, "./", 2) == 0) {
    rel += 2; // Move baseName pointer forward to remove "./"
  } else if (strncmp(rel, "../", 3) == 0) {
    // Handle cases where we move up one directory
    while (strncmp(rel, "../", 3) == 0) {
      rel += 3;
      dir = dirname(dir);
    }
  }
  // Step 3: Resolve the final path
  sprintf(result, "%s/%s", dir, rel);
}
