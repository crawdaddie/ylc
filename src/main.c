#include "llvm_backend.h"
#include <stdio.h>

#include <getopt.h>
#include <string.h>

static int usage(char *exe) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "Options:\n"
          "  [--backend dummy|alsa|pulseaudio|jack|coreaudio|wasapi]\n"
          "  [--device id]\n"
          "  [--raw]\n"
          "  [--name stream_name]\n"
          "  [--latency seconds]\n"
          "  [--sample-rate hz]\n"
          "  [--oscilloscope]\n",
          exe);
  return 1;
}

static int process_opts(int argc, char **argv, char **output, char **input) {
  int option;
  // Define the long options
  struct option longOptions[] = {{"emit", required_argument, NULL, 'e'},
                                 {NULL, 0, NULL, 0}};

  // Parse the command-line options
  while ((option = getopt_long(argc, argv, "e:", longOptions, NULL)) != -1) {
    switch (option) {
    case 'e':
      *output = optarg;
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
    fprintf(stderr, "Input file not specified\n");
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  setbuf(stdout, NULL); // turn off line buffering for stdout
  int repl = 0;

  char *input = NULL;
  char *output = NULL;
  process_opts(argc, argv, &output, &input);
  repl = input == NULL;

  return LLVMRuntime(repl, input, output);
}
