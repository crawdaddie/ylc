#include "runner.h"
#include <stdio.h>

#include <getopt.h>
#include <string.h>

static int usage(char *exe) {
  fprintf(
      stderr,
      "Usage: %s [options] input_file.ylc\n"
      "Options:\n"
      "  [--emit (file) compile input_file and output IR bytecode to file]\n"
      "  [--repl / -r start in repl mode]\n",
      exe);
  return 1;
}

static int process_opts(int argc, char **argv, char **output, char **input,
                        int *repl) {
  int option;
  // Define the long options
  struct option longOptions[] = {{"emit", required_argument, NULL, 'e'},
                                 {"include", required_argument, NULL, 'i'},
                                 {"repl", no_argument, NULL, 'r'},
                                 {NULL, 0, NULL, 0}};

  // Parse the command-line options
  while ((option = getopt_long(argc, argv, "e:i:r", longOptions, NULL)) != -1) {

    switch (option) {
    case 'e':
      *output = optarg;
      break;

    case 'i':
      // *include = optarg;
      break;

    case 'r':
      *repl = 1;
      // *include = optarg;
      break;

    default:

      fprintf(stderr, "Unknown option: %c\n", option);
      return 1;
    }
  }

  // Get the positional argument (input file)
  if (optind < argc) {
    *input = argv[optind];
  }

  if (input != NULL) {
    // printf("Input file: %s\n", *input);
  } else {
    usage(argv[0]);
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  setbuf(stdout, NULL); // turn off line buffering for stdout
  int repl = 0;

  char *input = NULL;
  char *output = NULL;
  process_opts(argc, argv, &output, &input, &repl);

  if (input == NULL) {
    repl = 1;
  }

  return LLVMRuntime(repl, input, output);
}
