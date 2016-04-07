#include <kazusa/common.h>
#include <kazusa/stomp.h>
#include <kazusa/optparse.h>
#include <kazusa/config.h>

int main(int argc, char **argv) {
  struct cmd_args args;
  kd_config config;
  int ret;

  // parse command-line arguments
  parse_opt(argc, argv, &args);
  if(args.config_path == NULL) {
    printf("[ERROR] failed to load configuration file");
    exit(1);
  }

  ret = load_config(args.config_path, &config);
  if(ret == RET_ERROR) {
    perror("failed to load configuration file");
    exit(1);
  }

  daemon_start(config);

  return 0;
}
