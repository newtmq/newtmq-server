#include <test.h>

#include "config.h"

int set_config(kd_config *config) {
  char confpath[512];

  getcwd(confpath, sizeof(confpath));
  sprintf(confpath, "%s/%s", confpath, TEST_CONFIG);

  load_config(confpath, config);
}
