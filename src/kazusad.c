#include <kazusa/common.h>
#include <kazusa/optparse.h>
#include <kazusa/config.h>
#include <kazusa/daemon.h>
#include <kazusa/logger.h>

#include <stdlib.h>

int main(int argc, char **argv) {
  struct cmd_args args;
  kd_config config;
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

  if(config.loglevel != NULL) {
    set_logger(config.loglevel);
  }

  if(daemon_initialize() == RET_ERROR) {
    exit(1);
  }

  if(daemon_start(config) == RET_ERROR) {
    exit(1);
  }

  return 0;
}
