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
  if (strcmp(dir, ".") == 0) {
    sprintf(result, "%s", rel);
  } else {
    sprintf(result, "%s/%s", dir, rel);
  }
}

bool has_extension(const char *str, const char *extension) {
  size_t strLen = strlen(str);
  size_t extensionLen = strlen(extension);

  if (strLen >= extensionLen) {
    const char *endOfStr = str + strLen - extensionLen;
    if (strcmp(endOfStr, extension) == 0) {
      return true; // The string ends with ".so"
    }
  }

  return false; // The string does not end with ".so"
}
