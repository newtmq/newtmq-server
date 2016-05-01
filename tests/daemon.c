#include <kazusa/daemon.h>

#include "config.h"
#include <unistd.h>

pid_t start_kazusad() {
  pid_t pid;
  kd_config config = {0};

  set_config(&config);
  if((pid = fork()) == 0) {
    daemon_initialize();
    daemon_start(config);
  }

  return pid;
}
