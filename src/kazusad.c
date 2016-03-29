#include "lib/stomp.h"
#include "lib/optparse.h"

int main(int argc, char **argv) {
  struct arguments args;
  parse_opt(argc, argv, &args);
  return 0;
}
