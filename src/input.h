#ifndef _LANG_RUNNER_H
#define _LANG_RUNNER_H
#include <stdio.h>
#include <stdlib.h>
void repl_input(char *input, int bufsize, const char *prompt);
char *read_file(const char *path);
#endif
