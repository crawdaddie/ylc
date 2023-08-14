#ifndef _PATHS_H
#define _PATHS_H
#include <stdbool.h>

void resolve_path(const char *dir_name, const char *relativePath,
                  char *resultPath);

bool has_extension(const char *path, const char *ext);

void remove_extension(char *filename);
#endif
