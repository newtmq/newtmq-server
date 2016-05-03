#include <newt/common.h>
#include <newt/optparse.h>
#include <newt/config.h>
#include <newt/daemon.h>
#include <newt/logger.h>

#include <stdlib.h>

int main(int argc, char **argv) {
  struct cmd_args args;
  newt_config config;
  int ret;

  /* register signal handler */
  init_signal_handler();

  // parse command-line arguments
  parse_opt(argc, argv, &args);
  if(args.config_path == NULL) {
    perror("failed to load configuration file");
    exit(1);
  }

  ret = load_config(args.config_path, &config);
  if(ret == RET_ERROR) {
    perror("failed to load configuration file");
    exit(1);
  }

  if(daemon_initialize() == RET_ERROR) {
    exit(1);
  }

  if(daemon_start(config) == RET_ERROR) {
    exit(1);
  }

  return 0;
}
