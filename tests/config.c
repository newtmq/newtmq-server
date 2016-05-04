#include <test.h>

#include <newt/config.h>

int set_config(newt_config *config) {
  char confpath[512];

  getcwd(confpath, sizeof(confpath));
  sprintf(confpath, "%s/%s", confpath, TEST_CONFIG);

  load_config(confpath, config);
}
