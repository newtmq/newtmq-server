#include "lib/stomp.h"
#include "lib/optparse.h"

int main(int argc, char **argv) {
  struct cmd_args args;

  // parse command-line arguments
  parse_opt(argc, argv, &args);

  return 0;
}
